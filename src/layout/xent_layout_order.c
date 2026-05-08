#include "../xent_internal.h"

typedef struct PrioritySortItem {
	XentNodeId id;
	uint32_t   order;
} PrioritySortItem;

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

static bool xent_work_push(XentContext *ctx, XentNodeId node) {
	if (!xent_ensure_work_capacity(ctx, ctx->work_count + 1u)) return false;
	ctx->work_order [ctx->work_count++] = node;
	return true;
}

static bool xent_stack_push(XentContext *ctx, XentNodeId **stack, uint32_t *top, uint32_t *capacity, XentNodeId node) {
	if (*top == *capacity) {
		uint32_t    new_cap = *capacity ? *capacity * 2u : 64u;
		XentNodeId *new_mem = ( XentNodeId * ) realloc(*stack, sizeof(XentNodeId) * ( size_t ) new_cap);
		if (!new_mem) return false;
		*stack    = new_mem;
		*capacity = new_cap;
	}
	(*stack) [(*top)++] = node;
	( void ) ctx;
	return true;
}

static bool xent_append_preorder_iterative(XentContext *ctx, XentNodeId root) {
	XentNodeId *stack    = NULL;
	uint32_t    top      = 0u;
	uint32_t    capacity = 0u;
	bool        ok       = xent_is_valid_node(ctx, root) && xent_stack_push(ctx, &stack, &top, &capacity, root);

	while (ok && top > 0u) {
		XentNodeId node = stack [--top];
		if (!xent_is_valid_node(ctx, node)) {
			ok = false;
			break;
		}
		ok = xent_work_push(ctx, node);
		for (XentNodeId child = ctx->nodes.topology.last_child [node]; ok && child != XENT_NODE_INVALID;
		  child               = ctx->nodes.topology.prev_sibling [child])
		{
			ok = xent_stack_push(ctx, &stack, &top, &capacity, child);
		}
	}

	free(stack);
	return ok;
}

bool xent_build_preorder_roots(XentContext *ctx, XentNodeId const *roots, uint32_t root_count) {
	if (!ctx || (!roots && root_count > 0u)) return false;
	ctx->work_count = 0u;
	for (uint32_t i = 0u; i < root_count; ++i)
		if (!xent_append_preorder_iterative(ctx, roots [i])) return false;
	return true;
}

bool xent_build_preorder(XentContext *ctx, XentNodeId root) { return xent_build_preorder_roots(ctx, &root, 1u); }

static bool
xent_priority_less(XentContext const *ctx, PrioritySortItem const *a, PrioritySortItem const *b, bool descending) {
	float ap = ctx->nodes.stack.priority [a->id];
	float bp = ctx->nodes.stack.priority [b->id];
	if (ap == bp) return a->order < b->order;
	return descending ? ap > bp : ap < bp;
}

static void xent_merge_priority_range(
  XentContext const *ctx, PrioritySortItem *items, PrioritySortItem *tmp, uint32_t lo, uint32_t mid, uint32_t hi,
  bool descending
) {
	uint32_t left  = lo;
	uint32_t right = mid;
	uint32_t out   = lo;
	while (left < mid && right < hi)
		if (xent_priority_less(ctx, &items [left], &items [right], descending)) tmp [out++] = items [left++];
		else tmp [out++] = items [right++];
	while (left < mid) tmp [out++] = items [left++];
	while (right < hi) tmp [out++] = items [right++];
	for (uint32_t i = lo; i < hi; ++i) items [i] = tmp [i];
}

static void xent_priority_merge_sort(
  XentContext const *ctx, PrioritySortItem *items, PrioritySortItem *tmp, uint32_t count, bool descending
) {
	for (uint32_t width = 1u; width < count; width *= 2u) {
		for (uint32_t lo = 0u; lo < count; lo += width * 2u) {
			uint32_t mid = lo + width;
			uint32_t hi  = lo + width * 2u;
			if (mid > count) mid = count;
			if (hi > count) hi = count;
			xent_merge_priority_range(ctx, items, tmp, lo, mid, hi, descending);
		}
	}
}

static bool xent_priority_should_move(float current, float key_priority, bool descending) {
	return descending ? (current < key_priority) : (current > key_priority);
}

static void xent_sort_by_priority_insertion(XentContext const *ctx, XentNodeId *ids, uint32_t count, bool descending) {
	for (uint32_t i = 1u; i < count; ++i) {
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

void xent_sort_by_priority(XentContext const *ctx, XentNodeId *ids, uint32_t count, bool descending) {
	if (!ctx || !ids || count < 2u) return;

	PrioritySortItem *items = ( PrioritySortItem * ) malloc(sizeof(*items) * ( size_t ) count);
	PrioritySortItem *tmp   = ( PrioritySortItem * ) malloc(sizeof(*tmp) * ( size_t ) count);
	if (!items || !tmp) {
		free(items);
		free(tmp);
		xent_sort_by_priority_insertion(ctx, ids, count, descending);
		return;
	}

	for (uint32_t i = 0u; i < count; ++i) items [i] = (PrioritySortItem) {ids [i], i};
	xent_priority_merge_sort(ctx, items, tmp, count, descending);
	for (uint32_t i = 0u; i < count; ++i) ids [i] = items [i].id;

	free(items);
	free(tmp);
}
