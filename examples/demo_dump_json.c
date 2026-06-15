#include <math.h>
#include <stdio.h>

#include "xent/xent_cli.h"
#include "xent/xent.h"

int main(int argc, char **argv) {
	XentContext *ctx = xent_create_context(NULL);
	if (!ctx) return 1;

	XentNodeId root      = xent_create_node(ctx);
	XentNodeId header    = xent_create_node(ctx);
	XentNodeId content   = xent_create_node(ctx);
	XentNodeId paragraph = xent_create_node(ctx);

	xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
	xent_set_flex_direction(ctx, root, XENT_FLEX_COLUMN);
	xent_set_size(ctx, root, (XentSize) {640.0f, 360.0f});
	xent_set_gap(ctx, root, 8.0f);
	xent_set_padding(ctx, root, (XentInsets) {8.0f, 8.0f, 8.0f, 8.0f});
	xent_set_semantic_label(ctx, root, "json_root");

	xent_set_size(ctx, header, (XentSize) {NAN, 40.0f});
	xent_set_semantic_label(ctx, header, "header");

	xent_set_flex_grow(ctx, content, 1.0f);
	xent_set_protocol(ctx, content, XENT_PROTOCOL_ABSOLUTE);
	xent_set_semantic_label(ctx, content, "content");

	xent_set_text(ctx, paragraph, "JSON dump from xent-core runtime");
	xent_set_size(ctx, paragraph, (XentSize) {NAN, NAN});
	xent_set_semantic_role(ctx, paragraph, XENT_SEMANTIC_TEXT);
	xent_set_semantic_label(ctx, paragraph, "paragraph");

	xent_append_child(ctx, root, header);
	xent_append_child(ctx, root, content);
	xent_append_child(ctx, content, paragraph);

	xent_layout(ctx, root, 640.0f, 360.0f);

	FILE *out = stdout;
	if (argc > 1) {
		FILE *file = NULL;
#if defined(_MSC_VER)
		if (fopen_s(&file, argv [1], "wb") != 0) file = NULL;
#else
		file = fopen(argv [1], "wb");
#endif
		out = file;
		if (!out) {
			xent_destroy_context(ctx);
			return 2;
		}
	}

	if (!xent_dump_layout_json(ctx, root, out)) {
		if (out != stdout) fclose(out);
		xent_destroy_context(ctx);
		return 3;
	}
	fputc('\n', out);

	if (out != stdout) fclose(out);
	xent_destroy_context(ctx);
	return 0;
}
