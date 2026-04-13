#include "../xent_internal.h"

#if XENT_ISPC_ENABLED
#include "xent_ispc_kernels_ispc.h"
#endif

typedef struct FlexChildData {
    XentNodeId id;
    float base_main;
    float final_main;
    float base_cross;
    float final_cross;
    float baseline_from_top;
    float margin_lead;
    float margin_trail;
    float margin_cross_lead;
    float margin_cross_trail;
    float grow;
    float shrink;
    uint8_t align_self;
    uint8_t resolved_align;
} FlexChildData;

typedef struct FlexLineData {
    uint32_t start;
    uint32_t count;
    float base_main;
    float sum_grow;
    float sum_shrink_weight;
    float cross_outer;
} FlexLineData;

static void xent_resolve_justify(XentFlexJustify justify,
                                 uint32_t item_count,
                                 float base_gap,
                                 float remaining_main,
                                 float *out_start_offset,
                                 float *out_effective_gap) {
    float start_offset = 0.0f;
    float effective_gap = base_gap;
    switch (justify) {
        case XENT_FLEX_JUSTIFY_END:
            start_offset = remaining_main;
            break;
        case XENT_FLEX_JUSTIFY_CENTER:
            start_offset = remaining_main * 0.5f;
            break;
        case XENT_FLEX_JUSTIFY_SPACE_BETWEEN:
            if (item_count > 1u) {
                effective_gap = base_gap + (remaining_main / (float)(item_count - 1u));
            }
            break;
        case XENT_FLEX_JUSTIFY_SPACE_AROUND:
            if (item_count > 0u) {
                effective_gap = base_gap + (remaining_main / (float)item_count);
                start_offset = remaining_main / (2.0f * (float)item_count);
            }
            break;
        case XENT_FLEX_JUSTIFY_SPACE_EVENLY:
            if (item_count > 0u) {
                float spacing = remaining_main / (float)(item_count + 1u);
                effective_gap = base_gap + spacing;
                start_offset = spacing;
            }
            break;
        case XENT_FLEX_JUSTIFY_START:
        default:
            break;
    }

    *out_start_offset = start_offset;
    *out_effective_gap = effective_gap;
}

static void xent_resolve_align_content(XentFlexAlignContent align_content,
                                       uint32_t line_count,
                                       float base_gap,
                                       float remaining_cross,
                                       float *out_start_offset,
                                       float *out_effective_gap) {
    float start_offset = 0.0f;
    float effective_gap = base_gap;
    switch (align_content) {
        case XENT_FLEX_ALIGN_CONTENT_END:
            start_offset = remaining_cross;
            break;
        case XENT_FLEX_ALIGN_CONTENT_CENTER:
            start_offset = remaining_cross * 0.5f;
            break;
        case XENT_FLEX_ALIGN_CONTENT_SPACE_BETWEEN:
            if (line_count > 1u) {
                effective_gap = base_gap + (remaining_cross / (float)(line_count - 1u));
            }
            break;
        case XENT_FLEX_ALIGN_CONTENT_SPACE_AROUND:
            if (line_count > 0u) {
                effective_gap = base_gap + (remaining_cross / (float)line_count);
                start_offset = remaining_cross / (2.0f * (float)line_count);
            }
            break;
        case XENT_FLEX_ALIGN_CONTENT_SPACE_EVENLY:
            if (line_count > 0u) {
                float spacing = remaining_cross / (float)(line_count + 1u);
                effective_gap = base_gap + spacing;
                start_offset = spacing;
            }
            break;
        case XENT_FLEX_ALIGN_CONTENT_START:
        default:
            break;
    }

    *out_start_offset = start_offset;
    *out_effective_gap = effective_gap;
}

