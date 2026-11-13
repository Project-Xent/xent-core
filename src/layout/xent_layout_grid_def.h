#ifndef XENT_LAYOUT_GRID_DEF_H
#define XENT_LAYOUT_GRID_DEF_H

#include <stdint.h>

typedef struct XentGridDef {
	uint32_t row_count;
	uint32_t col_count;
	float    row_gap;
	float    col_gap;
	uint8_t *row_modes;
	float   *row_values;
	uint8_t *col_modes;
	float   *col_values;
} XentGridDef;

void         xent_grid_def_free(XentGridDef *def);

XentGridDef *xent_grid_def_copy(XentGridDef const *src);

XentGridDef *xent_grid_def_ensure(XentGridDef **slot);

#endif
