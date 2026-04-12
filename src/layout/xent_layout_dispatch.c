#include "../xent_internal.h"

static float xent_clampf(float value, float min_v, float max_v) {
    if (value < min_v) {
        value = min_v;
    }
    if (value > max_v) {
        value = max_v;
    }
    return value;
}

static float xent_round_to_pixel_grid(const XentContext *ctx, float value) {
    if (!ctx->config.enable_pixel_rounding) {
        return value;
    }
    float scale = ctx->config.point_scale_factor;
    if (!(scale > 0.0f) || !isfinite(scale) || !isfinite(value)) {
        return value;
    }
    return roundf(value * scale) / scale;
}

void xent_quantize_node_layout(XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return;
    }
    ctx->nodes.abs_x[node] = xent_round_to_pixel_grid(ctx, ctx->nodes.abs_x[node]);
    ctx->nodes.abs_y[node] = xent_round_to_pixel_grid(ctx, ctx->nodes.abs_y[node]);
    ctx->nodes.decided_w[node] = xent_round_to_pixel_grid(ctx, ctx->nodes.decided_w[node]);
    ctx->nodes.decided_h[node] = xent_round_to_pixel_grid(ctx, ctx->nodes.decided_h[node]);
}

static void xent_invalidate_all_layout(XentContext *ctx) {
    if (!ctx) {
        return;
    }
    for (uint32_t i = 1u; i <= ctx->nodes.count; ++i) {
        if (ctx->nodes.alive[i]) {
            ctx->nodes.dirty_flags[i] |= XENT_DIRTY_LAYOUT | XENT_DIRTY_SUBTREE | XENT_DIRTY_SELF;
            ctx->nodes.text_intrinsic_valid[i] = 0u;
        }
    }
    ctx->last_layout_root = XENT_NODE_INVALID;
    ctx->last_layout_available_w = NAN;
    ctx->last_layout_available_h = NAN;
}

static bool xent_is_valid_direction(XentDirection direction) {
    return direction == XENT_DIRECTION_INHERIT || direction == XENT_DIRECTION_LTR || direction == XENT_DIRECTION_RTL;
}

static XentDirection xent_resolve_direction_internal(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return XENT_DIRECTION_LTR;
    }

    XentNodeId cursor = node;
    while (cursor != XENT_NODE_INVALID) {
        XentDirection direction = (XentDirection)ctx->nodes.direction[cursor];
        if (direction == XENT_DIRECTION_LTR || direction == XENT_DIRECTION_RTL) {
            return direction;
        }
        cursor = ctx->nodes.parent[cursor];
    }
    return XENT_DIRECTION_LTR;
}

static void xent_resolve_logical_insets(float left,
                                        float top,
                                        float right,
                                        float bottom,
                                        XentDirection direction,
                                        XentAxis main_axis,
                                        float *out_main_start,
                                        float *out_main_end,
                                        float *out_cross_start,
                                        float *out_cross_end) {
    if (main_axis == XENT_AXIS_HORIZONTAL) {
        *out_main_start = (direction == XENT_DIRECTION_RTL) ? right : left;
        *out_main_end = (direction == XENT_DIRECTION_RTL) ? left : right;
        *out_cross_start = top;
        *out_cross_end = bottom;
        return;
    }

    *out_main_start = top;
    *out_main_end = bottom;
    *out_cross_start = (direction == XENT_DIRECTION_RTL) ? right : left;
    *out_cross_end = (direction == XENT_DIRECTION_RTL) ? left : right;
}

