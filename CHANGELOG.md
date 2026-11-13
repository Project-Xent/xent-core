# Changelog

## 0.3.1

- added `xent_setscroller`: a flex container marked as a scroller lays its
  children out without shrinking them along the main axis, so content keeps its
  own extent instead of being compressed to the viewport;
- added style runs to text measurement: a request may carry a run array that
  tiles the text exactly, each run naming its own family, size, weight, stretch
  and slant;
- added text trimming policies and a line limit, and pinned when each is
  observable — never under an unconstrained or intrinsic measurement, and only
  where wrapping cannot absorb the overflow;
- added a `shape` backend slot producing positioned glyph runs through a
  Core-owned sink, plus `xent_text_shape()` and `xent_text_freeshape()`;
- added `first_baseline` to `XentTextMetrics`, and made baseline alignment use
  it instead of assuming a text node's baseline sat at 80% of its height;
- added `xent_setfontfamily`, `xent_settexttrim`, `xent_setmaxlines` and
  `xent_settextruns`; `xent_settext` now clears the node's runs, which index it;
- extended the text-cache key with the run array, family, trimming policy and
  line limit;
- added `XENT_ALLOC_TEXT` and `XENT_ALLOC_SEMANTICS` allocation sites and routed
  the text subsystem through the former, so allocation failures in text content,
  families, runs, the metrics cache and shaping are reachable by fault
  injection.

### Renamed

Public names now follow `<pkg>_<module>_<verbobject>` with `comp`/`ctrl` abbreviations.

| Old | New |
|---|---|
| `xent_dl_drawtext_style` | `xent_dl_drawtextstyle` |
| `xent_dl_fillrect_grad` | `xent_dl_fillrectgrad` |
| `xent_dl_pushclip_ellipse` | `xent_dl_pushclipellipse` |
| `xent_dl_pushclip_rect` | `xent_dl_pushcliprect` |
| `xent_dl_pushclip_rrect` | `xent_dl_pushcliprrect` |
| `xent_dl_strokerrect_grad` | `xent_dl_strokerrectgrad` |
| `xent_dump_layout_json` | `xent_dump_layoutjson` |
| `xent_dump_layout_text` | `xent_dump_layouttext` |
| `xent_dump_semantics_text` | `xent_dump_semanticstext` |
| `xent_is_simd_enabled` | `xent_simd_enabled` |
| `xent_make_node_id` | `xent_node_make` |
| `xent_set_tab_index` | `xent_set_tabindex` |
| `xent_get_tab_index` | `xent_get_tabindex` |
| `xent_text_backend_valid` | `xent_text_backendvalid` |

## 0.3.0

- made `XentNodeId` a generation-checked handle and retired slots whose
  generation would wrap;
- added ordered insert, move, and reparent operations;
- replaced node userdata/tag storage and the plugin registry with consumer
  sidecars and allocation-free lifecycle observers;
- added external measurement and immutable display-list contracts;
- made temporary layout storage use the context scratch allocator and exposed
  accurate profiling;
- promoted the 163-case Yoga conformance corpus and allocation fault injection
  into the test gate;
- made the ISPC backend a supported xmake configuration with scalar-equivalence
  tests;
- removed the unused shaping façade; shaping and drawing are Host concerns.
