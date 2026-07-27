#include "test_common.h"

int main(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root   = xent_node_create(ctx);
	XentNodeId child  = xent_node_create(ctx);
	XentNodeId child2 = xent_node_create(ctx);

	xent_setproto(ctx, root, XENT_PROTOCOL_FLEX);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setsize(ctx, root, (XentSize) {200.0f, 80.0f});
	xent_setsize(ctx, child, (XentSize) {50.0f, 50.0f});
	xent_setsize(ctx, child2, (XentSize) {50.0f, 50.0f});
	xent_node_append(ctx, root, child);
	xent_node_append(ctx, root, child2);

	TEST_ASSERT((xent_node_dirty(ctx, root) & XENT_DIRTY_SUBTREE) != 0u);
	TEST_ASSERT((xent_node_dirty(ctx, child) & XENT_DIRTY_SELF) != 0u);

	TEST_ASSERT(xent_layout(ctx, root, 200.0f, 80.0f));
	TEST_ASSERT(xent_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_FULL);

	TEST_ASSERT(xent_node_dirty(ctx, root) == 0u);
	TEST_ASSERT(xent_node_dirty(ctx, child) == 0u);

	TEST_ASSERT(xent_layout(ctx, root, 200.0f, 80.0f));
	TEST_ASSERT(xent_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_NONE);

	xent_setsize(ctx, child, (XentSize) {60.0f, 50.0f});
	TEST_ASSERT((xent_node_dirty(ctx, child) & XENT_DIRTY_LAYOUT) != 0u);
	TEST_ASSERT((xent_node_dirty(ctx, root) & XENT_DIRTY_SUBTREE) != 0u);
	TEST_ASSERT(xent_layout(ctx, root, 200.0f, 80.0f));
	XentRect child2_rect = {0};
	TEST_ASSERT(xent_layout_rect(ctx, child2, &child2_rect));
	TEST_ASSERT(test_float_near(child2_rect.x, 60.0f, 0.5f));
	TEST_ASSERT(xent_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_FULL);

	xent_setsize(ctx, child, (XentSize) {70.0f, 50.0f});
	xent_setsize(ctx, child2, (XentSize) {70.0f, 50.0f});
	TEST_ASSERT(xent_layout(ctx, root, 200.0f, 80.0f));
	TEST_ASSERT(xent_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_FULL);

	TEST_ASSERT(xent_layout(ctx, root, 220.0f, 80.0f));
	TEST_ASSERT(xent_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_FULL);

	TEST_ASSERT(xent_setflexdir(ctx, root, XENT_FLEX_COLUMN));
	XentNodeId group = xent_node_create(ctx);
	XentNodeId leaf  = xent_node_create(ctx);
	XentNodeId tail  = xent_node_create(ctx);
	TEST_ASSERT(xent_setproto(ctx, group, XENT_PROTOCOL_FLEX));
	TEST_ASSERT(xent_setflexdir(ctx, group, XENT_FLEX_ROW));
	TEST_ASSERT(xent_setsize(ctx, group, (XentSize) {120.0f, 30.0f}));
	TEST_ASSERT(xent_setsize(ctx, leaf, (XentSize) {20.0f, 20.0f}));
	TEST_ASSERT(xent_setsize(ctx, tail, (XentSize) {20.0f, 20.0f}));
	TEST_ASSERT(xent_node_append(ctx, group, leaf));
	TEST_ASSERT(xent_node_append(ctx, group, tail));
	TEST_ASSERT(xent_node_append(ctx, root, group));
	TEST_ASSERT(xent_layout(ctx, root, 220.0f, 80.0f));

	TEST_ASSERT(xent_setsize(ctx, leaf, (XentSize) {30.0f, 20.0f}));
	TEST_ASSERT(xent_layout(ctx, root, 220.0f, 80.0f));
	XentRect tail_rect = {0};
	TEST_ASSERT(xent_layout_rect(ctx, tail, &tail_rect));
	TEST_ASSERT(test_float_near(tail_rect.x, 30.0f, 0.5f));
	TEST_ASSERT(xent_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_DIRTY);

	xent_ctx_destroy(ctx);
	return 0;
}
