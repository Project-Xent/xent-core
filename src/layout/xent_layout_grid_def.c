#include "xent_layout_grid_def.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

void xent_grid_def_free(XentGridDef *def) {
	if (!def) return;
	free(def->row_modes);
	free(def->row_values);
	free(def->col_modes);
	free(def->col_values);
	free(def);
}

XentGridDef *xent_grid_def_ensure(XentGridDef **slot) {
	if (!*slot) *slot = ( XentGridDef * ) calloc(1, sizeof(XentGridDef));
	return *slot;
}

static bool dup_tracks(uint8_t **modes, float **values, uint32_t count, uint8_t const *sm, float const *sv) {
	*modes  = NULL;
	*values = NULL;
	if (count == 0u) return true;
	*modes  = ( uint8_t * ) malloc(sizeof(**modes) * ( size_t ) count);
	*values = ( float * ) malloc(sizeof(**values) * ( size_t ) count);
	if (!*modes || !*values) {
		free(*modes);
		free(*values);
		*modes  = NULL;
		*values = NULL;
		return false;
	}
	memcpy(*modes, sm, sizeof(**modes) * ( size_t ) count);
	memcpy(*values, sv, sizeof(**values) * ( size_t ) count);
	return true;
}

XentGridDef *xent_grid_def_copy(XentGridDef const *src) {
	if (!src) return NULL;
	XentGridDef *dst = ( XentGridDef * ) calloc(1, sizeof(*dst));
	if (!dst) return NULL;
	dst->row_count = src->row_count;
	dst->col_count = src->col_count;
	dst->row_gap   = src->row_gap;
	dst->col_gap   = src->col_gap;
	if (!dup_tracks(&dst->row_modes, &dst->row_values, src->row_count, src->row_modes, src->row_values)
		|| !dup_tracks(&dst->col_modes, &dst->col_values, src->col_count, src->col_modes, src->col_values))
	{
		xent_grid_def_free(dst);
		return NULL;
	}
	return dst;
}
