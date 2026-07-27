local root = path.absolute(path.join(os.scriptdir(), ".."))
local harness = path.join(root, "tests", "yoga", "test_yoga_generated.c")
local formatter_version = "22.1.2"
local src
local out

local flex_dir = {
  YGFlexDirectionRow = "XENT_FLEX_ROW",
  YGFlexDirectionColumn = "XENT_FLEX_COLUMN",
}

local justify = {
  YGJustifyFlexStart = "XENT_FLEX_JUSTIFY_START",
  YGJustifyFlexEnd = "XENT_FLEX_JUSTIFY_END",
  YGJustifyCenter = "XENT_FLEX_JUSTIFY_CENTER",
  YGJustifySpaceBetween = "XENT_FLEX_JUSTIFY_SPACE_BETWEEN",
  YGJustifySpaceAround = "XENT_FLEX_JUSTIFY_SPACE_AROUND",
  YGJustifySpaceEvenly = "XENT_FLEX_JUSTIFY_SPACE_EVENLY",
}

local align = {
  YGAlignFlexStart = "XENT_FLEX_ALIGN_START",
  YGAlignFlexEnd = "XENT_FLEX_ALIGN_END",
  YGAlignCenter = "XENT_FLEX_ALIGN_CENTER",
  YGAlignStretch = "XENT_FLEX_ALIGN_STRETCH",
  YGAlignBaseline = "XENT_FLEX_ALIGN_BASELINE",
}

local align_content = {
  YGAlignFlexStart = "XENT_FLEX_ALIGN_CONTENT_START",
  YGAlignFlexEnd = "XENT_FLEX_ALIGN_CONTENT_END",
  YGAlignCenter = "XENT_FLEX_ALIGN_CONTENT_CENTER",
  YGAlignSpaceBetween = "XENT_FLEX_ALIGN_CONTENT_SPACE_BETWEEN",
  YGAlignSpaceAround = "XENT_FLEX_ALIGN_CONTENT_SPACE_AROUND",
  YGAlignSpaceEvenly = "XENT_FLEX_ALIGN_CONTENT_SPACE_EVENLY",
  YGAlignStretch = "XENT_FLEX_ALIGN_CONTENT_STRETCH",
}

local edge = {
  YGEdgeLeft = "YT_EDGE_LEFT",
  YGEdgeTop = "YT_EDGE_TOP",
  YGEdgeRight = "YT_EDGE_RIGHT",
  YGEdgeBottom = "YT_EDGE_BOTTOM",
  YGEdgeAll = "YT_EDGE_ALL",
  YGEdgeHorizontal = "YT_EDGE_HORIZONTAL",
  YGEdgeVertical = "YT_EDGE_VERTICAL",
}

local side = {
  Left = "x",
  Top = "y",
  Width = "w",
  Height = "h",
}

local exclude = {
  nested_overflowing_child_in_constraint_parent
    = "yoga shrink:0 (CSS auto-min=0 for empty explicit item)",
  flex_wrap_align_stretch_fits_one_row
    = "yoga single-line align-content (CSS stretches the line to fill)",
}

local number_props = {
  Width = "yt_set_width",
  Height = "yt_set_height",
  MinWidth = "yt_set_min_width",
  MinHeight = "yt_set_min_height",
  MaxWidth = "yt_set_max_width",
  MaxHeight = "yt_set_max_height",
  FlexGrow = "xent_setgrow",
  FlexShrink = "xent_setshrink",
  FlexBasis = "xent_setbasis",
  AspectRatio = "xent_setaspect",
}

local enum_props = {
  FlexDirection = {"xent_setflexdir", flex_dir},
  JustifyContent = {"xent_setjustify", justify},
  AlignItems = {"xent_setitems", align},
  AlignSelf = {"xent_setself", align},
  AlignContent = {"xent_setcontent", align_content},
}

local function trim(text)
  return (text:gsub("^%s+", ""):gsub("%s+$", ""))
end

local function normalize(text)
  return text:gsub("\r\n", "\n")
end

