# xent-core

Pure-C11 layout, tree, text-shaping and semantics engine — the platform-independent
core of the Xent UI stack. No platform headers, no external deps; builds standalone.

It computes geometry only (box rects + shaped glyph positions); drawing is left to a
backend (e.g. FluXent on Windows).

## Layout protocols

`XentProtocol`: `ABSOLUTE`, `FLEX`, `SWIFTSTACK`, `GRID`.

## Text

Measurement/shaping goes through an injectable `XentTextBackend` (measure + shape
function pointers). The default is a platform-independent monospace fallback; a host
injects a real backend (FluXent uses DirectWrite) via `xent_set_text_backend`.

## Use

```c
XentContext *ctx = xent_create_context(NULL);
XentNodeId root  = xent_create_node(ctx);
xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
xent_layout(ctx, root, width, height);

XentRect r;
xent_get_layout_rect(ctx, node, &r);   // {x, y, w, h}
```

## Build

```sh
xmake                 # build the static lib
xmake test            # run the unit-test gate
xmake run demo_basic  # a demo
```

Config options: `--simd=y` (experimental SIMD scaffolding), `--ispc=y` (ISPC backend,
needs `ispc` on PATH).

## Consume (xmake)

```lua
add_repositories("xent-repo https://github.com/Project-Xent/xent-repo.git")
add_requires("xent-core")

target("app")
    add_files("src/*.c")
    add_packages("xent-core")
```

## License

0BSD — see [LICENSE](LICENSE).