void xent_compute_intrinsic_size(XentContext *ctx,
                                 XentNodeId node,
                                 float available_w,
                                 float available_h,
                                 float *out_w,
                                 float *out_h) {
    float width = ctx->nodes.style_w[node];
    float height = ctx->nodes.style_h[node];

    if (ctx->nodes.text[node] && (isnan(width) || isnan(height))) {
        XentTextMetrics metrics = {0};
        float text_constraint = isnan(width) ? available_w : width;
        XentMeasureMode width_mode = XENT_MEASURE_UNDEFINED;
        if (isnan(width)) {
            if (isfinite(text_constraint) && text_constraint > 0.0f) {
                width_mode = XENT_MEASURE_AT_MOST;
            } else {
                text_constraint = INFINITY;
            }
        } else {
            width_mode = XENT_MEASURE_EXACTLY;
        }
        if (width_mode != XENT_MEASURE_EXACTLY && (!isfinite(text_constraint) || text_constraint <= 0.0f)) {
            text_constraint = INFINITY;
        }
        XentLineBreakPolicy line_break_policy = (XentLineBreakPolicy)ctx->nodes.text_line_break_policy[node];
        bool has_node_cache = ctx->nodes.text_intrinsic_valid[node] != 0u &&
                              ctx->nodes.text_intrinsic_font_size[node] == ctx->nodes.font_size[node] &&
                              ctx->nodes.text_intrinsic_constraint_w[node] == text_constraint &&
                              ctx->nodes.text_intrinsic_line_break_policy[node] == (uint8_t)line_break_policy &&
                              ctx->nodes.text_intrinsic_width_mode[node] == (uint8_t)width_mode;

        if (has_node_cache) {
            metrics.width = ctx->nodes.text_intrinsic_w[node];
            metrics.height = ctx->nodes.text_intrinsic_h[node];
            metrics.line_count = ctx->nodes.text_intrinsic_lines[node];
        } else if (xent_measure_text(ctx,
                                     ctx->nodes.text[node],
                                     ctx->nodes.font_size[node],
                                     text_constraint,
                                     line_break_policy,
                                     width_mode,
                                     &metrics)) {
            /* Per-node intrinsic cache avoids repeated hash/lookups for stable text nodes. */
            ctx->nodes.text_intrinsic_valid[node] = 1u;
            ctx->nodes.text_intrinsic_constraint_w[node] = text_constraint;
            ctx->nodes.text_intrinsic_font_size[node] = ctx->nodes.font_size[node];
            ctx->nodes.text_intrinsic_line_break_policy[node] = (uint8_t)line_break_policy;
            ctx->nodes.text_intrinsic_width_mode[node] = (uint8_t)width_mode;
            ctx->nodes.text_intrinsic_w[node] = metrics.width;
            ctx->nodes.text_intrinsic_h[node] = metrics.height;
            ctx->nodes.text_intrinsic_lines[node] = metrics.line_count;
        }

        if (has_node_cache || metrics.line_count > 0u || metrics.width > 0.0f || metrics.height > 0.0f) {
            if (isnan(width)) {
                width = metrics.width;
            }
            if (isnan(height)) {
                height = metrics.height;
            }
        }
    }

    if (isnan(width)) {
        width = available_w;
    }
    if (isnan(height)) {
        height = available_h;
    }
    if (!isfinite(width) || width < 0.0f) {
        width = 0.0f;
    }
    if (!isfinite(height) || height < 0.0f) {
        height = 0.0f;
    }

    width = xent_clampf(width, ctx->nodes.min_w[node], ctx->nodes.max_w[node]);
    height = xent_clampf(height, ctx->nodes.min_h[node], ctx->nodes.max_h[node]);

    *out_w = width;
    *out_h = height;
}

void xent_layout_dispatch_node(XentContext *ctx,
                               XentNodeId node,
                               float available_w,
                               float available_h,
                               float origin_x,
                               float origin_y) {
    XentProtocol protocol = (XentProtocol)ctx->nodes.protocol[node];
    switch (protocol) {
        case XENT_PROTOCOL_FLEX:
            xent_layout_node_flex(ctx, node, available_w, available_h, origin_x, origin_y);
            break;
        case XENT_PROTOCOL_SWIFTSTACK:
            xent_layout_node_swiftstack(ctx, node, available_w, available_h, origin_x, origin_y);
            break;
        case XENT_PROTOCOL_GRID:
            xent_layout_node_grid(ctx, node, available_w, available_h, origin_x, origin_y);
            break;
        case XENT_PROTOCOL_ABSOLUTE:
        default:
            xent_layout_node_absolute(ctx, node, available_w, available_h, origin_x, origin_y);
            break;
    }
}

