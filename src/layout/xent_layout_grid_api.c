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
	if (!xent_node_valid(ctx, node)) return false;
	XentGridComp *g   = xent_grid_comp_ensure(&ctx->nodes, xent_node_index(node));
	XentGridDef  *def = g ? xent_grid_def_ensure(&g->def) : NULL;
	if (!def) return false;
	return set_grid_tracks(
	  ctx, node, (GridTrackRequest) {modes, values, count},
	  (GridTrackTarget) {&def->row_count, &def->row_modes, &def->row_values}
	);
}

bool xent_grid_setcols(
  XentCtx *ctx, XentNodeId node, XentGridSizeMode const *modes, float const *values, uint32_t count
) {
	if (!xent_node_valid(ctx, node)) return false;
	XentGridComp *g   = xent_grid_comp_ensure(&ctx->nodes, xent_node_index(node));
	XentGridDef  *def = g ? xent_grid_def_ensure(&g->def) : NULL;
	if (!def) return false;
	return set_grid_tracks(
	  ctx, node, (GridTrackRequest) {modes, values, count},
	  (GridTrackTarget) {&def->col_count, &def->col_modes, &def->col_values}
	);
}

bool xent_grid_setrow(XentCtx *ctx, XentNodeId node, uint32_t row) {
	if (!xent_node_valid(ctx, node)) return false;
	uint32_t            i   = xent_node_index(node);
	XentGridComp const *cur = xent_grid_comp_c(&ctx->nodes, i);
	if ((cur ? cur->row : 0u) == ( uint16_t ) row) return true;
	XentGridComp *g = xent_grid_comp_ensure(&ctx->nodes, i);
	if (!g) return false;
	g->row = ( uint16_t ) row;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_grid_setcol(XentCtx *ctx, XentNodeId node, uint32_t column) {
	if (!xent_node_valid(ctx, node)) return false;
	uint32_t            i   = xent_node_index(node);
	XentGridComp const *cur = xent_grid_comp_c(&ctx->nodes, i);
	if ((cur ? cur->column : 0u) == ( uint16_t ) column) return true;
	XentGridComp *g = xent_grid_comp_ensure(&ctx->nodes, i);
	if (!g) return false;
	g->column = ( uint16_t ) column;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_grid_setrowspan(XentCtx *ctx, XentNodeId node, uint32_t span) {
	if (!xent_node_valid(ctx, node) || span == 0) return false;
	uint32_t            i   = xent_node_index(node);
	XentGridComp const *cur = xent_grid_comp_c(&ctx->nodes, i);
	if ((cur ? cur->row_span : 1u) == ( uint16_t ) span) return true;
	XentGridComp *g = xent_grid_comp_ensure(&ctx->nodes, i);
	if (!g) return false;
	g->row_span = ( uint16_t ) span;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_grid_setcolspan(XentCtx *ctx, XentNodeId node, uint32_t span) {
	if (!xent_node_valid(ctx, node) || span == 0) return false;
	uint32_t            i   = xent_node_index(node);
	XentGridComp const *cur = xent_grid_comp_c(&ctx->nodes, i);
	if ((cur ? cur->column_span : 1u) == ( uint16_t ) span) return true;
	XentGridComp *g = xent_grid_comp_ensure(&ctx->nodes, i);
	if (!g) return false;
	g->column_span = ( uint16_t ) span;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

static bool set_grid_axis_gap(XentCtx *ctx, XentNodeId node, float *target, float gap) {
	if (!xent_node_valid(ctx, node) || !target) return false;
	if (xent_f32_same(*target, gap)) return true;
	*target = gap;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_grid_setrowgap(XentCtx *ctx, XentNodeId node, float gap) {
	if (!xent_node_valid(ctx, node)) return false;
	XentGridComp *g   = xent_grid_comp_ensure(&ctx->nodes, xent_node_index(node));
	XentGridDef  *def = g ? xent_grid_def_ensure(&g->def) : NULL;
	return set_grid_axis_gap(ctx, node, def ? &def->row_gap : NULL, gap);
}

bool xent_grid_setcolgap(XentCtx *ctx, XentNodeId node, float gap) {
	if (!xent_node_valid(ctx, node)) return false;
	XentGridComp *g   = xent_grid_comp_ensure(&ctx->nodes, xent_node_index(node));
	XentGridDef  *def = g ? xent_grid_def_ensure(&g->def) : NULL;
	return set_grid_axis_gap(ctx, node, def ? &def->col_gap : NULL, gap);
}

uint32_t xent_grid_row(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return 0;
	XentGridComp const *g = xent_grid_comp_c(&ctx->nodes, xent_node_index(node));
	return g ? g->row : 0u;
}

uint32_t xent_grid_col(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return 0;
	XentGridComp const *g = xent_grid_comp_c(&ctx->nodes, xent_node_index(node));
	return g ? g->column : 0u;
}

uint32_t xent_grid_rowspan(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return 1;
	XentGridComp const *g = xent_grid_comp_c(&ctx->nodes, xent_node_index(node));
	return g ? g->row_span : 1u;
}

uint32_t xent_grid_colspan(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return 1;
	XentGridComp const *g = xent_grid_comp_c(&ctx->nodes, xent_node_index(node));
	return g ? g->column_span : 1u;
}
