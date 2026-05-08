#include "test_common.h"

#define GRID_EPS         0.5f
#define GRID_MAX_TRACKS  4
#define GRID_MAX_CHILD   8

#define GCHK_X 1u
#define GCHK_Y 2u
#define GCHK_W 4u
#define GCHK_H 8u

typedef struct GridChildCase {
	uint32_t row;
	uint32_t col;
	uint32_t row_span;
	uint32_t col_span;
	float    pref_w;
	float    pref_h;
	float    expected_x;
	float    expected_y;
	float    expected_w;
	float    expected_h;
	uint32_t check_mask;
} GridChildCase;

typedef struct GridCase {
	XentSize         size;
	XentInsets       padding;
	float            row_gap;
	float            col_gap;
	uint32_t         row_count;
	XentGridSizeMode row_modes [GRID_MAX_TRACKS];
	float            row_vals  [GRID_MAX_TRACKS];
	uint32_t         col_count;
	XentGridSizeMode col_modes [GRID_MAX_TRACKS];
	float            col_vals  [GRID_MAX_TRACKS];
	uint32_t         child_count;
	GridChildCase    children  [GRID_MAX_CHILD];
} GridCase;

static int has_finite_inset(XentInsets insets) {
	return isfinite(insets.top) || isfinite(insets.right) || isfinite(insets.bottom) || isfinite(insets.left);
}

static XentNodeId grid_make_root(XentContext *ctx, GridCase const *spec) {
	XentNodeId grid = xent_create_node(ctx);
	xent_set_protocol(ctx, grid, XENT_PROTOCOL_GRID);
	xent_set_size(ctx, grid, spec->size);
	if (has_finite_inset(spec->padding)) xent_set_padding(ctx, grid, spec->padding);
	if (spec->row_gap > 0.0f) xent_set_grid_row_gap(ctx, grid, spec->row_gap);
	if (spec->col_gap > 0.0f) xent_set_grid_column_gap(ctx, grid, spec->col_gap);
	if (spec->row_count > 0) {
		xent_set_grid_rows(ctx, grid, (XentGridSizeMode *) spec->row_modes, (float *) spec->row_vals, spec->row_count);
	}
	if (spec->col_count > 0) {
		xent_set_grid_columns(ctx, grid, (XentGridSizeMode *) spec->col_modes, (float *) spec->col_vals, spec->col_count);
	}
	return grid;
}

static XentNodeId grid_make_cell(XentContext *ctx, XentNodeId grid, GridChildCase const *child) {
	XentNodeId node = xent_create_node(ctx);
	xent_set_grid_row(ctx, node, child->row);
	xent_set_grid_column(ctx, node, child->col);
	if (child->row_span > 1) xent_set_grid_row_span(ctx, node, child->row_span);
	if (child->col_span > 1) xent_set_grid_column_span(ctx, node, child->col_span);
	if (isfinite(child->pref_w) || isfinite(child->pref_h)) {
		xent_set_size(ctx, node, (XentSize) {child->pref_w, child->pref_h});
	}
	xent_append_child(ctx, grid, node);
	return node;
}

static int grid_check_rect(XentContext *ctx, XentNodeId node, GridChildCase const *expect) {
	XentRect rect = {0};
	TEST_ASSERT(xent_get_layout_rect(ctx, node, &rect));
	if (expect->check_mask & GCHK_X) TEST_ASSERT(test_float_near(rect.x, expect->expected_x, GRID_EPS));
	if (expect->check_mask & GCHK_Y) TEST_ASSERT(test_float_near(rect.y, expect->expected_y, GRID_EPS));
	if (expect->check_mask & GCHK_W) TEST_ASSERT(test_float_near(rect.width, expect->expected_w, GRID_EPS));
	if (expect->check_mask & GCHK_H) TEST_ASSERT(test_float_near(rect.height, expect->expected_h, GRID_EPS));
	return 0;
}

static int run_grid_case(GridCase const *spec) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId grid                      = grid_make_root(ctx, spec);
	XentNodeId nodes [GRID_MAX_CHILD]    = {XENT_NODE_INVALID};
	for (uint32_t i = 0; i < spec->child_count; ++i) nodes [i] = grid_make_cell(ctx, grid, &spec->children [i]);

	TEST_ASSERT(xent_layout(ctx, grid, spec->size.width, spec->size.height));

	for (uint32_t i = 0; i < spec->child_count; ++i)
		TEST_ASSERT(grid_check_rect(ctx, nodes [i], &spec->children [i]) == 0);

	xent_destroy_context(ctx);
	return 0;
}

