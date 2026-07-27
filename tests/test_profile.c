#include "test_common.h"

static int test_profile_tracks_flex_baseline_fallback(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root  = xent_node_create(ctx);
	XentNodeId small = xent_node_create(ctx);
	XentNodeId large = xent_node_create(ctx);

	TEST_ASSERT(xent_setproto(ctx, root, XENT_PROTOCOL_FLEX));
	TEST_ASSERT(xent_setflexdir(ctx, root, XENT_FLEX_ROW));
	TEST_ASSERT(xent_setitems(ctx, root, XENT_FLEX_ALIGN_BASELINE));
	TEST_ASSERT(xent_setsize(ctx, root, (XentSize) {300.0f, 100.0f}));
	TEST_ASSERT(xent_settext(ctx, small, "small"));
	TEST_ASSERT(xent_settext(ctx, large, "large"));
	TEST_ASSERT(xent_node_append(ctx, root, small));
	TEST_ASSERT(xent_node_append(ctx, root, large));

	xent_profile_reset(ctx);
	TEST_ASSERT(xent_layout(ctx, root, 300.0f, 100.0f));
	XentProfStats stats = xent_profile_get(ctx);
	TEST_ASSERT(stats.flex_layout_calls == 1u);
	TEST_ASSERT(stats.text_measure_calls >= 2u);
	TEST_ASSERT(stats.text_baseline_fallbacks >= 2u);

	xent_ctx_destroy(ctx);
	return 0;
}

static int test_profile_tracks_grid(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId grid  = xent_node_create(ctx);
	XentNodeId child = xent_node_create(ctx);
	TEST_ASSERT(xent_setproto(ctx, grid, XENT_PROTOCOL_GRID));
	TEST_ASSERT(xent_setsize(ctx, grid, (XentSize) {300.0f, 100.0f}));
	TEST_ASSERT(xent_settext(ctx, child, "grid text"));

	XentGridSizeMode row_modes [] = {XENT_GRID_AUTO};
	float            row_vals []  = {0.0f};
	XentGridSizeMode col_modes [] = {XENT_GRID_AUTO};
	float            col_vals []  = {0.0f};
	TEST_ASSERT(xent_grid_setrows(ctx, grid, row_modes, row_vals, 1u));
	TEST_ASSERT(xent_grid_setcols(ctx, grid, col_modes, col_vals, 1u));
	TEST_ASSERT(xent_node_append(ctx, grid, child));

	xent_profile_reset(ctx);
	TEST_ASSERT(xent_layout(ctx, grid, 300.0f, 100.0f));
	XentProfStats stats = xent_profile_get(ctx);
	TEST_ASSERT(stats.grid_layout_calls == 1u);
	TEST_ASSERT(stats.text_measure_calls >= 1u);

	xent_ctx_destroy(ctx);
	return 0;
}

int main(void) {
	XentTestFn tests [] = {
	  test_profile_tracks_flex_baseline_fallback,
	  test_profile_tracks_grid,
	};
	return test_run_all(tests, sizeof(tests) / sizeof(tests [0]));
}