bool xent_set_protocol(XentContext *ctx, XentNodeId node, XentProtocol protocol) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.protocol[node] = (uint8_t)protocol;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_direction(XentContext *ctx, XentNodeId node, XentDirection direction) {
    if (!xent_is_valid_node(ctx, node) || !xent_is_valid_direction(direction)) {
        return false;
    }
    if (ctx->nodes.direction[node] != (uint8_t)direction) {
        ctx->nodes.direction[node] = (uint8_t)direction;
        xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT | XENT_DIRTY_SUBTREE);
    }
    return true;
}

XentDirection xent_get_direction(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return XENT_DIRECTION_INHERIT;
    }
    return (XentDirection)ctx->nodes.direction[node];
}

XentDirection xent_get_resolved_direction(const XentContext *ctx, XentNodeId node) {
    return xent_resolve_direction_internal(ctx, node);
}

bool xent_get_resolved_margin(const XentContext *ctx,
                              XentNodeId node,
                              XentAxis main_axis,
                              float *out_main_start,
                              float *out_main_end,
                              float *out_cross_start,
                              float *out_cross_end) {
    if (!xent_is_valid_node(ctx, node) || !out_main_start || !out_main_end || !out_cross_start || !out_cross_end) {
        return false;
    }
    XentDirection direction = xent_resolve_direction_internal(ctx, node);
    xent_resolve_logical_insets(ctx->nodes.margin_l[node],
                                ctx->nodes.margin_t[node],
                                ctx->nodes.margin_r[node],
                                ctx->nodes.margin_b[node],
                                direction,
                                main_axis,
                                out_main_start,
                                out_main_end,
                                out_cross_start,
                                out_cross_end);
    return true;
}

bool xent_get_resolved_padding(const XentContext *ctx,
                               XentNodeId node,
                               XentAxis main_axis,
                               float *out_main_start,
                               float *out_main_end,
                               float *out_cross_start,
                               float *out_cross_end) {
    if (!xent_is_valid_node(ctx, node) || !out_main_start || !out_main_end || !out_cross_start || !out_cross_end) {
        return false;
    }
    XentDirection direction = xent_resolve_direction_internal(ctx, node);
    xent_resolve_logical_insets(ctx->nodes.padding_l[node],
                                ctx->nodes.padding_t[node],
                                ctx->nodes.padding_r[node],
                                ctx->nodes.padding_b[node],
                                direction,
                                main_axis,
                                out_main_start,
                                out_main_end,
                                out_cross_start,
                                out_cross_end);
    return true;
}

bool xent_set_size(XentContext *ctx, XentNodeId node, float width, float height) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.style_w[node] = width;
    ctx->nodes.style_h[node] = height;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_min_size(XentContext *ctx, XentNodeId node, float min_width, float min_height) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.min_w[node] = min_width;
    ctx->nodes.min_h[node] = min_height;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_max_size(XentContext *ctx, XentNodeId node, float max_width, float max_height) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.max_w[node] = max_width;
    ctx->nodes.max_h[node] = max_height;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_margin(XentContext *ctx, XentNodeId node, float left, float top, float right, float bottom) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.margin_l[node] = left;
    ctx->nodes.margin_t[node] = top;
    ctx->nodes.margin_r[node] = right;
    ctx->nodes.margin_b[node] = bottom;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_padding(XentContext *ctx, XentNodeId node, float left, float top, float right, float bottom) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.padding_l[node] = left;
    ctx->nodes.padding_t[node] = top;
    ctx->nodes.padding_r[node] = right;
    ctx->nodes.padding_b[node] = bottom;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_absolute_position(XentContext *ctx, XentNodeId node, float x, float y) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.abs_pos_x[node] = x;
    ctx->nodes.abs_pos_y[node] = y;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_gap(XentContext *ctx, XentNodeId node, float gap) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.gap[node] = gap < 0.0f ? 0.0f : gap;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_flex_grow(XentContext *ctx, XentNodeId node, float grow) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.flex_grow[node] = grow < 0.0f ? 0.0f : grow;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_flex_shrink(XentContext *ctx, XentNodeId node, float shrink) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.flex_shrink[node] = shrink < 0.0f ? 0.0f : shrink;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_flex_basis(XentContext *ctx, XentNodeId node, float basis) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.flex_basis[node] = basis;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_flex_direction(XentContext *ctx, XentNodeId node, XentFlexDirection direction) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.flex_direction[node] = (uint8_t)direction;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_flex_wrap(XentContext *ctx, XentNodeId node, XentFlexWrap wrap) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.flex_wrap[node] = (uint8_t)wrap;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_flex_justify_content(XentContext *ctx, XentNodeId node, XentFlexJustify justify) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.flex_justify_content[node] = (uint8_t)justify;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_flex_align_items(XentContext *ctx, XentNodeId node, XentFlexAlign align_items) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.flex_align_items[node] = (uint8_t)align_items;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_flex_align_self(XentContext *ctx, XentNodeId node, XentFlexAlign align_self) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.flex_align_self[node] = (uint8_t)align_self;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_flex_align_content(XentContext *ctx, XentNodeId node, XentFlexAlignContent align_content) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.flex_align_content[node] = (uint8_t)align_content;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_stack_axis(XentContext *ctx, XentNodeId node, XentAxis axis) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.stack_axis[node] = (uint8_t)axis;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_stack_alignment(XentContext *ctx, XentNodeId node, XentStackAlign alignment) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    if (alignment != XENT_STACK_ALIGN_START && alignment != XENT_STACK_ALIGN_BASELINE) {
        return false;
    }
    ctx->nodes.stack_align[node] = (uint8_t)alignment;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

