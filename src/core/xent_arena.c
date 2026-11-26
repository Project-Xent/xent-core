#include "../xent_internal.h"
#include "../xent_alloc_internal.h"

#define XENT_SLAB_ALIGN 16u

typedef struct XentHotField {
	size_t offset;
	size_t elem_size;
} XentHotField;

#define XENT_HOT_FIELD(field) {offsetof(XentNodeStore, field), sizeof((( XentNodeStore * ) 0)->field [0])}

static XentHotField const XENT_HOT_FIELDS [] = {
  XENT_HOT_FIELD(lifetime.alive),
  XENT_HOT_FIELD(lifetime.generation),
  XENT_HOT_FIELD(topology.parent),
  XENT_HOT_FIELD(topology.first_child),
  XENT_HOT_FIELD(topology.last_child),
  XENT_HOT_FIELD(topology.next_sibling),
  XENT_HOT_FIELD(topology.prev_sibling),
  XENT_HOT_FIELD(topology.child_count),
  XENT_HOT_FIELD(layout.protocol),
  XENT_HOT_FIELD(layout.direction),
  XENT_HOT_FIELD(layout.collapsed),
  XENT_HOT_FIELD(layout.wrap_content_w),
  XENT_HOT_FIELD(layout.wrap_content_h),
  XENT_HOT_FIELD(layout.scroller),
  XENT_HOT_FIELD(layout.dirty_flags),
  XENT_HOT_FIELD(layout.dirty_queued),
  XENT_HOT_FIELD(layout.decided_w),
  XENT_HOT_FIELD(layout.decided_h),
  XENT_HOT_FIELD(layout.abs_x),
  XENT_HOT_FIELD(layout.abs_y),
  XENT_HOT_FIELD(layout.style_w),
  XENT_HOT_FIELD(layout.style_h),
  XENT_HOT_FIELD(layout.style_w_percent),
  XENT_HOT_FIELD(layout.style_h_percent),
  XENT_HOT_FIELD(layout.aspect_ratio),
  XENT_HOT_FIELD(layout.min_w),
  XENT_HOT_FIELD(layout.min_h),
  XENT_HOT_FIELD(layout.max_w),
  XENT_HOT_FIELD(layout.max_h),
  XENT_HOT_FIELD(layout.margin_l),
  XENT_HOT_FIELD(layout.margin_t),
  XENT_HOT_FIELD(layout.margin_r),
  XENT_HOT_FIELD(layout.margin_b),
  XENT_HOT_FIELD(layout.padding_l),
  XENT_HOT_FIELD(layout.padding_t),
  XENT_HOT_FIELD(layout.padding_r),
  XENT_HOT_FIELD(layout.padding_b),
  XENT_HOT_FIELD(layout.gap),
  XENT_HOT_FIELD(layout.abs_pos_x),
  XENT_HOT_FIELD(layout.abs_pos_y),
  XENT_HOT_FIELD(layout.z_index),
  XENT_HOT_FIELD(flex.grow),
  XENT_HOT_FIELD(flex.shrink),
  XENT_HOT_FIELD(flex.basis),
  XENT_HOT_FIELD(flex.direction),
  XENT_HOT_FIELD(flex.wrap),
  XENT_HOT_FIELD(flex.justify_content),
  XENT_HOT_FIELD(flex.align_items),
  XENT_HOT_FIELD(flex.align_self),
  XENT_HOT_FIELD(flex.align_content),
  XENT_HOT_FIELD(cold.text),
  XENT_HOT_FIELD(cold.semantics),
  XENT_HOT_FIELD(cold.grid),
  XENT_HOT_FIELD(cold.stack),
  XENT_HOT_FIELD(cold.focus),
  XENT_HOT_FIELD(cold.measure),
};

#undef XENT_HOT_FIELD

static uint32_t hot_field_count(void) { return ( uint32_t ) (sizeof(XENT_HOT_FIELDS) / sizeof(XENT_HOT_FIELDS [0])); }

