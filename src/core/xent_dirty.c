#include "../xent_internal.h"
#include "../xent_alloc_internal.h"

static bool ensure_dirty_capacity(XentCtx *ctx, uint32_t needed) {
	if (needed <= ctx->dirty_capacity) return true;

	uint32_t new_cap = ctx->dirty_capacity ? ctx->dirty_capacity : 64u;
	while (new_cap < needed) {
		if (new_cap > UINT32_MAX / 2u) return false;
		new_cap *= 2u;
	}

	XentNodeId *new_mem = ( XentNodeId * ) xent_realloc_internal(
	  XENT_ALLOC_TOPOLOGY_MUTATION, ctx->dirty_nodes, sizeof(XentNodeId) * ( size_t ) new_cap
	);
	if (!new_mem) return false;

	ctx->dirty_nodes    = new_mem;
	ctx->dirty_capacity = new_cap;
	return true;
}

static void note_direct_dirty(XentCtx *ctx, uint32_t index, XentNodeId node, uint32_t flags) {
	if (!xent_dirty_direct(flags)) return;
	if (ctx->nodes.layout.dirty_queued [index]) return;
	if (!ensure_dirty_capacity(ctx, ctx->dirty_count + 1u)) return;
	ctx->nodes.layout.dirty_queued [index] = 1u;
	ctx->dirty_nodes [ctx->dirty_count++]  = node;
}

void xent_mark_dirty(XentCtx *ctx, XentNodeId node, uint32_t flags) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return;

	ctx->nodes.layout.dirty_flags [index] |= (flags | XENT_DIRTY_SELF);
	note_direct_dirty(ctx, index, node, ctx->nodes.layout.dirty_flags [index]);

	XentNodeId parent = ctx->nodes.topology.parent [index];
	while (parent != XENT_NODE_INVALID) {
		uint32_t parent_index                         = xent_node_index(parent);
		ctx->nodes.layout.dirty_flags [parent_index] |= XENT_DIRTY_SUBTREE;
		parent                                        = ctx->nodes.topology.parent [parent_index];
	}
}

uint32_t xent_node_dirty(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return XENT_DIRTY_NONE;
	return ctx->nodes.layout.dirty_flags [index];
}

void xent_compact_dirty_nodes(XentCtx *ctx) {
	if (!ctx || ctx->dirty_count == 0u) return;

	uint32_t write = 0u;
	for (uint32_t i = 0u; i < ctx->dirty_count; ++i) {
		XentNodeId node  = ctx->dirty_nodes [i];
		uint32_t   index = xent_live_index(ctx, node);
		if (index && xent_dirty_direct(ctx->nodes.layout.dirty_flags [index])) {
			ctx->nodes.layout.dirty_queued [index] = 1u;
			ctx->dirty_nodes [write++]             = node;
			continue;
		}
		uint32_t slot = xent_node_index(node);
		if (slot != 0u && slot < ctx->nodes.capacity) ctx->nodes.layout.dirty_queued [slot] = 0u;
	}
	ctx->dirty_count = write;
}

void xent_dirty_clear_order(XentCtx *ctx) {
	for (uint32_t i = 0; i < ctx->work_count; ++i) {
		uint32_t index = xent_live_index(ctx, ctx->work_order [i]);
		if (!index) continue;
		ctx->nodes.layout.dirty_flags [index]  = XENT_DIRTY_NONE;
		ctx->nodes.layout.dirty_queued [index] = 0u;
	}
	xent_compact_dirty_nodes(ctx);
}
