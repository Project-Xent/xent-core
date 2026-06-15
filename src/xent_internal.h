#ifndef XENT_INTERNAL_H
#define XENT_INTERNAL_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "xent/xent.h"

#define XENT_CACHE_MAX_CAP 4096u

typedef struct XentGridDef {
	uint32_t row_count;
	uint32_t col_count;
	float    row_gap;
	float    col_gap;
	uint8_t *row_modes;
	float   *row_values;
	uint8_t *col_modes;
	float   *col_values;
} XentGridDef;

typedef struct XentLayoutRequest {
	XentContext *ctx;
	XentNodeId   node;
	float        available_w;
	float        available_h;
	float        origin_x;
	float        origin_y;
	/* When true, available_w/h is a size the PARENT decided for this node (flex
	 * distribution, cross-axis stretch, a grid cell) and must be honored as the
	 * node's outer size — not treated as mere available space to size against.
	 * False for the root and absolutely-positioned children, which size
	 * themselves from their content (intrinsic). Defaults false on zero-init. */
	bool         definite_w;
	bool         definite_h;
} XentLayoutRequest;

typedef int (*XentSortCompareFn)(void const *a, void const *b, void *context);

typedef struct XentTextCacheKey {
	char const         *text;
	float               font_size;
	uint16_t            font_weight;
	float               width_constraint;
	XentLineBreakPolicy line_break_policy;
	XentMeasureMode     width_mode;
} XentTextCacheKey;

