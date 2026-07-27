# xent-core

The platform-neutral C23 foundation of Project Xent. It owns the generation-safe
node tree, layout, dirty propagation, intrinsic measurement, text measurement,
semantics, focus, lifecycle observers, and immutable display lists. It has no
platform headers or external runtime dependency.

Text shaping, rasterization, native input, and accessibility bridges belong to
the platform host. Fluxent, for example, measures and renders text with
DirectWrite.

## Contracts

- `XentNodeId` detects stale handles after a slot is reused.
- `ABSOLUTE`, `FLEX`, `SWIFTSTACK`, and `GRID` are the built-in layout
  protocols.
- external content registers a measurement callback by full node handle;
  destruction removes the registration.
- display-list builders copy commands and payloads into an immutable,
  platform-neutral list. Resources are opaque `XentResourceId` cookies.
- node lifecycle notifications are allocation-free and support multiple
  observers. Consumers keep their own sidecar stores.
- the text backend measures only. The built-in backend is a deterministic
  monospace fallback; hosts install a platform measurer.

## Example

```c
XentCtx *ctx = xent_ctx_create(NULL);
XentNodeId root = xent_node_create(ctx);
xent_setproto(ctx, root, XENT_PROTOCOL_FLEX);
xent_layout(ctx, root, width, height);

XentRect r;
xent_layout_rect(ctx, root, &r);
xent_ctx_destroy(ctx);
```

## Build

```sh
xmake
xmake test
xmake run demo_basic
```

The scalar implementation is always available. The supported ISPC
configuration is:

```sh
xmake f -m debug --ispc=y --simd=y --fault_injection=y
xmake test
```

`--ispc=y` requires `ispc` on `PATH`; `--simd=y` selects the SIMD path in the
default configuration. `XentCfg.enable_simd` can still disable it at runtime.
CI runs both scalar and ISPC configurations. The current 163-case
converter-supported Yoga corpus and allocation-failure tests are part of the
ordinary gate. The Yoga snapshot contains 590 cases; unsupported cases are not
included in that 163-case result.

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
