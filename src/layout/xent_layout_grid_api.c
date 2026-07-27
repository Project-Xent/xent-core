#include "../xent_internal.h"

typedef struct GridTrackRequest {
	XentGridSizeMode const *modes;
	float const            *values;
	uint32_t                count;
} GridTrackRequest;

typedef struct GridTrackTarget {
	uint32_t *count;
	uint8_t **modes;
	float   **values;
} GridTrackTarget;

static bool set_grid_tracks(XentCtx *ctx, XentNodeId node, GridTrackRequest request, GridTrackTarget target) {
	if (!xent_node_valid(ctx, node)
		|| request.count > UINT16_MAX
		|| (request.count > 0u && (!request.modes || !request.values)))
	{
		return false;
	}

	uint8_t *modes  = NULL;
	float   *values = NULL;
	if (request.count > 0u) {
		modes  = ( uint8_t * ) malloc(sizeof(*modes) * ( size_t ) request.count);
		values = ( float * ) malloc(sizeof(*values) * ( size_t ) request.count);
		if (!modes || !values) {
			free(modes);
			free(values);
			return false;
		}
	}

	for (uint32_t i = 0; i < request.count; ++i) {
		modes [i]  = ( uint8_t ) request.modes [i];
		values [i] = request.values [i];
	}

	free(*target.modes);
	free(*target.values);
	*target.modes  = modes;
	*target.values = values;
	*target.count  = request.count;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_grid_setrows(
  XentCtx *ctx, XentNodeId node, XentGridSizeMode const *modes, float const *values, uint32_t count
) {
	XentGridDef *def = xent_grid_def_ensure(&ctx->nodes.grid.def [xent_node_index(node)]);
	if (!def) return false;
	return set_grid_tracks(
	  ctx, node, (GridTrackRequest) {modes, values, count},
	  (GridTrackTarget) {&def->row_count, &def->row_modes, &def->row_values}
	);
}

bool xent_grid_setcols(
  XentCtx *ctx, XentNodeId node, XentGridSizeMode const *modes, float const *values, uint32_t count
) {
	XentGridDef *def = xent_grid_def_ensure(&ctx->nodes.grid.def [xent_node_index(node)]);
	if (!def) return false;
	return set_grid_tracks(
	  ctx, node, (GridTrackRequest) {modes, values, count},
	  (GridTrackTarget) {&def->col_count, &def->col_modes, &def->col_values}
	);
}

bool xent_grid_setrow(XentCtx *ctx, XentNodeId node, uint32_t row) {
	if (!xent_node_valid(ctx, node)) return false;
	ctx->nodes.grid.row [xent_node_index(node)] = ( uint16_t ) row;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_grid_setcol(XentCtx *ctx, XentNodeId node, uint32_t column) {
	if (!xent_node_valid(ctx, node)) return false;
	ctx->nodes.grid.column [xent_node_index(node)] = ( uint16_t ) column;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_grid_setrowspan(XentCtx *ctx, XentNodeId node, uint32_t span) {
	if (!xent_node_valid(ctx, node) || span == 0) return false;
	ctx->nodes.grid.row_span [xent_node_index(node)] = ( uint16_t ) span;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_grid_setcolspan(XentCtx *ctx, XentNodeId node, uint32_t span) {
	if (!xent_node_valid(ctx, node) || span == 0) return false;
	ctx->nodes.grid.column_span [xent_node_index(node)] = ( uint16_t ) span;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

static bool set_grid_axis_gap(XentCtx *ctx, XentNodeId node, float *target, float gap) {
	if (!xent_node_valid(ctx, node) || !target) return false;
	*target = gap;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_grid_setrowgap(XentCtx *ctx, XentNodeId node, float gap) {
	XentGridDef *def = xent_grid_def_ensure(&ctx->nodes.grid.def [xent_node_index(node)]);
	return set_grid_axis_gap(ctx, node, def ? &def->row_gap : NULL, gap);
}

bool xent_grid_setcolgap(XentCtx *ctx, XentNodeId node, float gap) {
	XentGridDef *def = xent_grid_def_ensure(&ctx->nodes.grid.def [xent_node_index(node)]);
	return set_grid_axis_gap(ctx, node, def ? &def->col_gap : NULL, gap);
}

uint32_t xent_grid_row(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return 0;
	return ctx->nodes.grid.row [xent_node_index(node)];
}

uint32_t xent_grid_col(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return 0;
	return ctx->nodes.grid.column [xent_node_index(node)];
}

uint32_t xent_grid_rowspan(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return 1;
	return ctx->nodes.grid.row_span [xent_node_index(node)];
}

uint32_t xent_grid_colspan(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return 1;
	return ctx->nodes.grid.column_span [xent_node_index(node)];
}
