#include "test_common.h"

static bool
fake_measure(XentTextBackend const *backend, XentTextMeasureRequest const *request, XentTextMetrics *out_metrics) {
	( void ) backend;
	out_metrics->width      = (request->width_mode == XENT_MEASURE_EXACTLY) ? request->width_constraint : 10.0f;
	out_metrics->height     = 20.0f;
	out_metrics->line_count = 1u;
	return true;
}

static bool
fake_shape(XentTextBackend const *backend, XentTextShapeRequest const *request, XentTextShapeOutput const *output) {
	( void ) backend;

	output->result->metrics.width  = (request->width_mode == XENT_MEASURE_EXACTLY) ? request->width_constraint : 10.0f;
	output->result->metrics.height = 20.0f;
	output->result->metrics.line_count = 1u;
	output->result->glyph_count        = 0u;
	output->result->run_count          = 0u;
	output->result->line_count         = 1u;
	output->result->truncated          = false;
	return true;
}

int main(void) {
	XentTextBackend invalid_missing_name    = {0};
	invalid_missing_name.measure            = fake_measure;
	invalid_missing_name.shape              = fake_shape;

	XentTextBackend invalid_missing_measure = {"invalid", NULL, fake_shape, NULL};
	XentTextBackend invalid_missing_shape   = {"invalid", fake_measure, NULL, NULL};
	XentTextBackend valid                   = {"fake_backend", fake_measure, fake_shape, NULL};

	TEST_ASSERT(!xent_validate_text_backend(NULL));
	TEST_ASSERT(!xent_validate_text_backend(&invalid_missing_name));
	TEST_ASSERT(!xent_validate_text_backend(&invalid_missing_measure));
	TEST_ASSERT(!xent_validate_text_backend(&invalid_missing_shape));
	TEST_ASSERT(xent_validate_text_backend(&valid));

	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentTextBackend const *original = xent_get_text_backend(ctx);
	TEST_ASSERT(original != NULL);

	TEST_ASSERT(!xent_set_text_backend(ctx, &invalid_missing_measure));
	TEST_ASSERT(xent_get_text_backend(ctx) == original);

	TEST_ASSERT(xent_set_text_backend(ctx, &valid));
	TEST_ASSERT(xent_get_text_backend(ctx) == &valid);

	XentTextMetrics        metrics = {0};
	XentTextMeasureRequest request = {"hello", 14.0f, 0u, 33.0f, XENT_LINE_BREAK_NO_WRAP, XENT_MEASURE_EXACTLY};
	TEST_ASSERT(xent_measure_text(ctx, &request, &metrics));
	TEST_ASSERT(test_float_near(metrics.width, 33.0f, 0.001f));
	TEST_ASSERT(test_float_near(metrics.height, 20.0f, 0.001f));

	TEST_ASSERT(xent_set_text_backend(ctx, NULL));
	TEST_ASSERT(xent_get_text_backend(ctx) != NULL);

	xent_destroy_context(ctx);
	return 0;
}