static void xent_compute_line_stats(FlexLineData *line,
                                    const FlexChildData *children,
                                    bool wrap_enabled,
                                    float available_cross,
                                    float gap) {
    line->base_main = 0.0f;
    line->sum_grow = 0.0f;
    line->sum_shrink_weight = 0.0f;
    line->cross_outer = wrap_enabled ? 0.0f : available_cross;

    uint32_t count = line->count;
    const FlexChildData *base = &children[line->start];

#if XENT_ISPC_ENABLED
    if (count >= 32u) {
        /* Extract SoA from the AoS slice — ISPC kernels need contiguous arrays. */
        float *bm  = (float *)malloc(sizeof(float) * count * 8u);
        if (bm) {
            float *ml  = bm + count;
            float *mt  = ml + count;
            float *bc  = mt + count;
            float *mcl = bc + count;
            float *mct = mcl + count;
            float *gw  = mct + count;
            float *sh  = gw + count;
            for (uint32_t i = 0; i < count; ++i) {
                bm[i]  = base[i].base_main;
                ml[i]  = base[i].margin_lead;
                mt[i]  = base[i].margin_trail;
                bc[i]  = base[i].base_cross;
                mcl[i] = base[i].margin_cross_lead;
                mct[i] = base[i].margin_cross_trail;
                gw[i]  = base[i].grow;
                sh[i]  = base[i].shrink;
            }
            float sum_outer, sum_grow, sum_sw, max_cross;
            xent_ispc_flex_line_stats(bm, ml, mt, bc, mcl, mct, gw, sh, count,
                                      &sum_outer, &sum_grow, &sum_sw, &max_cross);
            line->base_main = sum_outer + (count > 1u ? gap * (float)(count - 1u) : 0.0f);
            line->sum_grow = sum_grow;
            line->sum_shrink_weight = sum_sw;
            if (wrap_enabled) {
                line->cross_outer = max_cross;
            }
            free(bm);
            return;
        }
    }
#endif

    for (uint32_t i = 0u; i < count; ++i) {
        const FlexChildData *child = &base[i];
        if (i > 0u) {
            line->base_main += gap;
        }
        line->base_main += child->base_main + child->margin_lead + child->margin_trail;
        line->sum_grow += child->grow;
        line->sum_shrink_weight += child->shrink * (child->base_main > 0.0f ? child->base_main : 1.0f);

        if (wrap_enabled) {
            float cross_outer = child->base_cross + child->margin_cross_lead + child->margin_cross_trail;
            if (cross_outer > line->cross_outer) {
                line->cross_outer = cross_outer;
            }
        }
    }
}

static float xent_clampf(float value, float min_v, float max_v) {
    if (value < min_v) {
        return min_v;
    }
    if (value > max_v) {
        return max_v;
    }
    return value;
}

static float xent_estimate_baseline(const XentContext *ctx, XentNodeId node, float cross_size) {
    if (cross_size <= 0.0f) {
        return 0.0f;
    }
    if (ctx->nodes.text[node] && ctx->nodes.text[node][0] != '\0') {
        float ratio = 0.8f;
        return cross_size * ratio;
    }
    return cross_size;
}

