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

bool xent_build_preorder(XentContext *ctx, XentNodeId root) {
	if (!xent_is_valid_node(ctx, root)) return false;
	ctx->work_count = 0u;

	uint32_t    stack_cap = 64u;
	XentNodeId *stack     = ( XentNodeId * ) malloc(sizeof(XentNodeId) * stack_cap);
	if (!stack) return false;

	uint32_t stack_top  = 0u;
	stack [stack_top++] = root;

	while (stack_top > 0u) {
		XentNodeId node = stack [--stack_top];
		if (!xent_is_valid_node(ctx, node)) { free(stack); return false; }
		if (!xent_ensure_work_capacity(ctx, ctx->work_count + 1u)) { free(stack); return false; }

		ctx->work_order [ctx->work_count++] = node;

		uint32_t   children_start = stack_top;
		XentNodeId child          = ctx->nodes.topology.first_child [node];
		while (child != XENT_NODE_INVALID) {
			if (stack_top >= stack_cap) {
				uint32_t    new_cap = stack_cap * 2u;
				XentNodeId *new_mem = ( XentNodeId * ) realloc(stack, sizeof(XentNodeId) * new_cap);
				if (!new_mem) { free(stack); return false; }
				stack     = new_mem;
				stack_cap = new_cap;
			}
			stack [stack_top++] = child;
			child               = ctx->nodes.topology.next_sibling [child];
		}

		if (stack_top > children_start + 1u) {
			uint32_t lo = children_start;
			uint32_t hi = stack_top - 1u;
			while (lo < hi) {
				XentNodeId tmp = stack [lo];
				stack [lo]     = stack [hi];
				stack [hi]     = tmp;
				++lo;
				--hi;
			}
		}
	}

	free(stack);
	return true;
}

static bool xent_priority_should_move(float current, float key_priority, bool descending) {
	return descending ? (current < key_priority) : (current > key_priority);
}

typedef struct XentPrioritySortCtx {
	float const *priorities;
	bool         descending;
} XentPrioritySortCtx;

#ifdef _WIN32
static int xent_compare_priority(void *context, void const *a, void const *b) {
#else
static int xent_compare_priority(void const *a, void const *b, void *context) {
#endif
	XentPrioritySortCtx const *sc = ( XentPrioritySortCtx const * ) context;
	XentNodeId                 ia = *( XentNodeId const * ) a;
	XentNodeId                 ib = *( XentNodeId const * ) b;
	float                      pa = sc->priorities [ia];
	float                      pb = sc->priorities [ib];
	int diff = (pa > pb) - (pa < pb);
	if (sc->descending) diff = -diff;
	if (diff != 0) return diff;
	return (ia > ib) - (ia < ib);
}

#define XENT_SORT_INSERTION_THRESHOLD 32u

void xent_sort_by_priority(XentContext const *ctx, XentNodeId *ids, uint32_t count, bool descending) {
	if (!ctx || !ids || count < 2u) return;

	if (count <= XENT_SORT_INSERTION_THRESHOLD) {
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
		return;
	}

	XentPrioritySortCtx sc = {ctx->nodes.stack.priority, descending};
#ifdef _WIN32
	qsort_s(ids, ( size_t ) count, sizeof(XentNodeId), xent_compare_priority, &sc);
#else
	qsort_r(ids, ( size_t ) count, sizeof(XentNodeId), xent_compare_priority, &sc);
#endif
}