local function split(text, sep)
  local values = {}
  for value in (text .. sep):gmatch("(.-)" .. sep) do
    values[#values + 1] = trim(value)
  end
  return values
end

local function skip(reason)
  raise("__YOGA_SKIP__" .. reason)
end

local function num(token)
  token = trim(token)
  if token == "YGUndefined" or token == "YGValueUndefined" then return "NAN" end
  if token:match("[fF]$") then token = token:sub(1, -2) end
  local value = tonumber(token)
  if not value then skip("number: " .. token) end
  return string.format("%.6ff", value)
end

local function emit_number(prop, node, args, lines)
  local fn = number_props[prop]
  if not fn then return false end
  lines[#lines + 1] = string.format("\t%s(ctx, %s, %s);", fn, node, num(args[1]))
  return true
end

local function emit_percent(prop, node, args, lines)
  local fn
  if prop == "WidthPercent" then fn = "xent_setwpct" end
  if prop == "HeightPercent" then fn = "xent_sethpct" end
  if not fn then return false end
  local value = tonumber(args[1])
  if not value then skip("number: " .. tostring(args[1])) end
  lines[#lines + 1] = string.format("\t%s(ctx, %s, %.6ff);", fn, node, value / 100.0)
  return true
end

local function emit_enum(prop, node, args, lines)
  local spec = enum_props[prop]
  if not spec then return false end
  local value = spec[2][args[1]]
  if not value then skip(args[1]) end
  lines[#lines + 1] = string.format("\t%s(ctx, %s, %s);", spec[1], node, value)
  return true
end

local function emit_wrap(node, args, lines)
  if args[1] == "YGWrapNoWrap" then return end
  if args[1] ~= "YGWrapWrap" then skip(args[1]) end
  lines[#lines + 1] = string.format("\txent_setflexwrap(ctx, %s, XENT_FLEX_WRAP);", node)
end

local function emit_edge(prop, node, args, lines)
  local value = edge[args[1]]
  if not value then skip(args[1]) end
  local fn = prop == "Margin" and "yt_set_margin" or "yt_set_padding"
  lines[#lines + 1] = string.format("\t%s(ctx, %s, %s, %s);", fn, node, value, num(args[2]))
end

local function convert_style(prop, node, text, lines, root_node)
  local args = text == "" and {} or split(text, ",")
  if emit_number(prop, node, args, lines) then return end
  if emit_percent(prop, node, args, lines) then return end
  if emit_enum(prop, node, args, lines) then return end
  if prop == "FlexWrap" then
    emit_wrap(node, args, lines)
    return
  end
  if prop == "Margin" or prop == "Padding" then
    if prop == "Margin" and node == root_node then skip("root margin position") end
    emit_edge(prop, node, args, lines)
    return
  end
  if prop == "PositionType" then
    if node == root_node or args[1] == "YGPositionTypeStatic" then return end
    skip(args[1])
  end
  skip("YGNodeStyleSet" .. prop)
end

local function convert_statement(statement, state)
  local node = statement:match("^YGNodeRef%s+([%w_]+)%s*=%s*YGNodeNewWithConfig")
  if node then
    if not state.root then state.root = node end
    state.lines[#state.lines + 1] = string.format("\tXentNodeId %s = yt_create_node(ctx);", node)
    return
  end

  local parent, child = statement:match("^YGNodeInsertChild%(%s*([%w_]+)%s*,%s*([%w_]+)%s*,")
  if parent then
    state.lines[#state.lines + 1] = string.format("\txent_node_append(ctx, %s, %s);", parent, child)
    return
  end

  local layout_root, width, height, direction
    = statement:match("^YGNodeCalculateLayout%(%s*([%w_]+)%s*,%s*([%w%.]+)%s*,%s*([%w%.]+)%s*,%s*YGDirection([%w_]+)%s*%)$")
  if layout_root then
    state.root = layout_root
    state.lines[#state.lines + 1]
      = string.format("\txent_setdir(ctx, %s, XENT_DIRECTION_%s);", layout_root, direction:upper())
    state.lines[#state.lines + 1]
      = string.format("\tYT_ASSERT(xent_layout(ctx, %s, %s, %s));", layout_root, num(width), num(height))
    return
  end

  local expected, member, value
    = statement:match("^ASSERT_FLOAT_EQ%(%s*([%-%w%.]+)%s*,%s*YGNodeLayoutGet([%w_]+)%(%s*([%w_]+)%s*%)%s*%)$")
  if expected then
    if not side[member] then skip("YGNodeLayoutGet" .. member) end
    state.lines[#state.lines + 1]
      = string.format("\tYT_ASSERT_RECT(%s, %s, %s);", value, side[member], num(expected))
    return
  end

  local prop, style_node, args
    = statement:match("^YGNodeStyleSet([%w_]+)%(%s*([%w_]+)%s*,?%s*(.*)%)$")
  if prop then
    convert_style(prop, style_node, trim(args), state.lines, state.root)
    return
  end

  if statement:match("^YGNodeFreeRecursive")
    or statement:match("^YGConfigFree")
    or statement:match("^YGConfigRef")
    or statement:match("^YGConfigSetUseWebDefaults") then
    return
  end
  skip("unhandled: " .. statement:sub(1, 60))
end

local function convert_test(body)
  local state = {root = nil, lines = {}}
  body = normalize(body):gsub("\n", " ")
  for statement in body:gmatch("([^;]+)") do
    statement = trim(statement)
    if statement ~= "" then convert_statement(statement, state) end
  end
  return state.lines
end

local function replace_plain(text, old, new)
  local start = text:find(old, 1, true)
  assert(start, "generator harness fragment was not found")
  return text:sub(1, start - 1) .. new .. text:sub(start + #old)
end

local function load_preamble()
  local text = normalize(io.readfile(harness))
  local finish = assert(text:find("static%s+int%s+test_[%w_]+%s*%("), "test harness preamble was not found")
  local preamble = text:sub(1, finish - 1)
  preamble = replace_plain(
    preamble,
    "\txent_setflexdir(ctx, node, XENT_FLEX_COLUMN);\n\treturn node;",
    "\txent_setflexdir(ctx, node, XENT_FLEX_COLUMN);\n"
      .. "\txent_setshrink(ctx, node, 0.0f); /* Yoga default; see comment above */\n"
      .. "\treturn node;"
  )

  local macro_start = assert(preamble:find("#define YT_ASSERT_RECT", 1, true))
  local macro_end = assert(preamble:find("while (0)\n", macro_start, true)) + #"while (0)" - 1
  local macro = "#define YT_ASSERT_RECT(node, member, expected) do { XentRect rect = {0}; "
    .. "YT_ASSERT(xent_layout_rect(ctx, node, &rect)); float _v = rect.member; "
    .. "XentNodeId _p = xent_node_parent(ctx, node); "
    .. "if ((#member[0] == 'x' || #member[0] == 'y') && _p != XENT_NODE_INVALID) { "
    .. "XentRect _pr = {0}; xent_layout_rect(ctx, _p, &_pr); "
    .. "_v -= ((#member[0] == 'x') ? _pr.x : _pr.y); } "
    .. "if (fabsf(_v - (expected)) > YT_EPS) { fprintf(stderr, "
    .. "\"RECT FAILED: %s.%s expected %.3f got %.3f (%s:%d)\\n\", #node, #member, "
    .. "(double)(expected), (double)_v, __FILE__, __LINE__); return 1; } } while (0)"
  return preamble:sub(1, macro_start - 1) .. macro .. preamble:sub(macro_end + 1)
end

local function test_end(text, start, name)
  local depth = 1
  local index = start
  while index <= #text and depth > 0 do
    local ch = text:sub(index, index)
    if ch == "{" then depth = depth + 1 end
    if ch == "}" then depth = depth - 1 end
    index = index + 1
  end
  assert(depth == 0, "unclosed Yoga test: " .. name)
  return index
end

local function test_bodies(text)
  local tests = {}
  local cursor = 1
  while true do
    local start, finish, name = text:find("TEST%(YogaTest,%s*([%w_]+)%)%s*%{", cursor)
    if not start then return tests end
    local tail = test_end(text, finish + 1, name)
    tests[#tests + 1] = {name = name, body = text:sub(finish + 1, tail - 2)}
    cursor = tail
  end
end

local function test_function(cname, lines)
  return "static int " .. cname .. "(void) {\n"
    .. "\tyt_reset_state();\n"
    .. "\tXentCtx *ctx = xent_ctx_create(NULL);\n"
    .. "\tYT_ASSERT(ctx != NULL);\n"
    .. "\txent_setrounding(ctx, true); /* Yoga rounds to the integer pixel grid */\n"
    .. table.concat(lines, "\n")
    .. "\n\txent_ctx_destroy(ctx);\n"
    .. "\treturn 0;\n"
    .. "}\n"
end

local function output_tail(listing)
  local lines = {
    "typedef struct { const char *name; YtTestFn fn; } YtCase;",
    "static YtCase yt_cases[] = {",
  }
  for _, item in ipairs(listing) do
    lines[#lines + 1] = string.format("\t{\"%s\", %s},", item.name, item.cname)
  end
  lines[#lines + 1] = "};"
  lines[#lines + 1] = ""
  lines[#lines + 1] = "int main(void) {"
  lines[#lines + 1] = "\tunsigned n = (unsigned)(sizeof(yt_cases)/sizeof(yt_cases[0]));"
  lines[#lines + 1] = "\tunsigned pass = 0, fail = 0;"
  lines[#lines + 1] = "\tfor (unsigned i = 0; i < n; ++i) {"
  lines[#lines + 1] = "\t\tif (yt_cases[i].fn() == 0) pass++;"
  lines[#lines + 1] = "\t\telse { fail++; fprintf(stderr, \"FAIL: %s\\n\", yt_cases[i].name); }"
  lines[#lines + 1] = "\t}"
  lines[#lines + 1]
    = "\tprintf(\"xent yoga supported: %u passed, %u failed of %u\\n\", pass, fail, n);"
  lines[#lines + 1] = "\treturn fail ? 1 : 0;"
  lines[#lines + 1] = "}"
  lines[#lines + 1] = ""
  return table.concat(lines, "\n")
end

local function format_output()
  local formatter = os.getenv("CLANG_FORMAT") or "clang-format"
  local version = os.iorunv(formatter, {"--version"})
  assert(version:find("version " .. formatter_version, 1, true),
    "clang-format " .. formatter_version .. " required: " .. trim(version))
  os.execv(formatter, {"--style=file:" .. path.join(root, ".clang-format"), "-i", out})
end

local function generate()
  local files = os.files(path.join(path.absolute(src), "*.cpp"))
  table.sort(files)

  local functions = {}
  local listing = {}
  local skipped = {}
  local index = 0
  for _, filename in ipairs(files) do
    local text = normalize(io.readfile(filename))
    for _, test in ipairs(test_bodies(text)) do
      local cname = string.format("test_%04d_%s", index, test.name)
      local result
      local failure
      if exclude[test.name] then
        failure = "__YOGA_SKIP__non-css: " .. exclude[test.name]
      else
        try {
          function() result = convert_test(test.body) end,
          catch {function(errors) failure = tostring(errors) end},
        }
      end
      if result then
        functions[#functions + 1] = test_function(cname, result)
        listing[#listing + 1] = {cname = cname, name = test.name}
      else
        local reason = failure and failure:match("__YOGA_SKIP__([^\r\n]*)")
        if not reason then raise(failure or "Yoga conversion failed") end
        skipped[#skipped + 1] = {reason = reason}
      end
      index = index + 1
    end
  end

  io.writefile(
    out,
    "/* Generated by tools/gen_yoga.lua from Yoga's tests/generated corpus. */\n"
      .. load_preamble()
      .. table.concat(functions, "\n")
      .. "\n"
      .. output_tail(listing)
  )
  format_output()

  local counts = {}
  for _, item in ipairs(skipped) do counts[item.reason] = (counts[item.reason] or 0) + 1 end
  local reasons = {}
  for reason, count in pairs(counts) do reasons[#reasons + 1] = {reason = reason, count = count} end
  table.sort(reasons, function(a, b)
    if a.count ~= b.count then return a.count > b.count end
    return a.reason < b.reason
  end)

  print(string.format("converted: %d  skipped: %d", #listing, #skipped))
  print("top skip reasons:")
  for i = 1, math.min(20, #reasons) do
    print(string.format("  %4d  %s", reasons[i].count, reasons[i].reason))
  end
end

function main(...)
  local argv = {...}
  src = argv[1]
  assert(src and src ~= "", "usage: xmake lua tools/gen_yoga.lua <yoga-generated-dir> [out.c]")
  out = argv[2] or path.join(root, "tests", "yoga", "test_yoga_supported.c")
  generate()
end