static int test_grid_table_cases(void) {
	static GridCase const cases [] = {
	    {
	        .size      = {400.0f, 100.0f},
	        .padding   = {NAN, NAN, NAN, NAN},
	        .row_count = 1,
	        .row_modes = {XENT_GRID_STAR},
	        .row_vals  = {1.0f},
	        .col_count = 2,
	        .col_modes = {XENT_GRID_STAR, XENT_GRID_STAR},
	        .col_vals  = {1.0f, 1.0f},
	        .child_count = 2,
	        .children  = {
	            {.row = 0, .col = 0, .pref_w = NAN, .pref_h = NAN, .expected_x = 0.0f, .expected_w = 200.0f, .expected_h = 100.0f, .check_mask = GCHK_X | GCHK_W | GCHK_H},
	            {.row = 0, .col = 1, .pref_w = NAN, .pref_h = NAN, .expected_x = 200.0f, .expected_w = 200.0f, .check_mask = GCHK_X | GCHK_W},
	        },
	    },
	    {
	        .size      = {400.0f, 100.0f},
	        .padding   = {NAN, NAN, NAN, NAN},
	        .row_count = 1,
	        .row_modes = {XENT_GRID_STAR},
	        .row_vals  = {1.0f},
	        .col_count = 2,
	        .col_modes = {XENT_GRID_PIXEL, XENT_GRID_STAR},
	        .col_vals  = {100.0f, 1.0f},
	        .child_count = 2,
	        .children  = {
	            {.row = 0, .col = 0, .pref_w = NAN, .pref_h = NAN, .expected_x = 0.0f, .expected_w = 100.0f, .check_mask = GCHK_X | GCHK_W},
	            {.row = 0, .col = 1, .pref_w = NAN, .pref_h = NAN, .expected_x = 100.0f, .expected_w = 300.0f, .check_mask = GCHK_X | GCHK_W},
	        },
	    },
	    {
	        .size      = {400.0f, 100.0f},
	        .padding   = {NAN, NAN, NAN, NAN},
	        .row_count = 1,
	        .row_modes = {XENT_GRID_STAR},
	        .row_vals  = {1.0f},
	        .col_count = 2,
	        .col_modes = {XENT_GRID_AUTO, XENT_GRID_STAR},
	        .col_vals  = {0.0f, 1.0f},
	        .child_count = 2,
	        .children  = {
	            {.row = 0, .col = 0, .pref_w = 80.0f, .pref_h = 30.0f, .expected_w = 80.0f, .check_mask = GCHK_W},
	            {.row = 0, .col = 1, .pref_w = NAN, .pref_h = NAN, .expected_x = 80.0f, .expected_w = 320.0f, .check_mask = GCHK_X | GCHK_W},
	        },
	    },
	    {
	        .size      = {400.0f, 200.0f},
	        .padding   = {NAN, NAN, NAN, NAN},
	        .row_gap   = 10.0f,
	        .col_gap   = 20.0f,
	        .row_count = 2,
	        .row_modes = {XENT_GRID_STAR, XENT_GRID_STAR},
	        .row_vals  = {1.0f, 1.0f},
	        .col_count = 2,
	        .col_modes = {XENT_GRID_STAR, XENT_GRID_STAR},
	        .col_vals  = {1.0f, 1.0f},
	        .child_count = 4,
	        .children  = {
	            {.row = 0, .col = 0, .pref_w = NAN, .pref_h = NAN, .expected_w = 190.0f, .expected_h = 95.0f, .check_mask = GCHK_W | GCHK_H},
	            {.row = 0, .col = 1, .pref_w = NAN, .pref_h = NAN, .expected_x = 210.0f, .check_mask = GCHK_X},
	            {.row = 1, .col = 0, .pref_w = NAN, .pref_h = NAN, .expected_y = 105.0f, .check_mask = GCHK_Y},
	            {.row = 1, .col = 1, .pref_w = NAN, .pref_h = NAN, .expected_x = 210.0f, .expected_y = 105.0f, .check_mask = GCHK_X | GCHK_Y},
	        },
	    },
	    {
	        .size      = {300.0f, 100.0f},
	        .padding   = {NAN, NAN, NAN, NAN},
	        .row_count = 1,
	        .row_modes = {XENT_GRID_STAR},
	        .row_vals  = {1.0f},
	        .col_count = 3,
	        .col_modes = {XENT_GRID_PIXEL, XENT_GRID_PIXEL, XENT_GRID_PIXEL},
	        .col_vals  = {100.0f, 100.0f, 100.0f},
	        .child_count = 2,
	        .children  = {
	            {.row = 0, .col = 0, .col_span = 2, .pref_w = NAN, .pref_h = NAN, .expected_x = 0.0f, .expected_w = 200.0f, .check_mask = GCHK_X | GCHK_W},
	            {.row = 0, .col = 2, .pref_w = NAN, .pref_h = NAN, .expected_x = 200.0f, .expected_w = 100.0f, .check_mask = GCHK_X | GCHK_W},
	        },
	    },
	    {
	        .size      = {200.0f, 300.0f},
	        .padding   = {NAN, NAN, NAN, NAN},
	        .row_count = 3,
	        .row_modes = {XENT_GRID_PIXEL, XENT_GRID_PIXEL, XENT_GRID_PIXEL},
	        .row_vals  = {100.0f, 100.0f, 100.0f},
	        .col_count = 1,
	        .col_modes = {XENT_GRID_STAR},
	        .col_vals  = {1.0f},
	        .child_count = 2,
	        .children  = {
	            {.row = 0, .col = 0, .row_span = 2, .pref_w = NAN, .pref_h = NAN, .expected_y = 0.0f, .expected_w = 200.0f, .expected_h = 200.0f, .check_mask = GCHK_Y | GCHK_W | GCHK_H},
	            {.row = 2, .col = 0, .pref_w = NAN, .pref_h = NAN, .expected_y = 200.0f, .expected_w = 200.0f, .expected_h = 100.0f, .check_mask = GCHK_Y | GCHK_W | GCHK_H},
	        },
	    },
	    {
	        .size      = {300.0f, 100.0f},
	        .padding   = {NAN, NAN, NAN, NAN},
	        .row_count = 1,
	        .row_modes = {XENT_GRID_STAR},
	        .row_vals  = {1.0f},
	        .col_count = 2,
	        .col_modes = {XENT_GRID_STAR, XENT_GRID_STAR},
	        .col_vals  = {2.0f, 1.0f},
	        .child_count = 2,
	        .children  = {
	            {.row = 0, .col = 0, .pref_w = NAN, .pref_h = NAN, .expected_x = 0.0f, .expected_w = 200.0f, .check_mask = GCHK_X | GCHK_W},
	            {.row = 0, .col = 1, .pref_w = NAN, .pref_h = NAN, .expected_x = 200.0f, .expected_w = 100.0f, .check_mask = GCHK_X | GCHK_W},
	        },
	    },
	    {
	        .size      = {400.0f, 200.0f},
	        .padding   = {10.0f, 10.0f, 10.0f, 10.0f},
	        .row_count = 1,
	        .row_modes = {XENT_GRID_STAR},
	        .row_vals  = {1.0f},
	        .col_count = 1,
	        .col_modes = {XENT_GRID_STAR},
	        .col_vals  = {1.0f},
	        .child_count = 1,
	        .children  = {
	            {.row = 0, .col = 0, .pref_w = NAN, .pref_h = NAN, .expected_x = 10.0f, .expected_y = 10.0f, .expected_w = 380.0f, .expected_h = 180.0f, .check_mask = GCHK_X | GCHK_Y | GCHK_W | GCHK_H},
	        },
	    },
	    {
	        .size      = {200.0f, 150.0f},
	        .padding   = {NAN, NAN, NAN, NAN},
	        .child_count = 1,
	        .children  = {
	            {.row = 0, .col = 0, .pref_w = NAN, .pref_h = NAN, .expected_w = 200.0f, .expected_h = 150.0f, .check_mask = GCHK_W | GCHK_H},
	        },
	    },
	    {
	        .size      = {340.0f, 100.0f},
	        .padding   = {NAN, NAN, NAN, NAN},
	        .col_gap   = 20.0f,
	        .row_count = 1,
	        .row_modes = {XENT_GRID_STAR},
	        .row_vals  = {1.0f},
	        .col_count = 3,
	        .col_modes = {XENT_GRID_PIXEL, XENT_GRID_PIXEL, XENT_GRID_PIXEL},
	        .col_vals  = {100.0f, 100.0f, 100.0f},
	        .child_count = 2,
	        .children  = {
	            {.row = 0, .col = 0, .col_span = 2, .pref_w = NAN, .pref_h = NAN, .expected_x = 0.0f, .expected_w = 220.0f, .check_mask = GCHK_X | GCHK_W},
	            {.row = 0, .col = 2, .pref_w = NAN, .pref_h = NAN, .expected_x = 240.0f, .expected_w = 100.0f, .check_mask = GCHK_X | GCHK_W},
	        },
	    },
	    {
	        .size      = {300.0f, 32.0f},
	        .padding   = {NAN, NAN, NAN, NAN},
	        .row_count = 1,
	        .row_modes = {XENT_GRID_STAR},
	        .row_vals  = {1.0f},
	        .col_count = 2,
	        .col_modes = {XENT_GRID_STAR, XENT_GRID_PIXEL},
	        .col_vals  = {1.0f, 30.0f},
	        .child_count = 2,
	        .children  = {
	            {.row = 0, .col = 0, .pref_w = NAN, .pref_h = NAN, .expected_w = 270.0f, .check_mask = GCHK_W},
	            {.row = 0, .col = 1, .pref_w = NAN, .pref_h = NAN, .expected_x = 270.0f, .expected_w = 30.0f, .check_mask = GCHK_X | GCHK_W},
	        },
	    },
	};

	for (size_t i = 0; i < sizeof(cases) / sizeof(cases [0]); ++i)
		TEST_ASSERT(run_grid_case(&cases [i]) == 0);

	return 0;
}

