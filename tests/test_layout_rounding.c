#include "test_common.h"

int main(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root  = xent_create_node(ctx);
	XentNodeId child = xent_create_node(ctx);
	TEST_ASSERT(root != XENT_NODE_INVALID);
	TEST_ASSERT(child != XENT_NODE_INVALID);

	TEST_ASSERT(xent_set_protocol(ctx, root, XENT_PROTOCOL_ABSOLUTE));
	TEST_ASSERT(xent_set_size(ctx, root, (XentSize) {10.3f, 20.7f}));
	TEST_ASSERT(xent_set_size(ctx, child, (XentSize) {3.26f, 4.74f}));
	TEST_ASSERT(xent_set_absolute_position(ctx, child, (XentPoint) {1.24f, 2.74f}));
	TEST_ASSERT(xent_append_child(ctx, root, child));

	TEST_ASSERT(xent_layout(ctx, root, 10.3f, 20.7f));

	XentRect root_rect  = {0};
	XentRect child_rect = {0};
	TEST_ASSERT(xent_get_layout_rect(ctx, root, &root_rect));
	TEST_ASSERT(xent_get_layout_rect(ctx, child, &child_rect));
	TEST_ASSERT(test_float_near(root_rect.width, 10.3f, 0.001f));
	TEST_ASSERT(test_float_near(child_rect.x, 1.24f, 0.001f));

	TEST_ASSERT(xent_set_point_scale_factor(ctx, 2.0f));
	TEST_ASSERT(xent_set_pixel_rounding_enabled(ctx, true));
	TEST_ASSERT(xent_layout(ctx, root, 10.3f, 20.7f));

	TEST_ASSERT(xent_get_layout_rect(ctx, root, &root_rect));
	TEST_ASSERT(xent_get_layout_rect(ctx, child, &child_rect));
	TEST_ASSERT(test_float_near(root_rect.width, 10.5f, 0.001f));
	TEST_ASSERT(test_float_near(root_rect.height, 20.5f, 0.001f));
	TEST_ASSERT(test_float_near(child_rect.x, 1.0f, 0.001f));
	TEST_ASSERT(test_float_near(child_rect.y, 2.5f, 0.001f));
	TEST_ASSERT(test_float_near(child_rect.width, 3.5f, 0.001f));
	TEST_ASSERT(test_float_near(child_rect.height, 4.5f, 0.001f));

	TEST_ASSERT(xent_set_point_scale_factor(ctx, 4.0f));
	TEST_ASSERT(xent_layout(ctx, root, 10.3f, 20.7f));
	TEST_ASSERT(xent_get_layout_rect(ctx, root, &root_rect));
	TEST_ASSERT(test_float_near(root_rect.width, 10.25f, 0.001f));

	xent_destroy_context(ctx);
	return 0;
}
