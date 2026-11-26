#ifndef XENT_INTERNAL_H
#define XENT_INTERNAL_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "xent/xent.h"

#include "core/xent_dirty.h"
#include "core/xent_focus.h"
#include "core/xent_scratch.h"
#include "layout/xent_layout_grid_def.h"
#include "xent_alloc_internal.h"

#define XENT_CACHE_MAX_CAP       4096u
#define XENT_MEASURE_CACHE_SLOTS 4u
#define XENT_ROOT_SNAPSHOT_SLOTS 8u

typedef struct XentLayoutRequest {
	XentCtx   *ctx;
	XentNodeId node;
	float      available_w;
	float      available_h;
	float      origin_x;
	float      origin_y;
	bool       definite_w;
	bool       definite_h;
} XentLayoutRequest;

typedef int (*XentSortCompareFn)(void const *a, void const *b, void *context);

typedef struct XentTextStyleBlob {
	uint8_t const *bytes;
	uint32_t       size;
} XentTextStyleBlob;

typedef struct XentTextCacheKey {
	char const         *text;
	XentTextStyleBlob   style;
	float               font_size;
	uint16_t            font_weight;
	float               width_constraint;
	uint32_t            max_lines;
	XentTextTrim        trim;
	XentLineBreakPolicy line_break_policy;
	XentMeasureMode     width_mode;
} XentTextCacheKey;

typedef struct XentCachedTextKey {
	uint64_t hash;
	char    *text;
	uint8_t *style_bytes;
	uint32_t style_size;
	float    font_size;
	uint16_t font_weight;
	float    width_constraint;
	uint32_t max_lines;
	uint8_t  trim;
	uint8_t  line_break_policy;
	uint8_t  width_mode;
} XentCachedTextKey;

typedef struct XentTextCacheEntry {
	XentCachedTextKey key;
	XentTextMetrics   metrics;
	uint64_t          last_used;
} XentTextCacheEntry;

typedef struct XentTextCache {
	XentTextCacheEntry *entries;
	uint32_t            count;
	uint32_t            capacity;
	uint64_t            clock;
	XentTextCacheStats  stats;
} XentTextCache;

typedef struct XentNodeLifetimeStore {
	uint8_t  *alive;
	uint32_t *generation;
} XentNodeLifetimeStore;

typedef struct XentNodeObserverEntry {
	XentObsId   id;
	XentNodeObs observer;
} XentNodeObserverEntry;

typedef struct XentNodeTopologyStore {
	XentNodeId *parent;
	XentNodeId *first_child;
	XentNodeId *last_child;
	XentNodeId *next_sibling;
	XentNodeId *prev_sibling;
	uint32_t   *child_count;
} XentNodeTopologyStore;

typedef struct XentNodeLayoutStore {
	uint8_t  *protocol;
	uint8_t  *direction;
	uint8_t  *collapsed;
	uint8_t  *wrap_content_w;
	uint8_t  *wrap_content_h;
	uint8_t  *scroller;
	uint32_t *dirty_flags;
	uint8_t  *dirty_queued;
	float    *decided_w;
	float    *decided_h;
	float    *abs_x;
	float    *abs_y;
	float    *style_w;
	float    *style_h;
	float    *style_w_percent;
	float    *style_h_percent;
	float    *aspect_ratio;
	float    *min_w;
	float    *min_h;
	float    *max_w;
	float    *max_h;
	float    *margin_l;
	float    *margin_t;
	float    *margin_r;
	float    *margin_b;
	float    *padding_l;
	float    *padding_t;
	float    *padding_r;
	float    *padding_b;
	float    *gap;
	float    *abs_pos_x;
	float    *abs_pos_y;
	int32_t  *z_index;
} XentNodeLayoutStore;

typedef struct XentNodeFlexStore {
	float   *grow;
	float   *shrink;
	float   *basis;
	uint8_t *direction;
	uint8_t *wrap;
	uint8_t *justify_content;
	uint8_t *align_items;
	uint8_t *align_self;
	uint8_t *align_content;
} XentNodeFlexStore;

