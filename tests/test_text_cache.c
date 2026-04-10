#include "test_common.h"

int main(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentTextMetrics m1 = {0};
    XentTextMetrics m2 = {0};

    TEST_ASSERT(xent_measure_text(
        ctx, "cache me", 14.0f, 300.0f, XENT_LINE_BREAK_CHAR_WRAP, XENT_MEASURE_AT_MOST, &m1));
    TEST_ASSERT(xent_measure_text(
        ctx, "cache me", 14.0f, 300.0f, XENT_LINE_BREAK_CHAR_WRAP, XENT_MEASURE_AT_MOST, &m2));
    TEST_ASSERT(test_float_near(m1.width, m2.width, 0.001f));
    TEST_ASSERT(test_float_near(m1.height, m2.height, 0.001f));

    XentTextCacheStats stats = xent_get_text_cache_stats(ctx);
    TEST_ASSERT(stats.misses >= 1u);
    TEST_ASSERT(stats.hits >= 1u);
    TEST_ASSERT(stats.inserts >= 1u);

    TEST_ASSERT(xent_measure_text(
        ctx, "cache me", 14.0f, 300.0f, XENT_LINE_BREAK_CHAR_WRAP, XENT_MEASURE_UNDEFINED, &m2));
    XentTextCacheStats after_mode_change = xent_get_text_cache_stats(ctx);
    TEST_ASSERT(after_mode_change.misses > stats.misses);

    xent_destroy_context(ctx);
    return 0;
}
