#include "../xent_internal.h"
#include "../xent_alloc_internal.h"

static bool ensure_queue_capacity(XentNodeId **buf, uint32_t *cap, uint32_t needed, XentAllocSite site) {
	if (needed <= *cap) return true;
	uint32_t new_cap = *cap ? *cap : 64u;
	while (new_cap < needed) {
		if (new_cap > UINT32_MAX / 2u) return false;
		new_cap *= 2u;
	}
	XentNodeId *new_mem = ( XentNodeId * ) xent_realloc_internal(site, *buf, sizeof(XentNodeId) * ( size_t ) new_cap);
	if (!new_mem) return false;
	*buf = new_mem;
	*cap = new_cap;
	return true;
}

static void clear_measure_slot(XentNodeStore *nodes, uint32_t index) {
	XentMeasureCache *cache = xent_measure_cache(nodes, index);
	if (cache) cache->count = 0u;
}

static void note_layout_dirty(XentCtx *ctx, uint32_t index, XentNodeId node) {
	if (ctx->nodes.layout.dirty_queued [index] & XENT_QUEUE_LAYOUT) return;
	if (!ensure_queue_capacity(
		  &ctx->dirty_nodes, &ctx->dirty_capacity, ctx->dirty_count + 1u, XENT_ALLOC_TOPOLOGY_MUTATION
		))
		return;
	ctx->nodes.layout.dirty_queued [index] |= XENT_QUEUE_LAYOUT;
	ctx->dirty_nodes [ctx->dirty_count++]   = node;
}

static void note_sem_dirty(XentCtx *ctx, uint32_t index, XentNodeId node) {
	if (ctx->nodes.layout.dirty_queued [index] & XENT_QUEUE_SEM) return;
	if (!ensure_queue_capacity(
		  &ctx->sem_dirty_nodes, &ctx->sem_dirty_capacity, ctx->sem_dirty_count + 1u, XENT_ALLOC_SEMANTICS
		))
		return;
	ctx->nodes.layout.dirty_queued [index]        |= XENT_QUEUE_SEM;
	ctx->sem_dirty_nodes [ctx->sem_dirty_count++]  = node;
}

static void propagate_layout_subtree(XentCtx *ctx, uint32_t start_index) {
	bool       has_caches = ctx->nodes.measure_pool.live_count > 0u;
	XentNodeId parent     = ctx->nodes.topology.parent [start_index];
	while (parent != XENT_NODE_INVALID) {
		uint32_t pi                        = xent_node_index(parent);
		uint32_t existing                  = ctx->nodes.layout.dirty_flags [pi];
		ctx->nodes.layout.dirty_flags [pi] = existing | XENT_DIRTY_SUBTREE;
		clear_measure_slot(&ctx->nodes, pi);
		if ((existing & XENT_DIRTY_SUBTREE) && !has_caches) break;
		parent = ctx->nodes.topology.parent [pi];
	}
}

void xent_mark_dirty(XentCtx *ctx, XentNodeId node, uint32_t flags) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index || flags == XENT_DIRTY_NONE) return;

	if (flags & XENT_DIRTY_LAYOUT) {
		ctx->nodes.layout.dirty_flags [index] |= (flags | XENT_DIRTY_SELF);
		clear_measure_slot(&ctx->nodes, index);
		note_layout_dirty(ctx, index, node);
		propagate_layout_subtree(ctx, index);
	}
	if (flags & XENT_DIRTY_PAINT) ctx->nodes.layout.dirty_flags [index] |= XENT_DIRTY_PAINT;
	if (flags & XENT_DIRTY_SEMANTICS) {
		ctx->nodes.layout.dirty_flags [index] |= XENT_DIRTY_SEMANTICS;
		note_sem_dirty(ctx, index, node);
	}
}

void xent_mark_paint_dirty(XentCtx *ctx, XentNodeId node) { xent_mark_dirty(ctx, node, XENT_DIRTY_PAINT); }

void xent_clear_dirty(XentCtx *ctx, XentNodeId node, uint32_t mask) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return;
	ctx->nodes.layout.dirty_flags [index] &= ~mask;
}

uint32_t xent_sem_drain_dirty(XentCtx *ctx, XentNodeId *out_nodes, uint32_t max_count) {
	if (!ctx || ctx->sem_dirty_count == 0u) return 0u;
	uint32_t emitted = 0u;
	uint32_t read    = 0u;
	while (read < ctx->sem_dirty_count && (!out_nodes || emitted < max_count)) {
		XentNodeId node  = ctx->sem_dirty_nodes [read++];
		uint32_t   index = xent_live_index(ctx, node);
		if (index) {
			ctx->nodes.layout.dirty_queued [index] &= ( uint8_t ) ~XENT_QUEUE_SEM;
			if (ctx->nodes.layout.dirty_flags [index] & XENT_DIRTY_SEMANTICS) {
				ctx->nodes.layout.dirty_flags [index] &= ~XENT_DIRTY_SEMANTICS;
				if (out_nodes) out_nodes [emitted] = node;
				emitted++;
			}
		}
		else {
			uint32_t slot = xent_node_index(node);
			if (slot != 0u && slot < ctx->nodes.capacity)
				ctx->nodes.layout.dirty_queued [slot] &= ( uint8_t ) ~XENT_QUEUE_SEM;
		}
	}
	uint32_t rem = ctx->sem_dirty_count - read;
	if (rem > 0u) memmove(ctx->sem_dirty_nodes, ctx->sem_dirty_nodes + read, sizeof(XentNodeId) * rem);
	ctx->sem_dirty_count = rem;
	return emitted;
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
			ctx->nodes.layout.dirty_queued [index] |= XENT_QUEUE_LAYOUT;
			ctx->dirty_nodes [write++]              = node;
			continue;
		}
		uint32_t slot = xent_node_index(node);
		if (slot != 0u && slot < ctx->nodes.capacity)
			ctx->nodes.layout.dirty_queued [slot] &= ( uint8_t ) ~XENT_QUEUE_LAYOUT;
	}
	ctx->dirty_count = write;
}

void xent_dirty_clear_order(XentCtx *ctx) {
	for (uint32_t i = 0u; i < ctx->work_count; ++i) {
		uint32_t index = xent_live_index(ctx, ctx->work_order [i]);
		if (!index) continue;
		ctx->nodes.layout.dirty_flags [index]  &= ~XENT_DIRTY_LAYOUT_MASK;
		ctx->nodes.layout.dirty_queued [index] &= ( uint8_t ) ~XENT_QUEUE_LAYOUT;
	}
	xent_compact_dirty_nodes(ctx);
}