typedef struct XentNodeColdSlots {
	uint32_t *text;
	uint32_t *semantics;
	uint32_t *grid;
	uint32_t *stack;
	uint32_t *focus;
	uint32_t *measure;
} XentNodeColdSlots;

typedef struct XentTextComp {
	uint8_t      active;
	uint8_t      line_break_policy;
	uint8_t      trim;
	uint8_t      intrinsic_valid;
	uint8_t      intrinsic_line_break_policy;
	uint8_t      intrinsic_width_mode;
	uint16_t     font_weight;
	uint16_t     intrinsic_font_weight;
	uint32_t     run_count;
	uint32_t     max_lines;
	uint32_t     intrinsic_lines;
	float        font_size;
	float        intrinsic_constraint_w;
	float        intrinsic_font_size;
	float        intrinsic_w;
	float        intrinsic_h;
	float        intrinsic_baseline;
	char        *content;
	char        *font_family;
	XentTextRun *runs;
} XentTextComp;

typedef struct XentSemComp {
	uint8_t  active;
	uint8_t  role;
	uint8_t  checked;
	uint8_t  enabled;
	uint8_t  expanded;
	uint8_t  selected;
	uint32_t flags;
	float    value_now;
	float    value_min;
	float    value_max;
	char    *label;
} XentSemComp;

typedef struct XentGridComp {
	uint8_t      active;
	uint16_t     row;
	uint16_t     column;
	uint16_t     row_span;
	uint16_t     column_span;
	XentGridDef *def;
} XentGridComp;

typedef struct XentStackComp {
	uint8_t active;
	uint8_t axis;
	uint8_t align;
	uint8_t spacer;
	float   priority;
} XentStackComp;

typedef struct XentFocusComp {
	uint8_t active;
	uint8_t focusable;
	int32_t tab_index;
} XentFocusComp;

typedef struct XentMeasureCacheSlot {
	uint32_t avail_w_bits;
	uint32_t avail_h_bits;
	uint32_t forced_w_bits;
	uint32_t forced_h_bits;
	uint8_t  sizing_mode;
	float    out_w;
	float    out_h;
} XentMeasureCacheSlot;

typedef struct XentMeasureCache {
	uint8_t              active;
	uint8_t              count;
	uint8_t              next_evict;
	XentMeasureCacheSlot slots [XENT_MEASURE_CACHE_SLOTS];
} XentMeasureCache;

typedef struct XentColdPool {
	uint8_t  *data;
	uint32_t *free_list;
	uint32_t  count;
	uint32_t  capacity;
	uint32_t  live_count;
	uint32_t  free_count;
	uint32_t  free_cap;
} XentColdPool;

typedef struct XentNodeStore {
	uint32_t              capacity;
	uint32_t              count;
	uint32_t              live_count;
	uint32_t              min_generation;
	void                 *hot_slab;
	XentNodeLifetimeStore lifetime;
	XentNodeTopologyStore topology;
	XentNodeLayoutStore   layout;
	XentNodeFlexStore     flex;
	XentNodeColdSlots     cold;
	XentColdPool          text_pool;
	XentColdPool          sem_pool;
	XentColdPool          grid_pool;
	XentColdPool          stack_pool;
	XentColdPool          focus_pool;
	XentColdPool          measure_pool;
} XentNodeStore;

typedef struct XentRootSnapshot {
	XentNodeId root;
	float      available_w;
	float      available_h;
	uint32_t   node_count;
	uint64_t   stamp;
} XentRootSnapshot;

typedef struct XentMonoBackendState {
	float glyph_width;
	float line_height;
} XentMonoBackendState;

enum
{
	XENT_SIZING_NORMAL      = 0,
	XENT_SIZING_MIN_CONTENT = 1,
	XENT_SIZING_MAX_CONTENT = 2,
};

struct XentCtx {
	XentCfg                         config;
	XentNodeStore                   nodes;

	uint32_t                       *free_indices;
	uint32_t                        free_count;
	uint32_t                        free_capacity;

	XentNodeId                     *work_order;
	uint32_t                        work_count;
	uint32_t                        work_capacity;
	XentNodeId                     *dirty_nodes;
	uint32_t                        dirty_count;
	uint32_t                        dirty_capacity;
	XentNodeId                     *sem_dirty_nodes;
	uint32_t                        sem_dirty_count;
	uint32_t                        sem_dirty_capacity;

