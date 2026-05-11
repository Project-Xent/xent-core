#include "test_common.h"

static int test_profile_tracks_flex_baseline_fallback(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root  = xent_create_node(ctx);
	XentNodeId small = xent_create_node(ctx);
	XentNodeId large = xent_create_node(ctx);

	TEST_ASSERT(xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX));
	TEST_ASSERT(xent_set_flex_direction(ctx, root, XENT_FLEX_ROW));
	TEST_ASSERT(xent_set_flex_align_items(ctx, root, XENT_FLEX_ALIGN_BASELINE));
	TEST_ASSERT(xent_set_size(ctx, root, (XentSize) {300.0f, 100.0f}));
	TEST_ASSERT(xent_set_text(ctx, small, "small"));
	TEST_ASSERT(xent_set_text(ctx, large, "large"));
	TEST_ASSERT(xent_append_child(ctx, root, small));
	TEST_ASSERT(xent_append_child(ctx, root, large));

	xent_profile_reset(ctx);
	TEST_ASSERT(xent_layout(ctx, root, 300.0f, 100.0f));
	XentProfileStats stats = xent_profile_get(ctx);
	TEST_ASSERT(stats.flex_layout_calls == 1u);
	TEST_ASSERT(stats.text_measure_calls >= 2u);
	TEST_ASSERT(stats.text_baseline_fallbacks >= 2u);

	xent_destroy_context(ctx);
	return 0;
}

static int test_profile_tracks_grid(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId grid  = xent_create_node(ctx);
	XentNodeId child = xent_create_node(ctx);
	TEST_ASSERT(xent_set_protocol(ctx, grid, XENT_PROTOCOL_GRID));
	TEST_ASSERT(xent_set_size(ctx, grid, (XentSize) {300.0f, 100.0f}));
	TEST_ASSERT(xent_set_text(ctx, child, "grid text"));

	XentGridSizeMode row_modes [] = {XENT_GRID_AUTO};
	float            row_vals []  = {0.0f};
	XentGridSizeMode col_modes [] = {XENT_GRID_AUTO};
	float            col_vals []  = {0.0f};
	TEST_ASSERT(xent_set_grid_rows(ctx, grid, row_modes, row_vals, 1u));
	TEST_ASSERT(xent_set_grid_columns(ctx, grid, col_modes, col_vals, 1u));
	TEST_ASSERT(xent_append_child(ctx, grid, child));

	xent_profile_reset(ctx);
	TEST_ASSERT(xent_layout(ctx, grid, 300.0f, 100.0f));
	XentProfileStats stats = xent_profile_get(ctx);
	TEST_ASSERT(stats.grid_layout_calls == 1u);
	TEST_ASSERT(stats.text_measure_calls >= 1u);

	xent_destroy_context(ctx);
	return 0;
}

int main(void) {
	XentTestFn tests [] = {
	  test_profile_tracks_flex_baseline_fallback,
	  test_profile_tracks_grid,
	};
	return test_run_all(tests, sizeof(tests) / sizeof(tests [0]));
}
