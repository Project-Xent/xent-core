# Migrating xent-core code to 0.3

`XentNodeId` is an opaque 64-bit generation handle. Do not store it in
`uint32_t`, use it as an array index, or derive an index yourself. Public APIs
validate the full handle; internal consumers that own a dense sidecar may use
`xent_node_index` only while the handle is known live.

Node `userdata` and tag APIs are gone. Keep consumer state in a sidecar keyed by
the full handle and subscribe to node destruction to remove it.

`XentPlugin`, plugin frame hooks, and plugin semantic broadcasts are gone.
Diagnostics call the CLI dump API directly. IME belongs to the platform input
layer. Narrow node lifecycle integrations use observers.

The old shaping callback/result/cache surface is gone. Install a
`XentTextBackend` for measurement and perform shaping/rasterization in the
platform Host.

External content registers a measurement callback through `xent_measure.h`.
Custom drawing records an immutable platform-neutral list through
`xent_display.h`; backend objects must not be stored in it.

Code that changes child order should use `xent_insert_before`,
`xent_move_before`, or `xent_reparent_before` instead of rebuilding a subtree.
