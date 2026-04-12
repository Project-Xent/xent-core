#include "../xent_internal.h"

/*
 * Grid layout algorithm (WinUI 3 semantics).
 *
 * Track sizing is resolved in three passes per axis:
 *   1. Auto tracks  – sized to the maximum intrinsic size of their children.
 *   2. Pixel tracks – fixed to the value given in the grid definition.
 *   3. Star tracks  – proportional share of the remaining space.
 *
 * Children are placed into cells according to grid_row / grid_column and may
 * span multiple rows / columns via grid_row_span / grid_column_span.
 */

/* ------------------------------------------------------------------ */
/* helpers                                                             */
/* ------------------------------------------------------------------ */

static float maxf(float a, float b) {
    return a > b ? a : b;
}

/* Clamp a track index so it never exceeds (track_count - 1). */
static uint16_t clamp_track(uint16_t idx, uint8_t track_count) {
    if (idx >= track_count) {
        return (uint16_t)(track_count - 1u);
    }
    return idx;
}

/* Clamp a span so that (idx + span) never exceeds track_count. */
static uint16_t clamp_span(uint16_t idx, uint16_t span, uint8_t track_count) {
    if (span == 0u) {
        span = 1u;
    }
    if (idx + span > track_count) {
        span = (uint16_t)(track_count - idx);
    }
    if (span == 0u) {
        span = 1u;
    }
    return span;
}

/* Sum track sizes and inter-track gaps for a contiguous span.
 * For a span of N tracks starting at `start`, there are (N - 1) gaps. */
static float span_extent(const float *sizes, const float *positions,
                         uint16_t start, uint16_t span, float gap) {
    (void)positions;
    float total = 0.0f;
    for (uint16_t i = 0; i < span; i++) {
        total += sizes[start + i];
    }
    if (span > 1u) {
        total += gap * (float)(span - 1u);
    }
    return total;
}

/* ------------------------------------------------------------------ */
/* resolve_tracks – three-pass algorithm for one axis                  */
/* ------------------------------------------------------------------ */

