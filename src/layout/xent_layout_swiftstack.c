#include "../xent_internal.h"

typedef struct StackChildData {
    XentNodeId node_id;
    float preferred_main;
    float preferred_cross;
    float margin_lead;
    float margin_trail;
    float margin_cross_lead;
    float margin_cross_trail;
    float priority;
    bool fixed_main;
    bool spacer;
} StackChildData;

static const StackChildData *g_swiftstack_sort_children = NULL;

static float xent_swiftstack_clampf(float v, float min_v, float max_v) {
    if (v < min_v) {
        v = min_v;
    }
    if (v > max_v) {
        v = max_v;
    }
    return v;
}

static int xent_compare_swiftstack_order_asc(const void *a, const void *b) {
    uint32_t ia = *(const uint32_t *)a;
    uint32_t ib = *(const uint32_t *)b;
    float pa = g_swiftstack_sort_children[ia].priority;
    float pb = g_swiftstack_sort_children[ib].priority;
    if (pa < pb) {
        return -1;
    }
    if (pa > pb) {
        return 1;
    }
    if (ia < ib) {
        return -1;
    }
    if (ia > ib) {
        return 1;
    }
    return 0;
}

static bool xent_same_priority(float a, float b) {
    const float eps = 0.0001f;
    return fabsf(a - b) <= eps;
}

static float xent_swiftstack_estimate_baseline(const XentContext *ctx, XentNodeId node, float cross_size) {
    if (cross_size <= 0.0f) {
        return 0.0f;
    }
    if (ctx->nodes.text[node] && ctx->nodes.text[node][0] != '\0') {
        return cross_size * 0.8f;
    }
    return cross_size;
}