static size_t   slab_total_bytes(uint32_t cap) {
	size_t   total = 0u;
	uint32_t count = hot_field_count();
	for (uint32_t i = 0u; i < count; ++i) {
		size_t bytes = XENT_HOT_FIELDS [i].elem_size * ( size_t ) cap;
		total        = xent_align_up_size(total + bytes, XENT_SLAB_ALIGN);
	}
	return total;
}

static void migrate_hot_slab(XentNodeStore *nodes, uint8_t *new_slab, uint32_t copy_cap, uint32_t new_cap) {
	size_t   cursor = 0u;
	uint32_t count  = hot_field_count();
	for (uint32_t i = 0u; i < count; ++i) {
		size_t   esize   = XENT_HOT_FIELDS [i].elem_size;
		void   **slot    = ( void ** ) (( uint8_t * ) nodes + XENT_HOT_FIELDS [i].offset);
		void    *old_ptr = *slot;
		uint8_t *dst     = new_slab + cursor;
		size_t   keep    = esize * ( size_t ) copy_cap;
		size_t   total   = esize * ( size_t ) new_cap;
		if (old_ptr && keep > 0u) memcpy(dst, old_ptr, keep);
		if (total > keep) memset(dst + keep, 0, total - keep);
		*slot  = dst;
		cursor = xent_align_up_size(cursor + total, XENT_SLAB_ALIGN);
	}
	free(nodes->hot_slab);
	nodes->hot_slab = new_slab;
}

static void init_node_defaults(XentNodeStore *nodes, uint32_t from, uint32_t to) {
	for (uint32_t i = from; i < to; ++i) {
		nodes->layout.style_w [i]         = NAN;
		nodes->layout.style_h [i]         = NAN;
		nodes->layout.style_w_percent [i] = NAN;
		nodes->layout.style_h_percent [i] = NAN;
		nodes->layout.aspect_ratio [i]    = NAN;
		nodes->layout.min_w [i]           = 0.0f;
		nodes->layout.min_h [i]           = 0.0f;
		nodes->layout.max_w [i]           = INFINITY;
		nodes->layout.max_h [i]           = INFINITY;
		nodes->layout.protocol [i]        = ( uint8_t ) XENT_PROTOCOL_ABSOLUTE;
		nodes->layout.direction [i]       = ( uint8_t ) XENT_DIRECTION_INHERIT;
		nodes->flex.basis [i]             = NAN;
		nodes->flex.shrink [i]            = 1.0f;
		nodes->flex.direction [i]         = ( uint8_t ) XENT_FLEX_ROW;
		nodes->flex.wrap [i]              = ( uint8_t ) XENT_FLEX_NO_WRAP;
		nodes->flex.justify_content [i]   = ( uint8_t ) XENT_FLEX_JUSTIFY_START;
		nodes->flex.align_items [i]       = ( uint8_t ) XENT_FLEX_ALIGN_STRETCH;
		nodes->flex.align_self [i]        = ( uint8_t ) XENT_FLEX_ALIGN_AUTO;
		nodes->flex.align_content [i]     = ( uint8_t ) XENT_FLEX_ALIGN_CONTENT_START;
	}
}

static uint32_t next_node_capacity(uint32_t old_cap, uint32_t needed) {
	uint32_t new_cap = old_cap ? old_cap : 32u;
	while (new_cap <= needed) {
		if (new_cap > UINT32_MAX / 2u) return 0u;
		new_cap *= 2u;
	}
	return new_cap;
}

char *xent_strdup(XentAllocSite site, char const *s) {
	if (!s) return NULL;
	size_t len  = strlen(s);
	char  *copy = ( char * ) xent_alloc_internal(site, len + 1u);
	if (!copy) return NULL;
	memcpy(copy, s, len + 1u);
	return copy;
}

void xent_arena_reset_node(XentNodeStore *nodes, uint32_t i) {
	uint32_t count = hot_field_count();
	for (uint32_t f = 0u; f < count; ++f) {
		uint8_t *base = *( uint8_t ** ) (( uint8_t * ) nodes + XENT_HOT_FIELDS [f].offset);
		memset(base + ( size_t ) i * XENT_HOT_FIELDS [f].elem_size, 0, XENT_HOT_FIELDS [f].elem_size);
	}
	init_node_defaults(nodes, i, i + 1u);
}

