#include "../xent_internal.h"

typedef enum GridAxis
{
	GRID_AXIS_COLUMNS,
	GRID_AXIS_ROWS,
} GridAxis;

typedef struct GridTrackSet {
	uint8_t        count;
	uint8_t const *modes;
	float const   *values;
	uint8_t        modes_buf [XENT_GRID_MAX_TRACKS];
	float          values_buf [XENT_GRID_MAX_TRACKS];
	float          gap;
	float          available;
	float          sizes [XENT_GRID_MAX_TRACKS];
	float          positions [XENT_GRID_MAX_TRACKS];
} GridTrackSet;

typedef struct GridLayoutFrame {
	XentNodeId node;
	float      content_x;
	float      content_y;
	float      content_w;
	float      content_h;
} GridLayoutFrame;

typedef struct GridAxisPlacement {
	uint16_t index;
	uint16_t span;
} GridAxisPlacement;

typedef struct GridChildPlacement {
	GridAxisPlacement column;
	GridAxisPlacement row;
	float             cell_x;
	float             cell_y;
	float             cell_w;
	float             cell_h;
	float             margin_l;
	float             margin_t;
	float             margin_r;
	float             margin_b;
} GridChildPlacement;

static float    maxf(float a, float b) { return a > b ? a : b; }

static uint16_t clamp_track(uint16_t idx, uint8_t track_count) {
	if (idx >= track_count) return ( uint16_t ) (track_count - 1u);
	return idx;
}

static uint16_t clamp_span(uint16_t idx, uint16_t span, uint8_t track_count) {
	if (span == 0u) span = 1u;
	if (idx + span > track_count) span = ( uint16_t ) (track_count - idx);
	return span == 0u ? 1u : span;
}

static float span_extent(float const *sizes, uint16_t start, uint16_t span, float gap) {
	float total = 0.0f;
	for (uint16_t i = 0u; i < span; ++i) total += sizes [start + i];
	if (span > 1u) total += gap * ( float ) (span - 1u);
	return total;
}

static float grid_content_extent(float size, float before, float after) {
	float extent = size - (before + after);
	return extent < 0.0f ? 0.0f : extent;
}

static void grid_commit_container(XentLayoutRequest const *request, GridLayoutFrame *frame) {
	XentContext *ctx    = request->ctx;
	XentNodeId   node   = request->node;
	float        width  = 0.0f;
	float        height = 0.0f;
	xent_compute_intrinsic_size(ctx, node, request->available_w, request->available_h, &width, &height);

	ctx->nodes.layout.proposed_w [node] = request->available_w;
	ctx->nodes.layout.proposed_h [node] = request->available_h;
	ctx->nodes.layout.decided_w [node]  = width;
	ctx->nodes.layout.decided_h [node]  = height;
	ctx->nodes.layout.abs_x [node]      = request->origin_x + ctx->nodes.layout.abs_pos_x [node];
	ctx->nodes.layout.abs_y [node]      = request->origin_y + ctx->nodes.layout.abs_pos_y [node];
	xent_quantize_node_layout(ctx, node);

	width            = ctx->nodes.layout.decided_w [node];
	height           = ctx->nodes.layout.decided_h [node];
	frame->node      = node;
	frame->content_x = ctx->nodes.layout.abs_x [node] + ctx->nodes.layout.padding_l [node];
	frame->content_y = ctx->nodes.layout.abs_y [node] + ctx->nodes.layout.padding_t [node];
	frame->content_w = grid_content_extent(width, ctx->nodes.layout.padding_l [node], ctx->nodes.layout.padding_r [node]);
	frame->content_h
	  = grid_content_extent(height, ctx->nodes.layout.padding_t [node], ctx->nodes.layout.padding_b [node]);
}

static void grid_layout_fallback_children(XentContext *ctx, GridLayoutFrame const *frame) {
	for (XentNodeId child = ctx->nodes.topology.first_child [frame->node]; child != XENT_NODE_INVALID;
	  child               = ctx->nodes.topology.next_sibling [child])
	{
		xent_layout_dispatch_node(
		  &(XentLayoutRequest) {ctx, child, frame->content_w, frame->content_h, frame->content_x, frame->content_y}
		);
	}
}

static void grid_init_default_track(GridTrackSet *tracks) {
	tracks->count          = 1u;
	tracks->modes_buf [0]  = ( uint8_t ) XENT_GRID_STAR;
	tracks->values_buf [0] = 1.0f;
	tracks->modes          = tracks->modes_buf;
	tracks->values         = tracks->values_buf;
}