void xent_layout_node_swiftstack(XentContext *ctx,
                                 XentNodeId node,
                                 float available_w,
                                 float available_h,
                                 float origin_x,
                                 float origin_y) {
    double swiftstack_start_ms = xent_now_ms();
    ctx->swiftstack_scope_depth += 1u;

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
        goto done;
    }

    double collect_start_ms = xent_now_ms();
    size_t children_bytes = sizeof(StackChildData) * (size_t)child_count;
    size_t order_bytes = sizeof(uint32_t) * (size_t)child_count;
    size_t main_bytes = sizeof(float) * (size_t)child_count;
    size_t align_order = _Alignof(uint32_t);
    size_t align_main = _Alignof(float);
    size_t total_bytes = children_bytes + (align_order - 1u) + order_bytes + (align_main - 1u) + main_bytes;
    uint8_t *block = (uint8_t *)xent_scratch_alloc(ctx, total_bytes, _Alignof(StackChildData));
    StackChildData *children = NULL;
    uint32_t *priority_order = NULL;
    float *main_sizes = NULL;
    if (block) {
        children = (StackChildData *)block;
        uintptr_t order_addr = (uintptr_t)(block + children_bytes);
        order_addr = (order_addr + (uintptr_t)(align_order - 1u)) & ~(uintptr_t)(align_order - 1u);
        priority_order = (uint32_t *)order_addr;
        uintptr_t main_addr = order_addr + order_bytes;
        main_addr = (main_addr + (uintptr_t)(align_main - 1u)) & ~(uintptr_t)(align_main - 1u);
        main_sizes = (float *)main_addr;
    }
    if (!children || !priority_order || !main_sizes) {
        goto done;
    }

    bool horizontal = ctx->nodes.stack_axis[node] == (uint8_t)XENT_AXIS_HORIZONTAL;
    bool baseline_align = horizontal && ctx->nodes.stack_align[node] == (uint8_t)XENT_STACK_ALIGN_BASELINE;

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

    float available_main = horizontal ? content_w : content_h;
    float available_cross = horizontal ? content_h : content_w;

    float gap = ctx->nodes.gap[node];
    if (gap < 0.0f) {
        gap = 0.0f;
    }

    float sum_min = 0.0f;
    uint32_t spacer_count = 0u;
    float priority_sum = 0.0f;
    uint32_t collected_count = 0u;

    XentNodeId child = ctx->nodes.first_child[node];
    while (child != XENT_NODE_INVALID && collected_count < child_count) {
        ctx->profile.sibling_scans += 1u;

        bool spacer = ctx->nodes.is_spacer[child] != 0u;
        bool fixed_main = false;
        float preferred_main = 0.0f;
        float preferred_cross = 0.0f;
        float intrinsic_w = 0.0f;
        float intrinsic_h = 0.0f;
        bool have_intrinsic = false;

        if (!spacer) {
            /* Hot-path optimization:
             * when main-axis size is explicit, avoid full intrinsic/text measurement. */
            if (horizontal && !isnan(ctx->nodes.style_w[child])) {
                preferred_main =
                    xent_swiftstack_clampf(ctx->nodes.style_w[child], ctx->nodes.min_w[child], ctx->nodes.max_w[child]);
                fixed_main = true;
                if (baseline_align && isnan(ctx->nodes.style_h[child])) {
                    xent_compute_intrinsic_size(ctx, child, content_w, content_h, &intrinsic_w, &intrinsic_h);
                    have_intrinsic = true;
                }
            } else if (!horizontal && !isnan(ctx->nodes.style_h[child])) {
                preferred_main =
                    xent_swiftstack_clampf(ctx->nodes.style_h[child], ctx->nodes.min_h[child], ctx->nodes.max_h[child]);
                fixed_main = true;
            } else {
                xent_compute_intrinsic_size(ctx, child, content_w, content_h, &intrinsic_w, &intrinsic_h);
                preferred_main = horizontal ? intrinsic_w : intrinsic_h;
                have_intrinsic = true;
            }

            if (horizontal) {
                if (!isnan(ctx->nodes.style_h[child])) {
                    preferred_cross =
                        xent_swiftstack_clampf(ctx->nodes.style_h[child], ctx->nodes.min_h[child], ctx->nodes.max_h[child]);
                } else if (have_intrinsic) {
                    preferred_cross = xent_swiftstack_clampf(intrinsic_h, ctx->nodes.min_h[child], ctx->nodes.max_h[child]);
                }
            } else {
                if (!isnan(ctx->nodes.style_w[child])) {
                    preferred_cross =
                        xent_swiftstack_clampf(ctx->nodes.style_w[child], ctx->nodes.min_w[child], ctx->nodes.max_w[child]);
                } else if (have_intrinsic) {
                    preferred_cross = xent_swiftstack_clampf(intrinsic_w, ctx->nodes.min_w[child], ctx->nodes.max_w[child]);
                }
            }
        }

        children[collected_count].node_id = child;
        children[collected_count].preferred_main = preferred_main;
        children[collected_count].preferred_cross = preferred_cross;
        children[collected_count].margin_lead = horizontal ? ctx->nodes.margin_l[child] : ctx->nodes.margin_t[child];
        children[collected_count].margin_trail = horizontal ? ctx->nodes.margin_r[child] : ctx->nodes.margin_b[child];
        children[collected_count].margin_cross_lead = horizontal ? ctx->nodes.margin_t[child] : ctx->nodes.margin_l[child];
        children[collected_count].margin_cross_trail = horizontal ? ctx->nodes.margin_b[child] : ctx->nodes.margin_r[child];
        children[collected_count].priority = ctx->nodes.layout_priority[child];
        children[collected_count].fixed_main = fixed_main;
        children[collected_count].spacer = spacer;
        main_sizes[collected_count] = preferred_main;

        sum_min += preferred_main + children[collected_count].margin_lead + children[collected_count].margin_trail;
        if (spacer) {
            spacer_count += 1u;
        }
        if (!fixed_main && children[collected_count].priority > 0.0f) {
            priority_sum += children[collected_count].priority;
        }

        priority_order[collected_count] = collected_count;
        child = ctx->nodes.next_sibling[child];
        ++collected_count;
    }
    ctx->profile.swiftstack_collect_ms += (xent_now_ms() - collect_start_ms);

    if (collected_count > 1u) {
        sum_min += gap * (float)(collected_count - 1u);
    }

    float remainder = available_main - sum_min;
    if (remainder > 0.0f) {
        if (spacer_count > 0u) {
            float each = remainder / (float)spacer_count;
            for (uint32_t i = 0; i < collected_count; ++i) {
                if (children[i].spacer) {
                    main_sizes[i] += each;
                }
            }
        } else if (priority_sum > 0.0f) {
            for (uint32_t i = 0; i < collected_count; ++i) {
                if (!children[i].fixed_main && children[i].priority > 0.0f) {
                    main_sizes[i] += remainder * (children[i].priority / priority_sum);
                }
            }
        }
    } else if (remainder < 0.0f) {
        float deficit = -remainder;
        float reducible_total = xent_simd_sum_f32(main_sizes, collected_count);

        if (deficit >= reducible_total) {
            /* SIMD-friendly bulk zero when every child fully collapses. */
            xent_simd_fill_f32(main_sizes, collected_count, 0.0f);
        } else {
            double sort_start_ms = xent_now_ms();
            ctx->profile.sort_calls += 1u;

            /* Old path used insertion sort + nested ID lookup (O(N^2) + O(N^2)).
             * Sorting child indices and reducing in one pass keeps this phase near O(N log N). */
            g_swiftstack_sort_children = children;
            qsort(priority_order, (size_t)collected_count, sizeof(uint32_t), xent_compare_swiftstack_order_asc);
            g_swiftstack_sort_children = NULL;
            ctx->profile.swiftstack_sort_ms += (xent_now_ms() - sort_start_ms);

            uint32_t rank = 0u;
            while (rank < collected_count && deficit > 0.0f) {
                float group_priority = children[priority_order[rank]].priority;
                uint32_t group_start = rank;
                float group_total = 0.0f;
                while (rank < collected_count &&
                       xent_same_priority(children[priority_order[rank]].priority, group_priority)) {
                    group_total += main_sizes[priority_order[rank]];
                    ++rank;
                }

                if (group_total <= 0.0f) {
                    continue;
                }

                float group_reduce = (deficit < group_total) ? deficit : group_total;
                float flexible_total = 0.0f;
                float fixed_total = 0.0f;
                for (uint32_t i = group_start; i < rank; ++i) {
                    uint32_t child_index = priority_order[i];
                    if (children[child_index].fixed_main) {
                        fixed_total += main_sizes[child_index];
                    } else {
                        flexible_total += main_sizes[child_index];
                    }
                }

                float reduce_flexible = group_reduce < flexible_total ? group_reduce : flexible_total;
                float remaining_reduce = group_reduce - reduce_flexible;

                if (reduce_flexible > 0.0f && flexible_total > 0.0f) {
                    for (uint32_t i = group_start; i < rank; ++i) {
                        uint32_t child_index = priority_order[i];
                        if (children[child_index].fixed_main) {
                            continue;
                        }
                        float current = main_sizes[child_index];
                        float share = reduce_flexible * (current / flexible_total);
                        if (share > current) {
                            share = current;
                        }
                        main_sizes[child_index] = current - share;
                    }
                }

                if (remaining_reduce > 0.0f && fixed_total > 0.0f) {
                    for (uint32_t i = group_start; i < rank; ++i) {
                        uint32_t child_index = priority_order[i];
                        if (!children[child_index].fixed_main) {
                            continue;
                        }
                        float current = main_sizes[child_index];
                        float share = remaining_reduce * (current / fixed_total);
                        if (share > current) {
                            share = current;
                        }
                        main_sizes[child_index] = current - share;
                    }
                }

                deficit -= group_reduce;
            }
        }
    }

    float cursor = horizontal ? content_x : content_y;
    float baseline_target = 0.0f;
    bool has_baseline_children = false;
    if (baseline_align) {
        for (uint32_t i = 0; i < collected_count; ++i) {
            if (children[i].spacer) {
                continue;
            }
            float cross_size = children[i].preferred_cross;
            if (cross_size <= 0.0f) {
                cross_size =
                    fmaxf(0.0f, available_cross - children[i].margin_cross_lead - children[i].margin_cross_trail);
            }
            float baseline = xent_swiftstack_estimate_baseline(ctx, children[i].node_id, cross_size);
            float candidate = children[i].margin_cross_lead + baseline;
            if (!has_baseline_children || candidate > baseline_target) {
                baseline_target = candidate;
            }
            has_baseline_children = true;
        }
    }

    for (uint32_t i = 0; i < collected_count; ++i) {
        cursor += children[i].margin_lead;
        float child_main = main_sizes[i];

        float child_origin_x = horizontal ? cursor : content_x + children[i].margin_cross_lead;
        float child_origin_y = horizontal ? content_y + children[i].margin_cross_lead : cursor;

        float child_w = horizontal
                            ? child_main
                            : fmaxf(0.0f, available_cross - children[i].margin_cross_lead - children[i].margin_cross_trail);
        float child_h = horizontal ? 0.0f : child_main;
        if (horizontal) {
            bool use_baseline = baseline_align && has_baseline_children && !children[i].spacer;
            if (use_baseline) {
                child_h = children[i].preferred_cross;
                if (child_h <= 0.0f) {
                    child_h = fmaxf(0.0f, available_cross - children[i].margin_cross_lead - children[i].margin_cross_trail);
                }
                float cross_free = available_cross - child_h - children[i].margin_cross_lead - children[i].margin_cross_trail;
                if (cross_free < 0.0f) {
                    cross_free = 0.0f;
                }
                float baseline = xent_swiftstack_estimate_baseline(ctx, children[i].node_id, child_h);
                float desired = baseline_target - baseline;
                float min_offset = children[i].margin_cross_lead;
                float max_offset = children[i].margin_cross_lead + cross_free;
                child_origin_y = content_y + xent_swiftstack_clampf(desired, min_offset, max_offset);
            } else {
                child_h = fmaxf(0.0f, available_cross - children[i].margin_cross_lead - children[i].margin_cross_trail);
            }
        }

        if (child_w < 0.0f) {
            child_w = 0.0f;
        }
        if (child_h < 0.0f) {
            child_h = 0.0f;
        }

        xent_layout_dispatch_node(ctx, children[i].node_id, child_w, child_h, child_origin_x, child_origin_y);
        cursor += child_main + children[i].margin_trail;
        if (i + 1u < collected_count) {
            cursor += gap;
        }
    }

done:
    ctx->swiftstack_scope_depth -= 1u;
    ctx->profile.swiftstack_total_ms += (xent_now_ms() - swiftstack_start_ms);
}
