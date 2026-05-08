#include <math.h>
#include <stdio.h>

#include "xent/xent.h"

int main(void) {
	XentContext *ctx = xent_create_context(NULL);
	if (!ctx) {
		fprintf(stderr, "failed to create context\n");
		return 1;
	}

	XentNodeId root    = xent_create_node(ctx);
	XentNodeId sidebar = xent_create_node(ctx);
	XentNodeId content = xent_create_node(ctx);
	XentNodeId toolbar = xent_create_node(ctx);
	XentNodeId body    = xent_create_node(ctx);
	XentNodeId text    = xent_create_node(ctx);

	xent_set_semantic_label(ctx, root, "root");
	xent_set_semantic_label(ctx, sidebar, "sidebar");
	xent_set_semantic_label(ctx, content, "content");
	xent_set_semantic_label(ctx, toolbar, "toolbar");
	xent_set_semantic_label(ctx, body, "body");
	xent_set_semantic_label(ctx, text, "title");

	xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
	xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
	xent_set_size(ctx, root, (XentSize) {800.0f, 600.0f});
	xent_set_gap(ctx, root, 8.0f);
	xent_set_padding(ctx, root, (XentInsets) {8.0f, 8.0f, 8.0f, 8.0f});

	xent_set_size(ctx, sidebar, (XentSize) {200.0f, 580.0f});
	xent_set_flex_shrink(ctx, sidebar, 0.0f);

	xent_set_protocol(ctx, content, XENT_PROTOCOL_FLEX);
	xent_set_flex_direction(ctx, content, XENT_FLEX_COLUMN);
	xent_set_flex_grow(ctx, content, 1.0f);
	xent_set_gap(ctx, content, 8.0f);

	xent_set_size(ctx, toolbar, (XentSize) {NAN, 48.0f});
	xent_set_flex_shrink(ctx, toolbar, 0.0f);

	xent_set_flex_grow(ctx, body, 1.0f);

	xent_set_text(ctx, text, "Hello from xent-core CLI demo");
	xent_set_size(ctx, text, (XentSize) {NAN, NAN});
	xent_set_semantic_role(ctx, text, XENT_SEMANTIC_TEXT);

	xent_append_child(ctx, root, sidebar);
	xent_append_child(ctx, root, content);
	xent_append_child(ctx, content, toolbar);
	xent_append_child(ctx, content, body);
	xent_append_child(ctx, body, text);

	xent_begin_frame(ctx);
	xent_layout(ctx, root, 800.0f, 600.0f);
	xent_dump_layout_text(ctx, root, stdout);
	xent_end_frame(ctx);

	xent_destroy_context(ctx);
	return 0;
}