typedef struct GridTrackInit {
	uint8_t        count;
	uint8_t const *modes;
	float const   *values;
	float          gap;
	float          available;
} GridTrackInit;

static void grid_init_tracks(GridTrackSet *tracks, GridTrackInit init) {
	*tracks           = (GridTrackSet) {0};
	tracks->count     = init.count;
	tracks->modes     = init.modes;
	tracks->values    = init.values;
	tracks->gap       = init.gap < 0.0f ? 0.0f : init.gap;
	tracks->available = init.available;
	if (tracks->count == 0u) grid_init_default_track(tracks);
}

static GridAxisPlacement
grid_axis_placement(XentContext const *ctx, XentNodeId child, GridAxis axis, uint8_t track_count) {
	uint16_t index = axis == GRID_AXIS_COLUMNS ? ctx->nodes.grid.column [child] : ctx->nodes.grid.row [child];
	uint16_t span  = axis == GRID_AXIS_COLUMNS ? ctx->nodes.grid.column_span [child] : ctx->nodes.grid.row_span [child];
	index          = clamp_track(index, track_count);
	span           = clamp_span(index, span, track_count);
	return (GridAxisPlacement) {index, span};
}

static float grid_child_axis_need(XentContext *ctx, XentNodeId child, GridAxis axis, float available) {
	float intrinsic_w = 0.0f;
	float intrinsic_h = 0.0f;
	xent_compute_intrinsic_size(ctx, child, available, available, &intrinsic_w, &intrinsic_h);
	if (axis == GRID_AXIS_COLUMNS)
		return intrinsic_w + ctx->nodes.layout.margin_l [child] + ctx->nodes.layout.margin_r [child];
	return intrinsic_h + ctx->nodes.layout.margin_t [child] + ctx->nodes.layout.margin_b [child];
}

static float
grid_auto_track_size(XentContext *ctx, XentNodeId container, GridTrackSet const *tracks, GridAxis axis, uint8_t track) {
	float max_size = 0.0f;
	for (XentNodeId child = ctx->nodes.topology.first_child [container]; child != XENT_NODE_INVALID;
	  child               = ctx->nodes.topology.next_sibling [child])
	{
		GridAxisPlacement placement = grid_axis_placement(ctx, child, axis, tracks->count);
		if (placement.index == track && placement.span == 1u)
			max_size = maxf(max_size, grid_child_axis_need(ctx, child, axis, tracks->available));
	}
	return max_size;
}

static void grid_resolve_auto_tracks(XentContext *ctx, XentNodeId container, GridTrackSet *tracks, GridAxis axis) {
	for (uint8_t i = 0u; i < tracks->count; ++i)
		if (tracks->modes [i] == ( uint8_t ) XENT_GRID_AUTO)
			tracks->sizes [i] = grid_auto_track_size(ctx, container, tracks, axis, i);
}

static void grid_resolve_pixel_tracks(GridTrackSet *tracks) {
	for (uint8_t i = 0u; i < tracks->count; ++i)
		if (tracks->modes [i] == ( uint8_t ) XENT_GRID_PIXEL) tracks->sizes [i] = maxf(tracks->values [i], 0.0f);
}

static float grid_non_star_size(GridTrackSet const *tracks) {
	float used = 0.0f;
	for (uint8_t i = 0u; i < tracks->count; ++i)
		if (tracks->modes [i] != ( uint8_t ) XENT_GRID_STAR) used += tracks->sizes [i];
	return used;
}

static float grid_total_star_weight(GridTrackSet const *tracks) {
	float total = 0.0f;
	for (uint8_t i = 0u; i < tracks->count; ++i)
		if (tracks->modes [i] == ( uint8_t ) XENT_GRID_STAR)
			total += tracks->values [i] > 0.0f ? tracks->values [i] : 1.0f;
	return total;
}

static float grid_remaining_track_space(GridTrackSet const *tracks) {
	float total_gap = tracks->count > 1u ? tracks->gap * ( float ) (tracks->count - 1u) : 0.0f;
	float remaining = tracks->available - total_gap - grid_non_star_size(tracks);
	return remaining < 0.0f ? 0.0f : remaining;
}

