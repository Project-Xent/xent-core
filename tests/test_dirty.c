#include "test_common.h"

int main(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root   = xent_create_node(ctx);
	XentNodeId child  = xent_create_node(ctx);
	XentNodeId child2 = xent_create_node(ctx);

	xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
	xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
	xent_set_size(ctx, root, (XentSize) {200.0f, 80.0f});
	xent_set_size(ctx, child, (XentSize) {50.0f, 50.0f});
	xent_set_size(ctx, child2, (XentSize) {50.0f, 50.0f});
	xent_append_child(ctx, root, child);
	xent_append_child(ctx, root, child2);

	TEST_ASSERT((xent_get_dirty_flags(ctx, root) & XENT_DIRTY_SUBTREE) != 0u);
	TEST_ASSERT((xent_get_dirty_flags(ctx, child) & XENT_DIRTY_SELF) != 0u);

	TEST_ASSERT(xent_layout(ctx, root, 200.0f, 80.0f));
	TEST_ASSERT(xent_get_last_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_FULL_TREE);

	TEST_ASSERT(xent_get_dirty_flags(ctx, root) == 0u);
	TEST_ASSERT(xent_get_dirty_flags(ctx, child) == 0u);

	TEST_ASSERT(xent_layout(ctx, root, 200.0f, 80.0f));
	TEST_ASSERT(xent_get_last_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_NONE);

	xent_set_size(ctx, child, (XentSize) {60.0f, 50.0f});
	TEST_ASSERT((xent_get_dirty_flags(ctx, child) & XENT_DIRTY_LAYOUT) != 0u);
	TEST_ASSERT((xent_get_dirty_flags(ctx, root) & XENT_DIRTY_SUBTREE) != 0u);
	TEST_ASSERT(xent_layout(ctx, root, 200.0f, 80.0f));
	TEST_ASSERT(xent_get_last_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_DIRTY_SUBTREE);

	xent_set_size(ctx, child, (XentSize) {70.0f, 50.0f});
	xent_set_size(ctx, child2, (XentSize) {70.0f, 50.0f});
	TEST_ASSERT(xent_layout(ctx, root, 200.0f, 80.0f));
	TEST_ASSERT(xent_get_last_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_FULL_TREE);

	TEST_ASSERT(xent_layout(ctx, root, 220.0f, 80.0f));
	TEST_ASSERT(xent_get_last_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_FULL_TREE);

	xent_destroy_context(ctx);
	return 0;
}
