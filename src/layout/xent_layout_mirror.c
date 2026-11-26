#include "../xent_internal.h"

#include "../core/xent_scratch.h"

#include <string.h>

enum
{
	MIRROR_ROOT = 1u,
	MIRROR_RTL  = 2u,
};

static bool mirror_needed(XentCtx const *ctx, XentNodeId const *roots, uint32_t root_count) {
	for (uint32_t i = 0; i < root_count; ++i)
		if (xent_resolved_dir(ctx, roots [i]) == XENT_DIRECTION_RTL) return true;
	for (uint32_t i = 0; i < ctx->work_count; ++i)
		if (ctx->nodes.layout.direction [xent_node_index(ctx->work_order [i])] == XENT_DIRECTION_RTL) return true;
	return false;
}

static uint8_t child_state(XentCtx const *ctx, XentNodeId node, uint8_t parent_state) {
	uint8_t own = ctx->nodes.layout.direction [xent_node_index(node)];
	if (own == XENT_DIRECTION_RTL) return MIRROR_RTL;
	if (own == XENT_DIRECTION_LTR) return 0u;
	return parent_state & MIRROR_RTL;
}

static void mirror_mark_roots(XentCtx const *ctx, uint8_t *state, XentNodeId const *roots, uint32_t root_count) {
	for (uint32_t i = 0; i < root_count; ++i) {
		bool rtl                           = xent_resolved_dir(ctx, roots [i]) == XENT_DIRECTION_RTL;
		state [xent_node_index(roots [i])] = MIRROR_ROOT | (rtl ? MIRROR_RTL : 0u);
	}
}

static void mirror_to_parent_space(XentCtx *ctx, uint8_t const *state) {
	float *x = ctx->nodes.layout.abs_x;
	for (uint32_t i = ctx->work_count; i-- > 0u;) {
		uint32_t c = xent_node_index(ctx->work_order [i]);
		if (!(state [c] & MIRROR_ROOT)) x [c] -= x [xent_node_index(ctx->nodes.topology.parent [c])];
	}
}

static void mirror_place(XentCtx *ctx, uint8_t *state) {
	float *x = ctx->nodes.layout.abs_x;
	float *w = ctx->nodes.layout.decided_w;
	for (uint32_t i = 0; i < ctx->work_count; ++i) {
		uint32_t c = xent_node_index(ctx->work_order [i]);
		if (state [c] & MIRROR_ROOT) continue;
		uint32_t p = xent_node_index(ctx->nodes.topology.parent [c]);
		state [c]  = child_state(ctx, ctx->work_order [i], state [p]);
		x [c]      = x [p] + ((state [p] & MIRROR_RTL) ? w [p] - x [c] - w [c] : x [c]);
	}
}

void xent_layout_mirror(XentCtx *ctx, XentNodeId const *roots, uint32_t root_count) {
	if (!mirror_needed(ctx, roots, root_count)) return;
	XentScratchMark mark  = xent_scratch_mark(ctx);
	uint8_t        *state = ( uint8_t * ) xent_scratch_alloc(ctx, ctx->nodes.capacity, 1u);
	if (!state) return;
	memset(state, 0, ctx->nodes.capacity);
	mirror_mark_roots(ctx, state, roots, root_count);
	mirror_to_parent_space(ctx, state);
	mirror_place(ctx, state);
	xent_scratch_rewind(ctx, mark);
}
