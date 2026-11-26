#include "../xent_internal.h"

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

static bool grow_scratch_stack(XentCtx *ctx, XentNodeId **stack, uint32_t *cap) {
	uint32_t    next_cap = *cap * 2u;
	XentNodeId *next
	  = ( XentNodeId * ) xent_scratch_alloc(ctx, sizeof(XentNodeId) * ( size_t ) next_cap, _Alignof(XentNodeId));
	if (!next) return false;
	memcpy(next, *stack, sizeof(XentNodeId) * ( size_t ) (*cap));
	*stack = next;
	*cap   = next_cap;
	return true;
}

static bool append_preorder_iterative(XentCtx *ctx, XentNodeId root) {
	if (!xent_node_valid(ctx, root)) return false;
	uint32_t        cap   = 256u;
	XentScratchMark mark  = xent_scratch_mark(ctx);
	XentNodeId     *stack = ( XentNodeId * ) xent_scratch_alloc(ctx, sizeof(XentNodeId) * cap, _Alignof(XentNodeId));
	if (!stack) return false;

	uint32_t top  = 0u;
	stack [top++] = root;
	bool ok       = true;
	while (ok && top > 0u) {
		XentNodeId node = stack [--top];
		if (!xent_node_valid(ctx, node) || !work_push(ctx, node)) {
			ok = false;
			break;
		}
		for (XentNodeId child = ctx->nodes.topology.last_child [xent_node_index(node)]; child != XENT_NODE_INVALID;
		  child               = ctx->nodes.topology.prev_sibling [xent_node_index(child)])
		{
			if (top == cap && !grow_scratch_stack(ctx, &stack, &cap)) {
				ok = false;
				break;
			}
			stack [top++] = child;
		}
	}

	xent_scratch_rewind(ctx, mark);
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
