#include "test_common.h"

int main(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root   = xent_create_node(ctx);
	XentNodeId icon   = xent_create_node(ctx);
	XentNodeId text   = xent_create_node(ctx);
	XentNodeId spacer = xent_create_node(ctx);
	XentNodeId tail   = xent_create_node(ctx);

	xent_set_protocol(ctx, root, XENT_PROTOCOL_SWIFTSTACK);
	xent_set_stack_axis(ctx, root, XENT_AXIS_HORIZONTAL);
	xent_set_size(ctx, root, (XentSize) {220.0f, 60.0f});
	xent_set_gap(ctx, root, 4.0f);

	xent_set_size(ctx, icon, (XentSize) {24.0f, 24.0f});
	xent_set_text(ctx, text, "abc");
	xent_set_layout_priority(ctx, text, 1.0f);
	xent_set_is_spacer(ctx, spacer, true);
	xent_set_size(ctx, tail, (XentSize) {16.0f, 16.0f});

	xent_append_child(ctx, root, icon);
	xent_append_child(ctx, root, text);
	xent_append_child(ctx, root, spacer);
	xent_append_child(ctx, root, tail);

	TEST_ASSERT(xent_layout(ctx, root, 220.0f, 60.0f));

	XentRect spacer_rect = {0};
	XentRect tail_rect   = {0};
	TEST_ASSERT(xent_get_layout_rect(ctx, spacer, &spacer_rect));
	TEST_ASSERT(xent_get_layout_rect(ctx, tail, &tail_rect));

	TEST_ASSERT(spacer_rect.width > 40.0f);
	TEST_ASSERT(tail_rect.x > spacer_rect.x);

	xent_destroy_context(ctx);
	return 0;
}
