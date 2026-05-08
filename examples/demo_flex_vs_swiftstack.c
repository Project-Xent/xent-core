#include <math.h>
#include <stdio.h>

#include "xent/xent.h"

static void build_row(
  XentContext *ctx, XentNodeId container, bool use_swiftstack, char const *label_text, char const *semantic_label
) {
	XentNodeId icon     = xent_create_node(ctx);
	XentNodeId text     = xent_create_node(ctx);
	XentNodeId spacer   = xent_create_node(ctx);
	XentNodeId trailing = xent_create_node(ctx);

	xent_set_semantic_label(ctx, container, semantic_label);
	xent_set_size(ctx, icon, (XentSize) {24.0f, 24.0f});
	xent_set_size(ctx, trailing, (XentSize) {20.0f, 20.0f});
	xent_set_text(ctx, text, label_text);

	if (use_swiftstack) {
		xent_set_protocol(ctx, container, XENT_PROTOCOL_SWIFTSTACK);
		xent_set_stack_axis(ctx, container, XENT_AXIS_HORIZONTAL);
		xent_set_layout_priority(ctx, text, 1.0f);
		xent_set_is_spacer(ctx, spacer, true);
	}
	else {
		xent_set_protocol(ctx, container, XENT_PROTOCOL_FLEX);
		xent_set_flex_direction(ctx, container, XENT_FLEX_ROW);
		xent_set_flex_grow(ctx, spacer, 1.0f);
		xent_set_flex_shrink(ctx, spacer, 1.0f);
	}
	xent_set_gap(ctx, container, 6.0f);

	xent_append_child(ctx, container, icon);
	xent_append_child(ctx, container, text);
	xent_append_child(ctx, container, spacer);
	xent_append_child(ctx, container, trailing);
}

static void run_case(float width) {
	XentContext *ctx   = xent_create_context(NULL);
	XentNodeId   root  = xent_create_node(ctx);
	XentNodeId   left  = xent_create_node(ctx);
	XentNodeId   right = xent_create_node(ctx);

	xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
	xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
	xent_set_size(ctx, root, (XentSize) {width, 120.0f});
	xent_set_gap(ctx, root, 12.0f);
	xent_set_padding(ctx, root, (XentInsets) {4.0f, 4.0f, 4.0f, 4.0f});

	xent_set_flex_grow(ctx, left, 1.0f);
	xent_set_flex_grow(ctx, right, 1.0f);
	xent_set_size(ctx, left, (XentSize) {NAN, 48.0f});
	xent_set_size(ctx, right, (XentSize) {NAN, 48.0f});

	build_row(ctx, left, false, "Flex row behavior", "flex_row");
	build_row(ctx, right, true, "SwiftStack behavior", "swiftstack_row");

	xent_append_child(ctx, root, left);
	xent_append_child(ctx, root, right);

	printf("\n=== width=%.0f ===\n", width);
	xent_layout(ctx, root, width, 120.0f);
	xent_dump_layout_text(ctx, root, stdout);

	xent_destroy_context(ctx);
}

int main(void) {
	run_case(420.0f);
	run_case(300.0f);
	run_case(220.0f);
	return 0;
}
