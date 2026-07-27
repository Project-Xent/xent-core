#include "test_common.h"

static bool
fake_measure(XentTextBackend const *backend, XentTextMeasureReq const *request, XentTextMetrics *out_metrics) {
	( void ) backend;
	out_metrics->width      = (request->width_mode == XENT_MEASURE_EXACTLY) ? request->width_constraint : 10.0f;
	out_metrics->height     = 20.0f;
	out_metrics->line_count = 1u;
	return true;
}

int main(void) {
	XentTextBackend invalid_missing_name    = {0};
	invalid_missing_name.measure            = fake_measure;

	XentTextBackend invalid_missing_measure = {"invalid", NULL, NULL};
	XentTextBackend valid                   = {"fake_backend", fake_measure, NULL};

	TEST_ASSERT(!xent_text_backend_valid(NULL));
	TEST_ASSERT(!xent_text_backend_valid(&invalid_missing_name));
	TEST_ASSERT(!xent_text_backend_valid(&invalid_missing_measure));
	TEST_ASSERT(xent_text_backend_valid(&valid));

	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentTextBackend const *original = xent_text_backend(ctx);
	TEST_ASSERT(original != NULL);

	TEST_ASSERT(!xent_text_setbackend(ctx, &invalid_missing_measure));
	TEST_ASSERT(xent_text_backend(ctx) == original);

	TEST_ASSERT(xent_text_setbackend(ctx, &valid));
	TEST_ASSERT(xent_text_backend(ctx) == &valid);

	XentTextMetrics    metrics = {0};
	XentTextMeasureReq request = {"hello", 14.0f, 0u, 33.0f, XENT_LINEBREAK_NOWRAP, XENT_MEASURE_EXACTLY};
	TEST_ASSERT(xent_text_measure(ctx, &request, &metrics));
	TEST_ASSERT(test_float_near(metrics.width, 33.0f, 0.001f));
	TEST_ASSERT(test_float_near(metrics.height, 20.0f, 0.001f));

	TEST_ASSERT(xent_text_setbackend(ctx, NULL));
	TEST_ASSERT(xent_text_backend(ctx) != NULL);

	xent_ctx_destroy(ctx);
	return 0;
}