XentStackAlign xent_get_stack_alignment(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return XENT_STACK_ALIGN_START;
    }
    return (XentStackAlign)ctx->nodes.stack_align[node];
}

bool xent_set_layout_priority(XentContext *ctx, XentNodeId node, float priority) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.layout_priority[node] = priority;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_is_spacer(XentContext *ctx, XentNodeId node, bool is_spacer) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.is_spacer[node] = is_spacer ? 1u : 0u;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_point_scale_factor(XentContext *ctx, float point_scale_factor) {
    if (!ctx || !(point_scale_factor > 0.0f) || !isfinite(point_scale_factor)) {
        return false;
    }
    if (ctx->config.point_scale_factor != point_scale_factor) {
        ctx->config.point_scale_factor = point_scale_factor;
        xent_invalidate_all_layout(ctx);
    }
    return true;
}

float xent_get_point_scale_factor(const XentContext *ctx) {
    if (!ctx) {
        return 1.0f;
    }
    return ctx->config.point_scale_factor;
}

bool xent_set_pixel_rounding_enabled(XentContext *ctx, bool enabled) {
    if (!ctx) {
        return false;
    }
    if (ctx->config.enable_pixel_rounding != enabled) {
        ctx->config.enable_pixel_rounding = enabled;
        xent_invalidate_all_layout(ctx);
    }
    return true;
}

bool xent_is_pixel_rounding_enabled(const XentContext *ctx) {
    if (!ctx) {
        return false;
    }
    return ctx->config.enable_pixel_rounding;
}

XentProtocol xent_get_protocol(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return XENT_PROTOCOL_ABSOLUTE;
    }
    return (XentProtocol)ctx->nodes.protocol[node];
}

static bool xent_is_layout_dirty(uint32_t flags) {
    return (flags & (XENT_DIRTY_LAYOUT | XENT_DIRTY_SELF)) != 0u;
}

static bool xent_is_auto_sized_parent(const XentContext *ctx, XentNodeId node) {
    return isnan(ctx->nodes.style_w[node]) || isnan(ctx->nodes.style_h[node]);
}

static XentNodeId xent_promote_dirty_root(const XentContext *ctx, XentNodeId dirty_node, XentNodeId root) {
    XentNodeId candidate = dirty_node;
    XentNodeId parent = ctx->nodes.parent[candidate];
    while (parent != XENT_NODE_INVALID && parent != root) {
        XentProtocol protocol = (XentProtocol)ctx->nodes.protocol[parent];
        if ((protocol == XENT_PROTOCOL_FLEX || protocol == XENT_PROTOCOL_SWIFTSTACK) &&
            xent_is_auto_sized_parent(ctx, parent)) {
            candidate = parent;
            parent = ctx->nodes.parent[candidate];
            continue;
        }
        break;
    }
    return candidate;
}

