#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "xent/xent.h"

#define YOGA_NATIVE_REPETITIONS 1000u

static double yoga_native_now_ms(void) { return ( double ) clock() * 1000.0 / ( double ) CLOCKS_PER_SEC; }

static bool   yoga_native_measure_text(
  XentTextBackend const *backend, XentTextMeasureReq const *request, XentTextMetrics *out_metrics
) {
	( void ) backend;
	float width             = request->width_mode == XENT_MEASURE_UNDEFINED ? 10.0f : request->width_constraint;
	out_metrics->width      = width;
	out_metrics->height     = 10.0f;
	out_metrics->line_count = 1u;
	return true;
}

static XentTextBackend const yoga_native_text_backend = {
  "yoga-native-measure",
  yoga_native_measure_text,
  NULL,
};

static void yoga_native_print(char const *name, double total_ms) {
	printf("%s: avg: %.6f ms total: %.3f ms\n", name, total_ms / ( double ) YOGA_NATIVE_REPETITIONS, total_ms);
}

static XentNodeId yoga_native_flex_node(XentCtx *ctx) {
	XentNodeId node = xent_node_create(ctx);
	xent_setproto(ctx, node, XENT_PROTOCOL_FLEX);
	xent_setflexdir(ctx, node, XENT_FLEX_COLUMN);
	xent_setitems(ctx, node, XENT_FLEX_ALIGN_STRETCH);
	return node;
}

typedef void  (*YogaNativeBuildFn)(XentCtx *ctx, XentNodeId root);

static double yoga_native_run(YogaNativeBuildFn build, uint32_t expected_nodes) {
	double start = yoga_native_now_ms();
	for (uint32_t rep = 0u; rep < YOGA_NATIVE_REPETITIONS; ++rep) {
		XentCtx *ctx = xent_ctx_create(NULL);
		xent_text_setbackend(ctx, &yoga_native_text_backend);
		xent_node_reserve(ctx, expected_nodes);
		XentNodeId root = yoga_native_flex_node(ctx);
		build(ctx, root);
		xent_layout(ctx, root, NAN, NAN);
		xent_ctx_destroy(ctx);
	}
	return yoga_native_now_ms() - start;
}

static void yoga_native_append_flex_text(XentCtx *ctx, XentNodeId parent, uint32_t count) {
	for (uint32_t i = 0u; i < count; ++i) {
		XentNodeId child = yoga_native_flex_node(ctx);
		xent_settext(ctx, child, "measure");
		xent_setgrow(ctx, child, 1.0f);
		xent_node_append(ctx, parent, child);
	}
}

static void yoga_native_append_stretch_text(XentCtx *ctx, XentNodeId parent, uint32_t count) {
	for (uint32_t i = 0u; i < count; ++i) {
		XentNodeId child = yoga_native_flex_node(ctx);
		xent_settext(ctx, child, "measure");
		xent_setsize(ctx, child, (XentSize) {NAN, 20.0f});
		xent_node_append(ctx, parent, child);
	}
}

static void yoga_native_build_stack_with_flex(XentCtx *ctx, XentNodeId root) {
	xent_setsize(ctx, root, (XentSize) {100.0f, 100.0f});
	yoga_native_append_flex_text(ctx, root, 10u);
}

static void yoga_native_build_align_stretch_undefined_axis(XentCtx *ctx, XentNodeId root) {
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_STRETCH);
	yoga_native_append_stretch_text(ctx, root, 10u);
}

static void yoga_native_append_nested_child(XentCtx *ctx, XentNodeId root) {
	XentNodeId child = yoga_native_flex_node(ctx);
	xent_setgrow(ctx, child, 1.0f);
	xent_node_append(ctx, root, child);
	yoga_native_append_flex_text(ctx, child, 10u);
}

static void yoga_native_build_nested_flex(XentCtx *ctx, XentNodeId root) {
	for (uint32_t i = 0u; i < 10u; ++i) yoga_native_append_nested_child(ctx, root);
}

static XentNodeId yoga_native_sized_grow_node(XentCtx *ctx, XentNodeId parent) {
	XentNodeId node = yoga_native_flex_node(ctx);
	xent_setsize(ctx, node, (XentSize) {10.0f, 10.0f});
	xent_setgrow(ctx, node, 1.0f);
	xent_node_append(ctx, parent, node);
	return node;
}

static void yoga_native_append_huge_leaves(XentCtx *ctx, XentNodeId great) {
	for (uint32_t n = 0u; n < 10u; ++n) {
		XentNodeId leaf = yoga_native_sized_grow_node(ctx, great);
		xent_setflexdir(ctx, leaf, XENT_FLEX_ROW);
	}
}

static void yoga_native_append_huge_great_nodes(XentCtx *ctx, XentNodeId grandchild) {
	for (uint32_t k = 0u; k < 10u; ++k) {
		XentNodeId great = yoga_native_sized_grow_node(ctx, grandchild);
		yoga_native_append_huge_leaves(ctx, great);
	}
}

static void yoga_native_append_huge_grandchildren(XentCtx *ctx, XentNodeId child) {
	for (uint32_t j = 0u; j < 10u; ++j) {
		XentNodeId grandchild = yoga_native_sized_grow_node(ctx, child);
		xent_setflexdir(ctx, grandchild, XENT_FLEX_ROW);
		yoga_native_append_huge_great_nodes(ctx, grandchild);
	}
}

static void yoga_native_build_huge_nested_layout(XentCtx *ctx, XentNodeId root) {
	for (uint32_t i = 0u; i < 10u; ++i) {
		XentNodeId child = yoga_native_sized_grow_node(ctx, root);
		yoga_native_append_huge_grandchildren(ctx, child);
	}
}

int main(void) {
	yoga_native_print("Stack with flex", yoga_native_run(yoga_native_build_stack_with_flex, 11u));
	yoga_native_print(
	  "Align stretch in undefined axis", yoga_native_run(yoga_native_build_align_stretch_undefined_axis, 11u)
	);
	yoga_native_print("Nested flex", yoga_native_run(yoga_native_build_nested_flex, 111u));
	yoga_native_print("Huge nested layout", yoga_native_run(yoga_native_build_huge_nested_layout, 11111u));
	return 0;
}
