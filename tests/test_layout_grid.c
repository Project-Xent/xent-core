#include "test_common.h"

static int test_basic_grid(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId grid  = xent_create_node(ctx);
	XentNodeId left  = xent_create_node(ctx);
	XentNodeId right = xent_create_node(ctx);

	xent_set_protocol(ctx, grid, XENT_PROTOCOL_GRID);
	xent_set_size(ctx, grid, (XentSize) {400.0f, 100.0f});

	XentGridSizeMode row_modes [] = {XENT_GRID_STAR};
	float            row_vals []  = {1.0f};
	xent_set_grid_rows(ctx, grid, row_modes, row_vals, 1);

	XentGridSizeMode col_modes [] = {XENT_GRID_PIXEL, XENT_GRID_STAR};
	float            col_vals []  = {120.0f, 1.0f};
	xent_set_grid_columns(ctx, grid, col_modes, col_vals, 2);

	xent_set_grid_row(ctx, left, 0);
	xent_set_grid_column(ctx, left, 0);
	xent_set_grid_row(ctx, right, 0);
	xent_set_grid_column(ctx, right, 1);

	TEST_ASSERT(xent_append_child(ctx, grid, left));
	TEST_ASSERT(xent_append_child(ctx, grid, right));
	TEST_ASSERT(xent_layout(ctx, grid, 400.0f, 100.0f));

	XentRect left_rect  = {0};
	XentRect right_rect = {0};
	TEST_ASSERT(xent_get_layout_rect(ctx, left, &left_rect));
	TEST_ASSERT(xent_get_layout_rect(ctx, right, &right_rect));

	TEST_ASSERT(test_float_near(left_rect.width, 120.0f, 0.5f));
	TEST_ASSERT(test_float_near(right_rect.width, 280.0f, 0.5f));
	TEST_ASSERT(test_float_near(right_rect.x, 120.0f, 0.5f));

	xent_destroy_context(ctx);
	return 0;
}

static int test_grid_auto_span_contributes_to_tracks(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId grid = xent_create_node(ctx);
	XentNodeId span = xent_create_node(ctx);
	XentNodeId tail = xent_create_node(ctx);
	TEST_ASSERT(xent_set_protocol(ctx, grid, XENT_PROTOCOL_GRID));
	TEST_ASSERT(xent_set_size(ctx, grid, (XentSize) {300.0f, 50.0f}));

	XentGridSizeMode row_modes [] = {XENT_GRID_STAR};
	float            row_vals []  = {1.0f};
	XentGridSizeMode col_modes [] = {XENT_GRID_AUTO, XENT_GRID_AUTO, XENT_GRID_STAR};
	float            col_vals []  = {0.0f, 0.0f, 1.0f};
	TEST_ASSERT(xent_set_grid_rows(ctx, grid, row_modes, row_vals, 1));
	TEST_ASSERT(xent_set_grid_columns(ctx, grid, col_modes, col_vals, 3));

	TEST_ASSERT(xent_set_grid_column(ctx, span, 0));
	TEST_ASSERT(xent_set_grid_column_span(ctx, span, 2));
	TEST_ASSERT(xent_set_size(ctx, span, (XentSize) {120.0f, 20.0f}));
	TEST_ASSERT(xent_set_grid_column(ctx, tail, 2));
	TEST_ASSERT(xent_append_child(ctx, grid, span));
	TEST_ASSERT(xent_append_child(ctx, grid, tail));
	TEST_ASSERT(xent_layout(ctx, grid, 300.0f, 50.0f));

	XentRect span_rect = {0};
	XentRect tail_rect = {0};
	TEST_ASSERT(xent_get_layout_rect(ctx, span, &span_rect));
	TEST_ASSERT(xent_get_layout_rect(ctx, tail, &tail_rect));
	TEST_ASSERT(test_float_near(span_rect.width, 120.0f, 0.5f));
	TEST_ASSERT(test_float_near(tail_rect.x, 120.0f, 0.5f));
	TEST_ASSERT(test_float_near(tail_rect.width, 180.0f, 0.5f));

	xent_destroy_context(ctx);
	return 0;
}

static int test_grid_dirty_root_promotion(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root    = xent_create_node(ctx);
	XentNodeId grid    = xent_create_node(ctx);
	XentNodeId child   = xent_create_node(ctx);
	XentNodeId sibling = xent_create_node(ctx);
	TEST_ASSERT(xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX));
	TEST_ASSERT(xent_set_size(ctx, root, (XentSize) {300.0f, 100.0f}));
	TEST_ASSERT(xent_set_protocol(ctx, grid, XENT_PROTOCOL_GRID));
	TEST_ASSERT(xent_set_size(ctx, grid, (XentSize) {NAN, NAN}));
	TEST_ASSERT(xent_set_size(ctx, child, (XentSize) {50.0f, 20.0f}));
	TEST_ASSERT(xent_set_size(ctx, sibling, (XentSize) {30.0f, 20.0f}));
	TEST_ASSERT(xent_append_child(ctx, root, grid));
	TEST_ASSERT(xent_append_child(ctx, grid, child));
	TEST_ASSERT(xent_append_child(ctx, root, sibling));
	TEST_ASSERT(xent_layout(ctx, root, 300.0f, 100.0f));

	TEST_ASSERT(xent_set_size(ctx, child, (XentSize) {60.0f, 20.0f}));
	TEST_ASSERT(xent_layout(ctx, root, 300.0f, 100.0f));
	TEST_ASSERT(xent_get_last_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_DIRTY_SUBTREE);

	xent_destroy_context(ctx);
	return 0;
}

static int test_percent_size_and_aspect_ratio(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root  = xent_create_node(ctx);
	XentNodeId child = xent_create_node(ctx);
	TEST_ASSERT(xent_set_size(ctx, root, (XentSize) {400.0f, 200.0f}));
	TEST_ASSERT(xent_set_width_percent(ctx, child, 0.5f));
	TEST_ASSERT(xent_set_aspect_ratio(ctx, child, 2.0f));
	TEST_ASSERT(xent_append_child(ctx, root, child));
	TEST_ASSERT(xent_layout(ctx, root, 400.0f, 200.0f));

	XentRect rect = {0};
	TEST_ASSERT(xent_get_layout_rect(ctx, child, &rect));
	TEST_ASSERT(test_float_near(rect.width, 200.0f, 0.5f));
	TEST_ASSERT(test_float_near(rect.height, 100.0f, 0.5f));

	xent_destroy_context(ctx);
	return 0;
}

int main(void) {
	XentTestFn const tests [] = {
	  test_basic_grid,
	  test_grid_auto_span_contributes_to_tracks,
	  test_grid_dirty_root_promotion,
	  test_percent_size_and_aspect_ratio,
	};
	return test_run_all(tests, sizeof(tests) / sizeof(tests [0]));
}