static bool xent_is_ancestor_of(const XentContext *ctx, XentNodeId maybe_ancestor, XentNodeId node) {
    XentNodeId cursor = ctx->nodes.parent[node];
    while (cursor != XENT_NODE_INVALID) {
        if (cursor == maybe_ancestor) {
            return true;
        }
        cursor = ctx->nodes.parent[cursor];
    }
    return false;
}

static uint32_t xent_collect_recompute_roots(const XentContext *ctx,
                                             XentNodeId root,
                                             const XentNodeId *direct_dirty,
                                             uint32_t direct_count,
                                             XentNodeId *out_roots) {
    uint32_t root_count = 0u;
    for (uint32_t i = 0; i < direct_count; ++i) {
        XentNodeId promoted = xent_promote_dirty_root(ctx, direct_dirty[i], root);
        if (promoted == root) {
            return 0u;
        }

        bool duplicate = false;
        for (uint32_t j = 0; j < root_count; ++j) {
            if (out_roots[j] == promoted) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            out_roots[root_count++] = promoted;
        }
    }

    /* Keep only highest roots; if A contains B, recomputing A covers B. */
    for (uint32_t i = 0; i < root_count; ++i) {
        if (out_roots[i] == XENT_NODE_INVALID) {
            continue;
        }
        for (uint32_t j = 0; j < root_count; ++j) {
            if (i == j || out_roots[j] == XENT_NODE_INVALID) {
                continue;
            }
            if (xent_is_ancestor_of(ctx, out_roots[j], out_roots[i])) {
                out_roots[i] = XENT_NODE_INVALID;
                break;
            }
        }
    }

    uint32_t compact = 0u;
    for (uint32_t i = 0; i < root_count; ++i) {
        if (out_roots[i] != XENT_NODE_INVALID) {
            out_roots[compact++] = out_roots[i];
        }
    }
    return compact;
}

static bool xent_can_reuse_root_snapshot(const XentContext *ctx,
                                         XentNodeId root,
                                         float available_width,
                                         float available_height) {
    if (ctx->last_layout_root != root) {
        return false;
    }
    if (!isfinite(ctx->nodes.decided_w[root]) || ctx->nodes.decided_w[root] < 0.0f ||
        !isfinite(ctx->nodes.decided_h[root]) || ctx->nodes.decided_h[root] < 0.0f) {
        return false;
    }
    if (!isfinite(ctx->last_layout_available_w) || !isfinite(ctx->last_layout_available_h)) {
        return false;
    }
    return ctx->last_layout_available_w == available_width && ctx->last_layout_available_h == available_height;
}

static uint32_t xent_count_subtree_nodes(const XentContext *ctx,
                                         XentNodeId root,
                                         XentNodeId *stack,
                                         uint32_t stack_capacity) {
    if (!stack || stack_capacity == 0u || !xent_is_valid_node(ctx, root)) {
        return 0u;
    }

    uint32_t count = 0u;
    uint32_t stack_size = 0u;
    stack[stack_size++] = root;

    while (stack_size > 0u) {
        XentNodeId node = stack[--stack_size];
        count += 1u;

        for (XentNodeId child = ctx->nodes.first_child[node]; child != XENT_NODE_INVALID;
             child = ctx->nodes.next_sibling[child]) {
            if (stack_size >= stack_capacity) {
                return 0u;
            }
            stack[stack_size++] = child;
        }
    }

    return count;
}