bool xent_ensure_node_capacity(XentCtx *ctx, uint32_t needed) {
	if (!ctx || needed == 0u) return false;
	XentNodeStore *n = &ctx->nodes;
	if (needed <= n->capacity) return true;

	uint32_t old_cap = n->capacity;
	uint32_t new_cap = next_node_capacity(old_cap, needed);
	if (new_cap == 0u) return false;

	uint8_t *new_slab = ( uint8_t * ) xent_alloc_internal(XENT_ALLOC_NODE_GROW, slab_total_bytes(new_cap));
	if (!new_slab) return false;

	migrate_hot_slab(n, new_slab, old_cap, new_cap);
	init_node_defaults(n, old_cap, new_cap);
	n->capacity = new_cap;
	return true;
}

static void compact_free_indices_below(XentCtx *ctx, uint32_t limit) {
	uint32_t write = 0u;
	for (uint32_t i = 0u; i < ctx->free_count; ++i)
		if (ctx->free_indices [i] <= limit) ctx->free_indices [write++] = ctx->free_indices [i];
	ctx->free_count = write;
}

static void shrink_aux_buffers(XentCtx *ctx) {
	if (ctx->free_count == 0u && ctx->free_capacity > 256u) {
		uint32_t *mem = ( uint32_t * ) realloc(ctx->free_indices, sizeof(uint32_t) * 64u);
		if (mem) {
			ctx->free_indices  = mem;
			ctx->free_capacity = 64u;
		}
	}
	if (ctx->work_count == 0u && ctx->work_capacity > 1024u) {
		XentNodeId *mem = ( XentNodeId * ) realloc(ctx->work_order, sizeof(XentNodeId) * 64u);
		if (mem) {
			ctx->work_order    = mem;
			ctx->work_capacity = 64u;
		}
	}
}

void xent_arena_maybe_shrink(XentCtx *ctx) {
	XentNodeStore *n = &ctx->nodes;
	if (n->capacity <= 128u) return;

	uint32_t target = 32u;
	while (target <= n->count) target *= 2u;
	if (n->live_count == 0u) {
		n->count        = 0u;
		ctx->free_count = 0u;
		target          = 32u;
	}
	else if (n->count * 4u >= n->capacity || target >= n->capacity) return;

	uint8_t *new_slab = ( uint8_t * ) malloc(slab_total_bytes(target));
	if (!new_slab) return;
	if (n->min_generation < UINT32_MAX) n->min_generation++;
	uint32_t keep = (n->count + 1u) < target ? (n->count + 1u) : target;
	compact_free_indices_below(ctx, n->count);
	migrate_hot_slab(n, new_slab, keep, target);
	if (keep < target) init_node_defaults(n, keep, target);
	n->capacity = target;
	shrink_aux_buffers(ctx);
}

void xent_arena_free(XentNodeStore *nodes) {
	free(nodes->hot_slab);
	nodes->hot_slab = NULL;
	uint32_t count  = hot_field_count();
	for (uint32_t f = 0u; f < count; ++f) {
		void **slot = ( void ** ) (( uint8_t * ) nodes + XENT_HOT_FIELDS [f].offset);
		*slot       = NULL;
	}
	xent_pool_destroy(&nodes->text_pool);
	xent_pool_destroy(&nodes->sem_pool);
	xent_pool_destroy(&nodes->grid_pool);
	xent_pool_destroy(&nodes->stack_pool);
	xent_pool_destroy(&nodes->focus_pool);
	xent_pool_destroy(&nodes->measure_pool);
}

static void pool_compact_free_list(XentColdPool *pool) {
	uint32_t write = 0u;
	for (uint32_t i = 0u; i < pool->free_count; ++i)
		if (pool->free_list [i] > 0u && pool->free_list [i] <= pool->count)
			pool->free_list [write++] = pool->free_list [i];
	pool->free_count = write;
}

