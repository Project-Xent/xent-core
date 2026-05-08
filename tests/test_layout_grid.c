#include "test_common.h"

int main(void) {
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
