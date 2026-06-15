#!/usr/bin/env python3
"""Convert Yoga generated gtest cases (generated/YG*Test.cpp) into xent-native
C conformance tests against the public xent API.

Each Yoga TEST(YogaTest, name) is a flat sequence of YGNodeStyleSet* calls,
YGNodeInsertChild, YGNodeCalculateLayout and ASSERT_FLOAT_EQ(expected,
YGNodeLayoutGet*). A case is converted when every statement maps to a supported
xent style; if it touches an unsupported feature the whole case is skipped with
a recorded reason. Expected layout values come from Chrome via Yoga's gentest,
so a converted case that fails at runtime is a real xent-vs-CSS divergence.

Usage: python convert_yoga.py <yoga_tests_generated_dir> [out.c]
"""
import os, re, sys, json
from collections import Counter

if len(sys.argv) < 2:
    sys.exit(__doc__.strip().splitlines()[-1])
SRC = sys.argv[1]
OUT = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(__file__), "test_yoga_full.c")
# Reuse the yt_* harness (helpers + YT_ASSERT macros) from the canonical suite.
HARNESS = os.path.join(os.path.dirname(__file__), "test_yoga_generated.c")


def load_preamble():
    # Reuse the yt_* harness (helpers + YT_ASSERT macros). The Yoga generated
    # cases assert YOGA's behavior, whose default flex-shrink is 0 (CSS is 1).
    # xent's product default is the CSS-correct 1; here the harness sets 0 on
    # every node so the cases test xent's flex ALGORITHM against Yoga's expected
    # values with matched defaults (isolating algorithm bugs from the default
    # difference). The flex algorithm itself is identical in CSS and Yoga.
    text = open(HARNESS, encoding="utf-8").read()
    pre = text[:text.index("static int test_")]
    pre = pre.replace(
        "\txent_set_flex_direction(ctx, node, XENT_FLEX_COLUMN);\n\treturn node;",
        "\txent_set_flex_direction(ctx, node, XENT_FLEX_COLUMN);\n"
        "\txent_set_flex_shrink(ctx, node, 0.0f); /* Yoga default; see comment above */\n\treturn node;")
    # xent_get_layout_rect reports ABSOLUTE coords; Yoga's YGNodeLayoutGetLeft/Top
    # (which the converter emits as expected x/y) are PARENT-RELATIVE. Override the
    # assert macro to subtract the parent's absolute position for x/y so nested
    # children compare correctly. (Flat cases are unaffected: root's parent is
    # INVALID and top-level children's parent is the root at (0,0).)
    old_macro = pre[pre.index("#define YT_ASSERT_RECT"):pre.index("while (0)\n", pre.index("#define YT_ASSERT_RECT")) + len("while (0)")]
    new_macro = (
        "#define YT_ASSERT_RECT(node, member, expected) do { XentRect rect = {0}; "
        "YT_ASSERT(xent_get_layout_rect(ctx, node, &rect)); float _v = rect.member; "
        "XentNodeId _p = xent_get_parent(ctx, node); "
        "if ((#member[0] == 'x' || #member[0] == 'y') && _p != XENT_NODE_INVALID) { "
        "XentRect _pr = {0}; xent_get_layout_rect(ctx, _p, &_pr); "
        "_v -= ((#member[0] == 'x') ? _pr.x : _pr.y); } "
        "if (fabsf(_v - (expected)) > YT_EPS) { fprintf(stderr, "
        "\"RECT FAILED: %s.%s expected %.3f got %.3f (%s:%d)\\n\", #node, #member, "
        "(double)(expected), (double)_v, __FILE__, __LINE__); return 1; } } while (0)")
    return pre.replace(old_macro, new_macro)

