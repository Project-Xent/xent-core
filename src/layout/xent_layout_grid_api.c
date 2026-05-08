#include "../xent_internal.h"

typedef struct GridTrackRequest {
	XentGridSizeMode const *modes;
	float const            *values;
	uint32_t                count;
} GridTrackRequest;

typedef struct GridTrackTarget {
	uint8_t *count;
	uint8_t *modes;
	float   *values;
} GridTrackTarget;

static XentGridDef *xent_ensure_grid_def(XentContext *ctx, XentNodeId node) {
	if (!ctx->nodes.grid.def [node]) {
		XentGridDef *def = ( XentGridDef * ) calloc(1, sizeof(XentGridDef));
		if (!def) return NULL;
		ctx->nodes.grid.def [node] = def;
	}
	return ctx->nodes.grid.def [node];
}

static bool xent_set_grid_tracks(
  XentContext *ctx, XentNodeId node, GridTrackRequest request, GridTrackTarget target
) {
	if (!xent_is_valid_node(ctx, node) || request.count > XENT_GRID_MAX_TRACKS) return false;
	*target.count = ( uint8_t ) request.count;
	for (uint32_t i = 0; i < request.count; ++i) {
		target.modes [i]  = ( uint8_t ) request.modes [i];
		target.values [i] = request.values [i];
	}
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_set_grid_rows(
  XentContext *ctx, XentNodeId node, XentGridSizeMode const *modes, float const *values, uint32_t count
) {
	XentGridDef *def = xent_ensure_grid_def(ctx, node);
	if (!def) return false;
	return xent_set_grid_tracks(
	  ctx, node, (GridTrackRequest) {modes, values, count}, (GridTrackTarget) {&def->row_count, def->row_modes, def->row_values}
	);
}

bool xent_set_grid_columns(
  XentContext *ctx, XentNodeId node, XentGridSizeMode const *modes, float const *values, uint32_t count
) {
	XentGridDef *def = xent_ensure_grid_def(ctx, node);
	if (!def) return false;
	return xent_set_grid_tracks(
	  ctx, node, (GridTrackRequest) {modes, values, count}, (GridTrackTarget) {&def->col_count, def->col_modes, def->col_values}
	);
}

bool xent_set_grid_row(XentContext *ctx, XentNodeId node, uint32_t row) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.grid.row [node] = ( uint16_t ) row;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_set_grid_column(XentContext *ctx, XentNodeId node, uint32_t column) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.grid.column [node] = ( uint16_t ) column;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_set_grid_row_span(XentContext *ctx, XentNodeId node, uint32_t span) {
	if (!xent_is_valid_node(ctx, node) || span == 0) return false;
	ctx->nodes.grid.row_span [node] = ( uint16_t ) span;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_set_grid_column_span(XentContext *ctx, XentNodeId node, uint32_t span) {
	if (!xent_is_valid_node(ctx, node) || span == 0) return false;
	ctx->nodes.grid.column_span [node] = ( uint16_t ) span;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

static bool xent_set_grid_axis_gap(XentContext *ctx, XentNodeId node, float *target, float gap) {
	if (!xent_is_valid_node(ctx, node) || !target) return false;
	*target = gap;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_set_grid_row_gap(XentContext *ctx, XentNodeId node, float gap) {
	XentGridDef *def = xent_ensure_grid_def(ctx, node);
	return xent_set_grid_axis_gap(ctx, node, def ? &def->row_gap : NULL, gap);
}

bool xent_set_grid_column_gap(XentContext *ctx, XentNodeId node, float gap) {
	XentGridDef *def = xent_ensure_grid_def(ctx, node);
	return xent_set_grid_axis_gap(ctx, node, def ? &def->col_gap : NULL, gap);
}

uint32_t xent_get_grid_row(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return 0;
	return ctx->nodes.grid.row [node];
}

uint32_t xent_get_grid_column(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return 0;
	return ctx->nodes.grid.column [node];
}

uint32_t xent_get_grid_row_span(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return 1;
	return ctx->nodes.grid.row_span [node];
}

uint32_t xent_get_grid_column_span(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return 1;
	return ctx->nodes.grid.column_span [node];
}
