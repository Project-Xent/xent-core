#include "test_common.h"

int main(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root         = xent_node_create(ctx);
	XentNodeId left         = xent_node_create(ctx);
	XentNodeId right        = xent_node_create(ctx);
	XentNodeId right_spacer = xent_node_create(ctx);
	XentNodeId right_tail   = xent_node_create(ctx);

	xent_setproto(ctx, root, XENT_PROTOCOL_FLEX);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setsize(ctx, root, (XentSize) {400.0f, 100.0f});
	xent_setgap(ctx, root, 4.0f);

	xent_setproto(ctx, left, XENT_PROTOCOL_ABSOLUTE);
	xent_setsize(ctx, left, (XentSize) {120.0f, 100.0f});
	xent_setshrink(ctx, left, 0.0f);

	xent_setproto(ctx, right, XENT_PROTOCOL_SWIFTSTACK);
	xent_stack_setaxis(ctx, right, XENT_AXIS_HORIZONTAL);
	xent_setgrow(ctx, right, 1.0f);
	xent_setsize(ctx, right, (XentSize) {NAN, 100.0f});

	xent_stack_setspacer(ctx, right_spacer, true);
	xent_setsize(ctx, right_tail, (XentSize) {30.0f, 30.0f});

	xent_node_append(ctx, right, right_spacer);
	xent_node_append(ctx, right, right_tail);

	xent_node_append(ctx, root, left);
	xent_node_append(ctx, root, right);

	TEST_ASSERT(xent_layout(ctx, root, 400.0f, 100.0f));

	XentRect left_rect  = {0};
	XentRect right_rect = {0};
	XentRect tail_rect  = {0};
	TEST_ASSERT(xent_layout_rect(ctx, left, &left_rect));
	TEST_ASSERT(xent_layout_rect(ctx, right, &right_rect));
	TEST_ASSERT(xent_layout_rect(ctx, right_tail, &tail_rect));

	TEST_ASSERT(left_rect.w > 0.0f);
	TEST_ASSERT(right_rect.x + 0.01f >= left_rect.x + left_rect.w);
	TEST_ASSERT(tail_rect.x > right_rect.x);

	xent_ctx_destroy(ctx);
	return 0;
}
