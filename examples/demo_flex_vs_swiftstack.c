#include <math.h>
#include <stdio.h>

#include "xent/xent_cli.h"
#include "xent/xent.h"

static void
build_row(XentCtx *ctx, XentNodeId container, bool use_swiftstack, char const *label_text, char const *semantic_label) {
	XentNodeId icon     = xent_node_create(ctx);
	XentNodeId text     = xent_node_create(ctx);
	XentNodeId spacer   = xent_node_create(ctx);
	XentNodeId trailing = xent_node_create(ctx);

	xent_sem_setlabel(ctx, container, semantic_label);
	xent_setsize(ctx, icon, (XentSize) {24.0f, 24.0f});
	xent_setsize(ctx, trailing, (XentSize) {20.0f, 20.0f});
	xent_settext(ctx, text, label_text);

	if (use_swiftstack) {
		xent_setproto(ctx, container, XENT_PROTOCOL_SWIFTSTACK);
		xent_stack_setaxis(ctx, container, XENT_AXIS_HORIZONTAL);
		xent_stack_setprio(ctx, text, 1.0f);
		xent_stack_setspacer(ctx, spacer, true);
	}
	else {
		xent_setproto(ctx, container, XENT_PROTOCOL_FLEX);
		xent_setflexdir(ctx, container, XENT_FLEX_ROW);
		xent_setgrow(ctx, spacer, 1.0f);
		xent_setshrink(ctx, spacer, 1.0f);
	}
	xent_setgap(ctx, container, 6.0f);

	xent_node_append(ctx, container, icon);
	xent_node_append(ctx, container, text);
	xent_node_append(ctx, container, spacer);
	xent_node_append(ctx, container, trailing);
}

static void run_case(float width) {
	XentCtx   *ctx   = xent_ctx_create(NULL);
	XentNodeId root  = xent_node_create(ctx);
	XentNodeId left  = xent_node_create(ctx);
	XentNodeId right = xent_node_create(ctx);

	xent_setproto(ctx, root, XENT_PROTOCOL_FLEX);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setsize(ctx, root, (XentSize) {width, 120.0f});
	xent_setgap(ctx, root, 12.0f);
	xent_setp(ctx, root, (XentInsets) {4.0f, 4.0f, 4.0f, 4.0f});

	xent_setgrow(ctx, left, 1.0f);
	xent_setgrow(ctx, right, 1.0f);
	xent_setsize(ctx, left, (XentSize) {NAN, 48.0f});
	xent_setsize(ctx, right, (XentSize) {NAN, 48.0f});

	build_row(ctx, left, false, "Flex row behavior", "flex_row");
	build_row(ctx, right, true, "SwiftStack behavior", "swiftstack_row");

	xent_node_append(ctx, root, left);
	xent_node_append(ctx, root, right);

	printf("\n=== width=%.0f ===\n", width);
	xent_layout(ctx, root, width, 120.0f);
	xent_dump_layout_text(ctx, root, stdout);

	xent_ctx_destroy(ctx);
}

int main(void) {
	run_case(420.0f);
	run_case(300.0f);
	run_case(220.0f);
	return 0;
}