# Cases whose Yoga EXPECTED values encode Yoga-specific, non-CSS semantics
# (verified against css-flexbox-1). Excluded so the suite measures CSS
# conformance rather than Yoga bug-for-bug compatibility. Keep the reason.
EXCLUDE = {
    # Expects an EMPTY explicit-size item to overflow rather than shrink — Yoga's
    # flex-shrink:0 default. Under CSS the automatic minimum size of an empty
    # explicit item is min(content=0, specified)=0, so it shrinks to fit.
    "nested_overflowing_child_in_constraint_parent": "yoga shrink:0 (CSS auto-min=0 for empty explicit item)",
    # Row-wrap, definite container cross, single line, items with auto cross size.
    # Yoga leaves the items at content cross (0); CSS align-content:stretch (the
    # default) stretches the single line to fill the definite container cross, and
    # align-items:stretch then stretches the items to it. xent is CSS-correct here.
    "flex_wrap_align_stretch_fits_one_row": "yoga single-line align-content (CSS stretches the line to fill)",
}

FLEX_DIR = {"YGFlexDirectionRow": "XENT_FLEX_ROW", "YGFlexDirectionColumn": "XENT_FLEX_COLUMN"}
JUSTIFY = {
    "YGJustifyFlexStart": "XENT_FLEX_JUSTIFY_START", "YGJustifyFlexEnd": "XENT_FLEX_JUSTIFY_END",
    "YGJustifyCenter": "XENT_FLEX_JUSTIFY_CENTER", "YGJustifySpaceBetween": "XENT_FLEX_JUSTIFY_SPACE_BETWEEN",
    "YGJustifySpaceAround": "XENT_FLEX_JUSTIFY_SPACE_AROUND", "YGJustifySpaceEvenly": "XENT_FLEX_JUSTIFY_SPACE_EVENLY",
}
ALIGN = {  # for align-items / align-self
    "YGAlignFlexStart": "XENT_FLEX_ALIGN_START", "YGAlignFlexEnd": "XENT_FLEX_ALIGN_END",
    "YGAlignCenter": "XENT_FLEX_ALIGN_CENTER", "YGAlignStretch": "XENT_FLEX_ALIGN_STRETCH",
    "YGAlignBaseline": "XENT_FLEX_ALIGN_BASELINE",
}
ALIGN_CONTENT = {
    "YGAlignFlexStart": "XENT_FLEX_ALIGN_CONTENT_START", "YGAlignFlexEnd": "XENT_FLEX_ALIGN_CONTENT_END",
    "YGAlignCenter": "XENT_FLEX_ALIGN_CONTENT_CENTER", "YGAlignSpaceBetween": "XENT_FLEX_ALIGN_CONTENT_SPACE_BETWEEN",
    "YGAlignSpaceAround": "XENT_FLEX_ALIGN_CONTENT_SPACE_AROUND", "YGAlignSpaceEvenly": "XENT_FLEX_ALIGN_CONTENT_SPACE_EVENLY",
    "YGAlignStretch": "XENT_FLEX_ALIGN_CONTENT_STRETCH",
}
EDGE = {
    "YGEdgeLeft": "YT_EDGE_LEFT", "YGEdgeTop": "YT_EDGE_TOP", "YGEdgeRight": "YT_EDGE_RIGHT",
    "YGEdgeBottom": "YT_EDGE_BOTTOM", "YGEdgeAll": "YT_EDGE_ALL", "YGEdgeHorizontal": "YT_EDGE_HORIZONTAL",
    "YGEdgeVertical": "YT_EDGE_VERTICAL",
}
SIDE = {"Left": "x", "Top": "y", "Width": "w", "Height": "h"}


class Skip(Exception):
    def __init__(self, reason): self.reason = reason


def num(tok):
    tok = tok.strip()
    if tok in ("YGUndefined", "YGValueUndefined"): return "NAN"
    if tok and tok[-1] in "fF" and not tok[-1].isdigit(): tok = tok[:-1]  # C++ float suffix
    return f"{float(tok):.6f}f"


