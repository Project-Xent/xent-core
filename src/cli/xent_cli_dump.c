#include "../xent_internal.h"

static char const *xent_protocol_name(XentProtocol p) {
	switch (p) {
	case XENT_PROTOCOL_ABSOLUTE   : return "absolute";
	case XENT_PROTOCOL_FLEX       : return "flex";
	case XENT_PROTOCOL_SWIFTSTACK : return "swiftstack";
	default                       : return "unknown";
	}
}

static char const *xent_semantic_name(XentSemanticRole role) {
	switch (role) {
	case XENT_SEMANTIC_ROOT      : return "root";
	case XENT_SEMANTIC_CONTAINER : return "container";
	case XENT_SEMANTIC_TEXT      : return "text";
	case XENT_SEMANTIC_BUTTON    : return "button";
	case XENT_SEMANTIC_IMAGE     : return "image";
	case XENT_SEMANTIC_CUSTOM    : return "custom";
	case XENT_SEMANTIC_NONE      :
	default                      : return "none";
	}
}

static void dump_tree_rec(XentContext const *ctx, XentNodeId node, FILE *out, uint32_t depth) {
	for (uint32_t i = 0; i < depth; ++i) fputs("  ", out);

	char const *label = xent_get_semantic_label(ctx, node);
	char const *text  = xent_get_text(ctx, node);

	fprintf(
	  out, "[%u] rect=(%.1f,%.1f %.1fx%.1f) protocol=%s role=%s", node, ctx->nodes.layout.abs_x [node],
	  ctx->nodes.layout.abs_y [node], ctx->nodes.layout.decided_w [node], ctx->nodes.layout.decided_h [node],
	  xent_protocol_name(( XentProtocol ) ctx->nodes.layout.protocol [node]),
	  xent_semantic_name(( XentSemanticRole ) ctx->nodes.semantics.role [node])
	);

	if (label && label [0] != '\0') fprintf(out, " label=\"%s\"", label);
	if (text && text [0] != '\0') fprintf(out, " text=\"%s\"", text);

	fputc('\n', out);

	XentNodeId child = ctx->nodes.topology.first_child [node];
	while (child != XENT_NODE_INVALID) {
		dump_tree_rec(ctx, child, out, depth + 1u);
		child = ctx->nodes.topology.next_sibling [child];
	}
}

void xent_dump_tree(XentContext const *ctx, XentNodeId root, FILE *out) {
	if (!xent_is_valid_node(ctx, root)) return;
	dump_tree_rec(ctx, root, out ? out : stdout, 0u);
}

void xent_dump_layout_text(XentContext const *ctx, XentNodeId root, FILE *out) {
	xent_dump_tree(ctx, root, out ? out : stdout);
}

static void dump_semantics_rec(XentContext const *ctx, XentNodeId node, FILE *out, uint32_t depth) {
	for (uint32_t i = 0; i < depth; ++i) fputs("  ", out);

	char const *label = xent_get_semantic_label(ctx, node);
	fprintf(
	  out, "[%u] role=%s bounds=(%.1f,%.1f %.1fx%.1f)", node,
	  xent_semantic_name(( XentSemanticRole ) ctx->nodes.semantics.role [node]), ctx->nodes.layout.abs_x [node],
	  ctx->nodes.layout.abs_y [node], ctx->nodes.layout.decided_w [node], ctx->nodes.layout.decided_h [node]
	);

	if (label && label [0] != '\0') fprintf(out, " label=\"%s\"", label);
	fputc('\n', out);

	XentNodeId child = ctx->nodes.topology.first_child [node];
	while (child != XENT_NODE_INVALID) {
		dump_semantics_rec(ctx, child, out, depth + 1u);
		child = ctx->nodes.topology.next_sibling [child];
	}
}

void xent_dump_semantics_text(XentContext const *ctx, XentNodeId root, FILE *out) {
	if (!ctx) return;

	FILE      *stream = out ? out : stdout;
	XentNodeId target = root;
	if (target == XENT_NODE_INVALID) target = ctx->last_layout_root;
	if (!xent_is_valid_node(ctx, target)) return;

	dump_semantics_rec(ctx, target, stream, 0u);
}
