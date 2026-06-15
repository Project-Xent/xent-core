#include "test_common.h"

static int test_flex_rtl_baseline(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root  = xent_create_node(ctx);
	XentNodeId small = xent_create_node(ctx);
	XentNodeId large = xent_create_node(ctx);

	xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
	xent_set_direction(ctx, root, XENT_DIRECTION_RTL);
	xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
	xent_set_flex_align_items(ctx, root, XENT_FLEX_ALIGN_BASELINE);
	xent_set_size(ctx, root, (XentSize) {300.0f, 100.0f});

	xent_set_text(ctx, small, "small");
	xent_set_size(ctx, small, (XentSize) {80.0f, 20.0f});
	xent_set_text(ctx, large, "large");
	xent_set_size(ctx, large, (XentSize) {80.0f, 40.0f});

	xent_append_child(ctx, root, small);
	xent_append_child(ctx, root, large);
	TEST_ASSERT(xent_layout(ctx, root, 300.0f, 100.0f));

	XentRect sr = {0};
	XentRect lr = {0};
	TEST_ASSERT(xent_get_layout_rect(ctx, small, &sr));
	TEST_ASSERT(xent_get_layout_rect(ctx, large, &lr));

	TEST_ASSERT(sr.x > lr.x);
	TEST_ASSERT(sr.y > lr.y);
	TEST_ASSERT(test_float_near(sr.y + 16.0f, lr.y + 32.0f, 0.5f));

	xent_destroy_context(ctx);

	ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	root                 = xent_create_node(ctx);
	XentNodeId tall      = xent_create_node(ctx);
	XentNodeId short_box = xent_create_node(ctx);

	xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
	xent_set_direction(ctx, root, XENT_DIRECTION_RTL);
	xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
	xent_set_flex_align_items(ctx, root, XENT_FLEX_ALIGN_BASELINE);
	xent_set_size(ctx, root, (XentSize) {300.0f, 120.0f});

	xent_set_size(ctx, tall, (XentSize) {70.0f, 60.0f});
	xent_set_size(ctx, short_box, (XentSize) {70.0f, 20.0f});

	xent_append_child(ctx, root, tall);
	xent_append_child(ctx, root, short_box);
	TEST_ASSERT(xent_layout(ctx, root, 300.0f, 120.0f));

	XentRect tr = {0};
	sr          = (XentRect) {0};
	TEST_ASSERT(xent_get_layout_rect(ctx, tall, &tr));
	TEST_ASSERT(xent_get_layout_rect(ctx, short_box, &sr));
	TEST_ASSERT(tr.x > sr.x);
	TEST_ASSERT(test_float_near(tr.y + tr.h, sr.y + sr.h, 0.3f));

	xent_destroy_context(ctx);
	return 0;
}

int main(void) {
	return test_flex_rtl_baseline();
}
