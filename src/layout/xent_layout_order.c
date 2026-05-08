#include "../xent_internal.h"

static bool xent_ensure_work_capacity(XentContext *ctx, uint32_t needed) {
	if (needed <= ctx->work_capacity) return true;

	uint32_t new_cap = ctx->work_capacity ? ctx->work_capacity * 2u : 64u;
	while (new_cap < needed) new_cap *= 2u;

	XentNodeId *new_mem = ( XentNodeId * ) realloc(ctx->work_order, sizeof(XentNodeId) * ( size_t ) new_cap);
	if (!new_mem) return false;

	ctx->work_order    = new_mem;
	ctx->work_capacity = new_cap;
	return true;
}

static bool xent_preorder_visit(XentContext *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return false;
	if (!xent_ensure_work_capacity(ctx, ctx->work_count + 1u)) return false;

	ctx->work_order [ctx->work_count++] = node;

	XentNodeId child                    = ctx->nodes.topology.first_child [node];
	while (child != XENT_NODE_INVALID) {
		if (!xent_preorder_visit(ctx, child)) return false;
		child = ctx->nodes.topology.next_sibling [child];
	}
	return true;
}

bool xent_build_preorder(XentContext *ctx, XentNodeId root) {
	if (!xent_is_valid_node(ctx, root)) return false;
	ctx->work_count = 0u;
	return xent_preorder_visit(ctx, root);
}

static bool xent_priority_should_move(float current, float key_priority, bool descending) {
	return descending ? (current < key_priority) : (current > key_priority);
}

void xent_sort_by_priority(XentContext const *ctx, XentNodeId *ids, uint32_t count, bool descending) {
	if (!ctx || !ids || count < 2u) return;

	for (uint32_t i = 1; i < count; ++i) {
		XentNodeId key          = ids [i];
		float      key_priority = ctx->nodes.stack.priority [key];
		int32_t    j            = ( int32_t ) i - 1;
		while (j >= 0 && xent_priority_should_move(ctx->nodes.stack.priority [ids [j]], key_priority, descending)) {
			ids [j + 1] = ids [j];
			--j;
		}
		ids [j + 1] = key;
	}
}
