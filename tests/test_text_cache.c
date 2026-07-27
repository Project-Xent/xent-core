#include "test_common.h"

static int test_basic_hit_miss(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentTextMetrics    m1      = {0};
	XentTextMetrics    m2      = {0};
	XentTextMeasureReq request = {"cache me", 14.0f, 0u, 300.0f, XENT_LINEBREAK_CHARWRAP, XENT_MEASURE_AT_MOST};

	TEST_ASSERT(xent_text_measure(ctx, &request, &m1));
	TEST_ASSERT(xent_text_measure(ctx, &request, &m2));
	TEST_ASSERT(test_float_near(m1.width, m2.width, 0.001f));
	TEST_ASSERT(test_float_near(m1.height, m2.height, 0.001f));

	XentTextCacheStats stats = xent_text_stats(ctx);
	TEST_ASSERT(stats.misses >= 1u);
	TEST_ASSERT(stats.hits >= 1u);
	TEST_ASSERT(stats.inserts >= 1u);

	request.width_mode = XENT_MEASURE_UNDEFINED;
	TEST_ASSERT(xent_text_measure(ctx, &request, &m2));
	XentTextCacheStats after_mode_change = xent_text_stats(ctx);
	TEST_ASSERT(after_mode_change.misses > stats.misses);

	xent_ctx_destroy(ctx);
	return 0;
}

static int test_mono_font_size_key_normalized(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentTextMetrics    m1      = {0};
	XentTextMetrics    m2      = {0};
	XentTextMeasureReq request = {"same mono width", 14.0f, 0u, 300.0f, XENT_LINEBREAK_CHARWRAP, XENT_MEASURE_AT_MOST};

	TEST_ASSERT(xent_text_measure(ctx, &request, &m1));
	XentTextCacheStats before_font_change = xent_text_stats(ctx);
	request.font_size                     = 28.0f;
	TEST_ASSERT(xent_text_measure(ctx, &request, &m2));
	XentTextCacheStats after_font_change = xent_text_stats(ctx);

	TEST_ASSERT(test_float_near(m1.width, m2.width, 0.001f));
	TEST_ASSERT(after_font_change.hits > before_font_change.hits);

	xent_ctx_destroy(ctx);
	return 0;
}

static int test_many_unique_keys(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	char buf [64];
	for (uint32_t i = 0u; i < 1024u; ++i) {
		snprintf(buf, sizeof(buf), "cache-key-%u", i);
		XentTextMetrics    metrics = {0};
		XentTextMeasureReq request = {buf, 14.0f, 0u, 300.0f, XENT_LINEBREAK_CHARWRAP, XENT_MEASURE_AT_MOST};
		TEST_ASSERT(xent_text_measure(ctx, &request, &metrics));
	}

	XentTextCacheStats stats = xent_text_stats(ctx);
	TEST_ASSERT(stats.inserts == 1024u);
	TEST_ASSERT(stats.evictions == 0u);

	snprintf(buf, sizeof(buf), "cache-key-%u", 777u);
	XentTextMetrics    metrics = {0};
	XentTextMeasureReq request = {buf, 14.0f, 0u, 300.0f, XENT_LINEBREAK_CHARWRAP, XENT_MEASURE_AT_MOST};
	TEST_ASSERT(xent_text_measure(ctx, &request, &metrics));
	XentTextCacheStats after_hit = xent_text_stats(ctx);
	TEST_ASSERT(after_hit.hits > stats.hits);

	xent_ctx_destroy(ctx);
	return 0;
}

static int test_eviction_on_overflow(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	char buf [64];
	for (uint32_t i = 0u; i < 5000u; ++i) {
		snprintf(buf, sizeof(buf), "overflow-key-%u", i);
		XentTextMetrics    metrics = {0};
		XentTextMeasureReq request = {buf, 14.0f, 0u, 300.0f, XENT_LINEBREAK_CHARWRAP, XENT_MEASURE_AT_MOST};
		TEST_ASSERT(xent_text_measure(ctx, &request, &metrics));
	}

	XentTextCacheStats stats = xent_text_stats(ctx);
	TEST_ASSERT(stats.inserts == 5000u);
	TEST_ASSERT(stats.evictions > 0u);

	xent_ctx_destroy(ctx);
	return 0;
}

int main(void) {
	XentTestFn tests [] = {
	  test_basic_hit_miss,
	  test_mono_font_size_key_normalized,
	  test_many_unique_keys,
	  test_eviction_on_overflow,
	};
	return test_run_all(tests, sizeof(tests) / sizeof(tests [0]));
}
