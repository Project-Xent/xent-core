#include "test_common.h"
#include <stdio.h>

static int test_basic_hit_miss(void) {
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

	xent_destroy_context(ctx);
	return 0;
}

static int test_many_unique_keys(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	char buf [64];
	XentTextMetrics m = {0};

	for (int i = 0; i < 200; ++i) {
		snprintf(buf, sizeof(buf), "text entry %d", i);
		XentTextMeasureRequest req = {buf, 14.0f, 300.0f, XENT_LINE_BREAK_CHAR_WRAP, XENT_MEASURE_AT_MOST};
		TEST_ASSERT(xent_measure_text(ctx, &req, &m));
	}

	XentTextCacheStats stats = xent_get_text_cache_stats(ctx);
	TEST_ASSERT(stats.inserts == 200u);
	TEST_ASSERT(stats.misses == 200u);

	for (int i = 0; i < 200; ++i) {
		snprintf(buf, sizeof(buf), "text entry %d", i);
		XentTextMeasureRequest req = {buf, 14.0f, 300.0f, XENT_LINE_BREAK_CHAR_WRAP, XENT_MEASURE_AT_MOST};
		TEST_ASSERT(xent_measure_text(ctx, &req, &m));
	}

	XentTextCacheStats after = xent_get_text_cache_stats(ctx);
	TEST_ASSERT(after.hits == 200u);
	TEST_ASSERT(after.inserts == 200u);

	xent_destroy_context(ctx);
	return 0;
}

static int test_eviction_on_overflow(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	char buf [64];
	XentTextMetrics m = {0};

	for (int i = 0; i < 5000; ++i) {
		snprintf(buf, sizeof(buf), "evict test string number %d", i);
		XentTextMeasureRequest req = {buf, 12.0f, 500.0f, XENT_LINE_BREAK_WORD_WRAP, XENT_MEASURE_AT_MOST};
		TEST_ASSERT(xent_measure_text(ctx, &req, &m));
	}

	XentTextCacheStats stats = xent_get_text_cache_stats(ctx);
	TEST_ASSERT(stats.inserts == 5000u);
	TEST_ASSERT(stats.evictions > 0u);

	xent_destroy_context(ctx);
	return 0;
}

int main(void) {
	XentTestFn tests [] = {test_basic_hit_miss, test_many_unique_keys, test_eviction_on_overflow};
	return test_run_all(tests, sizeof(tests) / sizeof(tests [0]));
}