static void resolve_tracks(
    XentContext *ctx,
    XentNodeId container,
    uint8_t track_count,
    const uint8_t *modes,
    const float *values,
    float gap,
    float available,
    /* axis == 0 → columns, axis == 1 → rows */
    int axis,
    float *out_sizes)
{
    float total_gap = (track_count > 1u) ? gap * (float)(track_count - 1u) : 0.0f;
    float remaining = available - total_gap;
    if (remaining < 0.0f) {
        remaining = 0.0f;
    }

    /* Initialise all track sizes to zero. */
    for (uint8_t i = 0; i < track_count; i++) {
        out_sizes[i] = 0.0f;
    }

    /* ------- Pass 1: Auto tracks ------- */
    for (uint8_t t = 0; t < track_count; t++) {
        if (modes[t] != (uint8_t)XENT_GRID_AUTO) {
            continue;
        }
        float max_size = 0.0f;

        XentNodeId child = ctx->nodes.first_child[container];
        while (child != XENT_NODE_INVALID) {
            uint16_t cidx, cspan;
            if (axis == 0) {
                /* columns */
                cidx = clamp_track(ctx->nodes.grid_column[child], track_count);
                cspan = clamp_span(cidx, ctx->nodes.grid_column_span[child], track_count);
            } else {
                /* rows */
                cidx = clamp_track(ctx->nodes.grid_row[child], track_count);
                cspan = clamp_span(cidx, ctx->nodes.grid_row_span[child], track_count);
            }

            /* Only consider children that actually sit in this track and
             * don't span multiple tracks (spanning children are trickier –
             * we attribute their size only when they occupy a single auto
             * track in this axis to keep the algorithm simple & WinUI-like). */
            if (cidx == t && cspan == 1u) {
                float ml = ctx->nodes.margin_l[child];
                float mr = ctx->nodes.margin_r[child];
                float mt = ctx->nodes.margin_t[child];
                float mb = ctx->nodes.margin_b[child];

                float child_avail_w = available;
                float child_avail_h = available;

                float intr_w = 0.0f;
                float intr_h = 0.0f;
                xent_compute_intrinsic_size(ctx, child,
                                            child_avail_w, child_avail_h,
                                            &intr_w, &intr_h);

                float needed;
                if (axis == 0) {
                    needed = intr_w + ml + mr;
                } else {
                    needed = intr_h + mt + mb;
                }
                if (needed > max_size) {
                    max_size = needed;
                }
            }
            child = ctx->nodes.next_sibling[child];
        }
        out_sizes[t] = max_size;
    }

    /* ------- Pass 2: Pixel tracks ------- */
    for (uint8_t t = 0; t < track_count; t++) {
        if (modes[t] == (uint8_t)XENT_GRID_PIXEL) {
            out_sizes[t] = maxf(values[t], 0.0f);
        }
    }

    /* Compute space consumed by auto + pixel tracks so far. */
    float used = 0.0f;
    for (uint8_t t = 0; t < track_count; t++) {
        if (modes[t] != (uint8_t)XENT_GRID_STAR) {
            used += out_sizes[t];
        }
    }

    /* ------- Pass 3: Star tracks ------- */
    float star_space = remaining - used;
    if (star_space < 0.0f) {
        star_space = 0.0f;
    }

    float total_stars = 0.0f;
    for (uint8_t t = 0; t < track_count; t++) {
        if (modes[t] == (uint8_t)XENT_GRID_STAR) {
            float sv = values[t];
            if (sv <= 0.0f) {
                sv = 1.0f; /* default star weight */
            }
            total_stars += sv;
        }
    }

    if (total_stars > 0.0f) {
        for (uint8_t t = 0; t < track_count; t++) {
            if (modes[t] == (uint8_t)XENT_GRID_STAR) {
                float sv = values[t];
                if (sv <= 0.0f) {
                    sv = 1.0f;
                }
                out_sizes[t] = star_space * (sv / total_stars);
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* compute_positions – running sum of sizes + gaps                     */
/* ------------------------------------------------------------------ */

static void compute_positions(const float *sizes, uint8_t count, float gap,
                              float origin, float *out_positions) {
    float pos = origin;
    for (uint8_t i = 0; i < count; i++) {
        out_positions[i] = pos;
        pos += sizes[i] + gap;
    }
}

/* ------------------------------------------------------------------ */
/* xent_layout_node_grid                                               */
/* ------------------------------------------------------------------ */

void xent_layout_node_grid(XentContext *ctx,
                           XentNodeId node,
                           float available_w,
                           float available_h,
                           float origin_x,
                           float origin_y) {
    /* ---- Step 1: compute the grid container's own size ---- */
    float width = 0.0f;
    float height = 0.0f;
    xent_compute_intrinsic_size(ctx, node, available_w, available_h,
                                &width, &height);

    const float x = origin_x + ctx->nodes.abs_pos_x[node];
    const float y = origin_y + ctx->nodes.abs_pos_y[node];

    /* ---- Step 2: commit the grid container's layout ---- */
    ctx->nodes.proposed_w[node] = available_w;
    ctx->nodes.proposed_h[node] = available_h;
    ctx->nodes.decided_w[node] = width;
    ctx->nodes.decided_h[node] = height;
    ctx->nodes.abs_x[node] = x;
    ctx->nodes.abs_y[node] = y;
    xent_quantize_node_layout(ctx, node);

    /* Read back quantised values. */
    width  = ctx->nodes.decided_w[node];
    height = ctx->nodes.decided_h[node];
    const float qx = ctx->nodes.abs_x[node];
    const float qy = ctx->nodes.abs_y[node];

    /* ---- Step 3: content area ---- */
    float content_w = width - (ctx->nodes.padding_l[node] + ctx->nodes.padding_r[node]);
    float content_h = height - (ctx->nodes.padding_t[node] + ctx->nodes.padding_b[node]);
    if (content_w < 0.0f) {
        content_w = 0.0f;
    }
    if (content_h < 0.0f) {
        content_h = 0.0f;
    }

    const float content_x = qx + ctx->nodes.padding_l[node];
    const float content_y = qy + ctx->nodes.padding_t[node];

    /* ---- Step 4: no grid_def → fall back to absolute-style layout ---- */
    const XentGridDef *def = ctx->nodes.grid_def[node];
    if (!def) {
        XentNodeId child = ctx->nodes.first_child[node];
        while (child != XENT_NODE_INVALID) {
            xent_layout_dispatch_node(ctx, child,
                                      content_w, content_h,
                                      content_x, content_y);
            child = ctx->nodes.next_sibling[child];
        }
        return;
    }

    /* ---- Normalise track counts (default 1×1 star) ---- */
    uint8_t row_count = def->row_count;
    uint8_t col_count = def->col_count;

    uint8_t row_modes_buf[XENT_GRID_MAX_TRACKS];
    float   row_values_buf[XENT_GRID_MAX_TRACKS];
    uint8_t col_modes_buf[XENT_GRID_MAX_TRACKS];
    float   col_values_buf[XENT_GRID_MAX_TRACKS];

    const uint8_t *row_modes  = def->row_modes;
    const float   *row_values = def->row_values;
    const uint8_t *col_modes  = def->col_modes;
    const float   *col_values = def->col_values;

    if (row_count == 0u) {
        row_count = 1u;
        row_modes_buf[0] = (uint8_t)XENT_GRID_STAR;
        row_values_buf[0] = 1.0f;
        row_modes = row_modes_buf;
        row_values = row_values_buf;
    }
    if (col_count == 0u) {
        col_count = 1u;
        col_modes_buf[0] = (uint8_t)XENT_GRID_STAR;
        col_values_buf[0] = 1.0f;
        col_modes = col_modes_buf;
        col_values = col_values_buf;
    }

    float row_gap = def->row_gap;
    float col_gap = def->col_gap;
    if (row_gap < 0.0f) {
        row_gap = 0.0f;
    }
    if (col_gap < 0.0f) {
        col_gap = 0.0f;
    }

    /* ---- Step 5: resolve track sizes ---- */
    float col_sizes[XENT_GRID_MAX_TRACKS];
    float row_sizes[XENT_GRID_MAX_TRACKS];

    resolve_tracks(ctx, node, col_count, col_modes, col_values,
                   col_gap, content_w, /*axis=*/0, col_sizes);
    resolve_tracks(ctx, node, row_count, row_modes, row_values,
                   row_gap, content_h, /*axis=*/1, row_sizes);

    /* ---- Step 6: compute track positions ---- */
    float col_positions[XENT_GRID_MAX_TRACKS];
    float row_positions[XENT_GRID_MAX_TRACKS];

    compute_positions(col_sizes, col_count, col_gap, content_x, col_positions);
    compute_positions(row_sizes, row_count, row_gap, content_y, row_positions);

    /* ---- Step 7: position each child ---- */
    XentNodeId child = ctx->nodes.first_child[node];
    while (child != XENT_NODE_INVALID) {
        /* Read and clamp the child's grid placement. */
        uint16_t col_idx  = clamp_track(ctx->nodes.grid_column[child], col_count);
        uint16_t row_idx  = clamp_track(ctx->nodes.grid_row[child], row_count);
        uint16_t col_span = clamp_span(col_idx,
                                       ctx->nodes.grid_column_span[child],
                                       col_count);
        uint16_t row_span = clamp_span(row_idx,
                                       ctx->nodes.grid_row_span[child],
                                       row_count);

        /* Compute the cell's origin and extent (including inter-track gaps). */
        float cell_x = col_positions[col_idx];
        float cell_y = row_positions[row_idx];
        float cell_w = span_extent(col_sizes, col_positions, col_idx, col_span, col_gap);
        float cell_h = span_extent(row_sizes, row_positions, row_idx, row_span, row_gap);

        /* Subtract child margins to get the available area for the child. */
        float ml = ctx->nodes.margin_l[child];
        float mt = ctx->nodes.margin_t[child];
        float mr = ctx->nodes.margin_r[child];
        float mb = ctx->nodes.margin_b[child];

        float child_origin_x = cell_x + ml;
        float child_origin_y = cell_y + mt;
        float child_avail_w  = cell_w - (ml + mr);
        float child_avail_h  = cell_h - (mt + mb);

        if (child_avail_w < 0.0f) {
            child_avail_w = 0.0f;
        }
        if (child_avail_h < 0.0f) {
            child_avail_h = 0.0f;
        }

        xent_layout_dispatch_node(ctx, child,
                                  child_avail_w, child_avail_h,
                                  child_origin_x, child_origin_y);

        child = ctx->nodes.next_sibling[child];
    }
}