static int test_grid_inside_flex(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId flex_root = xent_create_node(ctx);
	XentNodeId child_a   = xent_create_node(ctx);
	XentNodeId grid_b    = xent_create_node(ctx);

	xent_set_protocol(ctx, flex_root, XENT_PROTOCOL_FLEX);
	xent_set_flex_direction(ctx, flex_root, XENT_FLEX_ROW);
	xent_set_size(ctx, flex_root, (XentSize) {600.0f, 100.0f});

	xent_set_size(ctx, child_a, (XentSize) {200.0f, 100.0f});
	xent_set_flex_shrink(ctx, child_a, 0.0f);

	GridCase const grid_spec = {
	    .size      = {NAN, 100.0f},
	    .padding   = {NAN, NAN, NAN, NAN},
	    .row_count = 1,
	    .row_modes = {XENT_GRID_STAR},
	    .row_vals  = {1.0f},
	    .col_count = 2,
	    .col_modes = {XENT_GRID_STAR, XENT_GRID_PIXEL},
	    .col_vals  = {1.0f, 100.0f},
	};
	xent_set_protocol(ctx, grid_b, XENT_PROTOCOL_GRID);
	xent_set_size(ctx, grid_b, grid_spec.size);
	xent_set_flex_grow(ctx, grid_b, 1.0f);
	xent_set_grid_rows(
	  ctx, grid_b, (XentGridSizeMode *) grid_spec.row_modes, (float *) grid_spec.row_vals, grid_spec.row_count
	);
	xent_set_grid_columns(
	  ctx, grid_b, (XentGridSizeMode *) grid_spec.col_modes, (float *) grid_spec.col_vals, grid_spec.col_count
	);

	XentNodeId gc0 = xent_create_node(ctx);
	XentNodeId gc1 = xent_create_node(ctx);
	xent_set_grid_row(ctx, gc0, 0);
	xent_set_grid_column(ctx, gc0, 0);
	xent_set_grid_row(ctx, gc1, 0);
	xent_set_grid_column(ctx, gc1, 1);
	xent_append_child(ctx, grid_b, gc0);
	xent_append_child(ctx, grid_b, gc1);

	xent_append_child(ctx, flex_root, child_a);
	xent_append_child(ctx, flex_root, grid_b);

	TEST_ASSERT(xent_layout(ctx, flex_root, 600.0f, 100.0f));

	XentRect ra = {0}, rb = {0}, rg0 = {0}, rg1 = {0};
	TEST_ASSERT(xent_get_layout_rect(ctx, child_a, &ra));
	TEST_ASSERT(xent_get_layout_rect(ctx, grid_b, &rb));
	TEST_ASSERT(xent_get_layout_rect(ctx, gc0, &rg0));
	TEST_ASSERT(xent_get_layout_rect(ctx, gc1, &rg1));
	TEST_ASSERT(test_float_near(ra.width, 200.0f, GRID_EPS));
	TEST_ASSERT(test_float_near(rb.width, 400.0f, GRID_EPS));
	TEST_ASSERT(test_float_near(rb.x, 200.0f, GRID_EPS));
	TEST_ASSERT(test_float_near(rg0.width, 300.0f, GRID_EPS));
	TEST_ASSERT(test_float_near(rg1.width, 100.0f, GRID_EPS));

	xent_destroy_context(ctx);
	return 0;
}

int main(void) {
	XentTestFn const tests[] = {
	    test_grid_table_cases,
	    test_grid_inside_flex,
	};

	return test_run_all(tests, sizeof(tests) / sizeof(tests [0]));
}