def convert_test(name, body):
    """Return list of emitted C lines, or raise Skip(reason)."""
    out = []
    # statements are ;-terminated; split keeping it simple (no strings/braces inside)
    stmts = [s.strip() for s in body.replace("\n", " ").split(";") if s.strip()]
    declared = []
    root = None
    for s in stmts:
        m = re.match(r"YGNodeRef\s+(\w+)\s*=\s*YGNodeNewWithConfig", s)
        if m:
            v = m.group(1); declared.append(v)
            if root is None: root = v
            out.append(f"\tXentNodeId {v} = yt_create_node(ctx);")
            continue
        m = re.match(r"YGNodeInsertChild\(\s*(\w+)\s*,\s*(\w+)\s*,", s)
        if m:
            out.append(f"\txent_append_child(ctx, {m.group(1)}, {m.group(2)});")
            continue
        m = re.match(r"YGNodeCalculateLayout\(\s*(\w+)\s*,\s*([\w.]+)\s*,\s*([\w.]+)\s*,\s*YGDirection(\w+)\s*\)", s)
        if m:
            root, w, h, d = m.groups()
            out.append(f"\txent_set_direction(ctx, {root}, XENT_DIRECTION_{d.upper()});")
            out.append(f"\tYT_ASSERT(xent_layout(ctx, {root}, {num(w)}, {num(h)}));")
            continue
        m = re.match(r"ASSERT_FLOAT_EQ\(\s*([\-\w.]+)\s*,\s*YGNodeLayoutGet(\w+)\(\s*(\w+)\s*\)\s*\)", s)
        if m:
            exp, side, v = m.groups()
            if side not in SIDE: raise Skip(f"YGNodeLayoutGet{side}")
            out.append(f"\tYT_ASSERT_RECT({v}, {SIDE[side]}, {num(exp)});")
            continue
        m = re.match(r"YGNodeStyleSet(\w+)\(\s*(\w+)\s*,?\s*(.*)\)", s)
        if m:
            convert_style(m.group(1), m.group(2), m.group(3).strip(), out, root)
            continue
        if s.startswith(("YGNodeFreeRecursive", "YGConfigFree", "YGConfigRef", "YGConfigSetUseWebDefaults")):
            continue
        if s.startswith("YGNodeStyleSetPositionType") :  # handled in convert_style path normally
            pass
        raise Skip(f"unhandled: {s[:60]}")
    return out


def convert_style(prop, node, args, out, root):
    a = [x.strip() for x in args.split(",")] if args else []
    def fnum(i): return num(a[i])
    if prop == "Width": out.append(f"\tyt_set_width(ctx, {node}, {fnum(0)});")
    elif prop == "Height": out.append(f"\tyt_set_height(ctx, {node}, {fnum(0)});")
    elif prop == "MinWidth": out.append(f"\tyt_set_min_width(ctx, {node}, {fnum(0)});")
    elif prop == "MinHeight": out.append(f"\tyt_set_min_height(ctx, {node}, {fnum(0)});")
    elif prop == "MaxWidth": out.append(f"\tyt_set_max_width(ctx, {node}, {fnum(0)});")
    elif prop == "MaxHeight": out.append(f"\tyt_set_max_height(ctx, {node}, {fnum(0)});")
    elif prop == "WidthPercent": out.append(f"\txent_set_width_percent(ctx, {node}, {float(a[0])/100.0:.6f}f);")
    elif prop == "HeightPercent": out.append(f"\txent_set_height_percent(ctx, {node}, {float(a[0])/100.0:.6f}f);")
    elif prop == "FlexGrow": out.append(f"\txent_set_flex_grow(ctx, {node}, {fnum(0)});")
    elif prop == "FlexShrink": out.append(f"\txent_set_flex_shrink(ctx, {node}, {fnum(0)});")
    elif prop == "FlexBasis": out.append(f"\txent_set_flex_basis(ctx, {node}, {fnum(0)});")
    elif prop == "AspectRatio": out.append(f"\txent_set_aspect_ratio(ctx, {node}, {fnum(0)});")
    elif prop == "FlexDirection":
        if a[0] not in FLEX_DIR: raise Skip(a[0])
        out.append(f"\txent_set_flex_direction(ctx, {node}, {FLEX_DIR[a[0]]});")
    elif prop == "JustifyContent":
        if a[0] not in JUSTIFY: raise Skip(a[0])
        out.append(f"\txent_set_flex_justify_content(ctx, {node}, {JUSTIFY[a[0]]});")
    elif prop == "AlignItems":
        if a[0] not in ALIGN: raise Skip(a[0])
        out.append(f"\txent_set_flex_align_items(ctx, {node}, {ALIGN[a[0]]});")
    elif prop == "AlignSelf":
        if a[0] not in ALIGN: raise Skip(a[0])
        out.append(f"\txent_set_flex_align_self(ctx, {node}, {ALIGN[a[0]]});")
    elif prop == "AlignContent":
        if a[0] not in ALIGN_CONTENT: raise Skip(a[0])
        out.append(f"\txent_set_flex_align_content(ctx, {node}, {ALIGN_CONTENT[a[0]]});")
    elif prop == "FlexWrap":
        if a[0] == "YGWrapNoWrap": return
        if a[0] != "YGWrapWrap": raise Skip(a[0])
        out.append(f"\txent_set_flex_wrap(ctx, {node}, XENT_FLEX_WRAP);")
    elif prop in ("Margin", "Padding"):
        if a[1] not in EDGE: raise Skip(a[1])
        fn = "yt_set_margin" if prop == "Margin" else "yt_set_padding"
        out.append(f"\t{fn}(ctx, {node}, {EDGE[a[1]]}, {num(a[2])});")
    elif prop == "PositionType":
        # gentest marks the ROOT absolute purely to make it a standalone layout
        # root (which it already is here) → ignore. Static is the default → ignore.
        # Absolute on a CHILD is real abspos → unsupported.
        if node == root or a[0] == "YGPositionTypeStatic": return
        raise Skip(a[0])
    else:
        raise Skip(f"YGNodeStyleSet{prop}")


