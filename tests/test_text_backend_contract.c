#include "test_common.h"

static bool fake_measure(const XentTextBackend *backend,
                         const char *text,
                         float font_size,
                         float width_constraint,
                         XentLineBreakPolicy line_break_policy,
                         XentMeasureMode width_mode,
                         XentTextMetrics *out_metrics) {
    (void)backend;
    (void)text;
    (void)font_size;
    (void)line_break_policy;
    out_metrics->width = (width_mode == XENT_MEASURE_EXACTLY) ? width_constraint : 10.0f;
    out_metrics->height = 20.0f;
    out_metrics->line_count = 1u;
    return true;
}

static bool fake_shape(const XentTextBackend *backend,
                       const char *text,
                       float font_size,
                       float width_constraint,
                       XentLineBreakPolicy line_break_policy,
                       XentMeasureMode width_mode,
                       XentShapedGlyph *out_glyphs,
                       uint32_t glyph_capacity,
                       XentShapedRun *out_runs,
                       uint32_t run_capacity,
                       XentShapedLine *out_lines,
                       uint32_t line_capacity,
                       XentShapingResult *out_result) {
    (void)backend;
    (void)text;
    (void)font_size;
    (void)line_break_policy;
    (void)out_glyphs;
    (void)glyph_capacity;
    (void)out_runs;
    (void)run_capacity;
    (void)out_lines;
    (void)line_capacity;

    out_result->metrics.width = (width_mode == XENT_MEASURE_EXACTLY) ? width_constraint : 10.0f;
    out_result->metrics.height = 20.0f;
    out_result->metrics.line_count = 1u;
    out_result->glyph_count = 0u;
    out_result->run_count = 0u;
    out_result->line_count = 1u;
    out_result->truncated = false;
    return true;
}

int main(void) {
    XentTextBackend invalid_missing_name = {0};
    invalid_missing_name.measure = fake_measure;
    invalid_missing_name.shape = fake_shape;

    XentTextBackend invalid_missing_measure = {"invalid", NULL, fake_shape, NULL};
    XentTextBackend invalid_missing_shape = {"invalid", fake_measure, NULL, NULL};
    XentTextBackend valid = {"fake_backend", fake_measure, fake_shape, NULL};

    TEST_ASSERT(!xent_validate_text_backend(NULL));
    TEST_ASSERT(!xent_validate_text_backend(&invalid_missing_name));
    TEST_ASSERT(!xent_validate_text_backend(&invalid_missing_measure));
    TEST_ASSERT(!xent_validate_text_backend(&invalid_missing_shape));
    TEST_ASSERT(xent_validate_text_backend(&valid));

    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    const XentTextBackend *original = xent_get_text_backend(ctx);
    TEST_ASSERT(original != NULL);

    TEST_ASSERT(!xent_set_text_backend(ctx, &invalid_missing_measure));
    TEST_ASSERT(xent_get_text_backend(ctx) == original);

    TEST_ASSERT(xent_set_text_backend(ctx, &valid));
    TEST_ASSERT(xent_get_text_backend(ctx) == &valid);

    XentTextMetrics metrics = {0};
    TEST_ASSERT(xent_measure_text(
        ctx, "hello", 14.0f, 33.0f, XENT_LINE_BREAK_NO_WRAP, XENT_MEASURE_EXACTLY, &metrics));
    TEST_ASSERT(test_float_near(metrics.width, 33.0f, 0.001f));
    TEST_ASSERT(test_float_near(metrics.height, 20.0f, 0.001f));

    TEST_ASSERT(xent_set_text_backend(ctx, NULL));
    TEST_ASSERT(xent_get_text_backend(ctx) != NULL);

    xent_destroy_context(ctx);
    return 0;
}
