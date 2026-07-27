#include "test_common.h"

int main(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentTextMetrics    m       = {0};
	XentTextMeasureReq measure = {"ab cd ef", 14.0f, 0u, 40.0f, XENT_LINEBREAK_NOWRAP, XENT_MEASURE_AT_MOST};
	TEST_ASSERT(xent_text_measure(ctx, &measure, &m));
	TEST_ASSERT(m.line_count == 1u);
	TEST_ASSERT(test_float_near(m.width, 64.0f, 0.001f));

	measure.line_break_policy = XENT_LINEBREAK_CHARWRAP;
	TEST_ASSERT(xent_text_measure(ctx, &measure, &m));
	TEST_ASSERT(m.line_count == 2u);
	TEST_ASSERT(test_float_near(m.width, 40.0f, 0.001f));

	measure.width_constraint = 16.0f;
	measure.width_mode       = XENT_MEASURE_UNDEFINED;
	TEST_ASSERT(xent_text_measure(ctx, &measure, &m));
	TEST_ASSERT(m.line_count == 1u);
	TEST_ASSERT(test_float_near(m.width, 64.0f, 0.001f));

	measure.width_constraint  = 40.0f;
	measure.line_break_policy = XENT_LINEBREAK_NOWRAP;
	measure.width_mode        = XENT_MEASURE_EXACTLY;
	TEST_ASSERT(xent_text_measure(ctx, &measure, &m));
	TEST_ASSERT(m.line_count == 1u);
	TEST_ASSERT(test_float_near(m.width, 40.0f, 0.001f));

	measure.width_constraint  = INFINITY;
	measure.line_break_policy = XENT_LINEBREAK_WORDWRAP;
	measure.width_mode        = XENT_MEASURE_MIN_CONTENT;
	TEST_ASSERT(xent_text_measure(ctx, &measure, &m));
	TEST_ASSERT(test_float_near(m.width, 24.0f, 0.001f));
	TEST_ASSERT(m.line_count == 3u);

	measure.line_break_policy = XENT_LINEBREAK_CHARWRAP;
	TEST_ASSERT(xent_text_measure(ctx, &measure, &m));
	TEST_ASSERT(test_float_near(m.width, 8.0f, 0.001f));
	TEST_ASSERT(m.line_count == 8u);

	measure.line_break_policy = XENT_LINEBREAK_NOWRAP;
	TEST_ASSERT(xent_text_measure(ctx, &measure, &m));
	TEST_ASSERT(test_float_near(m.width, 64.0f, 0.001f));
	TEST_ASSERT(m.line_count == 1u);

	XentNodeId node = xent_node_create(ctx);
	TEST_ASSERT(node != XENT_NODE_INVALID);
	TEST_ASSERT(xent_setlinebreak(ctx, node, XENT_LINEBREAK_WORDWRAP));
	TEST_ASSERT(xent_linebreak(ctx, node) == XENT_LINEBREAK_WORDWRAP);

	xent_ctx_destroy(ctx);
	return 0;
}