bool xent_layout(XentContext *ctx, XentNodeId root, float available_width, float available_height) {
    if (!xent_is_valid_node(ctx, root)) {
        return false;
    }

    if (!xent_build_preorder(ctx, root)) {
        return false;
    }

    xent_scratch_reset(ctx);

    const uint32_t total_nodes = ctx->work_count;
    bool did_subtree_recompute = false;
    XentLayoutStrategy strategy = XENT_LAYOUT_STRATEGY_FULL_TREE;

    uint32_t direct_dirty_count = 0u;
    for (uint32_t i = 0; i < total_nodes; ++i) {
        XentNodeId node = ctx->work_order[i];
        if (xent_is_layout_dirty(ctx->nodes.dirty_flags[node])) {
            direct_dirty_count += 1u;
        }
    }

    bool root_layout_dirty = xent_is_layout_dirty(ctx->nodes.dirty_flags[root]);
    bool snapshot_valid = xent_can_reuse_root_snapshot(ctx, root, available_width, available_height);

    if (!root_layout_dirty && snapshot_valid && direct_dirty_count == 0u) {
        strategy = XENT_LAYOUT_STRATEGY_NONE;
    } else if (!root_layout_dirty && snapshot_valid && direct_dirty_count > 0u && total_nodes > 0u) {
        XentNodeId *direct_dirty =
            (XentNodeId *)xent_scratch_alloc(ctx, sizeof(XentNodeId) * (size_t)direct_dirty_count, _Alignof(XentNodeId));
        XentNodeId *roots =
            (XentNodeId *)xent_scratch_alloc(ctx, sizeof(XentNodeId) * (size_t)direct_dirty_count, _Alignof(XentNodeId));
        XentNodeId *stack =
            (XentNodeId *)xent_scratch_alloc(ctx, sizeof(XentNodeId) * (size_t)total_nodes, _Alignof(XentNodeId));

        if (direct_dirty && roots && stack) {
            uint32_t write = 0u;
            for (uint32_t i = 0; i < total_nodes; ++i) {
                XentNodeId node = ctx->work_order[i];
                if (xent_is_layout_dirty(ctx->nodes.dirty_flags[node])) {
                    direct_dirty[write++] = node;
                }
            }

            uint32_t root_count = xent_collect_recompute_roots(ctx, root, direct_dirty, write, roots);
            if (root_count > 0u) {
                bool roots_valid = true;
                uint32_t affected_nodes = 0u;

                for (uint32_t i = 0; i < root_count; ++i) {
                    XentNodeId r = roots[i];
                    if (!isfinite(ctx->nodes.decided_w[r]) || ctx->nodes.decided_w[r] < 0.0f ||
                        !isfinite(ctx->nodes.decided_h[r]) || ctx->nodes.decided_h[r] < 0.0f) {
                        roots_valid = false;
                        break;
                    }

                    uint32_t subtree_nodes = xent_count_subtree_nodes(ctx, r, stack, total_nodes);
                    if (subtree_nodes == 0u) {
                        roots_valid = false;
                        break;
                    }
                    affected_nodes += subtree_nodes;
                }

                if (roots_valid && (affected_nodes + 1u) < total_nodes) {
                    for (uint32_t i = 0; i < root_count; ++i) {
                        XentNodeId r = roots[i];
                        float origin_x = ctx->nodes.abs_x[r] - ctx->nodes.abs_pos_x[r];
                        float origin_y = ctx->nodes.abs_y[r] - ctx->nodes.abs_pos_y[r];
                        xent_layout_dispatch_node(ctx, r, ctx->nodes.decided_w[r], ctx->nodes.decided_h[r], origin_x, origin_y);
                    }
                    did_subtree_recompute = true;
                    strategy = XENT_LAYOUT_STRATEGY_DIRTY_SUBTREE;
                }
            }
        }
    }

    if (!did_subtree_recompute && strategy == XENT_LAYOUT_STRATEGY_FULL_TREE) {
        xent_layout_dispatch_node(ctx, root, available_width, available_height, 0.0f, 0.0f);
        strategy = XENT_LAYOUT_STRATEGY_FULL_TREE;
    }

    xent_clear_dirty_in_work_order(ctx);
    ctx->last_layout_root = root;
    ctx->last_layout_available_w = available_width;
    ctx->last_layout_available_h = available_height;
    ctx->last_layout_strategy = (uint8_t)strategy;
    return true;
}

bool xent_get_layout_rect(const XentContext *ctx, XentNodeId node, XentRect *out_rect) {
    if (!xent_is_valid_node(ctx, node) || !out_rect) {
        return false;
    }
    out_rect->x = ctx->nodes.abs_x[node];
    out_rect->y = ctx->nodes.abs_y[node];
    out_rect->width = ctx->nodes.decided_w[node];
    out_rect->height = ctx->nodes.decided_h[node];
    return true;
}

