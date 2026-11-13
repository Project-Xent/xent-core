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

#define XENT_CACHE_MAX_CAP 4096u

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
	uint8_t  *wrap_content_w;
	uint8_t  *wrap_content_h;
	uint8_t  *scroller;
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
	char        **content;
	char        **font_family;
	XentTextRun **runs;
	uint32_t     *run_count;
	float        *font_size;
	uint16_t     *font_weight;
	uint8_t      *line_break_policy;
	uint8_t      *trim;
	uint32_t     *max_lines;
	uint8_t      *intrinsic_valid;
	float        *intrinsic_constraint_w;
	float        *intrinsic_font_size;
	uint16_t     *intrinsic_font_weight;
	uint8_t      *intrinsic_line_break_policy;
	uint8_t      *intrinsic_width_mode;
	float        *intrinsic_w;
	float        *intrinsic_h;
	float        *intrinsic_baseline;
	uint32_t     *intrinsic_lines;
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
	XentNodeFocusStore    focus;
	XentNodeGridStore     grid;
} XentNodeStore;

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

	XentTextCache                   text_cache;
	XentTextBackend const          *text_backend;
	XentTextBackend                 mono_backend;
	XentMonoBackendState            mono_state;
	uint8_t                        *style_key_buf;
	uint32_t                        style_key_cap;

	XentNodeId                      last_layout_root;
	float                           last_layout_available_w;
	float                           last_layout_available_h;
	uint8_t                         last_layout_strategy;
	uint32_t                        last_layout_node_count;

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

char    *xent_strdup(XentAllocSite site, char const *s);
bool     xent_ensure_node_capacity(XentCtx *ctx, uint32_t needed);
void     xent_arena_reset_node(XentNodeStore *nodes, uint32_t i);
void     xent_arena_free(XentNodeStore *nodes);
uint32_t xent_live_index(XentCtx const *ctx, XentNodeId node);
void     xent_notify_node_observers(XentCtx *ctx, XentNodeEvent const *lifecycle);

void     xent_extmeasure_on_destroy(XentCtx *ctx, XentNodeId node);
void     xent_extmeasure_clear(XentCtx *ctx);
bool     xent_resolve_external_measure(
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

void xent_layout_node_absolute(XentLayoutRequest const *request);

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
