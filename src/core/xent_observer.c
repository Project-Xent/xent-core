#include "../xent_internal.h"

static void compact_node_observers(XentCtx *ctx) {
	if (!ctx->node_observer_dead) return;

	uint32_t write = 0u;
	for (uint32_t read = 0u; read < ctx->node_observer_count; read++) {
		XentNodeObserverEntry entry = ctx->node_observers [read];
		if (entry.id == XENT_NODE_OBSERVER_INVALID) continue;
		ctx->node_observers [write++] = entry;
	}

	ctx->node_observer_count = write;
	ctx->node_observer_dead  = 0u;
}

static bool reserve_node_observers(XentCtx *ctx) {
	if (ctx->node_observer_count < ctx->node_observer_capacity) return true;
	if (ctx->node_observer_capacity > UINT32_MAX / 2u) return false;

	uint32_t               capacity = ctx->node_observer_capacity ? ctx->node_observer_capacity * 2u : 4u;
	XentNodeObserverEntry *entries
	  = ( XentNodeObserverEntry * ) realloc(ctx->node_observers, sizeof(*entries) * ( size_t ) capacity);
	if (!entries) return false;

	ctx->node_observers         = entries;
	ctx->node_observer_capacity = capacity;
	return true;
}

static XentObsId take_node_observer_id(XentCtx *ctx) {
	if (ctx->next_node_observer_id == UINT64_MAX) return XENT_NODE_OBSERVER_INVALID;
	return ++ctx->next_node_observer_id;
}

XentObsId xent_node_addobs(XentCtx *ctx, XentNodeObs const *observer) {
	if (!ctx || !observer || !observer->notify) return XENT_NODE_OBSERVER_INVALID;
	if (!ctx->node_observer_dispatch_depth) compact_node_observers(ctx);
	if (!reserve_node_observers(ctx)) return XENT_NODE_OBSERVER_INVALID;

	XentObsId id = take_node_observer_id(ctx);
	if (id == XENT_NODE_OBSERVER_INVALID) return id;

	ctx->node_observers [ctx->node_observer_count++] = (XentNodeObserverEntry) {id, *observer};
	return id;
}

bool xent_node_delobs(XentCtx *ctx, XentObsId observer) {
	if (!ctx || observer == XENT_NODE_OBSERVER_INVALID) return false;

	for (uint32_t i = 0u; i < ctx->node_observer_count; i++) {
		XentNodeObserverEntry *entry = &ctx->node_observers [i];
		if (entry->id != observer) continue;
		entry->id                = XENT_NODE_OBSERVER_INVALID;
		entry->observer.notify   = NULL;
		entry->observer.userdata = NULL;
		ctx->node_observer_dead++;
		if (!ctx->node_observer_dispatch_depth) compact_node_observers(ctx);
		return true;
	}

	return false;
}

void xent_notify_node_observers(XentCtx *ctx, XentNodeEvent const *lifecycle) {
	if (!ctx || !lifecycle) return;

	uint32_t count = ctx->node_observer_count;
	ctx->node_observer_dispatch_depth++;
	for (uint32_t i = 0u; i < count; i++) {
		XentNodeObs observer = ctx->node_observers [i].observer;
		if (ctx->node_observers [i].id == XENT_NODE_OBSERVER_INVALID) continue;
		observer.notify(ctx, lifecycle, observer.userdata);
	}
	ctx->node_observer_dispatch_depth--;

	if (!ctx->node_observer_dispatch_depth) compact_node_observers(ctx);
}
