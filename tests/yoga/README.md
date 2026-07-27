# Yoga Correctness Tests

xent-native C ports of Yoga's generated layout correctness cases, exercising the
public xent API for CSS conformance. The imported upstream snapshot contains 590
cases; neither executable below represents all 590.

Both runnable targets are wired into `xmake test`.

Files:
- `test_yoga_generated.c`: the manually accepted 81-case native corpus.
- `yoga_generated_manifest.json`: its 81 accepted and 509 skipped cases.
- `test_yoga_supported.c`: the 196 cases expressible by the current converter.
- `LICENSE-yoga`: upstream Yoga MIT license for the ported test source.

Unsupported Yoga APIs and intentional semantic differences are not compiled by
the corresponding corpus. A 196/196 result therefore means every case admitted
by the current converter passed; it is not a claim that all 590 upstream cases
passed.

`test_yoga_supported.c` is an offline-generated fixture. Regenerate it from a
Yoga checkout with:

```text
xmake gen-yoga --source=/path/to/yoga/tests/generated
```

The Lua generator requires clang-format 22.1.2 and rejects Yoga cases whose
operations cannot be represented by the current public xent API.
