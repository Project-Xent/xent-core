#include "test_common.h"

#include "xent/xent_measure.h"

typedef struct {
	float           w, h;
	uint32_t        calls;
	XentMeasureMode last_wm;
	XentMeasureMode last_hm;
	float           last_avail_w;
	float           last_avail_h;
} MeasureProbe;

static bool probe_measure(void *userdata, XentExtMeasureReq const *request, XentSize *out_size) {
	MeasureProbe *p = ( MeasureProbe * ) userdata;
	p->calls++;
	p->last_wm      = request->width_mode;
	p->last_hm      = request->height_mode;
	p->last_avail_w = request->available_w;
	p->last_avail_h = request->available_h;
	out_size->w     = p->w;
	out_size->h     = p->h;
	return true;
}

static int test_both_axes_auto(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root = xent_node_create(ctx);
	XentNodeId leaf = xent_node_create(ctx);
	TEST_ASSERT(xent_node_append(ctx, root, leaf));
	TEST_ASSERT(xent_setproto(ctx, root, XENT_PROTOCOL_FLEX));
	TEST_ASSERT(xent_setitems(ctx, root, XENT_FLEX_ALIGN_START));
	TEST_ASSERT(xent_setsize(ctx, root, (XentSize) {200.0f, 100.0f}));

	MeasureProbe probe = {.w = 64.0f, .h = 48.0f};
	TEST_ASSERT(xent_node_setmeasure(ctx, leaf, probe_measure, &probe));
	TEST_ASSERT(xent_layout(ctx, root, 200.0f, 100.0f));

	XentRect rect = {0};
	TEST_ASSERT(xent_layout_rect(ctx, leaf, &rect));
	TEST_ASSERT(rect.w == 64.0f && rect.h == 48.0f);
	TEST_ASSERT(probe.calls > 0u);

	XentNodeId stale = leaf;
	TEST_ASSERT(xent_node_destroy(ctx, leaf));
	TEST_ASSERT(!xent_node_hasmeasure(ctx, stale));
	TEST_ASSERT(!xent_node_setmeasure(ctx, stale, probe_measure, &probe));

	xent_ctx_destroy(ctx);
	return 0;
}

static int test_width_fixed_height_auto(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root = xent_node_create(ctx);
	XentNodeId leaf = xent_node_create(ctx);
	TEST_ASSERT(xent_node_append(ctx, root, leaf));
	TEST_ASSERT(xent_setproto(ctx, root, XENT_PROTOCOL_FLEX));
	TEST_ASSERT(xent_setitems(ctx, root, XENT_FLEX_ALIGN_START));
	TEST_ASSERT(xent_setsize(ctx, root, (XentSize) {200.0f, 100.0f}));
	TEST_ASSERT(xent_setsize(ctx, leaf, (XentSize) {80.0f, NAN}));

	MeasureProbe probe = {.w = 999.0f, .h = 36.0f};
	TEST_ASSERT(xent_node_setmeasure(ctx, leaf, probe_measure, &probe));
	TEST_ASSERT(xent_layout(ctx, root, 200.0f, 100.0f));

	TEST_ASSERT(probe.calls > 0u);
	TEST_ASSERT(probe.last_wm == XENT_MEASURE_EXACTLY);
	TEST_ASSERT(probe.last_hm == XENT_MEASURE_AT_MOST || probe.last_hm == XENT_MEASURE_UNDEFINED);
	TEST_ASSERT(probe.last_avail_w == 80.0f);

	XentRect rect = {0};
	TEST_ASSERT(xent_layout_rect(ctx, leaf, &rect));
	TEST_ASSERT(rect.w == 80.0f);
	TEST_ASSERT(rect.h == 36.0f);

	xent_ctx_destroy(ctx);
	return 0;
}

static int test_height_fixed_width_auto(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root = xent_node_create(ctx);
	XentNodeId leaf = xent_node_create(ctx);
	TEST_ASSERT(xent_node_append(ctx, root, leaf));
	TEST_ASSERT(xent_setproto(ctx, root, XENT_PROTOCOL_FLEX));
	TEST_ASSERT(xent_setflexdir(ctx, root, XENT_FLEX_COLUMN));
	TEST_ASSERT(xent_setitems(ctx, root, XENT_FLEX_ALIGN_START));
	TEST_ASSERT(xent_setsize(ctx, root, (XentSize) {200.0f, 100.0f}));
	TEST_ASSERT(xent_setsize(ctx, leaf, (XentSize) {NAN, 50.0f}));

	MeasureProbe probe = {.w = 72.0f, .h = 999.0f};
	TEST_ASSERT(xent_node_setmeasure(ctx, leaf, probe_measure, &probe));
	TEST_ASSERT(xent_layout(ctx, root, 200.0f, 100.0f));

	TEST_ASSERT(probe.calls > 0u);
	TEST_ASSERT(probe.last_hm == XENT_MEASURE_EXACTLY);
	TEST_ASSERT(probe.last_wm == XENT_MEASURE_AT_MOST || probe.last_wm == XENT_MEASURE_UNDEFINED);
	TEST_ASSERT(probe.last_avail_h == 50.0f);

	XentRect rect = {0};
	TEST_ASSERT(xent_layout_rect(ctx, leaf, &rect));
	TEST_ASSERT(rect.h == 50.0f);
	TEST_ASSERT(rect.w == 72.0f);

	xent_ctx_destroy(ctx);
	return 0;
}

int main(void) {
	XentTestFn const tests [] = {
	  test_both_axes_auto,
	  test_width_fixed_height_auto,
	  test_height_fixed_width_auto,
	};
	return test_run_all(tests, sizeof(tests) / sizeof(tests [0]));
}
