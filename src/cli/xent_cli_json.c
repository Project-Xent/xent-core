#include "../xent_internal.h"

static void json_escape(FILE *out, char const *text) {
	if (!text) return;
	while (*text) {
		char c = *text++;
		if (c == '\n') {
			fputs("\\n", out);
			continue;
		}
		if (c == '"' || c == '\\') fputc('\\', out);
		fputc(c, out);
	}
}

static bool dump_json_node(XentContext const *ctx, XentNodeId node, FILE *out) {
	if (!xent_is_valid_node(ctx, node)) return false;

	fprintf(
	  out, "{\"id\":%u,\"protocol\":%u,\"rect\":{\"x\":%.3f,\"y\":%.3f,\"w\":%.3f,\"h\":%.3f}", node,
	  ( unsigned ) ctx->nodes.layout.protocol [node], ctx->nodes.layout.abs_x [node], ctx->nodes.layout.abs_y [node],
	  ctx->nodes.layout.decided_w [node], ctx->nodes.layout.decided_h [node]
	);

	char const *label = xent_get_semantic_label(ctx, node);
	char const *text  = xent_get_text(ctx, node);

	if (label) {
		fputs(",\"label\":\"", out);
		json_escape(out, label);
		fputc('"', out);
	}
	if (text) {
		fputs(",\"text\":\"", out);
		json_escape(out, text);
		fputc('"', out);
	}

	fputs(",\"children\":[", out);
	XentNodeId child = ctx->nodes.topology.first_child [node];
	bool       first = true;
	while (child != XENT_NODE_INVALID) {
		if (!first) fputc(',', out);
		if (!dump_json_node(ctx, child, out)) return false;
		first = false;
		child = ctx->nodes.topology.next_sibling [child];
	}
	fputs("]}", out);
	return true;
}

bool xent_dump_layout_json(XentContext const *ctx, XentNodeId root, FILE *out) {
	if (!xent_is_valid_node(ctx, root) || !out) return false;
	return dump_json_node(ctx, root, out);
}
