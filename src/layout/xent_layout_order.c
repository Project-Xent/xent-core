#include "../xent_internal.h"

typedef struct PrioritySortItem {
	XentNodeId id;
	uint32_t   order;
} PrioritySortItem;

static bool ensure_work_capacity(XentCtx *ctx, uint32_t needed) {
	if (needed <= ctx->work_capacity) return true;

	uint32_t new_cap = ctx->work_capacity ? ctx->work_capacity * 2u : 64u;
	while (new_cap < needed) new_cap *= 2u;

	XentNodeId *new_mem = ( XentNodeId * ) realloc(ctx->work_order, sizeof(XentNodeId) * ( size_t ) new_cap);
	if (!new_mem) return false;

	ctx->work_order    = new_mem;
	ctx->work_capacity = new_cap;
	return true;
}

static bool work_push(XentCtx *ctx, XentNodeId node) {
	if (!ensure_work_capacity(ctx, ctx->work_count + 1u)) return false;
	ctx->work_order [ctx->work_count++] = node;
	return true;
}

static bool stack_push(XentNodeId **stack, uint32_t *top, uint32_t *capacity, XentNodeId node) {
	if (*top == *capacity) {
		uint32_t    new_cap = *capacity ? *capacity * 2u : 64u;
		XentNodeId *new_mem = ( XentNodeId * ) realloc(*stack, sizeof(XentNodeId) * ( size_t ) new_cap);
		if (!new_mem) return false;
		*stack    = new_mem;
		*capacity = new_cap;
	}
	(*stack) [(*top)++] = node;
	return true;
}

static bool append_preorder_iterative(XentCtx *ctx, XentNodeId root) {
	XentNodeId *stack    = NULL;
	uint32_t    top      = 0u;
	uint32_t    capacity = 0u;
	bool        ok       = xent_node_valid(ctx, root) && stack_push(&stack, &top, &capacity, root);

	while (ok && top > 0u) {
		XentNodeId node = stack [--top];
		if (!xent_node_valid(ctx, node)) {
			ok = false;
			break;
		}
		ok = work_push(ctx, node);
		for (XentNodeId child                     = ctx->nodes.topology.last_child [xent_node_index(node)];
		  ok && child != XENT_NODE_INVALID; child = ctx->nodes.topology.prev_sibling [xent_node_index(child)])
		{
			ok = stack_push(&stack, &top, &capacity, child);
		}
	}

	free(stack);
	return ok;
}

bool xent_build_preorder_roots(XentCtx *ctx, XentNodeId const *roots, uint32_t root_count) {
	if (!ctx || (!roots && root_count > 0u)) return false;
	ctx->work_count = 0u;
	for (uint32_t i = 0u; i < root_count; ++i)
		if (!append_preorder_iterative(ctx, roots [i])) return false;
	return true;
}

bool xent_build_preorder(XentCtx *ctx, XentNodeId root) { return xent_build_preorder_roots(ctx, &root, 1u); }
