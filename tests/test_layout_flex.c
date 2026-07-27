#include "test_common.h"

int main(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root  = xent_node_create(ctx);
	XentNodeId fixed = xent_node_create(ctx);
	XentNodeId fluid = xent_node_create(ctx);

	xent_setproto(ctx, root, XENT_PROTOCOL_FLEX);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setsize(ctx, root, (XentSize) {300.0f, 100.0f});

	xent_setsize(ctx, fixed, (XentSize) {100.0f, 80.0f});
	xent_setshrink(ctx, fixed, 0.0f);

	xent_setsize(ctx, fluid, (XentSize) {NAN, 80.0f});
	xent_setgrow(ctx, fluid, 1.0f);

	TEST_ASSERT(xent_node_append(ctx, root, fixed));
	TEST_ASSERT(xent_node_append(ctx, root, fluid));
	TEST_ASSERT(xent_layout(ctx, root, 300.0f, 100.0f));

	XentRect fixed_rect = {0};
	XentRect fluid_rect = {0};
	TEST_ASSERT(xent_layout_rect(ctx, fixed, &fixed_rect));
	TEST_ASSERT(xent_layout_rect(ctx, fluid, &fluid_rect));

	TEST_ASSERT(test_float_near(fixed_rect.w, 100.0f, 0.01f));
	TEST_ASSERT(test_float_near(fluid_rect.w, 200.0f, 0.1f));
	TEST_ASSERT(test_float_near(fluid_rect.x, 100.0f, 0.1f));

	xent_ctx_destroy(ctx);
	return 0;
}
