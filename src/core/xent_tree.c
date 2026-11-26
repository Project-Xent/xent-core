#include "../xent_internal.h"
#include "../xent_alloc_internal.h"

uint32_t xent_live_index(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_node_index(node);
	if (!ctx || index == 0u || index >= ctx->nodes.capacity) return 0u;
	if (!ctx->nodes.lifetime.alive [index]) return 0u;
	if (ctx->nodes.lifetime.generation [index] != xent_node_generation(node)) return 0u;
	return index;
}

bool        xent_node_valid(XentCtx const *ctx, XentNodeId node) { return xent_live_index(ctx, node) != 0u; }

static bool push_free_index(XentCtx *ctx, uint32_t index) {
	if (ctx->free_count == ctx->free_capacity) {
		if (ctx->free_capacity > UINT32_MAX / 2u) return false;
		uint32_t  new_cap = ctx->free_capacity ? ctx->free_capacity * 2u : 64u;
		uint32_t *new_mem = ( uint32_t * ) xent_realloc_internal(
		  XENT_ALLOC_TOPOLOGY_MUTATION, ctx->free_indices, sizeof(uint32_t) * ( size_t ) new_cap
		);
		if (!new_mem) return false;
		ctx->free_indices  = new_mem;
		ctx->free_capacity = new_cap;
	}
	ctx->free_indices [ctx->free_count++] = index;
	return true;
}

static void
notify_lifecycle(XentCtx *ctx, XentNodeId node, XentNodeEventKind event, XentNodeId old_parent, XentNodeId new_parent) {
	XentNodeEvent lifecycle = {node, old_parent, new_parent, event};
	xent_notify_node_observers(ctx, &lifecycle);
}

static void unlink_child(XentCtx *ctx, uint32_t parent, uint32_t child) {
	uint32_t prev = xent_node_index(ctx->nodes.topology.prev_sibling [child]);
	uint32_t next = xent_node_index(ctx->nodes.topology.next_sibling [child]);
	if (prev == 0u) ctx->nodes.topology.first_child [parent] = ctx->nodes.topology.next_sibling [child];
	else ctx->nodes.topology.next_sibling [prev] = ctx->nodes.topology.next_sibling [child];
	if (next == 0u) ctx->nodes.topology.last_child [parent] = ctx->nodes.topology.prev_sibling [child];
	else ctx->nodes.topology.prev_sibling [next] = ctx->nodes.topology.prev_sibling [child];

	ctx->nodes.topology.parent [child]       = XENT_NODE_INVALID;
	ctx->nodes.topology.next_sibling [child] = XENT_NODE_INVALID;
	ctx->nodes.topology.prev_sibling [child] = XENT_NODE_INVALID;
	ctx->nodes.topology.child_count [parent]--;
}

static uint32_t alloc_slot(XentCtx *ctx, uint32_t *out_generation) {
	while (ctx->free_count > 0u) {
		uint32_t index = ctx->free_indices [--ctx->free_count];
		if (index == 0u || index > ctx->nodes.count || ctx->nodes.lifetime.alive [index]) continue;
		uint32_t gen    = ctx->nodes.lifetime.generation [index];
		*out_generation = gen == 0u ? 1u : gen;
		return index;
	}

	if (ctx->nodes.count >= UINT32_MAX - 1u) return 0u;
	uint32_t index   = ctx->nodes.count + 1u;
	ctx->nodes.count = index;
	if (!xent_ensure_node_capacity(ctx, index + 1u)) {
		ctx->nodes.count--;
		return 0u;
	}
	uint32_t gen    = ctx->nodes.lifetime.generation [index];
	uint32_t min_g  = ctx->nodes.min_generation ? ctx->nodes.min_generation : 1u;
	*out_generation = gen < min_g ? min_g : gen;
	return index;
}

XentNodeId xent_node_create(XentCtx *ctx) {
	if (!ctx) return XENT_NODE_INVALID;
	if (ctx->node_observer_dispatch_depth) return XENT_NODE_INVALID;

	uint32_t gen   = 0u;
	uint32_t index = alloc_slot(ctx, &gen);
	if (!index) return XENT_NODE_INVALID;

	xent_release_node_cold_slots(&ctx->nodes, index);
	xent_arena_reset_node(&ctx->nodes, index);
	ctx->nodes.lifetime.generation [index] = gen;
	ctx->nodes.lifetime.alive [index]      = 1u;
	ctx->nodes.live_count++;
	XentNodeId id = xent_node_make(index, gen);
	xent_mark_dirty(ctx, id, XENT_DIRTY_LAYOUT);
	return id;
}

static void compact_free_indices(XentCtx *ctx, uint32_t limit) {
	uint32_t write = 0u;
	for (uint32_t i = 0u; i < ctx->free_count; ++i)
		if (ctx->free_indices [i] <= limit) ctx->free_indices [write++] = ctx->free_indices [i];
	ctx->free_count = write;
}

