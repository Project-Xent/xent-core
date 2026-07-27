#include "test_common.h"

int main(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root   = xent_node_create(ctx);
	XentNodeId icon   = xent_node_create(ctx);
	XentNodeId text   = xent_node_create(ctx);
	XentNodeId spacer = xent_node_create(ctx);
	XentNodeId tail   = xent_node_create(ctx);

	xent_setproto(ctx, root, XENT_PROTOCOL_SWIFTSTACK);
	xent_stack_setaxis(ctx, root, XENT_AXIS_HORIZONTAL);
	xent_setsize(ctx, root, (XentSize) {220.0f, 60.0f});
	xent_setgap(ctx, root, 4.0f);

	xent_setsize(ctx, icon, (XentSize) {24.0f, 24.0f});
	xent_settext(ctx, text, "abc");
	xent_stack_setprio(ctx, text, 1.0f);
	xent_stack_setspacer(ctx, spacer, true);
	xent_setsize(ctx, tail, (XentSize) {16.0f, 16.0f});

	xent_node_append(ctx, root, icon);
	xent_node_append(ctx, root, text);
	xent_node_append(ctx, root, spacer);
	xent_node_append(ctx, root, tail);

	TEST_ASSERT(xent_layout(ctx, root, 220.0f, 60.0f));

	XentRect spacer_rect = {0};
	XentRect tail_rect   = {0};
	TEST_ASSERT(xent_layout_rect(ctx, spacer, &spacer_rect));
	TEST_ASSERT(xent_layout_rect(ctx, tail, &tail_rect));

	TEST_ASSERT(spacer_rect.w > 40.0f);
	TEST_ASSERT(tail_rect.x > spacer_rect.x);

	xent_ctx_destroy(ctx);
	return 0;
}
