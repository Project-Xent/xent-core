#include "test_common.h"

static int test_basic_grid(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId grid  = xent_node_create(ctx);
	XentNodeId left  = xent_node_create(ctx);
	XentNodeId right = xent_node_create(ctx);

	xent_setproto(ctx, grid, XENT_PROTOCOL_GRID);
	xent_setsize(ctx, grid, (XentSize) {400.0f, 100.0f});

	XentGridSizeMode row_modes [] = {XENT_GRID_STAR};
	float            row_vals []  = {1.0f};
	xent_grid_setrows(ctx, grid, row_modes, row_vals, 1);

	XentGridSizeMode col_modes [] = {XENT_GRID_PIXEL, XENT_GRID_STAR};
	float            col_vals []  = {120.0f, 1.0f};
	xent_grid_setcols(ctx, grid, col_modes, col_vals, 2);

	xent_grid_setrow(ctx, left, 0);
	xent_grid_setcol(ctx, left, 0);
	xent_grid_setrow(ctx, right, 0);
	xent_grid_setcol(ctx, right, 1);

	TEST_ASSERT(xent_node_append(ctx, grid, left));
	TEST_ASSERT(xent_node_append(ctx, grid, right));
	TEST_ASSERT(xent_layout(ctx, grid, 400.0f, 100.0f));

	XentRect left_rect  = {0};
	XentRect right_rect = {0};
	TEST_ASSERT(xent_layout_rect(ctx, left, &left_rect));
	TEST_ASSERT(xent_layout_rect(ctx, right, &right_rect));

	TEST_ASSERT(test_float_near(left_rect.w, 120.0f, 0.5f));
	TEST_ASSERT(test_float_near(right_rect.w, 280.0f, 0.5f));
	TEST_ASSERT(test_float_near(right_rect.x, 120.0f, 0.5f));

	xent_ctx_destroy(ctx);
	return 0;
}

static int test_grid_auto_span_contributes_to_tracks(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId grid = xent_node_create(ctx);
	XentNodeId span = xent_node_create(ctx);
	XentNodeId tail = xent_node_create(ctx);
	TEST_ASSERT(xent_setproto(ctx, grid, XENT_PROTOCOL_GRID));
	TEST_ASSERT(xent_setsize(ctx, grid, (XentSize) {300.0f, 50.0f}));

	XentGridSizeMode row_modes [] = {XENT_GRID_STAR};
	float            row_vals []  = {1.0f};
	XentGridSizeMode col_modes [] = {XENT_GRID_AUTO, XENT_GRID_AUTO, XENT_GRID_STAR};
	float            col_vals []  = {0.0f, 0.0f, 1.0f};
	TEST_ASSERT(xent_grid_setrows(ctx, grid, row_modes, row_vals, 1));
	TEST_ASSERT(xent_grid_setcols(ctx, grid, col_modes, col_vals, 3));

	TEST_ASSERT(xent_grid_setcol(ctx, span, 0));
	TEST_ASSERT(xent_grid_setcolspan(ctx, span, 2));
	TEST_ASSERT(xent_setsize(ctx, span, (XentSize) {120.0f, 20.0f}));
	TEST_ASSERT(xent_grid_setcol(ctx, tail, 2));
	TEST_ASSERT(xent_node_append(ctx, grid, span));
	TEST_ASSERT(xent_node_append(ctx, grid, tail));
	TEST_ASSERT(xent_layout(ctx, grid, 300.0f, 50.0f));

	XentRect span_rect = {0};
	XentRect tail_rect = {0};
	TEST_ASSERT(xent_layout_rect(ctx, span, &span_rect));
	TEST_ASSERT(xent_layout_rect(ctx, tail, &tail_rect));
	TEST_ASSERT(test_float_near(span_rect.w, 120.0f, 0.5f));
	TEST_ASSERT(test_float_near(tail_rect.x, 120.0f, 0.5f));
	TEST_ASSERT(test_float_near(tail_rect.w, 180.0f, 0.5f));

	xent_ctx_destroy(ctx);
	return 0;
}

static int test_grid_dirty_root_promotion(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root    = xent_node_create(ctx);
	XentNodeId grid    = xent_node_create(ctx);
	XentNodeId child   = xent_node_create(ctx);
	XentNodeId sibling = xent_node_create(ctx);
	TEST_ASSERT(xent_setproto(ctx, root, XENT_PROTOCOL_FLEX));
	TEST_ASSERT(xent_setflexdir(ctx, root, XENT_FLEX_ROW));
	TEST_ASSERT(xent_setsize(ctx, root, (XentSize) {300.0f, 100.0f}));
	TEST_ASSERT(xent_setproto(ctx, grid, XENT_PROTOCOL_GRID));
	TEST_ASSERT(xent_setsize(ctx, grid, (XentSize) {NAN, NAN}));
	TEST_ASSERT(xent_setsize(ctx, child, (XentSize) {50.0f, 20.0f}));
	TEST_ASSERT(xent_setsize(ctx, sibling, (XentSize) {30.0f, 20.0f}));
	TEST_ASSERT(xent_node_append(ctx, root, grid));
	TEST_ASSERT(xent_node_append(ctx, grid, child));
	TEST_ASSERT(xent_node_append(ctx, root, sibling));
	TEST_ASSERT(xent_layout(ctx, root, 300.0f, 100.0f));

	TEST_ASSERT(xent_setsize(ctx, child, (XentSize) {60.0f, 20.0f}));
	TEST_ASSERT(xent_layout(ctx, root, 300.0f, 100.0f));
	XentRect child_rect = {0};
	TEST_ASSERT(xent_layout_rect(ctx, child, &child_rect));
	TEST_ASSERT(test_float_near(child_rect.w, 60.0f, 0.5f));
	TEST_ASSERT(xent_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_FULL);

	xent_ctx_destroy(ctx);
	return 0;
}

static int test_percent_size_and_aspect_ratio(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root  = xent_node_create(ctx);
	XentNodeId child = xent_node_create(ctx);
	TEST_ASSERT(xent_setsize(ctx, root, (XentSize) {400.0f, 200.0f}));
	TEST_ASSERT(xent_setwpct(ctx, child, 0.5f));
	TEST_ASSERT(xent_setaspect(ctx, child, 2.0f));
	TEST_ASSERT(xent_node_append(ctx, root, child));
	TEST_ASSERT(xent_layout(ctx, root, 400.0f, 200.0f));

	XentRect rect = {0};
	TEST_ASSERT(xent_layout_rect(ctx, child, &rect));
	TEST_ASSERT(test_float_near(rect.w, 200.0f, 0.5f));
	TEST_ASSERT(test_float_near(rect.h, 100.0f, 0.5f));

	xent_ctx_destroy(ctx);
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