void xent_layout_node_flex(XentContext *ctx,
                           XentNodeId node,
                           float available_w,
                           float available_h,
                           float origin_x,
                           float origin_y) {
    float width = 0.0f;
    float height = 0.0f;
    xent_compute_intrinsic_size(ctx, node, available_w, available_h, &width, &height);

    const float x = origin_x + ctx->nodes.abs_pos_x[node];
    const float y = origin_y + ctx->nodes.abs_pos_y[node];

    ctx->nodes.proposed_w[node] = available_w;
    ctx->nodes.proposed_h[node] = available_h;
    ctx->nodes.decided_w[node] = width;
    ctx->nodes.decided_h[node] = height;
    ctx->nodes.abs_x[node] = x;
    ctx->nodes.abs_y[node] = y;
    xent_quantize_node_layout(ctx, node);

    width = ctx->nodes.decided_w[node];
    height = ctx->nodes.decided_h[node];
    const float qx = ctx->nodes.abs_x[node];
    const float qy = ctx->nodes.abs_y[node];

    uint32_t child_count = ctx->nodes.child_count[node];
    if (child_count == 0u) {
        return;
    }

    FlexChildData *children = (FlexChildData *)malloc(sizeof(FlexChildData) * (size_t)child_count);
    FlexLineData *lines = (FlexLineData *)malloc(sizeof(FlexLineData) * (size_t)child_count);
    if (!children || !lines) {
        free(children);
        free(lines);
        return;
    }
    memset(children, 0, sizeof(FlexChildData) * (size_t)child_count);
    memset(lines, 0, sizeof(FlexLineData) * (size_t)child_count);

    bool row = ctx->nodes.flex_direction[node] == (uint8_t)XENT_FLEX_ROW;
    bool wrap_enabled = ctx->nodes.flex_wrap[node] == (uint8_t)XENT_FLEX_WRAP;
    XentDirection direction = xent_get_resolved_direction(ctx, node);
    bool rtl_main = row && direction == XENT_DIRECTION_RTL;
    bool rtl_cross = !row && direction == XENT_DIRECTION_RTL;

    const float content_x = qx + ctx->nodes.padding_l[node];
    const float content_y = qy + ctx->nodes.padding_t[node];
    float content_w = width - (ctx->nodes.padding_l[node] + ctx->nodes.padding_r[node]);
    float content_h = height - (ctx->nodes.padding_t[node] + ctx->nodes.padding_b[node]);
    if (content_w < 0.0f) {
        content_w = 0.0f;
    }
    if (content_h < 0.0f) {
        content_h = 0.0f;
    }

    float available_main = row ? content_w : content_h;
    float available_cross = row ? content_h : content_w;
    float gap = ctx->nodes.gap[node];
    if (gap < 0.0f) {
        gap = 0.0f;
    }

    XentNodeId child = ctx->nodes.first_child[node];
    uint32_t idx = 0u;
    while (child != XENT_NODE_INVALID && idx < child_count) {
        float intrinsic_w = 0.0f;
        float intrinsic_h = 0.0f;
        xent_compute_intrinsic_size(ctx, child, content_w, content_h, &intrinsic_w, &intrinsic_h);

        float basis = ctx->nodes.flex_basis[child];
        float base_main = row ? intrinsic_w : intrinsic_h;
        if (!isnan(basis)) {
            base_main = basis;
        }

        children[idx].id = child;
        children[idx].base_main = base_main < 0.0f ? 0.0f : base_main;
        children[idx].base_cross = row ? intrinsic_h : intrinsic_w;
        children[idx].final_cross = children[idx].base_cross;
        children[idx].margin_lead =
            row ? (rtl_main ? ctx->nodes.margin_r[child] : ctx->nodes.margin_l[child]) : ctx->nodes.margin_t[child];
        children[idx].margin_trail =
            row ? (rtl_main ? ctx->nodes.margin_l[child] : ctx->nodes.margin_r[child]) : ctx->nodes.margin_b[child];
        children[idx].margin_cross_lead =
            row ? ctx->nodes.margin_t[child] : (rtl_cross ? ctx->nodes.margin_r[child] : ctx->nodes.margin_l[child]);
        children[idx].margin_cross_trail =
            row ? ctx->nodes.margin_b[child] : (rtl_cross ? ctx->nodes.margin_l[child] : ctx->nodes.margin_r[child]);
        children[idx].grow = ctx->nodes.flex_grow[child];
        children[idx].shrink = ctx->nodes.flex_shrink[child];
        children[idx].align_self = ctx->nodes.flex_align_self[child];

        child = ctx->nodes.next_sibling[child];
        ++idx;
    }
    child_count = idx;
    if (child_count == 0u) {
        free(children);
        free(lines);
        return;
    }

    uint32_t line_count = 0u;
    if (!wrap_enabled || !isfinite(available_main) || available_main <= 0.0f) {
        lines[0].start = 0u;
        lines[0].count = child_count;
        line_count = 1u;
    } else {
        uint32_t line_start = 0u;
        uint32_t line_items = 0u;
        float line_main = 0.0f;
        for (uint32_t i = 0u; i < child_count; ++i) {
            float item_outer_main = children[i].base_main + children[i].margin_lead + children[i].margin_trail;
            float candidate = (line_items == 0u) ? item_outer_main : (line_main + gap + item_outer_main);

            if (line_items > 0u && candidate > available_main) {
                lines[line_count].start = line_start;
                lines[line_count].count = line_items;
                line_count += 1u;
                line_start = i;
                line_items = 0u;
                line_main = 0.0f;
                candidate = item_outer_main;
            }

            line_main = candidate;
            line_items += 1u;
        }

        if (line_items > 0u) {
            lines[line_count].start = line_start;
            lines[line_count].count = line_items;
            line_count += 1u;
        }
    }

    for (uint32_t i = 0u; i < line_count; ++i) {
        xent_compute_line_stats(&lines[i], children, wrap_enabled, available_cross, gap);
    }

    float total_lines_cross = 0.0f;
    for (uint32_t i = 0u; i < line_count; ++i) {
        total_lines_cross += lines[i].cross_outer;
    }
    float line_gap = (line_count > 1u && wrap_enabled) ? gap : 0.0f;
    if (line_count > 1u) {
        total_lines_cross += line_gap * (float)(line_count - 1u);
    }

    float remaining_cross = available_cross - total_lines_cross;
    if (remaining_cross < 0.0f) {
        remaining_cross = 0.0f;
    }

    float cross_start_offset = 0.0f;
    float effective_line_gap = line_gap;
    xent_resolve_align_content((XentFlexAlignContent)ctx->nodes.flex_align_content[node],
                               line_count,
                               line_gap,
                               remaining_cross,
                               &cross_start_offset,
                               &effective_line_gap);

    float cross_cursor = cross_start_offset;
    for (uint32_t li = 0u; li < line_count; ++li) {
        FlexLineData *line = &lines[li];
        float delta = available_main - line->base_main;

        {
#if XENT_ISPC_ENABLED
            bool ispc_done = false;
            if (line->count >= 32u && (delta > 0.0f || delta < 0.0f)) {
                float *bm = (float *)malloc(sizeof(float) * line->count * 3u);
                if (bm) {
                    float *gw = bm + line->count;
                    float *sh = gw + line->count;
                    FlexChildData *base = &children[line->start];
                    for (uint32_t i = 0; i < line->count; ++i) {
                        bm[i] = base[i].base_main;
                        gw[i] = base[i].grow;
                        sh[i] = base[i].shrink;
                    }
                    if (delta > 0.0f && line->sum_grow > 0.0f) {
                        /* Use bm as both input base_main and output final_main (in-place is fine
                           because ISPC reads all inputs before writing outputs in foreach). */
                        xent_ispc_flex_distribute_grow(bm, gw, bm, line->count, delta, line->sum_grow);
                    } else if (delta < 0.0f && line->sum_shrink_weight > 0.0f) {
                        xent_ispc_flex_distribute_shrink(bm, sh, bm, line->count, delta, line->sum_shrink_weight);
                    }
                    for (uint32_t i = 0; i < line->count; ++i) {
                        base[i].final_main = bm[i];
                    }
                    free(bm);
                    ispc_done = true;
                }
            }
            if (!ispc_done)
#endif
            {
                for (uint32_t i = 0u; i < line->count; ++i) {
                    FlexChildData *entry = &children[line->start + i];
                    float size = entry->base_main;
                    if (delta > 0.0f && line->sum_grow > 0.0f) {
                        size += delta * (entry->grow / line->sum_grow);
                    } else if (delta < 0.0f && line->sum_shrink_weight > 0.0f) {
                        float weight = entry->shrink * (entry->base_main > 0.0f ? entry->base_main : 1.0f);
                        size += delta * (weight / line->sum_shrink_weight);
                        if (size < 0.0f) {
                            size = 0.0f;
                        }
                    }
                    entry->final_main = size;
                }
            }
        }

        float occupied_main = (line->count > 1u) ? (gap * (float)(line->count - 1u)) : 0.0f;
        {
#if XENT_ISPC_ENABLED
            bool ispc_done = false;
            if (line->count >= 32u) {
                float *fm = (float *)malloc(sizeof(float) * line->count * 3u);
                if (fm) {
                    float *ml = fm + line->count;
                    float *mt = ml + line->count;
                    FlexChildData *base = &children[line->start];
                    for (uint32_t i = 0; i < line->count; ++i) {
                        fm[i] = base[i].final_main;
                        ml[i] = base[i].margin_lead;
                        mt[i] = base[i].margin_trail;
                    }
                    occupied_main += xent_ispc_sum_outer_main(fm, ml, mt, line->count);
                    free(fm);
                    ispc_done = true;
                }
            }
            if (!ispc_done)
#endif
            {
                for (uint32_t i = 0u; i < line->count; ++i) {
                    FlexChildData *entry = &children[line->start + i];
                    occupied_main += entry->final_main + entry->margin_lead + entry->margin_trail;
                }
            }
        }
        float remaining_main = available_main - occupied_main;
        if (remaining_main < 0.0f) {
            remaining_main = 0.0f;
        }

        float main_start_offset = 0.0f;
        float effective_gap = gap;
        xent_resolve_justify((XentFlexJustify)ctx->nodes.flex_justify_content[node],
                             line->count,
                             gap,
                             remaining_main,
                             &main_start_offset,
                             &effective_gap);

        float line_baseline_target = 0.0f;
        bool has_baseline_items = false;
        for (uint32_t i = 0u; i < line->count; ++i) {
            FlexChildData *entry = &children[line->start + i];
            XentNodeId id = entry->id;

            XentFlexAlign align = (entry->align_self != (uint8_t)XENT_FLEX_ALIGN_AUTO)
                                      ? (XentFlexAlign)entry->align_self
                                      : (XentFlexAlign)ctx->nodes.flex_align_items[node];
            entry->resolved_align = (uint8_t)align;

            bool cross_auto = row ? isnan(ctx->nodes.style_h[id]) : isnan(ctx->nodes.style_w[id]);
            float cross_size = entry->base_cross;
            if (align == XENT_FLEX_ALIGN_STRETCH && cross_auto) {
                cross_size = line->cross_outer - entry->margin_cross_lead - entry->margin_cross_trail;
            }
            if (cross_size < 0.0f) {
                cross_size = 0.0f;
            }
            entry->final_cross = cross_size;
            entry->baseline_from_top = xent_estimate_baseline(ctx, id, cross_size);

            if (row && align == XENT_FLEX_ALIGN_BASELINE) {
                float baseline_edge = entry->margin_cross_lead + entry->baseline_from_top;
                if (!has_baseline_items || baseline_edge > line_baseline_target) {
                    line_baseline_target = baseline_edge;
                }
                has_baseline_items = true;
            }
        }

        float main_cursor = main_start_offset;
        for (uint32_t i = 0u; i < line->count; ++i) {
            FlexChildData *entry = &children[line->start + i];
            XentNodeId id = entry->id;

            main_cursor += entry->margin_lead;
            XentFlexAlign align = (XentFlexAlign)entry->resolved_align;
            float cross_size = entry->final_cross;

            float cross_free = line->cross_outer - cross_size - entry->margin_cross_lead - entry->margin_cross_trail;
            if (cross_free < 0.0f) {
                cross_free = 0.0f;
            }

            float cross_offset = entry->margin_cross_lead;
            if (row && align == XENT_FLEX_ALIGN_BASELINE && has_baseline_items) {
                float desired = line_baseline_target - entry->baseline_from_top;
                cross_offset = xent_clampf(desired, entry->margin_cross_lead, entry->margin_cross_lead + cross_free);
            } else if (align == XENT_FLEX_ALIGN_END) {
                cross_offset = entry->margin_cross_lead + cross_free;
            } else if (align == XENT_FLEX_ALIGN_CENTER) {
                cross_offset = entry->margin_cross_lead + (cross_free * 0.5f);
            }

            float child_origin_x = 0.0f;
            float child_origin_y = row ? (content_y + cross_cursor + cross_offset) : (content_y + main_cursor);

            float child_w = row ? entry->final_main : cross_size;
            float child_h = row ? cross_size : entry->final_main;
            if (child_w < 0.0f) {
                child_w = 0.0f;
            }
            if (child_h < 0.0f) {
                child_h = 0.0f;
            }

            if (row) {
                if (rtl_main) {
                    child_origin_x = content_x + (content_w - main_cursor - child_w);
                } else {
                    child_origin_x = content_x + main_cursor;
                }
            } else if (rtl_cross) {
                child_origin_x = content_x + (content_w - cross_cursor - cross_offset - child_w);
            } else {
                child_origin_x = content_x + cross_cursor + cross_offset;
            }

            xent_layout_dispatch_node(ctx, id, child_w, child_h, child_origin_x, child_origin_y);
            main_cursor += entry->final_main + entry->margin_trail;
            if (i + 1u < line->count) {
                main_cursor += effective_gap;
            }
        }

        cross_cursor += line->cross_outer;
        if (li + 1u < line_count) {
            cross_cursor += effective_line_gap;
        }
    }

    free(children);
    free(lines);
}
