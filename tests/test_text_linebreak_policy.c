#include "test_common.h"

int main(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentTextMetrics m = {0};
    TEST_ASSERT(xent_measure_text(
        ctx, "ab cd ef", 14.0f, 40.0f, XENT_LINE_BREAK_NO_WRAP, XENT_MEASURE_AT_MOST, &m));
    TEST_ASSERT(m.line_count == 1u);
    TEST_ASSERT(test_float_near(m.width, 64.0f, 0.001f));

    TEST_ASSERT(xent_measure_text(
        ctx, "ab cd ef", 14.0f, 40.0f, XENT_LINE_BREAK_CHAR_WRAP, XENT_MEASURE_AT_MOST, &m));
    TEST_ASSERT(m.line_count == 2u);
    TEST_ASSERT(test_float_near(m.width, 40.0f, 0.001f));

    TEST_ASSERT(xent_measure_text(
        ctx, "ab cd ef", 14.0f, 16.0f, XENT_LINE_BREAK_CHAR_WRAP, XENT_MEASURE_UNDEFINED, &m));
    TEST_ASSERT(m.line_count == 1u);
    TEST_ASSERT(test_float_near(m.width, 64.0f, 0.001f));

    TEST_ASSERT(
        xent_measure_text(ctx, "ab cd ef", 14.0f, 40.0f, XENT_LINE_BREAK_NO_WRAP, XENT_MEASURE_EXACTLY, &m));
    TEST_ASSERT(m.line_count == 1u);
    TEST_ASSERT(test_float_near(m.width, 40.0f, 0.001f));

    XentShapedLine lines_char[8] = {0};
    XentShapingResult shaped_char = {0};
    TEST_ASSERT(xent_shape_text(ctx,
                                "ab cd ef",
                                14.0f,
                                40.0f,
                                XENT_LINE_BREAK_CHAR_WRAP,
                                XENT_MEASURE_AT_MOST,
                                NULL,
                                0u,
                                NULL,
                                0u,
                                lines_char,
                                8u,
                                &shaped_char));

    XentShapedLine lines_word[8] = {0};
    XentShapingResult shaped_word = {0};
    TEST_ASSERT(xent_shape_text(ctx,
                                "ab cd ef",
                                14.0f,
                                40.0f,
                                XENT_LINE_BREAK_WORD_WRAP,
                                XENT_MEASURE_AT_MOST,
                                NULL,
                                0u,
                                NULL,
                                0u,
                                lines_word,
                                8u,
                                &shaped_word));

    TEST_ASSERT(shaped_char.line_count == 2u);
    TEST_ASSERT(shaped_word.line_count == 2u);
    TEST_ASSERT(lines_char[0].glyph_count == 5u);
    TEST_ASSERT(lines_word[0].glyph_count == 3u);

    XentShapingResult shaped_exact = {0};
    TEST_ASSERT(xent_shape_text(ctx,
                                "ab cd ef",
                                14.0f,
                                40.0f,
                                XENT_LINE_BREAK_NO_WRAP,
                                XENT_MEASURE_EXACTLY,
                                NULL,
                                0u,
                                NULL,
                                0u,
                                NULL,
                                0u,
                                &shaped_exact));
    TEST_ASSERT(shaped_exact.line_count == 1u);
    TEST_ASSERT(test_float_near(shaped_exact.metrics.width, 40.0f, 0.001f));

    XentShapedLine punct_word[8] = {0};
    XentShapingResult punct_result = {0};
    TEST_ASSERT(xent_shape_text(ctx,
                                "a,bcde",
                                14.0f,
                                24.0f,
                                XENT_LINE_BREAK_WORD_WRAP,
                                XENT_MEASURE_AT_MOST,
                                NULL,
                                0u,
                                NULL,
                                0u,
                                punct_word,
                                8u,
                                &punct_result));
    TEST_ASSERT(punct_result.line_count >= 2u);
    TEST_ASSERT(punct_word[0].glyph_count == 2u);

    XentNodeId node = xent_create_node(ctx);
    TEST_ASSERT(node != XENT_NODE_INVALID);
    TEST_ASSERT(xent_set_text_line_break_policy(ctx, node, XENT_LINE_BREAK_WORD_WRAP));
    TEST_ASSERT(xent_get_text_line_break_policy(ctx, node) == XENT_LINE_BREAK_WORD_WRAP);

    xent_destroy_context(ctx);
    return 0;
}
