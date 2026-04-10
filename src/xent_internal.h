#ifndef XENT_INTERNAL_H
#define XENT_INTERNAL_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "xent/xent.h"

typedef struct XentTextCacheEntry {
    uint64_t hash;
    char *text;
    float font_size;
    float width_constraint;
    uint8_t line_break_policy;
    uint8_t width_mode;
    XentTextMetrics metrics;
} XentTextCacheEntry;

typedef struct XentTextCache {
    XentTextCacheEntry *entries;
    uint32_t count;
    uint32_t capacity;
    XentTextCacheStats stats;
} XentTextCache;

typedef struct XentShapeCacheEntry {
    uint64_t hash;
    char *text;
    float font_size;
    float width_constraint;
    uint8_t line_break_policy;
    uint8_t width_mode;
    XentShapingResult result;
} XentShapeCacheEntry;

typedef struct XentShapeCache {
    XentShapeCacheEntry *entries;
    uint32_t count;
    uint32_t capacity;
    XentTextCacheStats stats;
} XentShapeCache;

typedef struct XentNodeStore {
    uint32_t capacity;
    uint32_t count;

    uint8_t *alive;

    XentNodeId *parent;
    XentNodeId *first_child;
    XentNodeId *next_sibling;
    uint32_t *child_count;

    uint8_t *protocol;
    uint8_t *direction;
    uint32_t *dirty_flags;

    float *proposed_w;
    float *proposed_h;
    float *decided_w;
    float *decided_h;
    float *abs_x;
    float *abs_y;

    float *style_w;
    float *style_h;
    float *min_w;
    float *min_h;
    float *max_w;
    float *max_h;

    float *margin_l;
    float *margin_t;
    float *margin_r;
    float *margin_b;

    float *padding_l;
    float *padding_t;
    float *padding_r;
    float *padding_b;

    float *gap;

    float *abs_pos_x;
    float *abs_pos_y;

    float *flex_grow;
    float *flex_shrink;
    float *flex_basis;
    uint8_t *flex_direction;
    uint8_t *flex_wrap;
    uint8_t *flex_justify_content;
    uint8_t *flex_align_items;
    uint8_t *flex_align_self;
    uint8_t *flex_align_content;

    uint8_t *stack_axis;
    uint8_t *stack_align;
    float *layout_priority;
    uint8_t *is_spacer;

    char **text;
    float *font_size;
    uint8_t *text_line_break_policy;
    uint8_t *text_intrinsic_valid;
    float *text_intrinsic_constraint_w;
    float *text_intrinsic_font_size;
    uint8_t *text_intrinsic_line_break_policy;
    uint8_t *text_intrinsic_width_mode;
    float *text_intrinsic_w;
    float *text_intrinsic_h;
    uint32_t *text_intrinsic_lines;

    uint8_t *semantic_role;
    char **semantic_label;
    uint32_t *semantic_flags;

    void **userdata;
    uint8_t *control_type;

    uint8_t  *semantic_checked;
    uint8_t  *semantic_enabled;
    uint8_t  *semantic_expanded;
    uint8_t  *semantic_selected;
    float    *semantic_value_now;
    float    *semantic_value_min;
    float    *semantic_value_max;
} XentNodeStore;

typedef struct XentMonoBackendState {
    float glyph_width;
    float line_height;
} XentMonoBackendState;

struct XentContext {
    XentConfig config;
    XentNodeStore nodes;

    XentNodeId *free_ids;
    uint32_t free_count;
    uint32_t free_capacity;

    XentNodeId *work_order;
    uint32_t work_count;
    uint32_t work_capacity;

    XentPlugin *plugins;
    uint32_t plugin_count;
    uint32_t plugin_capacity;

    XentTextCache text_cache;
    XentShapeCache shape_cache;
    const XentTextBackend *text_backend;
    XentTextBackend mono_backend;
    XentMonoBackendState mono_state;

    uint64_t frame_index;
    bool in_frame;

    XentNodeId last_layout_root;
    float last_layout_available_w;
    float last_layout_available_h;
    uint8_t last_layout_strategy;

    uint8_t *scratch;
    size_t scratch_size;
    size_t scratch_capacity;

    uint32_t swiftstack_scope_depth;
    XentProfileStats profile;
};

char *xent_strdup(const char *s);
bool xent_ensure_node_capacity(XentContext *ctx, uint32_t needed);
void xent_mark_dirty(XentContext *ctx, XentNodeId node, uint32_t flags);

bool xent_build_preorder(XentContext *ctx, XentNodeId root);
void xent_clear_dirty_in_work_order(XentContext *ctx);

bool xent_text_cache_init(XentTextCache *cache);
void xent_text_cache_destroy(XentTextCache *cache);
bool xent_text_cache_lookup(XentTextCache *cache,
                            const char *text,
                            float font_size,
                            float width_constraint,
                            XentLineBreakPolicy line_break_policy,
                            XentMeasureMode width_mode,
                            XentTextMetrics *out_metrics);
void xent_text_cache_insert(XentTextCache *cache,
                            const char *text,
                            float font_size,
                            float width_constraint,
                            XentLineBreakPolicy line_break_policy,
                            XentMeasureMode width_mode,
                            const XentTextMetrics *metrics);
bool xent_shape_cache_init(XentShapeCache *cache);
void xent_shape_cache_destroy(XentShapeCache *cache);
bool xent_shape_cache_lookup(XentShapeCache *cache,
                             const char *text,
                             float font_size,
                             float width_constraint,
                             XentLineBreakPolicy line_break_policy,
                             XentMeasureMode width_mode,
                             XentShapingResult *out_result);
void xent_shape_cache_insert(XentShapeCache *cache,
                             const char *text,
                             float font_size,
                             float width_constraint,
                             XentLineBreakPolicy line_break_policy,
                             XentMeasureMode width_mode,
                             const XentShapingResult *result);

bool xent_text_backend_mono_init(XentContext *ctx);

void xent_layout_dispatch_node(XentContext *ctx,
                               XentNodeId node,
                               float available_w,
                               float available_h,
                               float origin_x,
                               float origin_y);

void xent_layout_node_absolute(XentContext *ctx,
                               XentNodeId node,
                               float available_w,
                               float available_h,
                               float origin_x,
                               float origin_y);

void xent_layout_node_flex(XentContext *ctx,
                           XentNodeId node,
                           float available_w,
                           float available_h,
                           float origin_x,
                           float origin_y);

void xent_layout_node_swiftstack(XentContext *ctx,
                                 XentNodeId node,
                                 float available_w,
                                 float available_h,
                                 float origin_x,
                                 float origin_y);

void xent_compute_intrinsic_size(XentContext *ctx,
                                 XentNodeId node,
                                 float available_w,
                                 float available_h,
                                 float *out_w,
                                 float *out_h);
void xent_quantize_node_layout(XentContext *ctx, XentNodeId node);

void xent_sort_by_priority(const XentContext *ctx, XentNodeId *ids, uint32_t count, bool descending);

double xent_now_ms(void);
void xent_scratch_reset(XentContext *ctx);
void *xent_scratch_alloc(XentContext *ctx, size_t bytes, size_t alignment);
float xent_simd_sum_f32(const float *values, uint32_t count);
void xent_simd_fill_f32(float *values, uint32_t count, float value);

#endif
