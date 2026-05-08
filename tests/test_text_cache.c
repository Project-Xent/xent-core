#include "test_common.h"

int main(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentTextMetrics        m1      = {0};
	XentTextMetrics        m2      = {0};
	XentTextMeasureRequest request = {"cache me", 14.0f, 300.0f, XENT_LINE_BREAK_CHAR_WRAP, XENT_MEASURE_AT_MOST};

	TEST_ASSERT(xent_measure_text(ctx, &request, &m1));
	TEST_ASSERT(xent_measure_text(ctx, &request, &m2));
	TEST_ASSERT(test_float_near(m1.width, m2.width, 0.001f));
	TEST_ASSERT(test_float_near(m1.height, m2.height, 0.001f));

	XentTextCacheStats stats = xent_get_text_cache_stats(ctx);
	TEST_ASSERT(stats.misses >= 1u);
	TEST_ASSERT(stats.hits >= 1u);
	TEST_ASSERT(stats.inserts >= 1u);

	request.width_mode = XENT_MEASURE_UNDEFINED;
	TEST_ASSERT(xent_measure_text(ctx, &request, &m2));
	XentTextCacheStats after_mode_change = xent_get_text_cache_stats(ctx);
	TEST_ASSERT(after_mode_change.misses > stats.misses);

	request.width_mode = XENT_MEASURE_AT_MOST;
	request.font_size  = 14.0f;
	TEST_ASSERT(xent_measure_text(ctx, &request, &m1));
	XentTextCacheStats before_font_change = xent_get_text_cache_stats(ctx);
	request.font_size                     = 28.0f;
	TEST_ASSERT(xent_measure_text(ctx, &request, &m2));
	XentTextCacheStats after_font_change = xent_get_text_cache_stats(ctx);
	TEST_ASSERT(test_float_near(m1.width, m2.width, 0.001f));
	TEST_ASSERT(after_font_change.hits > before_font_change.hits);

	xent_destroy_context(ctx);
	return 0;
}
