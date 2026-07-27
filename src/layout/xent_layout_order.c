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

bool        xent_build_preorder(XentCtx *ctx, XentNodeId root) { return xent_build_preorder_roots(ctx, &root, 1u); }

static bool priority_less(XentCtx const *ctx, PrioritySortItem const *a, PrioritySortItem const *b, bool descending) {
	float ap = ctx->nodes.stack.priority [xent_node_index(a->id)];
	float bp = ctx->nodes.stack.priority [xent_node_index(b->id)];
	if (ap == bp) return a->order < b->order;
	return descending ? ap > bp : ap < bp;
}

static void merge_priority_range(
  XentCtx const *ctx, PrioritySortItem *items, PrioritySortItem *tmp, uint32_t lo, uint32_t mid, uint32_t hi,
  bool descending
) {
	uint32_t left  = lo;
	uint32_t right = mid;
	uint32_t out   = lo;
	while (left < mid && right < hi)
		if (priority_less(ctx, &items [left], &items [right], descending)) tmp [out++] = items [left++];
		else tmp [out++] = items [right++];
	while (left < mid) tmp [out++] = items [left++];
	while (right < hi) tmp [out++] = items [right++];
	for (uint32_t i = lo; i < hi; ++i) items [i] = tmp [i];
}

static void priority_merge_pass(
  XentCtx const *ctx, PrioritySortItem *items, PrioritySortItem *tmp, uint32_t count, uint32_t width, bool descending
) {
	for (uint32_t lo = 0u; lo < count; lo += width * 2u) {
		uint32_t mid = lo + width;
		uint32_t hi  = lo + width * 2u;
		if (mid > count) mid = count;
		if (hi > count) hi = count;
		merge_priority_range(ctx, items, tmp, lo, mid, hi, descending);
	}
}

static void priority_merge_sort(
  XentCtx const *ctx, PrioritySortItem *items, PrioritySortItem *tmp, uint32_t count, bool descending
) {
	for (uint32_t width = 1u; width < count; width *= 2u)
		priority_merge_pass(ctx, items, tmp, count, width, descending);
}

static bool priority_should_move(float current, float key_priority, bool descending) {
	return descending ? (current < key_priority) : (current > key_priority);
}

static void sort_by_priority_insertion(XentCtx const *ctx, XentNodeId *ids, uint32_t count, bool descending) {
	for (uint32_t i = 1u; i < count; ++i) {
		XentNodeId key          = ids [i];
		float      key_priority = ctx->nodes.stack.priority [xent_node_index(key)];
		int32_t    j            = ( int32_t ) i - 1;
		while (j >= 0
			   && priority_should_move(ctx->nodes.stack.priority [xent_node_index(ids [j])], key_priority, descending))
		{
			ids [j + 1] = ids [j];
			--j;
		}
		ids [j + 1] = key;
	}
}

void xent_sort_by_priority(XentCtx const *ctx, XentNodeId *ids, uint32_t count, bool descending) {
	if (!ctx || !ids || count < 2u) return;

	PrioritySortItem *items = ( PrioritySortItem * ) malloc(sizeof(*items) * ( size_t ) count);
	PrioritySortItem *tmp   = ( PrioritySortItem * ) malloc(sizeof(*tmp) * ( size_t ) count);
	if (!items || !tmp) {
		free(items);
		free(tmp);
		sort_by_priority_insertion(ctx, ids, count, descending);
		return;
	}

	for (uint32_t i = 0u; i < count; ++i) items [i] = (PrioritySortItem) {ids [i], i};
	priority_merge_sort(ctx, items, tmp, count, descending);
	for (uint32_t i = 0u; i < count; ++i) ids [i] = items [i].id;

	free(items);
	free(tmp);
}