	XentTextCache                   text_cache;
	XentTextBackend const          *text_backend;
	XentTextBackend                 mono_backend;
	XentMonoBackendState            mono_state;
	uint8_t                        *style_key_buf;
	uint32_t                        style_key_cap;

	XentRootSnapshot                root_snapshots [XENT_ROOT_SNAPSHOT_SLOTS];
	uint64_t                        root_snapshot_clock;
	XentNodeId                      last_layout_root;
	uint8_t                         last_layout_strategy;

	XentScratchChunk               *scratch_head;
	XentScratchChunk               *scratch_current;
	size_t                          scratch_chunk_size;

	uint32_t                        swiftstack_scope_depth;
	uint32_t                        flex_scope_depth;
	uint32_t                        grid_scope_depth;
	uint8_t                         sizing_mode;
	XentProfStats                   profile;

	XentNodeObserverEntry          *node_observers;
	uint32_t                        node_observer_count;
	uint32_t                        node_observer_capacity;
	uint32_t                        node_observer_dead;
	uint32_t                        node_observer_dispatch_depth;
	XentObsId                       next_node_observer_id;

	struct XentExternalMeasureSlot *external_measures;
	uint32_t                        external_measure_cap;
};

char                     *xent_strdup(XentAllocSite site, char const *s);
bool                      xent_ensure_node_capacity(XentCtx *ctx, uint32_t needed);
void                      xent_arena_reset_node(XentNodeStore *nodes, uint32_t i);
void                      xent_arena_free(XentNodeStore *nodes);
void                      xent_arena_maybe_shrink(XentCtx *ctx);
uint32_t                  xent_live_index(XentCtx const *ctx, XentNodeId node);
void                      xent_notify_node_observers(XentCtx *ctx, XentNodeEvent const *lifecycle);

void                     *xent_pool_ensure(XentColdPool *pool, size_t elem_size, uint32_t *slot_ref);
void                      xent_pool_release(XentColdPool *pool, size_t elem_size, uint32_t *slot_ref);
void                      xent_pool_destroy(XentColdPool *pool);

XentTextComp             *xent_text_comp_ensure(XentNodeStore *nodes, uint32_t index);
XentSemComp              *xent_sem_comp_ensure(XentNodeStore *nodes, uint32_t index);
XentGridComp             *xent_grid_comp_ensure(XentNodeStore *nodes, uint32_t index);
XentStackComp            *xent_stack_comp_ensure(XentNodeStore *nodes, uint32_t index);
XentFocusComp            *xent_focus_comp_ensure(XentNodeStore *nodes, uint32_t index);
XentMeasureCache         *xent_measure_cache_ensure(XentNodeStore *nodes, uint32_t index);
void                      xent_release_node_cold_slots(XentNodeStore *nodes, uint32_t index);

static void const inline *xent_pool_get_c(XentColdPool const *pool, size_t elem_size, uint32_t slot) {
	if (!slot || slot > pool->count) return NULL;
	return pool->data + (( size_t ) (slot - 1u) * elem_size);
}

static void inline *xent_pool_get(XentColdPool *pool, size_t elem_size, uint32_t slot) {
	if (!slot || slot > pool->count) return NULL;
	return pool->data + (( size_t ) (slot - 1u) * elem_size);
}

static XentTextComp const inline *xent_text_comp_c(XentNodeStore const *nodes, uint32_t index) {
	return ( XentTextComp const * ) xent_pool_get_c(&nodes->text_pool, sizeof(XentTextComp), nodes->cold.text [index]);
}

static XentTextComp inline *xent_text_comp(XentNodeStore *nodes, uint32_t index) {
	return ( XentTextComp * ) xent_pool_get(&nodes->text_pool, sizeof(XentTextComp), nodes->cold.text [index]);
}

static XentSemComp const inline *xent_sem_comp_c(XentNodeStore const *nodes, uint32_t index) {
	return ( XentSemComp const * ) xent_pool_get_c(
	  &nodes->sem_pool, sizeof(XentSemComp), nodes->cold.semantics [index]
	);
}