static void pool_push_free(XentColdPool *pool, uint32_t slot) {
	if (pool->free_count == pool->free_cap) {
		uint32_t  cap  = pool->free_cap ? pool->free_cap * 2u : 16u;
		uint32_t *list = ( uint32_t * ) realloc(pool->free_list, sizeof(uint32_t) * ( size_t ) cap);
		if (!list) return;
		pool->free_list = list;
		pool->free_cap  = cap;
	}
	pool->free_list [pool->free_count++] = slot;
}

static uint32_t pool_take_slot(XentColdPool *pool, size_t elem_size) {
	while (pool->free_count > 0u) {
		uint32_t slot = pool->free_list [--pool->free_count];
		if (slot == 0u || slot > pool->count || pool->data [( size_t ) (slot - 1u) * elem_size] != 0u) continue;
		return slot;
	}
	if (pool->count == pool->capacity) {
		uint32_t new_cap  = pool->capacity ? pool->capacity * 2u : 16u;
		uint8_t *new_data = ( uint8_t * ) realloc(pool->data, elem_size * ( size_t ) new_cap);
		if (!new_data) return 0u;
		pool->data     = new_data;
		pool->capacity = new_cap;
	}
	return ++pool->count;
}

void *xent_pool_ensure(XentColdPool *pool, size_t elem_size, uint32_t *slot_ref) {
	if (*slot_ref != 0u) return pool->data + (( size_t ) (*slot_ref - 1u) * elem_size);
	uint32_t slot = pool_take_slot(pool, elem_size);
	if (!slot) return NULL;

	uint8_t *ptr = pool->data + (( size_t ) (slot - 1u) * elem_size);
	memset(ptr, 0, elem_size);
	*ptr = 1u;
	pool->live_count++;
	*slot_ref = slot;
	return ptr;
}

static uint32_t pool_shrink_target(XentColdPool const *pool) {
	if (pool->count * 4u >= pool->capacity || pool->capacity <= 64u) return pool->capacity;
	uint32_t cap = 64u;
	while (cap < pool->count) cap *= 2u;
	return cap;
}

static void pool_shrink(XentColdPool *pool, size_t elem_size) {
	uint32_t prev_count = pool->count;
	while (pool->count > 0u && pool->data [( size_t ) (pool->count - 1u) * elem_size] == 0u) pool->count--;
	if (pool->count != prev_count) pool_compact_free_list(pool);

	if (pool->live_count == 0u && pool->capacity > 64u) {
		xent_pool_destroy(pool);
		return;
	}
	uint32_t new_cap = pool_shrink_target(pool);
	if (new_cap >= pool->capacity) return;

	uint8_t *shrunk = ( uint8_t * ) realloc(pool->data, elem_size * ( size_t ) new_cap);
	if (!shrunk) return;
	pool->data     = shrunk;
	pool->capacity = new_cap;
}

void xent_pool_release(XentColdPool *pool, size_t elem_size, uint32_t *slot_ref) {
	uint32_t slot = *slot_ref;
	if (!slot) return;
	*slot_ref = 0u;
	if (slot > pool->count) return;

	uint8_t *ptr = pool->data + (( size_t ) (slot - 1u) * elem_size);
	if (!*ptr) return;
	*ptr = 0u;
	pool->live_count--;

	if (slot < pool->count) pool_push_free(pool, slot);
	pool_shrink(pool, elem_size);
}

void xent_pool_destroy(XentColdPool *pool) {
	free(pool->data);
	free(pool->free_list);
	*pool = (XentColdPool) {0};
}