static void trim_tail_slots(XentCtx *ctx) {
	uint32_t prev = ctx->nodes.count;
	while (ctx->nodes.count > 0u
	       && !ctx->nodes.lifetime.alive [ctx->nodes.count]
	       && ctx->nodes.lifetime.generation [ctx->nodes.count] != UINT32_MAX)
	{
		ctx->nodes.count--;
	}
	if (ctx->nodes.count != prev) compact_free_indices(ctx, ctx->nodes.count);
}

static void destroy_single_node(XentCtx *ctx, XentNodeId node, XentNodeId old_parent) {
	uint32_t index = xent_node_index(node);

	xent_release_node_cold_slots(&ctx->nodes, index);
	xent_extmeasure_on_destroy(ctx, node);
	xent_invalidate_root_snapshots(ctx, node);

	uint32_t gen                      = ctx->nodes.lifetime.generation [index];
	ctx->nodes.lifetime.alive [index] = 0u;
	if (ctx->nodes.live_count > 0u) ctx->nodes.live_count--;
	ctx->nodes.topology.parent [index]       = XENT_NODE_INVALID;
	ctx->nodes.topology.first_child [index]  = XENT_NODE_INVALID;
	ctx->nodes.topology.last_child [index]   = XENT_NODE_INVALID;
	ctx->nodes.topology.next_sibling [index] = XENT_NODE_INVALID;
	ctx->nodes.topology.prev_sibling [index] = XENT_NODE_INVALID;
	ctx->nodes.topology.child_count [index]  = 0u;
	ctx->nodes.layout.dirty_flags [index]    = XENT_DIRTY_NONE;
	ctx->nodes.layout.dirty_queued [index]   = 0u;
	notify_lifecycle(ctx, node, XENT_NODE_EVENT_DESTROY, old_parent, XENT_NODE_INVALID);

	if (gen == UINT32_MAX) return;
	ctx->nodes.lifetime.generation [index] = gen + 1u;
	( void ) push_free_index(ctx, index);
}

static void destroy_subtree(XentCtx *ctx, XentNodeId root, XentNodeId root_parent) {
	XentNodeId node = root;
	for (;;) {
		XentNodeId child = ctx->nodes.topology.first_child [xent_node_index(node)];
		if (child != XENT_NODE_INVALID) {
			node = child;
			continue;
		}

		XentNodeId parent = ctx->nodes.topology.parent [xent_node_index(node)];
		XentNodeId next   = ctx->nodes.topology.next_sibling [xent_node_index(node)];
		if (parent != XENT_NODE_INVALID) unlink_child(ctx, xent_node_index(parent), xent_node_index(node));

		bool done = node == root;
		destroy_single_node(ctx, node, done ? root_parent : parent);
		if (done) return;
		node = next != XENT_NODE_INVALID ? next : parent;
	}
}

bool xent_node_destroy(XentCtx *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	if (ctx->node_observer_dispatch_depth) return false;

	XentNodeId parent = ctx->nodes.topology.parent [index];
	if (parent != XENT_NODE_INVALID) {
		unlink_child(ctx, xent_node_index(parent), index);
		xent_mark_dirty(ctx, parent, XENT_DIRTY_SUBTREE | XENT_DIRTY_LAYOUT);
	}

	destroy_subtree(ctx, node, parent);
	if (ctx->nodes.capacity > 128u) {
		trim_tail_slots(ctx);
		xent_arena_maybe_shrink(ctx);
	}
	return true;
}

typedef enum XentPlaceMode
{
	XENT_PLACE_ANY,
	XENT_PLACE_INSERT,
	XENT_PLACE_MOVE,
	XENT_PLACE_REPARENT,
} XentPlaceMode;

static bool is_ancestor_index(XentCtx const *ctx, uint32_t ancestor, uint32_t node) {
	uint32_t cursor = ancestor;
	while (cursor != 0u) {
		if (cursor == node) return true;
		cursor = xent_node_index(ctx->nodes.topology.parent [cursor]);
	}
	return false;
}

typedef struct Place {
	XentNodeId parent, child, before;
	uint32_t   pi, ci, bi;
} Place;

static bool place_resolve(XentCtx *ctx, Place *p) {
	p->pi = xent_live_index(ctx, p->parent);
	p->ci = xent_live_index(ctx, p->child);
	if (!p->pi || !p->ci || p->pi == p->ci || ctx->node_observer_dispatch_depth) return false;
	return !is_ancestor_index(ctx, p->pi, p->ci);
}

static bool place_mode_ok(XentPlaceMode mode, XentNodeId parent, XentNodeId old_parent) {
	if (mode == XENT_PLACE_INSERT) return old_parent == XENT_NODE_INVALID;
	if (mode == XENT_PLACE_MOVE) return old_parent == parent;
	if (mode == XENT_PLACE_REPARENT) return old_parent != parent;
	return true;
}

