#include "test_common.h"

static int test_flex_rtl_baseline(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root  = xent_node_create(ctx);
	XentNodeId small = xent_node_create(ctx);
	XentNodeId large = xent_node_create(ctx);

	xent_setproto(ctx, root, XENT_PROTOCOL_FLEX);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_BASELINE);
	xent_setsize(ctx, root, (XentSize) {300.0f, 100.0f});

	xent_settext(ctx, small, "small");
	xent_setsize(ctx, small, (XentSize) {80.0f, 20.0f});
	xent_settext(ctx, large, "large");
	xent_setsize(ctx, large, (XentSize) {80.0f, 40.0f});

	xent_node_append(ctx, root, small);
	xent_node_append(ctx, root, large);
	TEST_ASSERT(xent_layout(ctx, root, 300.0f, 100.0f));

	XentRect sr = {0};
	XentRect lr = {0};
	TEST_ASSERT(xent_layout_rect(ctx, small, &sr));
	TEST_ASSERT(xent_layout_rect(ctx, large, &lr));

	TEST_ASSERT(sr.x > lr.x);
	TEST_ASSERT(sr.y > lr.y);
	TEST_ASSERT(test_float_near(sr.y + 16.0f, lr.y + 32.0f, 0.5f));

	xent_ctx_destroy(ctx);

	ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	root                 = xent_node_create(ctx);
	XentNodeId tall      = xent_node_create(ctx);
	XentNodeId short_box = xent_node_create(ctx);

	xent_setproto(ctx, root, XENT_PROTOCOL_FLEX);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_BASELINE);
	xent_setsize(ctx, root, (XentSize) {300.0f, 120.0f});

	xent_setsize(ctx, tall, (XentSize) {70.0f, 60.0f});
	xent_setsize(ctx, short_box, (XentSize) {70.0f, 20.0f});

	xent_node_append(ctx, root, tall);
	xent_node_append(ctx, root, short_box);
	TEST_ASSERT(xent_layout(ctx, root, 300.0f, 120.0f));

	XentRect tr = {0};
	sr          = (XentRect) {0};
	TEST_ASSERT(xent_layout_rect(ctx, tall, &tr));
	TEST_ASSERT(xent_layout_rect(ctx, short_box, &sr));
	TEST_ASSERT(tr.x > sr.x);
	TEST_ASSERT(test_float_near(tr.y + tr.h, sr.y + sr.h, 0.3f));

	xent_ctx_destroy(ctx);
	return 0;
}

int main(void) { return test_flex_rtl_baseline(); }