XentTextComp *xent_text_comp_ensure(XentNodeStore *nodes, uint32_t index) {
	bool          fresh = nodes->cold.text [index] == 0u;
	XentTextComp *c     = ( XentTextComp * ) xent_pool_ensure(&nodes->text_pool, sizeof(*c), &nodes->cold.text [index]);
	if (c && fresh) {
		c->font_size                   = NAN;
		c->line_break_policy           = ( uint8_t ) XENT_LINEBREAK_CHARWRAP;
		c->trim                        = ( uint8_t ) XENT_TEXT_TRIM_NONE;
		c->intrinsic_constraint_w      = NAN;
		c->intrinsic_line_break_policy = ( uint8_t ) XENT_LINEBREAK_CHARWRAP;
		c->intrinsic_width_mode        = ( uint8_t ) XENT_MEASURE_UNDEFINED;
	}
	return c;
}

XentSemComp *xent_sem_comp_ensure(XentNodeStore *nodes, uint32_t index) {
	bool         fresh = nodes->cold.semantics [index] == 0u;
	XentSemComp *c = ( XentSemComp * ) xent_pool_ensure(&nodes->sem_pool, sizeof(*c), &nodes->cold.semantics [index]);
	if (c && fresh) c->enabled = 1u;
	return c;
}

XentGridComp *xent_grid_comp_ensure(XentNodeStore *nodes, uint32_t index) {
	bool          fresh = nodes->cold.grid [index] == 0u;
	XentGridComp *c     = ( XentGridComp * ) xent_pool_ensure(&nodes->grid_pool, sizeof(*c), &nodes->cold.grid [index]);
	if (c && fresh) {
		c->row_span    = 1u;
		c->column_span = 1u;
	}
	return c;
}

XentStackComp *xent_stack_comp_ensure(XentNodeStore *nodes, uint32_t index) {
	bool           fresh = nodes->cold.stack [index] == 0u;
	XentStackComp *c = ( XentStackComp * ) xent_pool_ensure(&nodes->stack_pool, sizeof(*c), &nodes->cold.stack [index]);
	if (c && fresh) {
		c->axis  = ( uint8_t ) XENT_AXIS_HORIZONTAL;
		c->align = ( uint8_t ) XENT_STACK_ALIGN_START;
	}
	return c;
}

XentFocusComp *xent_focus_comp_ensure(XentNodeStore *nodes, uint32_t index) {
	return ( XentFocusComp * ) xent_pool_ensure(&nodes->focus_pool, sizeof(XentFocusComp), &nodes->cold.focus [index]);
}

XentMeasureCache *xent_measure_cache_ensure(XentNodeStore *nodes, uint32_t index) {
	return ( XentMeasureCache * ) xent_pool_ensure(
	  &nodes->measure_pool, sizeof(XentMeasureCache), &nodes->cold.measure [index]
	);
}

void xent_release_node_cold_slots(XentNodeStore *nodes, uint32_t index) {
	if (nodes->cold.text [index]) {
		xent_text_release_node(nodes, index);
		xent_pool_release(&nodes->text_pool, sizeof(XentTextComp), &nodes->cold.text [index]);
	}
	if (nodes->cold.semantics [index]) {
		XentSemComp *s = ( XentSemComp * ) xent_pool_get(&nodes->sem_pool, sizeof(*s), nodes->cold.semantics [index]);
		if (s) {
			free(s->label);
			s->label = NULL;
		}
		xent_pool_release(&nodes->sem_pool, sizeof(XentSemComp), &nodes->cold.semantics [index]);
	}
	if (nodes->cold.grid [index]) {
		XentGridComp *g = ( XentGridComp * ) xent_pool_get(&nodes->grid_pool, sizeof(*g), nodes->cold.grid [index]);
		if (g) {
			xent_grid_def_free(g->def);
			g->def = NULL;
		}
		xent_pool_release(&nodes->grid_pool, sizeof(XentGridComp), &nodes->cold.grid [index]);
	}
	if (nodes->cold.stack [index])
		xent_pool_release(&nodes->stack_pool, sizeof(XentStackComp), &nodes->cold.stack [index]);
	if (nodes->cold.focus [index])
		xent_pool_release(&nodes->focus_pool, sizeof(XentFocusComp), &nodes->cold.focus [index]);
	if (nodes->cold.measure [index])
		xent_pool_release(&nodes->measure_pool, sizeof(XentMeasureCache), &nodes->cold.measure [index]);
}