static bool place_resolve_before(XentCtx *ctx, Place *p) {
	if (p->before == XENT_NODE_INVALID) {
		p->bi = 0u;
		return true;
	}
	p->bi = xent_live_index(ctx, p->before);
	return p->bi && ctx->nodes.topology.parent [p->bi] == p->parent;
}

static bool place_is_noop(XentCtx const *ctx, Place const *p) {
	XentNodeTopologyStore const *t = &ctx->nodes.topology;
	if (t->parent [p->ci] != p->parent) return false;
	if (p->bi == 0u) return t->last_child [p->pi] == p->child;
	return t->next_sibling [p->ci] == p->before;
}

static void link_before(XentCtx *ctx, Place const *p) {
	XentNodeTopologyStore *t    = &ctx->nodes.topology;
	XentNodeId             prev = p->bi ? t->prev_sibling [p->bi] : t->last_child [p->pi];
	t->prev_sibling [p->ci]     = prev;
	t->next_sibling [p->ci]     = p->before;
	if (p->bi) t->prev_sibling [p->bi] = p->child;
	else t->last_child [p->pi] = p->child;
	if (prev == XENT_NODE_INVALID) t->first_child [p->pi] = p->child;
	else t->next_sibling [xent_node_index(prev)] = p->child;
	t->parent [p->ci]       = p->parent;
	t->child_count [p->pi] += 1u;
}

static bool place_before(XentCtx *ctx, XentNodeId parent, XentNodeId child, XentNodeId before, XentPlaceMode mode) {
	Place p = {.parent = parent, .child = child, .before = before};
	if (!place_resolve(ctx, &p)) return false;
	XentNodeId old_parent = ctx->nodes.topology.parent [p.ci];
	if (!place_mode_ok(mode, parent, old_parent)) return false;
	if (before == child) return old_parent == parent;
	if (!place_resolve_before(ctx, &p)) return false;
	if (place_is_noop(ctx, &p)) return true;

	if (old_parent != XENT_NODE_INVALID) unlink_child(ctx, xent_node_index(old_parent), p.ci);
	link_before(ctx, &p);

	if (old_parent != XENT_NODE_INVALID && old_parent != parent)
		xent_mark_dirty(ctx, old_parent, XENT_DIRTY_SUBTREE | XENT_DIRTY_LAYOUT);
	xent_mark_dirty(ctx, parent, XENT_DIRTY_SUBTREE | XENT_DIRTY_LAYOUT);
	notify_lifecycle(ctx, child, XENT_NODE_EVENT_REPARENT, old_parent, parent);
	return true;
}

bool xent_node_append(XentCtx *ctx, XentNodeId parent, XentNodeId child) {
	return place_before(ctx, parent, child, XENT_NODE_INVALID, XENT_PLACE_ANY);
}

bool xent_node_insert(XentCtx *ctx, XentNodeId parent, XentNodeId child, XentNodeId before) {
	return place_before(ctx, parent, child, before, XENT_PLACE_INSERT);
}

bool xent_node_move(XentCtx *ctx, XentNodeId parent, XentNodeId child, XentNodeId before) {
	return place_before(ctx, parent, child, before, XENT_PLACE_MOVE);
}

bool xent_node_reparent(XentCtx *ctx, XentNodeId new_parent, XentNodeId child, XentNodeId before) {
	return place_before(ctx, new_parent, child, before, XENT_PLACE_REPARENT);
}

bool xent_node_remove(XentCtx *ctx, XentNodeId parent, XentNodeId child) {
	uint32_t parent_index = xent_live_index(ctx, parent);
	uint32_t child_index  = xent_live_index(ctx, child);
	if (!parent_index || !child_index) return false;
	if (ctx->node_observer_dispatch_depth) return false;
	if (ctx->nodes.topology.parent [child_index] != parent) return false;

	unlink_child(ctx, parent_index, child_index);
	xent_mark_dirty(ctx, parent, XENT_DIRTY_SUBTREE | XENT_DIRTY_LAYOUT);
	notify_lifecycle(ctx, child, XENT_NODE_EVENT_REPARENT, parent, XENT_NODE_INVALID);
	return true;
}

XentNodeId xent_node_parent(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return XENT_NODE_INVALID;
	return ctx->nodes.topology.parent [index];
}

XentNodeId xent_node_first(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return XENT_NODE_INVALID;
	return ctx->nodes.topology.first_child [index];
}

XentNodeId xent_node_last(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return XENT_NODE_INVALID;
	return ctx->nodes.topology.last_child [index];
}

XentNodeId xent_node_next(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return XENT_NODE_INVALID;
	return ctx->nodes.topology.next_sibling [index];
}

XentNodeId xent_node_prev(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return XENT_NODE_INVALID;
	return ctx->nodes.topology.prev_sibling [index];
}

uint32_t xent_node_nchild(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return 0u;
	return ctx->nodes.topology.child_count [index];
}
