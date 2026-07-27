#include "test_common.h"

int main(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root  = xent_node_create(ctx);
	XentNodeId child = xent_node_create(ctx);
	TEST_ASSERT(root != XENT_NODE_INVALID);
	TEST_ASSERT(child != XENT_NODE_INVALID);

	TEST_ASSERT(xent_setproto(ctx, root, XENT_PROTOCOL_ABSOLUTE));
	TEST_ASSERT(xent_setsize(ctx, root, (XentSize) {10.3f, 20.7f}));
	TEST_ASSERT(xent_setsize(ctx, child, (XentSize) {3.26f, 4.74f}));
	TEST_ASSERT(xent_setpos(ctx, child, (XentPoint) {1.24f, 2.74f}));
	TEST_ASSERT(xent_node_append(ctx, root, child));

	TEST_ASSERT(xent_layout(ctx, root, 10.3f, 20.7f));

	XentRect root_rect  = {0};
	XentRect child_rect = {0};
	TEST_ASSERT(xent_layout_rect(ctx, root, &root_rect));
	TEST_ASSERT(xent_layout_rect(ctx, child, &child_rect));
	TEST_ASSERT(test_float_near(root_rect.w, 10.3f, 0.001f));
	TEST_ASSERT(test_float_near(child_rect.x, 1.24f, 0.001f));

	TEST_ASSERT(xent_setscale(ctx, 2.0f));
	TEST_ASSERT(xent_setrounding(ctx, true));
	TEST_ASSERT(xent_layout(ctx, root, 10.3f, 20.7f));

	TEST_ASSERT(xent_layout_rect(ctx, root, &root_rect));
	TEST_ASSERT(xent_layout_rect(ctx, child, &child_rect));
	TEST_ASSERT(test_float_near(root_rect.w, 10.5f, 0.001f));
	TEST_ASSERT(test_float_near(root_rect.h, 20.5f, 0.001f));
	TEST_ASSERT(test_float_near(child_rect.x, 1.0f, 0.001f));
	TEST_ASSERT(test_float_near(child_rect.y, 2.5f, 0.001f));
	TEST_ASSERT(test_float_near(child_rect.w, 3.5f, 0.001f));
	/* Edge rounding (not size rounding): top 2.74->2.5, bottom 2.74+4.74=7.48->7.5,
	 * so h = 7.5 - 2.5 = 5.0. (x/w coincide under both schemes; only h exposes it.) */
	TEST_ASSERT(test_float_near(child_rect.h, 5.0f, 0.001f));

	TEST_ASSERT(xent_setscale(ctx, 4.0f));
	TEST_ASSERT(xent_layout(ctx, root, 10.3f, 20.7f));
	TEST_ASSERT(xent_layout_rect(ctx, root, &root_rect));
	TEST_ASSERT(test_float_near(root_rect.w, 10.25f, 0.001f));

	xent_ctx_destroy(ctx);
	return 0;
}
