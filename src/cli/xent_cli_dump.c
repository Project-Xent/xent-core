#include "../xent_internal.h"

static char const *protocol_name(XentProtocol p) {
	switch (p) {
	case XENT_PROTOCOL_ABSOLUTE   : return "absolute";
	case XENT_PROTOCOL_FLEX       : return "flex";
	case XENT_PROTOCOL_SWIFTSTACK : return "swiftstack";
	case XENT_PROTOCOL_GRID       : return "grid";
	default                       : return "unknown";
	}
}

static char const *semantic_name(XentSemRole role) {
	switch (role) {
	case XENT_SEM_ROLE_ROOT      : return "root";
	case XENT_SEM_ROLE_CONTAINER : return "container";
	case XENT_SEM_ROLE_TEXT      : return "text";
	case XENT_SEM_ROLE_BUTTON    : return "button";
	case XENT_SEM_ROLE_IMAGE     : return "image";
	case XENT_SEM_ROLE_CUSTOM    : return "custom";
	case XENT_SEM_ROLE_NONE      :
	default                      : return "none";
	}
}

typedef struct DumpFrame {
	XentNodeId node;
	uint32_t   depth;
} DumpFrame;

typedef void (*DumpNodeFn)(XentCtx const *ctx, XentNodeId node, FILE *out, uint32_t depth);

static bool  dump_stack_push(DumpFrame **stack, uint32_t *top, uint32_t *capacity, DumpFrame frame) {
	if (*top == *capacity) {
		uint32_t   new_cap = *capacity ? *capacity * 2u : 64u;
		DumpFrame *new_mem = ( DumpFrame * ) realloc(*stack, sizeof(*new_mem) * ( size_t ) new_cap);
		if (!new_mem) return false;
		*stack    = new_mem;
		*capacity = new_cap;
	}
	(*stack) [(*top)++] = frame;
	return true;
}

static bool
dump_push_children(XentCtx const *ctx, DumpFrame **stack, uint32_t *top, uint32_t *capacity, DumpFrame frame) {
	for (XentNodeId child = ctx->nodes.topology.last_child [xent_node_index(frame.node)]; child != XENT_NODE_INVALID;
	  child               = ctx->nodes.topology.prev_sibling [xent_node_index(child)])
	{
		if (!dump_stack_push(stack, top, capacity, (DumpFrame) {child, frame.depth + 1u})) return false;
	}
	return true;
}

static void dump_walk(XentCtx const *ctx, XentNodeId root, FILE *out, DumpNodeFn emit) {
	if (!xent_node_valid(ctx, root)) return;
	DumpFrame *stack    = NULL;
	uint32_t   top      = 0u;
	uint32_t   capacity = 0u;
	if (!dump_stack_push(&stack, &top, &capacity, (DumpFrame) {root, 0u})) return;

	while (top > 0u) {
		DumpFrame frame = stack [--top];
		emit(ctx, frame.node, out, frame.depth);
		if (!dump_push_children(ctx, &stack, &top, &capacity, frame)) {
			free(stack);
			return;
		}
	}
	free(stack);
}

static void dump_tree_node(XentCtx const *ctx, XentNodeId node, FILE *out, uint32_t depth) {
	for (uint32_t i = 0; i < depth; ++i) fputs("  ", out);

	char const *label = xent_sem_label(ctx, node);
	char const *text  = xent_node_text(ctx, node);
	uint32_t    idx   = xent_node_index(node);

	fprintf(
	  out, "[%u#%u] rect=(%.1f,%.1f %.1fx%.1f) protocol=%s role=%s", idx, xent_node_generation(node),
	  ctx->nodes.layout.abs_x [idx], ctx->nodes.layout.abs_y [idx], ctx->nodes.layout.decided_w [idx],
	  ctx->nodes.layout.decided_h [idx], protocol_name(( XentProtocol ) ctx->nodes.layout.protocol [idx]),
	  semantic_name(( XentSemRole ) ctx->nodes.semantics.role [idx])
	);

	if (label && label [0] != '\0') fprintf(out, " label=\"%s\"", label);
	if (text && text [0] != '\0') fprintf(out, " text=\"%s\"", text);
	fputc('\n', out);
}

void xent_dump_tree(XentCtx const *ctx, XentNodeId root, FILE *out) {
	dump_walk(ctx, root, out ? out : stdout, dump_tree_node);
}

void xent_dump_layout_text(XentCtx const *ctx, XentNodeId root, FILE *out) {
	xent_dump_tree(ctx, root, out ? out : stdout);
}

static void dump_semantics_node(XentCtx const *ctx, XentNodeId node, FILE *out, uint32_t depth) {
	for (uint32_t i = 0; i < depth; ++i) fputs("  ", out);

	char const *label = xent_sem_label(ctx, node);
	uint32_t    idx   = xent_node_index(node);
	fprintf(
	  out, "[%u#%u] role=%s bounds=(%.1f,%.1f %.1fx%.1f)", idx, xent_node_generation(node),
	  semantic_name(( XentSemRole ) ctx->nodes.semantics.role [idx]), ctx->nodes.layout.abs_x [idx],
	  ctx->nodes.layout.abs_y [idx], ctx->nodes.layout.decided_w [idx], ctx->nodes.layout.decided_h [idx]
	);
	if (label && label [0] != '\0') fprintf(out, " label=\"%s\"", label);
	fputc('\n', out);
}

void xent_dump_semantics_text(XentCtx const *ctx, XentNodeId root, FILE *out) {
	if (!ctx) return;
	XentNodeId target = root;
	if (target == XENT_NODE_INVALID) target = ctx->last_layout_root;
	dump_walk(ctx, target, out ? out : stdout, dump_semantics_node);
}
