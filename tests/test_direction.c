#include "test_common.h"

static int test_direction(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root       = xent_create_node(ctx);
	XentNodeId child      = xent_create_node(ctx);
	XentNodeId grandchild = xent_create_node(ctx);
	TEST_ASSERT(root != XENT_NODE_INVALID && child != XENT_NODE_INVALID && grandchild != XENT_NODE_INVALID);
	TEST_ASSERT(xent_append_child(ctx, root, child));
	TEST_ASSERT(xent_append_child(ctx, child, grandchild));

	TEST_ASSERT(xent_get_direction(ctx, child) == XENT_DIRECTION_INHERIT);
	TEST_ASSERT(xent_get_resolved_direction(ctx, grandchild) == XENT_DIRECTION_LTR);

	TEST_ASSERT(xent_set_direction(ctx, root, XENT_DIRECTION_RTL));
	TEST_ASSERT(xent_get_resolved_direction(ctx, root) == XENT_DIRECTION_RTL);
	TEST_ASSERT(xent_get_resolved_direction(ctx, child) == XENT_DIRECTION_RTL);
	TEST_ASSERT(xent_get_resolved_direction(ctx, grandchild) == XENT_DIRECTION_RTL);

	TEST_ASSERT(xent_set_direction(ctx, child, XENT_DIRECTION_LTR));
	TEST_ASSERT(xent_get_resolved_direction(ctx, child) == XENT_DIRECTION_LTR);
	TEST_ASSERT(xent_get_resolved_direction(ctx, grandchild) == XENT_DIRECTION_LTR);

	xent_destroy_context(ctx);

	ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	root            = xent_create_node(ctx);
	XentNodeId node = xent_create_node(ctx);
	TEST_ASSERT(root != XENT_NODE_INVALID && node != XENT_NODE_INVALID);
	TEST_ASSERT(xent_append_child(ctx, root, node));

	TEST_ASSERT(xent_set_direction(ctx, root, XENT_DIRECTION_RTL));
	TEST_ASSERT(xent_set_margin(ctx, node, (XentInsets) {1.0f, 2.0f, 3.0f, 4.0f}));
	TEST_ASSERT(xent_set_padding(ctx, node, (XentInsets) {5.0f, 6.0f, 7.0f, 8.0f}));

	XentResolvedInsets resolved = {0};
	TEST_ASSERT(xent_get_resolved_margin(ctx, node, XENT_AXIS_HORIZONTAL, &resolved));
	TEST_ASSERT(test_float_near(resolved.main_start, 3.0f, 0.001f));
	TEST_ASSERT(test_float_near(resolved.main_end, 1.0f, 0.001f));
	TEST_ASSERT(test_float_near(resolved.cross_start, 2.0f, 0.001f));
	TEST_ASSERT(test_float_near(resolved.cross_end, 4.0f, 0.001f));

	TEST_ASSERT(xent_get_resolved_padding(ctx, node, XENT_AXIS_VERTICAL, &resolved));
	TEST_ASSERT(test_float_near(resolved.main_start, 6.0f, 0.001f));
	TEST_ASSERT(test_float_near(resolved.main_end, 8.0f, 0.001f));
	TEST_ASSERT(test_float_near(resolved.cross_start, 7.0f, 0.001f));
	TEST_ASSERT(test_float_near(resolved.cross_end, 5.0f, 0.001f));

	xent_destroy_context(ctx);
	return 0;
}

int main(void) {
	return test_direction();
}
