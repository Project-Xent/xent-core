#include "test_common.h"

static int test_direction(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root       = xent_node_create(ctx);
	XentNodeId child      = xent_node_create(ctx);
	XentNodeId grandchild = xent_node_create(ctx);
	TEST_ASSERT(root != XENT_NODE_INVALID && child != XENT_NODE_INVALID && grandchild != XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_append(ctx, root, child));
	TEST_ASSERT(xent_node_append(ctx, child, grandchild));

	TEST_ASSERT(xent_dir(ctx, child) == XENT_DIRECTION_INHERIT);
	TEST_ASSERT(xent_resolved_dir(ctx, grandchild) == XENT_DIRECTION_LTR);

	TEST_ASSERT(xent_setdir(ctx, root, XENT_DIRECTION_RTL));
	TEST_ASSERT(xent_resolved_dir(ctx, root) == XENT_DIRECTION_RTL);
	TEST_ASSERT(xent_resolved_dir(ctx, child) == XENT_DIRECTION_RTL);
	TEST_ASSERT(xent_resolved_dir(ctx, grandchild) == XENT_DIRECTION_RTL);

	TEST_ASSERT(xent_setdir(ctx, child, XENT_DIRECTION_LTR));
	TEST_ASSERT(xent_resolved_dir(ctx, child) == XENT_DIRECTION_LTR);
	TEST_ASSERT(xent_resolved_dir(ctx, grandchild) == XENT_DIRECTION_LTR);

	xent_ctx_destroy(ctx);

	ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	root            = xent_node_create(ctx);
	XentNodeId node = xent_node_create(ctx);
	TEST_ASSERT(root != XENT_NODE_INVALID && node != XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_append(ctx, root, node));

	TEST_ASSERT(xent_setdir(ctx, root, XENT_DIRECTION_RTL));
	TEST_ASSERT(xent_setm(ctx, node, (XentInsets) {1.0f, 2.0f, 3.0f, 4.0f}));
	TEST_ASSERT(xent_setp(ctx, node, (XentInsets) {5.0f, 6.0f, 7.0f, 8.0f}));

	XentResolvedInsets resolved = {0};
	TEST_ASSERT(xent_resolved_m(ctx, node, XENT_AXIS_HORIZONTAL, &resolved));
	TEST_ASSERT(test_float_near(resolved.main_start, 3.0f, 0.001f));
	TEST_ASSERT(test_float_near(resolved.main_end, 1.0f, 0.001f));
	TEST_ASSERT(test_float_near(resolved.cross_start, 2.0f, 0.001f));
	TEST_ASSERT(test_float_near(resolved.cross_end, 4.0f, 0.001f));

	TEST_ASSERT(xent_resolved_p(ctx, node, XENT_AXIS_VERTICAL, &resolved));
	TEST_ASSERT(test_float_near(resolved.main_start, 6.0f, 0.001f));
	TEST_ASSERT(test_float_near(resolved.main_end, 8.0f, 0.001f));
	TEST_ASSERT(test_float_near(resolved.cross_start, 7.0f, 0.001f));
	TEST_ASSERT(test_float_near(resolved.cross_end, 5.0f, 0.001f));

	xent_ctx_destroy(ctx);
	return 0;
}

int main(void) { return test_direction(); }
