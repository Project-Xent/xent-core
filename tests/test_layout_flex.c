#include "test_common.h"

int main(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root  = xent_create_node(ctx);
	XentNodeId fixed = xent_create_node(ctx);
	XentNodeId fluid = xent_create_node(ctx);

	xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
	xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
	xent_set_size(ctx, root, (XentSize) {300.0f, 100.0f});

	xent_set_size(ctx, fixed, (XentSize) {100.0f, 80.0f});
	xent_set_flex_shrink(ctx, fixed, 0.0f);

	xent_set_size(ctx, fluid, (XentSize) {NAN, 80.0f});
	xent_set_flex_grow(ctx, fluid, 1.0f);

	TEST_ASSERT(xent_append_child(ctx, root, fixed));
	TEST_ASSERT(xent_append_child(ctx, root, fluid));
	TEST_ASSERT(xent_layout(ctx, root, 300.0f, 100.0f));

	XentRect fixed_rect = {0};
	XentRect fluid_rect = {0};
	TEST_ASSERT(xent_get_layout_rect(ctx, fixed, &fixed_rect));
	TEST_ASSERT(xent_get_layout_rect(ctx, fluid, &fluid_rect));

	TEST_ASSERT(test_float_near(fixed_rect.w, 100.0f, 0.01f));
	TEST_ASSERT(test_float_near(fluid_rect.w, 200.0f, 0.1f));
	TEST_ASSERT(test_float_near(fluid_rect.x, 100.0f, 0.1f));

	xent_destroy_context(ctx);
	return 0;
}