static void grid_resolve_star_tracks(GridTrackSet *tracks) {
	float star_weight = grid_total_star_weight(tracks);
	if (star_weight <= 0.0f) return;

	float star_space = grid_remaining_track_space(tracks);
	for (uint8_t i = 0u; i < tracks->count; ++i) {
		if (tracks->modes [i] != ( uint8_t ) XENT_GRID_STAR) continue;
		float weight      = tracks->values [i] > 0.0f ? tracks->values [i] : 1.0f;
		tracks->sizes [i] = star_space * (weight / star_weight);
	}
}

static void grid_resolve_tracks(XentContext *ctx, XentNodeId container, GridTrackSet *tracks, GridAxis axis) {
	for (uint8_t i = 0u; i < tracks->count; ++i) tracks->sizes [i] = 0.0f;
	grid_resolve_auto_tracks(ctx, container, tracks, axis);
	grid_resolve_pixel_tracks(tracks);
	grid_resolve_star_tracks(tracks);
}

static void grid_compute_positions(GridTrackSet *tracks, float origin) {
	float cursor = origin;
	for (uint8_t i = 0u; i < tracks->count; ++i) {
		tracks->positions [i]  = cursor;
		cursor                += tracks->sizes [i] + tracks->gap;
	}
}

static GridChildPlacement
grid_child_placement(XentContext const *ctx, XentNodeId child, GridTrackSet const *columns, GridTrackSet const *rows) {
	GridChildPlacement placement = {0};
	placement.column             = grid_axis_placement(ctx, child, GRID_AXIS_COLUMNS, columns->count);
	placement.row                = grid_axis_placement(ctx, child, GRID_AXIS_ROWS, rows->count);
	placement.cell_x             = columns->positions [placement.column.index];
	placement.cell_y             = rows->positions [placement.row.index];
	placement.cell_w   = span_extent(columns->sizes, placement.column.index, placement.column.span, columns->gap);
	placement.cell_h   = span_extent(rows->sizes, placement.row.index, placement.row.span, rows->gap);
	placement.margin_l = ctx->nodes.layout.margin_l [child];
	placement.margin_t = ctx->nodes.layout.margin_t [child];
	placement.margin_r = ctx->nodes.layout.margin_r [child];
	placement.margin_b = ctx->nodes.layout.margin_b [child];
	return placement;
}

static void
grid_layout_child(XentContext *ctx, XentNodeId child, GridTrackSet const *columns, GridTrackSet const *rows) {
	GridChildPlacement placement = grid_child_placement(ctx, child, columns, rows);
	float              child_w   = grid_content_extent(placement.cell_w, placement.margin_l, placement.margin_r);
	float              child_h   = grid_content_extent(placement.cell_h, placement.margin_t, placement.margin_b);
	float              child_x   = placement.cell_x + placement.margin_l;
	float              child_y   = placement.cell_y + placement.margin_t;
	xent_layout_dispatch_node(&(XentLayoutRequest) {ctx, child, child_w, child_h, child_x, child_y});
}

static void grid_layout_children(
  XentContext *ctx, GridLayoutFrame const *frame, GridTrackSet const *columns, GridTrackSet const *rows
) {
	for (XentNodeId child = ctx->nodes.topology.first_child [frame->node]; child != XENT_NODE_INVALID;
	  child               = ctx->nodes.topology.next_sibling [child])
	{
		grid_layout_child(ctx, child, columns, rows);
	}
}

void xent_layout_node_grid(XentLayoutRequest const *request) {
	XentContext    *ctx   = request->ctx;
	XentNodeId      node  = request->node;
	GridLayoutFrame frame = {0};
	grid_commit_container(request, &frame);

	XentGridDef const *def = ctx->nodes.grid.def [node];
	if (!def) {
		grid_layout_fallback_children(ctx, &frame);
		return;
	}

	GridTrackSet rows    = {0};
	GridTrackSet columns = {0};
	grid_init_tracks(&rows, (GridTrackInit) {def->row_count, def->row_modes, def->row_values, def->row_gap, frame.content_h});
	grid_init_tracks(
	  &columns, (GridTrackInit) {def->col_count, def->col_modes, def->col_values, def->col_gap, frame.content_w}
	);

	grid_resolve_tracks(ctx, node, &columns, GRID_AXIS_COLUMNS);
	grid_resolve_tracks(ctx, node, &rows, GRID_AXIS_ROWS);
	grid_compute_positions(&columns, frame.content_x);
	grid_compute_positions(&rows, frame.content_y);
	grid_layout_children(ctx, &frame, &columns, &rows);
}