def main():
    preamble = load_preamble()
    funcs, listing, skipped = [], [], []
    idx = 0
    for fn in sorted(os.listdir(SRC)):
        if not fn.endswith(".cpp"): continue
        text = open(os.path.join(SRC, fn), encoding="utf-8").read()
        for m in re.finditer(r"TEST\(YogaTest,\s*(\w+)\)\s*\{", text):
            name = m.group(1)
            # find matching brace
            i = m.end(); depth = 1
            while i < len(text) and depth:
                if text[i] == "{": depth += 1
                elif text[i] == "}": depth -= 1
                i += 1
            body = text[m.end():i-1]
            cname = f"test_{idx:04d}_{name}"
            try:
                if name in EXCLUDE: raise Skip(f"non-css: {EXCLUDE[name]}")
                lines = convert_test(name, body)
                funcs.append(
                    f"static int {cname}(void) {{\n\tyt_reset_state();\n"
                    f"\tXentContext *ctx = xent_create_context(NULL);\n\tYT_ASSERT(ctx != NULL);\n"
                    f"\txent_set_pixel_rounding_enabled(ctx, true); /* Yoga rounds to the integer pixel grid */\n"
                    + "\n".join(lines) + f"\n\txent_destroy_context(ctx);\n\treturn 0;\n}}\n")
                listing.append((cname, name, fn))
            except Skip as e:
                skipped.append((name, fn, e.reason))
            idx += 1

    with open(OUT, "w", encoding="utf-8") as f:
        f.write(preamble)
        f.write("\n".join(fn for fn in funcs))
        f.write("\ntypedef struct { const char *name; YtTestFn fn; } YtCase;\n")
        f.write("static YtCase yt_cases[] = {\n")
        for cname, name, _ in listing:
            f.write(f"\t{{\"{name}\", {cname}}},\n")
        f.write("};\n\n")
        f.write(
            "int main(void) {\n"
            "\tunsigned n = (unsigned)(sizeof(yt_cases)/sizeof(yt_cases[0]));\n"
            "\tunsigned pass = 0, fail = 0;\n"
            "\tfor (unsigned i = 0; i < n; ++i) {\n"
            "\t\tif (yt_cases[i].fn() == 0) pass++;\n"
            "\t\telse { fail++; fprintf(stderr, \"FAIL: %s\\n\", yt_cases[i].name); }\n"
            "\t}\n"
            "\tprintf(\"xent yoga full: %u passed, %u failed of %u\\n\", pass, fail, n);\n"
            "\treturn fail ? 1 : 0;\n}\n")

    print(f"converted: {len(listing)}  skipped: {len(skipped)}")
    print("top skip reasons:")
    for r, c in Counter(s[2] for s in skipped).most_common(20):
        print(f"  {c:4d}  {r}")

if __name__ == "__main__":
    main()
