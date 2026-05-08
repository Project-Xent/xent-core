#include "../xent_internal.h"

#if XENT_ISPC_ENABLED
  #include "xent_ispc_kernels_ispc.h"
#endif

static bool xent_dirty_flags_are_direct(uint32_t flags) {
	return (flags & (XENT_DIRTY_LAYOUT | XENT_DIRTY_SELF)) != 0u;
}

static bool xent_ensure_dirty_capacity(XentContext *ctx, uint32_t needed) {
	if (needed <= ctx->dirty_capacity) return true;

	uint32_t new_cap = ctx->dirty_capacity ? ctx->dirty_capacity * 2u : 64u;
	while (new_cap < needed) new_cap *= 2u;

	XentNodeId *new_mem = ( XentNodeId * ) realloc(ctx->dirty_nodes, sizeof(XentNodeId) * ( size_t ) new_cap);
	if (!new_mem) return false;

	ctx->dirty_nodes    = new_mem;
	ctx->dirty_capacity = new_cap;
	return true;
}

static bool xent_dirty_list_contains(XentContext const *ctx, XentNodeId node) {
	for (uint32_t i = 0u; i < ctx->dirty_count; ++i)
		if (ctx->dirty_nodes [i] == node) return true;
	return false;
}

static void xent_note_direct_dirty(XentContext *ctx, XentNodeId node, uint32_t old_flags, uint32_t new_flags) {
	if (xent_dirty_flags_are_direct(old_flags) || !xent_dirty_flags_are_direct(new_flags)) return;
	if (xent_dirty_list_contains(ctx, node)) return;
	if (!xent_ensure_dirty_capacity(ctx, ctx->dirty_count + 1u)) return;
	ctx->dirty_nodes [ctx->dirty_count++] = node;
}

void xent_mark_dirty(XentContext *ctx, XentNodeId node, uint32_t flags) {
	if (!xent_is_valid_node(ctx, node)) return;

	uint32_t old_flags                    = ctx->nodes.layout.dirty_flags [node];
	ctx->nodes.layout.dirty_flags [node] |= (flags | XENT_DIRTY_SELF);
	xent_note_direct_dirty(ctx, node, old_flags, ctx->nodes.layout.dirty_flags [node]);

	XentNodeId parent = ctx->nodes.topology.parent [node];
	while (parent != XENT_NODE_INVALID) {
		ctx->nodes.layout.dirty_flags [parent] |= XENT_DIRTY_SUBTREE;
		parent                                  = ctx->nodes.topology.parent [parent];
	}
}

uint32_t xent_get_dirty_flags(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return XENT_DIRTY_NONE;
	return ctx->nodes.layout.dirty_flags [node];
}

void xent_compact_dirty_nodes(XentContext *ctx) {
	if (!ctx || ctx->dirty_count == 0u) return;

	uint32_t write = 0u;
	for (uint32_t i = 0u; i < ctx->dirty_count; ++i) {
		XentNodeId node = ctx->dirty_nodes [i];
		if (xent_is_valid_node(ctx, node) && xent_dirty_flags_are_direct(ctx->nodes.layout.dirty_flags [node]))
			ctx->dirty_nodes [write++] = node;
	}
	ctx->dirty_count = write;
}

void xent_clear_dirty_in_work_order(XentContext *ctx) {
#if XENT_ISPC_ENABLED
	if (ctx->work_count >= 64u) {
		xent_ispc_scatter_zero_u32(ctx->nodes.layout.dirty_flags, ctx->work_order, ctx->work_count);
		xent_compact_dirty_nodes(ctx);
		return;
	}
#endif
	for (uint32_t i = 0; i < ctx->work_count; ++i) {
		XentNodeId node = ctx->work_order [i];
		if (xent_is_valid_node(ctx, node)) ctx->nodes.layout.dirty_flags [node] = XENT_DIRTY_NONE;
	}
	xent_compact_dirty_nodes(ctx);
}
