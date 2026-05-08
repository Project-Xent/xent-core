#include "test_common.h"

int main(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentTextMetrics        m       = {0};
	XentTextMeasureRequest measure = {"ab cd ef", 14.0f, 40.0f, XENT_LINE_BREAK_NO_WRAP, XENT_MEASURE_AT_MOST};
	TEST_ASSERT(xent_measure_text(ctx, &measure, &m));
	TEST_ASSERT(m.line_count == 1u);
	TEST_ASSERT(test_float_near(m.width, 64.0f, 0.001f));

	measure.line_break_policy = XENT_LINE_BREAK_CHAR_WRAP;
	TEST_ASSERT(xent_measure_text(ctx, &measure, &m));
	TEST_ASSERT(m.line_count == 2u);
	TEST_ASSERT(test_float_near(m.width, 40.0f, 0.001f));

	measure.width_constraint = 16.0f;
	measure.width_mode       = XENT_MEASURE_UNDEFINED;
	TEST_ASSERT(xent_measure_text(ctx, &measure, &m));
	TEST_ASSERT(m.line_count == 1u);
	TEST_ASSERT(test_float_near(m.width, 64.0f, 0.001f));

	measure.width_constraint  = 40.0f;
	measure.line_break_policy = XENT_LINE_BREAK_NO_WRAP;
	measure.width_mode        = XENT_MEASURE_EXACTLY;
	TEST_ASSERT(xent_measure_text(ctx, &measure, &m));
	TEST_ASSERT(m.line_count == 1u);
	TEST_ASSERT(test_float_near(m.width, 40.0f, 0.001f));

	XentShapedLine       lines_char [8] = {0};
	XentShapingResult    shaped_char    = {0};
	XentTextShapeRequest shape          = {"ab cd ef", 14.0f, 40.0f, XENT_LINE_BREAK_CHAR_WRAP, XENT_MEASURE_AT_MOST};
	XentTextShapeOutput  shape_char_output = {NULL, 0u, NULL, 0u, lines_char, 8u, &shaped_char};
	TEST_ASSERT(xent_shape_text(ctx, &shape, &shape_char_output));

	XentShapedLine    lines_word [8]      = {0};
	XentShapingResult shaped_word         = {0};
	shape.line_break_policy               = XENT_LINE_BREAK_WORD_WRAP;
	XentTextShapeOutput shape_word_output = {NULL, 0u, NULL, 0u, lines_word, 8u, &shaped_word};
	TEST_ASSERT(xent_shape_text(ctx, &shape, &shape_word_output));

	TEST_ASSERT(shaped_char.line_count == 2u);
	TEST_ASSERT(shaped_word.line_count == 2u);
	TEST_ASSERT(lines_char [0].glyph_count == 5u);
	TEST_ASSERT(lines_word [0].glyph_count == 3u);

	XentShapingResult shaped_exact          = {0};
	shape.line_break_policy                 = XENT_LINE_BREAK_NO_WRAP;
	shape.width_mode                        = XENT_MEASURE_EXACTLY;
	XentTextShapeOutput shaped_exact_output = {0};
	shaped_exact_output.result              = &shaped_exact;
	TEST_ASSERT(xent_shape_text(ctx, &shape, &shaped_exact_output));
	TEST_ASSERT(shaped_exact.line_count == 1u);
	TEST_ASSERT(test_float_near(shaped_exact.metrics.width, 40.0f, 0.001f));

	XentShapedLine    punct_word [8] = {0};
	XentShapingResult punct_result   = {0};
	shape.text                       = "a,bcde";
	shape.width_constraint           = 24.0f;
	shape.line_break_policy          = XENT_LINE_BREAK_WORD_WRAP;
	shape.width_mode                 = XENT_MEASURE_AT_MOST;
	XentTextShapeOutput punct_output = {NULL, 0u, NULL, 0u, punct_word, 8u, &punct_result};
	TEST_ASSERT(xent_shape_text(ctx, &shape, &punct_output));
	TEST_ASSERT(punct_result.line_count >= 2u);
	TEST_ASSERT(punct_word [0].glyph_count == 2u);

	XentNodeId node = xent_create_node(ctx);
	TEST_ASSERT(node != XENT_NODE_INVALID);
	TEST_ASSERT(xent_set_text_line_break_policy(ctx, node, XENT_LINE_BREAK_WORD_WRAP));
	TEST_ASSERT(xent_get_text_line_break_policy(ctx, node) == XENT_LINE_BREAK_WORD_WRAP);

	xent_destroy_context(ctx);
	return 0;
}