static XentGridComp const inline *xent_grid_comp_c(XentNodeStore const *nodes, uint32_t index) {
	return ( XentGridComp const * ) xent_pool_get_c(&nodes->grid_pool, sizeof(XentGridComp), nodes->cold.grid [index]);
}

static XentStackComp const inline *xent_stack_comp_c(XentNodeStore const *nodes, uint32_t index) {
	return ( XentStackComp const * ) xent_pool_get_c(
	  &nodes->stack_pool, sizeof(XentStackComp), nodes->cold.stack [index]
	);
}

static XentFocusComp const inline *xent_focus_comp_c(XentNodeStore const *nodes, uint32_t index) {
	return ( XentFocusComp const * ) xent_pool_get_c(
	  &nodes->focus_pool, sizeof(XentFocusComp), nodes->cold.focus [index]
	);
}

static XentMeasureCache inline *xent_measure_cache(XentNodeStore *nodes, uint32_t index) {
	return ( XentMeasureCache * ) xent_pool_get(
	  &nodes->measure_pool, sizeof(XentMeasureCache), nodes->cold.measure [index]
	);
}

static bool inline xent_f32_same(float a, float b) { return a == b || (isnan(a) && isnan(b)); }

void xent_extmeasure_on_destroy(XentCtx *ctx, XentNodeId node);
void xent_extmeasure_clear(XentCtx *ctx);
bool xent_resolve_external_measure(
  XentCtx *ctx, XentNodeId node, float available_w, float available_h, float *width, float *height
);

bool xent_build_preorder(XentCtx *ctx, XentNodeId root);
bool xent_build_preorder_roots(XentCtx *ctx, XentNodeId const *roots, uint32_t root_count);

bool xent_text_cache_init(XentTextCache *cache);
void xent_text_cache_destroy(XentTextCache *cache);
bool xent_text_cache_lookup(XentTextCache *cache, XentTextCacheKey const *key, XentTextMetrics *out_metrics);
void xent_text_cache_insert(XentTextCache *cache, XentTextCacheKey const *key, XentTextMetrics const *metrics);

bool xent_text_mono_init(XentCtx *ctx);

void xent_text_release_node(XentNodeStore *nodes, uint32_t index);
bool xent_text_request_valid(XentCtx const *ctx, XentTextMeasureReq const *request);

void xent_layout_dispatch_node(XentLayoutRequest const *request);

bool xent_layout_skip_collapsed(XentCtx *ctx, XentNodeId child, float x, float y);
void xent_layout_zero_collapsed(XentCtx *ctx, XentNodeId parent, float x, float y);
void xent_layout_node_absolute(XentLayoutRequest const *request);
void xent_layout_mirror(XentCtx *ctx, XentNodeId const *roots, uint32_t root_count);

void xent_layout_node_swiftstack(XentLayoutRequest const *request);

void xent_layout_node_grid(XentLayoutRequest const *request);

void xent_compute_intrinsic_size(
  XentCtx *ctx, XentNodeId node, float available_w, float available_h, float *out_w, float *out_h
);
void xent_decide_node_box(
  XentCtx *ctx, XentNodeId node, float available_w, float available_h, bool definite_w, bool definite_h, float *out_w,
  float *out_h
);
float  xent_compute_hypothetical_cross(XentCtx *ctx, XentNodeId node, bool row, float used_main, float available_cross);
void   xent_quantize_node_layout(XentCtx *ctx, XentNodeId node);
void   xent_invalidate_root_snapshots(XentCtx *ctx, XentNodeId node);

double xent_now_ms(void);
void   xent_sort_r(void *base, size_t count, size_t size, XentSortCompareFn compare, void *context);
float  xent_node_baseline(XentCtx *ctx, XentNodeId node, float cross_size);
float  xent_simd_sum_f32(float const *values, uint32_t count);
void   xent_simd_fill_f32(float *values, uint32_t count, float value);
void   xent_batch_quantize_layout(XentCtx *ctx);

static size_t inline xent_align_up_size(size_t value, size_t alignment) {
	if (alignment == 0u) return value;
	size_t mask = alignment - 1u;
	return (value + mask) & ~mask;
}

#endif
