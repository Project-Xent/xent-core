#include "test_common.h"

int main(void) {
    float eps = 0.5f;

    /* ================================================================
     * Test 1: Basic 2-column Star layout (equal split)
     * Grid 400x100, 1 row (Star), 2 columns (Star, Star)
     * ================================================================ */
    {
        XentContext *ctx = xent_create_context(NULL);
        TEST_ASSERT(ctx != NULL);

        XentNodeId grid = xent_create_node(ctx);
        XentNodeId c0   = xent_create_node(ctx);
        XentNodeId c1   = xent_create_node(ctx);

        xent_set_protocol(ctx, grid, XENT_PROTOCOL_GRID);
        xent_set_size(ctx, grid, 400.0f, 100.0f);

        XentGridSizeMode row_modes[] = { XENT_GRID_STAR };
        float            row_vals[]  = { 1.0f };
        xent_set_grid_rows(ctx, grid, row_modes, row_vals, 1);

        XentGridSizeMode col_modes[] = { XENT_GRID_STAR, XENT_GRID_STAR };
        float            col_vals[]  = { 1.0f, 1.0f };
        xent_set_grid_columns(ctx, grid, col_modes, col_vals, 2);

        xent_set_grid_row(ctx, c0, 0);
        xent_set_grid_column(ctx, c0, 0);
        xent_set_grid_row(ctx, c1, 0);
        xent_set_grid_column(ctx, c1, 1);

        xent_append_child(ctx, grid, c0);
        xent_append_child(ctx, grid, c1);

        TEST_ASSERT(xent_layout(ctx, grid, 400.0f, 100.0f));

        XentRect r0 = {0}, r1 = {0};
        TEST_ASSERT(xent_get_layout_rect(ctx, c0, &r0));
        TEST_ASSERT(xent_get_layout_rect(ctx, c1, &r1));

        TEST_ASSERT(test_float_near(r0.width,  200.0f, eps));
        TEST_ASSERT(test_float_near(r0.height, 100.0f, eps));
        TEST_ASSERT(test_float_near(r0.x,        0.0f, eps));
        TEST_ASSERT(test_float_near(r0.y,        0.0f, eps));

        TEST_ASSERT(test_float_near(r1.width,  200.0f, eps));
        TEST_ASSERT(test_float_near(r1.height, 100.0f, eps));
        TEST_ASSERT(test_float_near(r1.x,      200.0f, eps));
        TEST_ASSERT(test_float_near(r1.y,        0.0f, eps));

        xent_destroy_context(ctx);
        printf("Test 1 PASSED: Basic 2-column Star layout\n");
    }

    /* ================================================================
     * Test 2: Pixel + Star columns
     * Grid 400x100, 2 columns: Pixel(100), Star(1)
     * ================================================================ */
    {
        XentContext *ctx = xent_create_context(NULL);
        TEST_ASSERT(ctx != NULL);

        XentNodeId grid = xent_create_node(ctx);
        XentNodeId c0   = xent_create_node(ctx);
        XentNodeId c1   = xent_create_node(ctx);

        xent_set_protocol(ctx, grid, XENT_PROTOCOL_GRID);
        xent_set_size(ctx, grid, 400.0f, 100.0f);

        XentGridSizeMode row_modes[] = { XENT_GRID_STAR };
        float            row_vals[]  = { 1.0f };
        xent_set_grid_rows(ctx, grid, row_modes, row_vals, 1);

        XentGridSizeMode col_modes[] = { XENT_GRID_PIXEL, XENT_GRID_STAR };
        float            col_vals[]  = { 100.0f, 1.0f };
        xent_set_grid_columns(ctx, grid, col_modes, col_vals, 2);

        xent_set_grid_row(ctx, c0, 0);
        xent_set_grid_column(ctx, c0, 0);
        xent_set_grid_row(ctx, c1, 0);
        xent_set_grid_column(ctx, c1, 1);

        xent_append_child(ctx, grid, c0);
        xent_append_child(ctx, grid, c1);

        TEST_ASSERT(xent_layout(ctx, grid, 400.0f, 100.0f));

        XentRect r0 = {0}, r1 = {0};
        TEST_ASSERT(xent_get_layout_rect(ctx, c0, &r0));
        TEST_ASSERT(xent_get_layout_rect(ctx, c1, &r1));

        TEST_ASSERT(test_float_near(r0.width, 100.0f, eps));
        TEST_ASSERT(test_float_near(r1.width, 300.0f, eps));
        TEST_ASSERT(test_float_near(r0.x,       0.0f, eps));
        TEST_ASSERT(test_float_near(r1.x,     100.0f, eps));

        xent_destroy_context(ctx);
        printf("Test 2 PASSED: Pixel + Star columns\n");
    }

    /* ================================================================
     * Test 3: Auto + Star columns
     * Grid 400x100, 2 columns: Auto, Star(1)
     * Child 0 has fixed size 80x30 → auto column = 80
     * Child 1 gets remaining 320px
     * ================================================================ */
    {
        XentContext *ctx = xent_create_context(NULL);
        TEST_ASSERT(ctx != NULL);

        XentNodeId grid = xent_create_node(ctx);
        XentNodeId c0   = xent_create_node(ctx);
        XentNodeId c1   = xent_create_node(ctx);

        xent_set_protocol(ctx, grid, XENT_PROTOCOL_GRID);
        xent_set_size(ctx, grid, 400.0f, 100.0f);

        XentGridSizeMode row_modes[] = { XENT_GRID_STAR };
        float            row_vals[]  = { 1.0f };
        xent_set_grid_rows(ctx, grid, row_modes, row_vals, 1);

        XentGridSizeMode col_modes[] = { XENT_GRID_AUTO, XENT_GRID_STAR };
        float            col_vals[]  = { 0.0f, 1.0f };
        xent_set_grid_columns(ctx, grid, col_modes, col_vals, 2);

        xent_set_size(ctx, c0, 80.0f, 30.0f);
        xent_set_grid_row(ctx, c0, 0);
        xent_set_grid_column(ctx, c0, 0);

        xent_set_grid_row(ctx, c1, 0);
        xent_set_grid_column(ctx, c1, 1);

        xent_append_child(ctx, grid, c0);
        xent_append_child(ctx, grid, c1);

        TEST_ASSERT(xent_layout(ctx, grid, 400.0f, 100.0f));

        XentRect r0 = {0}, r1 = {0};
        TEST_ASSERT(xent_get_layout_rect(ctx, c0, &r0));
        TEST_ASSERT(xent_get_layout_rect(ctx, c1, &r1));

        TEST_ASSERT(test_float_near(r0.width,  80.0f, eps));
        TEST_ASSERT(test_float_near(r1.width, 320.0f, eps));
        TEST_ASSERT(test_float_near(r1.x,      80.0f, eps));

        xent_destroy_context(ctx);
        printf("Test 3 PASSED: Auto + Star columns\n");
    }

    /* ================================================================
     * Test 4: Row + Column with gaps
     * Grid 400x200, 2 rows (Star, Star), 2 columns (Star, Star)
     * Row gap = 10, Column gap = 20
     * Col space = 400 - 20 = 380, each col = 190
     * Row space = 200 - 10 = 190, each row = 95
     * ================================================================ */
    {
        XentContext *ctx = xent_create_context(NULL);
        TEST_ASSERT(ctx != NULL);

        XentNodeId grid = xent_create_node(ctx);
        XentNodeId c00  = xent_create_node(ctx);  /* row=0 col=0 */
        XentNodeId c01  = xent_create_node(ctx);  /* row=0 col=1 */
        XentNodeId c10  = xent_create_node(ctx);  /* row=1 col=0 */
        XentNodeId c11  = xent_create_node(ctx);  /* row=1 col=1 */

        xent_set_protocol(ctx, grid, XENT_PROTOCOL_GRID);
        xent_set_size(ctx, grid, 400.0f, 200.0f);

        XentGridSizeMode row_modes[] = { XENT_GRID_STAR, XENT_GRID_STAR };
        float            row_vals[]  = { 1.0f, 1.0f };
        xent_set_grid_rows(ctx, grid, row_modes, row_vals, 2);

        XentGridSizeMode col_modes[] = { XENT_GRID_STAR, XENT_GRID_STAR };
        float            col_vals[]  = { 1.0f, 1.0f };
        xent_set_grid_columns(ctx, grid, col_modes, col_vals, 2);

        xent_set_grid_row_gap(ctx, grid, 10.0f);
        xent_set_grid_column_gap(ctx, grid, 20.0f);

        xent_set_grid_row(ctx, c00, 0); xent_set_grid_column(ctx, c00, 0);
        xent_set_grid_row(ctx, c01, 0); xent_set_grid_column(ctx, c01, 1);
        xent_set_grid_row(ctx, c10, 1); xent_set_grid_column(ctx, c10, 0);
        xent_set_grid_row(ctx, c11, 1); xent_set_grid_column(ctx, c11, 1);

        xent_append_child(ctx, grid, c00);
        xent_append_child(ctx, grid, c01);
        xent_append_child(ctx, grid, c10);
        xent_append_child(ctx, grid, c11);

        TEST_ASSERT(xent_layout(ctx, grid, 400.0f, 200.0f));

        XentRect r00 = {0}, r01 = {0}, r10 = {0}, r11 = {0};
        TEST_ASSERT(xent_get_layout_rect(ctx, c00, &r00));
        TEST_ASSERT(xent_get_layout_rect(ctx, c01, &r01));
        TEST_ASSERT(xent_get_layout_rect(ctx, c10, &r10));
        TEST_ASSERT(xent_get_layout_rect(ctx, c11, &r11));

        /* Each column = 190, each row = 95 */
        TEST_ASSERT(test_float_near(r00.width,  190.0f, eps));
        TEST_ASSERT(test_float_near(r00.height,  95.0f, eps));
        TEST_ASSERT(test_float_near(r00.x,        0.0f, eps));
        TEST_ASSERT(test_float_near(r00.y,        0.0f, eps));

        TEST_ASSERT(test_float_near(r01.width,  190.0f, eps));
        TEST_ASSERT(test_float_near(r01.height,  95.0f, eps));
        TEST_ASSERT(test_float_near(r01.x,      210.0f, eps));  /* 190 + 20 gap */
        TEST_ASSERT(test_float_near(r01.y,        0.0f, eps));

        TEST_ASSERT(test_float_near(r10.width,  190.0f, eps));
        TEST_ASSERT(test_float_near(r10.height,  95.0f, eps));
        TEST_ASSERT(test_float_near(r10.x,        0.0f, eps));
        TEST_ASSERT(test_float_near(r10.y,      105.0f, eps));  /* 95 + 10 gap */

        TEST_ASSERT(test_float_near(r11.width,  190.0f, eps));
        TEST_ASSERT(test_float_near(r11.height,  95.0f, eps));
        TEST_ASSERT(test_float_near(r11.x,      210.0f, eps));
        TEST_ASSERT(test_float_near(r11.y,      105.0f, eps));

        xent_destroy_context(ctx);
        printf("Test 4 PASSED: Row + Column with gaps\n");
    }

    /* ================================================================
     * Test 5: ColumnSpan test
     * Grid 300x100, 1 row Star, 3 columns: Pixel(100) each
     * Child 0: row=0, col=0, colSpan=2 → spans 200px
     * Child 1: row=0, col=2 → 100px
     * ================================================================ */
    {
        XentContext *ctx = xent_create_context(NULL);
        TEST_ASSERT(ctx != NULL);

        XentNodeId grid = xent_create_node(ctx);
        XentNodeId c0   = xent_create_node(ctx);
        XentNodeId c1   = xent_create_node(ctx);

        xent_set_protocol(ctx, grid, XENT_PROTOCOL_GRID);
        xent_set_size(ctx, grid, 300.0f, 100.0f);

        XentGridSizeMode row_modes[] = { XENT_GRID_STAR };
        float            row_vals[]  = { 1.0f };
        xent_set_grid_rows(ctx, grid, row_modes, row_vals, 1);

        XentGridSizeMode col_modes[] = { XENT_GRID_PIXEL, XENT_GRID_PIXEL, XENT_GRID_PIXEL };
        float            col_vals[]  = { 100.0f, 100.0f, 100.0f };
        xent_set_grid_columns(ctx, grid, col_modes, col_vals, 3);

        xent_set_grid_row(ctx, c0, 0);
        xent_set_grid_column(ctx, c0, 0);
        xent_set_grid_column_span(ctx, c0, 2);

        xent_set_grid_row(ctx, c1, 0);
        xent_set_grid_column(ctx, c1, 2);

        xent_append_child(ctx, grid, c0);
        xent_append_child(ctx, grid, c1);

        TEST_ASSERT(xent_layout(ctx, grid, 300.0f, 100.0f));

        XentRect r0 = {0}, r1 = {0};
        TEST_ASSERT(xent_get_layout_rect(ctx, c0, &r0));
        TEST_ASSERT(xent_get_layout_rect(ctx, c1, &r1));

        TEST_ASSERT(test_float_near(r0.width, 200.0f, eps));
        TEST_ASSERT(test_float_near(r0.x,       0.0f, eps));
        TEST_ASSERT(test_float_near(r1.width, 100.0f, eps));
        TEST_ASSERT(test_float_near(r1.x,     200.0f, eps));

        xent_destroy_context(ctx);
        printf("Test 5 PASSED: ColumnSpan test\n");
    }

    /* ================================================================
     * Test 6: RowSpan test
     * Grid 200x300, 3 rows: Pixel(100) each, 1 column Star
     * Child 0: row=0, col=0, rowSpan=2 → 200px tall
     * Child 1: row=2, col=0 → 100px tall
     * ================================================================ */
    {
        XentContext *ctx = xent_create_context(NULL);
        TEST_ASSERT(ctx != NULL);

        XentNodeId grid = xent_create_node(ctx);
        XentNodeId c0   = xent_create_node(ctx);
        XentNodeId c1   = xent_create_node(ctx);

        xent_set_protocol(ctx, grid, XENT_PROTOCOL_GRID);
        xent_set_size(ctx, grid, 200.0f, 300.0f);

        XentGridSizeMode row_modes[] = { XENT_GRID_PIXEL, XENT_GRID_PIXEL, XENT_GRID_PIXEL };
        float            row_vals[]  = { 100.0f, 100.0f, 100.0f };
        xent_set_grid_rows(ctx, grid, row_modes, row_vals, 3);

        XentGridSizeMode col_modes[] = { XENT_GRID_STAR };
        float            col_vals[]  = { 1.0f };
        xent_set_grid_columns(ctx, grid, col_modes, col_vals, 1);

        xent_set_grid_row(ctx, c0, 0);
        xent_set_grid_column(ctx, c0, 0);
        xent_set_grid_row_span(ctx, c0, 2);

        xent_set_grid_row(ctx, c1, 2);
        xent_set_grid_column(ctx, c1, 0);

        xent_append_child(ctx, grid, c0);
        xent_append_child(ctx, grid, c1);

        TEST_ASSERT(xent_layout(ctx, grid, 200.0f, 300.0f));

        XentRect r0 = {0}, r1 = {0};
        TEST_ASSERT(xent_get_layout_rect(ctx, c0, &r0));
        TEST_ASSERT(xent_get_layout_rect(ctx, c1, &r1));

        TEST_ASSERT(test_float_near(r0.height, 200.0f, eps));
        TEST_ASSERT(test_float_near(r0.y,        0.0f, eps));
        TEST_ASSERT(test_float_near(r1.height, 100.0f, eps));
        TEST_ASSERT(test_float_near(r1.y,      200.0f, eps));
        TEST_ASSERT(test_float_near(r0.width,  200.0f, eps));
        TEST_ASSERT(test_float_near(r1.width,  200.0f, eps));

        xent_destroy_context(ctx);
        printf("Test 6 PASSED: RowSpan test\n");
    }

    /* ================================================================
     * Test 7: Weighted Star columns (2:1 ratio)
     * Grid 300x100, 2 columns: Star(2), Star(1)
     * Child 0 = 200px, Child 1 = 100px
     * ================================================================ */
    {
        XentContext *ctx = xent_create_context(NULL);
        TEST_ASSERT(ctx != NULL);

        XentNodeId grid = xent_create_node(ctx);
        XentNodeId c0   = xent_create_node(ctx);
        XentNodeId c1   = xent_create_node(ctx);

        xent_set_protocol(ctx, grid, XENT_PROTOCOL_GRID);
        xent_set_size(ctx, grid, 300.0f, 100.0f);

        XentGridSizeMode row_modes[] = { XENT_GRID_STAR };
        float            row_vals[]  = { 1.0f };
        xent_set_grid_rows(ctx, grid, row_modes, row_vals, 1);

        XentGridSizeMode col_modes[] = { XENT_GRID_STAR, XENT_GRID_STAR };
        float            col_vals[]  = { 2.0f, 1.0f };
        xent_set_grid_columns(ctx, grid, col_modes, col_vals, 2);

        xent_set_grid_row(ctx, c0, 0);
        xent_set_grid_column(ctx, c0, 0);
        xent_set_grid_row(ctx, c1, 0);
        xent_set_grid_column(ctx, c1, 1);

        xent_append_child(ctx, grid, c0);
        xent_append_child(ctx, grid, c1);

        TEST_ASSERT(xent_layout(ctx, grid, 300.0f, 100.0f));

        XentRect r0 = {0}, r1 = {0};
        TEST_ASSERT(xent_get_layout_rect(ctx, c0, &r0));
        TEST_ASSERT(xent_get_layout_rect(ctx, c1, &r1));

        TEST_ASSERT(test_float_near(r0.width, 200.0f, eps));
        TEST_ASSERT(test_float_near(r1.width, 100.0f, eps));
        TEST_ASSERT(test_float_near(r0.x,       0.0f, eps));
        TEST_ASSERT(test_float_near(r1.x,     200.0f, eps));

        xent_destroy_context(ctx);
        printf("Test 7 PASSED: Weighted Star columns (2:1)\n");
    }

    /* ================================================================
     * Test 8: Grid with padding
     * Grid 400x200 with padding 10 on all sides
     * 1 row Star, 1 column Star → content area = 380x180
     * Child should be at (10,10) with size 380x180
     * ================================================================ */
    {
        XentContext *ctx = xent_create_context(NULL);
        TEST_ASSERT(ctx != NULL);

        XentNodeId grid  = xent_create_node(ctx);
        XentNodeId child = xent_create_node(ctx);

        xent_set_protocol(ctx, grid, XENT_PROTOCOL_GRID);
        xent_set_size(ctx, grid, 400.0f, 200.0f);
        xent_set_padding(ctx, grid, 10.0f, 10.0f, 10.0f, 10.0f);

        XentGridSizeMode row_modes[] = { XENT_GRID_STAR };
        float            row_vals[]  = { 1.0f };
        xent_set_grid_rows(ctx, grid, row_modes, row_vals, 1);

        XentGridSizeMode col_modes[] = { XENT_GRID_STAR };
        float            col_vals[]  = { 1.0f };
        xent_set_grid_columns(ctx, grid, col_modes, col_vals, 1);

        xent_set_grid_row(ctx, child, 0);
        xent_set_grid_column(ctx, child, 0);

        xent_append_child(ctx, grid, child);

        TEST_ASSERT(xent_layout(ctx, grid, 400.0f, 200.0f));

        XentRect r = {0};
        TEST_ASSERT(xent_get_layout_rect(ctx, child, &r));

        TEST_ASSERT(test_float_near(r.x,       10.0f, eps));
        TEST_ASSERT(test_float_near(r.y,       10.0f, eps));
        TEST_ASSERT(test_float_near(r.width,  380.0f, eps));
        TEST_ASSERT(test_float_near(r.height, 180.0f, eps));

        xent_destroy_context(ctx);
        printf("Test 8 PASSED: Grid with padding\n");
    }

    /* ================================================================
     * Test 9: Mixed protocol — Grid inside Flex
     * Flex row container 600x100 with two children:
     *   Child A: fixed 200px wide (flex_shrink=0)
     *   Child B: Grid container with flex_grow=1 (gets 400px)
     *     Grid has 2 columns: Star(1), Pixel(100)
     *     Grid child 0 → 300px wide, Grid child 1 → 100px wide
     * ================================================================ */
    {
        XentContext *ctx = xent_create_context(NULL);
        TEST_ASSERT(ctx != NULL);

        XentNodeId flex_root = xent_create_node(ctx);
        XentNodeId child_a   = xent_create_node(ctx);
        XentNodeId grid_b    = xent_create_node(ctx);
        XentNodeId gc0       = xent_create_node(ctx);
        XentNodeId gc1       = xent_create_node(ctx);

        /* Flex row container */
        xent_set_protocol(ctx, flex_root, XENT_PROTOCOL_FLEX);
        xent_set_flex_direction(ctx, flex_root, XENT_FLEX_ROW);
        xent_set_size(ctx, flex_root, 600.0f, 100.0f);

        /* Child A: fixed 200px */
        xent_set_size(ctx, child_a, 200.0f, 100.0f);
        xent_set_flex_shrink(ctx, child_a, 0.0f);

        /* Child B: Grid that grows to fill remaining space */
        xent_set_protocol(ctx, grid_b, XENT_PROTOCOL_GRID);
        xent_set_size(ctx, grid_b, NAN, 100.0f);
        xent_set_flex_grow(ctx, grid_b, 1.0f);

        XentGridSizeMode row_modes[] = { XENT_GRID_STAR };
        float            row_vals[]  = { 1.0f };
        xent_set_grid_rows(ctx, grid_b, row_modes, row_vals, 1);

        XentGridSizeMode col_modes[] = { XENT_GRID_STAR, XENT_GRID_PIXEL };
        float            col_vals[]  = { 1.0f, 100.0f };
        xent_set_grid_columns(ctx, grid_b, col_modes, col_vals, 2);

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
        TEST_ASSERT(xent_get_layout_rect(ctx, grid_b,  &rb));
        TEST_ASSERT(xent_get_layout_rect(ctx, gc0,     &rg0));
        TEST_ASSERT(xent_get_layout_rect(ctx, gc1,     &rg1));

        /* Child A = 200px, Grid B = 400px */
        TEST_ASSERT(test_float_near(ra.width,  200.0f, eps));
        TEST_ASSERT(test_float_near(rb.width,  400.0f, eps));
        TEST_ASSERT(test_float_near(rb.x,      200.0f, eps));

        /* Inside the grid: Star col = 300px, Pixel col = 100px */
        TEST_ASSERT(test_float_near(rg0.width, 300.0f, eps));
        TEST_ASSERT(test_float_near(rg1.width, 100.0f, eps));

        xent_destroy_context(ctx);
        printf("Test 9 PASSED: Mixed protocol — Grid inside Flex\n");
    }

    /* ================================================================
     * Test 10: Fallback — Grid protocol with no grid_def
     * Set protocol to GRID but don't call set_grid_rows/columns
     * Should fallback gracefully: children get full content area
     * ================================================================ */
    {
        XentContext *ctx = xent_create_context(NULL);
        TEST_ASSERT(ctx != NULL);

        XentNodeId grid  = xent_create_node(ctx);
        XentNodeId child = xent_create_node(ctx);

        xent_set_protocol(ctx, grid, XENT_PROTOCOL_GRID);
        xent_set_size(ctx, grid, 200.0f, 150.0f);

        /* Intentionally NOT calling xent_set_grid_rows / xent_set_grid_columns */

        xent_append_child(ctx, grid, child);

        TEST_ASSERT(xent_layout(ctx, grid, 200.0f, 150.0f));

        XentRect r = {0};
        TEST_ASSERT(xent_get_layout_rect(ctx, child, &r));

        /* Child should receive the full content area */
        TEST_ASSERT(test_float_near(r.width,  200.0f, eps));
        TEST_ASSERT(test_float_near(r.height, 150.0f, eps));
        TEST_ASSERT(test_float_near(r.x,        0.0f, eps));
        TEST_ASSERT(test_float_near(r.y,        0.0f, eps));

        xent_destroy_context(ctx);
        printf("Test 10 PASSED: Fallback — Grid with no grid_def\n");
    }

    /* ================================================================
     * Test 11: Span with gaps
     * Grid 340x100, 3 columns Pixel(100) each, col gap=20
     * Total: 100 + 20 + 100 + 20 + 100 = 340
     * Child 0: colSpan=2 at col=0 → width = 100 + 20 + 100 = 220
     * Child 1: col=2 → width = 100
     * ================================================================ */
    {
        XentContext *ctx = xent_create_context(NULL);
        TEST_ASSERT(ctx != NULL);

        XentNodeId grid = xent_create_node(ctx);
        XentNodeId c0   = xent_create_node(ctx);
        XentNodeId c1   = xent_create_node(ctx);

        xent_set_protocol(ctx, grid, XENT_PROTOCOL_GRID);
        xent_set_size(ctx, grid, 340.0f, 100.0f);

        XentGridSizeMode row_modes[] = { XENT_GRID_STAR };
        float            row_vals[]  = { 1.0f };
        xent_set_grid_rows(ctx, grid, row_modes, row_vals, 1);

        XentGridSizeMode col_modes[] = { XENT_GRID_PIXEL, XENT_GRID_PIXEL, XENT_GRID_PIXEL };
        float            col_vals[]  = { 100.0f, 100.0f, 100.0f };
        xent_set_grid_columns(ctx, grid, col_modes, col_vals, 3);

        xent_set_grid_column_gap(ctx, grid, 20.0f);

        xent_set_grid_row(ctx, c0, 0);
        xent_set_grid_column(ctx, c0, 0);
        xent_set_grid_column_span(ctx, c0, 2);

        xent_set_grid_row(ctx, c1, 0);
        xent_set_grid_column(ctx, c1, 2);

        xent_append_child(ctx, grid, c0);
        xent_append_child(ctx, grid, c1);

        TEST_ASSERT(xent_layout(ctx, grid, 340.0f, 100.0f));

        XentRect r0 = {0}, r1 = {0};
        TEST_ASSERT(xent_get_layout_rect(ctx, c0, &r0));
        TEST_ASSERT(xent_get_layout_rect(ctx, c1, &r1));

        /* colSpan=2 includes the gap between spanned columns: 100+20+100 = 220 */
        TEST_ASSERT(test_float_near(r0.width, 220.0f, eps));
        TEST_ASSERT(test_float_near(r0.x,       0.0f, eps));
        TEST_ASSERT(test_float_near(r1.width, 100.0f, eps));
        TEST_ASSERT(test_float_near(r1.x,     240.0f, eps));  /* 220 + 20 gap */

        xent_destroy_context(ctx);
        printf("Test 11 PASSED: Span with gaps\n");
    }

    /* ================================================================
     * Test 12: PasswordBox-like layout (realistic WinUI template)
     * Grid 300x32, 1 row Star, 2 columns: Star(1) and Pixel(30)
     * Child 0 (text area): col=0, should get 270px
     * Child 1 (reveal button): col=1, should get 30px
     * ================================================================ */
    {
        XentContext *ctx = xent_create_context(NULL);
        TEST_ASSERT(ctx != NULL);

        XentNodeId grid          = xent_create_node(ctx);
        XentNodeId text_area     = xent_create_node(ctx);
        XentNodeId reveal_button = xent_create_node(ctx);

        xent_set_protocol(ctx, grid, XENT_PROTOCOL_GRID);
        xent_set_size(ctx, grid, 300.0f, 32.0f);

        XentGridSizeMode row_modes[] = { XENT_GRID_STAR };
        float            row_vals[]  = { 1.0f };
        xent_set_grid_rows(ctx, grid, row_modes, row_vals, 1);

        XentGridSizeMode col_modes[] = { XENT_GRID_STAR, XENT_GRID_PIXEL };
        float            col_vals[]  = { 1.0f, 30.0f };
        xent_set_grid_columns(ctx, grid, col_modes, col_vals, 2);

        xent_set_grid_row(ctx, text_area, 0);
        xent_set_grid_column(ctx, text_area, 0);

        xent_set_grid_row(ctx, reveal_button, 0);
        xent_set_grid_column(ctx, reveal_button, 1);

        xent_append_child(ctx, grid, text_area);
        xent_append_child(ctx, grid, reveal_button);

        TEST_ASSERT(xent_layout(ctx, grid, 300.0f, 32.0f));

        XentRect rt = {0}, rb = {0};
        TEST_ASSERT(xent_get_layout_rect(ctx, text_area,     &rt));
        TEST_ASSERT(xent_get_layout_rect(ctx, reveal_button, &rb));

        /* Star(1) gets 300 - 30 = 270px */
        TEST_ASSERT(test_float_near(rt.width,  270.0f, eps));
        TEST_ASSERT(test_float_near(rt.height,  32.0f, eps));
        TEST_ASSERT(test_float_near(rt.x,        0.0f, eps));
        TEST_ASSERT(test_float_near(rt.y,        0.0f, eps));

        TEST_ASSERT(test_float_near(rb.width,   30.0f, eps));
        TEST_ASSERT(test_float_near(rb.height,  32.0f, eps));
        TEST_ASSERT(test_float_near(rb.x,      270.0f, eps));
        TEST_ASSERT(test_float_near(rb.y,        0.0f, eps));

        xent_destroy_context(ctx);
        printf("Test 12 PASSED: PasswordBox-like layout\n");
    }

    printf("\nALL GRID TESTS PASSED\n");
    return 0;
}