XentNodeId xent_get_last_layout_root(const XentContext *ctx) {
    if (!ctx) {
        return XENT_NODE_INVALID;
    }
    return ctx->last_layout_root;
}

float xent_get_layout_priority(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return 0.0f;
    }
    return ctx->nodes.layout_priority[node];
}

XentLayoutStrategy xent_get_last_layout_strategy(const XentContext *ctx) {
    if (!ctx) {
        return XENT_LAYOUT_STRATEGY_NONE;
    }
    return (XentLayoutStrategy)ctx->last_layout_strategy;
}

/* ── Grid layout API ────────────────────────────────────────────────── */

static XentGridDef *xent_ensure_grid_def(XentContext *ctx, XentNodeId node) {
    if (!ctx->nodes.grid_def[node]) {
        XentGridDef *def = (XentGridDef *)calloc(1, sizeof(XentGridDef));
        if (!def) return NULL;
        ctx->nodes.grid_def[node] = def;
    }
    return ctx->nodes.grid_def[node];
}

bool xent_set_grid_rows(XentContext *ctx, XentNodeId node,
                        const XentGridSizeMode *modes, const float *values,
                        uint32_t count) {
    if (!xent_is_valid_node(ctx, node) || count > XENT_GRID_MAX_TRACKS) {
        return false;
    }
    XentGridDef *def = xent_ensure_grid_def(ctx, node);
    if (!def) return false;
    def->row_count = (uint8_t)count;
    for (uint32_t i = 0; i < count; ++i) {
        def->row_modes[i] = (uint8_t)modes[i];
        def->row_values[i] = values[i];
    }
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_grid_columns(XentContext *ctx, XentNodeId node,
                           const XentGridSizeMode *modes, const float *values,
                           uint32_t count) {
    if (!xent_is_valid_node(ctx, node) || count > XENT_GRID_MAX_TRACKS) {
        return false;
    }
    XentGridDef *def = xent_ensure_grid_def(ctx, node);
    if (!def) return false;
    def->col_count = (uint8_t)count;
    for (uint32_t i = 0; i < count; ++i) {
        def->col_modes[i] = (uint8_t)modes[i];
        def->col_values[i] = values[i];
    }
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_grid_row(XentContext *ctx, XentNodeId node, uint32_t row) {
    if (!xent_is_valid_node(ctx, node)) return false;
    ctx->nodes.grid_row[node] = (uint16_t)row;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_grid_column(XentContext *ctx, XentNodeId node, uint32_t column) {
    if (!xent_is_valid_node(ctx, node)) return false;
    ctx->nodes.grid_column[node] = (uint16_t)column;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_grid_row_span(XentContext *ctx, XentNodeId node, uint32_t span) {
    if (!xent_is_valid_node(ctx, node) || span == 0) return false;
    ctx->nodes.grid_row_span[node] = (uint16_t)span;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_grid_column_span(XentContext *ctx, XentNodeId node, uint32_t span) {
    if (!xent_is_valid_node(ctx, node) || span == 0) return false;
    ctx->nodes.grid_column_span[node] = (uint16_t)span;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_grid_row_gap(XentContext *ctx, XentNodeId node, float gap) {
    if (!xent_is_valid_node(ctx, node)) return false;
    XentGridDef *def = xent_ensure_grid_def(ctx, node);
    if (!def) return false;
    def->row_gap = gap;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_grid_column_gap(XentContext *ctx, XentNodeId node, float gap) {
    if (!xent_is_valid_node(ctx, node)) return false;
    XentGridDef *def = xent_ensure_grid_def(ctx, node);
    if (!def) return false;
    def->col_gap = gap;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

uint32_t xent_get_grid_row(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) return 0;
    return ctx->nodes.grid_row[node];
}

uint32_t xent_get_grid_column(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) return 0;
    return ctx->nodes.grid_column[node];
}

uint32_t xent_get_grid_row_span(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) return 1;
    return ctx->nodes.grid_row_span[node];
}

uint32_t xent_get_grid_column_span(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) return 1;
    return ctx->nodes.grid_column_span[node];
}
