# Yoga Correctness Tests

xent-native C ports of Yoga's generated layout correctness cases, exercising the
public xent API for CSS conformance.

The runnable target is `test_yoga_generated`, wired into `xmake test`.

Files:
- `test_yoga_generated.c`: native C conformance cases using the public xent API.
- `yoga_generated_manifest.json`: case manifest listing converted and skipped cases.
- `LICENSE-yoga`: upstream Yoga MIT license for the ported test source.

Only cases expressible against current xent semantics and passing native
validation are executable here. Unsupported API features and intentional
semantic differences (xent follows CSS defaults) are recorded as skipped cases
in the manifest rather than kept as failing tests.

## Converter

`convert_yoga.py` parses Yoga's `generated/YG*Test.cpp` cases and emits
`test_yoga_full.c` (target `test_yoga_full`). It reports pass/fail counts rather
than all-pass, so it is not part of `xmake test`. Regenerate and run:

```
python tests/yoga/convert_yoga.py <path-to-yoga>/tests/generated
xmake build test_yoga_full && xmake run test_yoga_full
```
