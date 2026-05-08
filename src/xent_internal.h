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
} XentLayoutRequest;

typedef struct XentTextCacheKey {
	char const         *text;
	float               font_size;
	float               width_constraint;
	XentLineBreakPolicy line_break_policy;
	XentMeasureMode     width_mode;
} XentTextCacheKey;

typedef struct XentCachedTextKey {
	uint64_t hash;
	char    *text;
	float    font_size;
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
	uint32_t *dirty_flags;
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
	uint8_t  *line_break_policy;
	uint8_t  *intrinsic_valid;
	float    *intrinsic_constraint_w;
	float    *intrinsic_font_size;
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
	void                    **userdata;
	void                    **payload;
	uint32_t                 *payload_type;
	XentNodePayloadDestroyFn *payload_destroy;
	void                    **payload_destroy_userdata;
	uint8_t                  *control_type;
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
	XentProfileStats       profile;
	XentNodeLifecycleFn    node_lifecycle;
	void                  *node_lifecycle_userdata;
};

char *xent_strdup(char const *s);
bool  xent_ensure_node_capacity(XentContext *ctx, uint32_t needed);
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
void   xent_quantize_node_layout(XentContext *ctx, XentNodeId node);

void   xent_sort_by_priority(XentContext const *ctx, XentNodeId *ids, uint32_t count, bool descending);

double xent_now_ms(void);
void   xent_scratch_reset(XentContext *ctx);
void  *xent_scratch_alloc(XentContext *ctx, size_t bytes, size_t alignment);
float  xent_simd_sum_f32(float const *values, uint32_t count);
void   xent_simd_fill_f32(float *values, uint32_t count, float value);
void   xent_batch_quantize_layout(XentContext *ctx);

#endif