typedef struct XentCachedTextKey {
	uint64_t hash;
	char    *text;
	float    font_size;
	uint16_t font_weight;
	float    width_constraint;
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

typedef struct XentShapeCacheEntry {
	XentCachedTextKey key;
	XentShapingResult result;
	uint64_t          last_used;
} XentShapeCacheEntry;

typedef struct XentShapeCache {
	XentShapeCacheEntry *entries;
	uint32_t             count;
	uint32_t             capacity;
	uint64_t             clock;
	XentTextCacheStats   stats;
} XentShapeCache;

typedef struct XentNodeLifetimeStore {
	uint8_t *alive;
} XentNodeLifetimeStore;

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
	uint8_t  *wrap_content_w; /**< Size this axis to children's extent (fit-content) when auto. */
	uint8_t  *wrap_content_h;
	uint32_t *dirty_flags;
	uint8_t  *dirty_queued;
	float    *proposed_w;
	float    *proposed_h;
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

typedef struct XentNodeStackStore {
	uint8_t *axis;
	uint8_t *align;
	float   *priority;
	uint8_t *spacer;
} XentNodeStackStore;

typedef struct XentNodeTextStore {
	char    **content;
	float    *font_size;
	uint16_t *font_weight;
	uint8_t  *line_break_policy;
	uint8_t  *intrinsic_valid;
	float    *intrinsic_constraint_w;
	float    *intrinsic_font_size;
	uint16_t *intrinsic_font_weight;
	uint8_t  *intrinsic_line_break_policy;
	uint8_t  *intrinsic_width_mode;
	float    *intrinsic_w;
	float    *intrinsic_h;
	uint32_t *intrinsic_lines;
} XentNodeTextStore;

typedef struct XentNodeSemanticStore {
	uint8_t  *role;
	char    **label;
	uint32_t *flags;
	uint8_t  *checked;
	uint8_t  *enabled;
	uint8_t  *expanded;
	uint8_t  *selected;
	float    *value_now;
	float    *value_min;
	float    *value_max;
} XentNodeSemanticStore;

typedef struct XentNodeExternalStore {
	void   **userdata;
	uint8_t *tag;
} XentNodeExternalStore;

typedef struct XentNodeFocusStore {
	uint8_t *focusable;
	int32_t *tab_index;
} XentNodeFocusStore;

typedef struct XentNodeGridStore {
	XentGridDef **def;
	uint16_t     *row;
	uint16_t     *column;
	uint16_t     *row_span;
	uint16_t     *column_span;
} XentNodeGridStore;

typedef struct XentNodeStore {
	uint32_t              capacity;
	uint32_t              count;
	XentNodeLifetimeStore lifetime;
	XentNodeTopologyStore topology;
	XentNodeLayoutStore   layout;
	XentNodeFlexStore     flex;
	XentNodeStackStore    stack;
	XentNodeTextStore     text;
	XentNodeSemanticStore semantics;
	XentNodeExternalStore external;
	XentNodeFocusStore    focus;
	XentNodeGridStore     grid;
} XentNodeStore;

typedef struct XentMonoBackendState {
	float glyph_width;
	float line_height;
} XentMonoBackendState;

/* Intrinsic-sizing intent (CSS §4.1). NORMAL sizes to the available space;
 * MIN_CONTENT / MAX_CONTENT compute the intrinsic extents regardless of it. */
enum {
	XENT_SIZING_NORMAL      = 0,
	XENT_SIZING_MIN_CONTENT = 1,
	XENT_SIZING_MAX_CONTENT = 2,
};

struct XentContext {
	XentConfig             config;
	XentNodeStore          nodes;

	XentNodeId            *free_ids;
	uint32_t               free_count;
	uint32_t               free_capacity;

	XentNodeId            *work_order;
	uint32_t               work_count;
	uint32_t               work_capacity;
	XentNodeId            *dirty_nodes;
	uint32_t               dirty_count;
	uint32_t               dirty_capacity;

	XentPlugin            *plugins;
	uint32_t               plugin_count;
	uint32_t               plugin_capacity;

	XentTextCache          text_cache;
	XentShapeCache         shape_cache;
	XentTextBackend const *text_backend;
	XentTextBackend        mono_backend;
	XentMonoBackendState   mono_state;

	uint64_t               frame_index;
	bool                   in_frame;

	XentNodeId             last_layout_root;
	float                  last_layout_available_w;
	float                  last_layout_available_h;
	uint8_t                last_layout_strategy;
	uint32_t               last_layout_node_count;

	uint8_t               *scratch;
	size_t                 scratch_size;
	size_t                 scratch_capacity;

	uint32_t               swiftstack_scope_depth;
	uint32_t               flex_scope_depth;
	uint32_t               grid_scope_depth;
	/* Transient intrinsic-sizing intent for the current measurement (CSS §4.1
	 * min-content / max-content). NORMAL = size to the given available space;
	 * MIN/MAX = ignore available and compute the intrinsic extent. Set/restored
	 * around a measurement; read by the text backend and the flex content path. */
	uint8_t                sizing_mode;
	XentProfileStats       profile;
	XentNodeLifecycleFn    node_lifecycle;
	void                  *node_lifecycle_userdata;
};

char *xent_strdup(char const *s);
bool  xent_ensure_node_capacity(XentContext *ctx, uint32_t needed);
/** Zero every SoA column for node @p i via the grow-field table, then reapply
 * non-zero defaults. Owned pointers (content, label, grid def) must be freed
 * first; recycled ids must never inherit their previous occupant's style. */
void  xent_arena_reset_node(XentNodeStore *nodes, uint32_t i);
void  xent_mark_dirty(XentContext *ctx, XentNodeId node, uint32_t flags);

bool  xent_build_preorder(XentContext *ctx, XentNodeId root);
bool  xent_build_preorder_roots(XentContext *ctx, XentNodeId const *roots, uint32_t root_count);
void  xent_clear_dirty_in_work_order(XentContext *ctx);
void  xent_compact_dirty_nodes(XentContext *ctx);

bool  xent_text_cache_init(XentTextCache *cache);
void  xent_text_cache_destroy(XentTextCache *cache);
bool  xent_text_cache_lookup(XentTextCache *cache, XentTextCacheKey const *key, XentTextMetrics *out_metrics);
void  xent_text_cache_insert(XentTextCache *cache, XentTextCacheKey const *key, XentTextMetrics const *metrics);
bool  xent_shape_cache_init(XentShapeCache *cache);
void  xent_shape_cache_destroy(XentShapeCache *cache);
bool  xent_shape_cache_lookup(XentShapeCache *cache, XentTextCacheKey const *key, XentShapingResult *out_result);
void  xent_shape_cache_insert(XentShapeCache *cache, XentTextCacheKey const *key, XentShapingResult const *result);

bool  xent_text_backend_mono_init(XentContext *ctx);

void  xent_layout_dispatch_node(XentLayoutRequest const *request);

void  xent_layout_node_absolute(XentLayoutRequest const *request);

void  xent_layout_node_flex(XentLayoutRequest const *request);

void  xent_layout_node_swiftstack(XentLayoutRequest const *request);

void  xent_layout_node_grid(XentLayoutRequest const *request);

void  xent_compute_intrinsic_size(
  XentContext *ctx, XentNodeId node, float available_w, float available_h, float *out_w, float *out_h
);
/* Decide a node's outer width/height for a layout pass. A definite axis is
 * honored as-is (clamped to that axis's min/max); a non-definite axis is sized
 * from content via xent_compute_intrinsic_size. */
void  xent_decide_node_box(
  XentContext *ctx, XentNodeId node, float available_w, float available_h, bool definite_w, bool definite_h,
  float *out_w, float *out_h
);
/* Hypothetical cross size of a flex item (Flexbox §9.4 algo-cross-item):
 * lay the item out with its used main size and read the resulting cross size.
 * `row` is the container's main axis (true = main is width). */
float xent_compute_hypothetical_cross(
  XentContext *ctx, XentNodeId node, bool row, float used_main, float available_cross
);
/* Content-box main/cross extents of a flex container, per Flexbox §9.4/§9.9:
 * resolve used main sizes, measure each item's cross at its used main, sum the
 * line cross sizes. Used for content-based (wrap-content) container sizing. */
void  xent_flex_intrinsic_content(
  XentContext *ctx, XentNodeId node, float avail_main, float avail_cross, bool row, float *out_main, float *out_cross
);
void   xent_quantize_node_layout(XentContext *ctx, XentNodeId node);

void   xent_sort_by_priority(XentContext const *ctx, XentNodeId *ids, uint32_t count, bool descending);

double xent_now_ms(void);
void   xent_scratch_reset(XentContext *ctx);
void  *xent_scratch_alloc(XentContext *ctx, size_t bytes, size_t alignment);
void   xent_sort_r(void *base, size_t count, size_t size, XentSortCompareFn compare, void *context);
float  xent_estimate_text_baseline(XentContext *ctx, XentNodeId node, float cross_size);
float  xent_simd_sum_f32(float const *values, uint32_t count);
void   xent_simd_fill_f32(float *values, uint32_t count, float value);
void   xent_batch_quantize_layout(XentContext *ctx);

static size_t inline xent_align_up_size(size_t value, size_t alignment) {
	if (alignment == 0u) return value;
	size_t mask = alignment - 1u;
	return (value + mask) & ~mask;
}

#endif
