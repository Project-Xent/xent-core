#include <math.h>
#include <stdio.h>
#include <time.h>

#include "xent/xent.h"

static double now_ms(void) { return ( double ) clock() * 1000.0 / ( double ) CLOCKS_PER_SEC; }

static void   configure_protocol_axis(XentContext *ctx, XentNodeId node, XentProtocol protocol) {
	if (protocol == XENT_PROTOCOL_FLEX) xent_set_flex_direction(ctx, node, XENT_FLEX_COLUMN);
	else if (protocol == XENT_PROTOCOL_SWIFTSTACK) xent_set_stack_axis(ctx, node, XENT_AXIS_VERTICAL);
}

static void configure_child_behavior(XentContext *ctx, XentNodeId child, XentProtocol protocol, uint32_t index) {
	if (protocol == XENT_PROTOCOL_FLEX) {
		xent_set_flex_shrink(ctx, child, 1.0f);
		return;
	}

	if (protocol == XENT_PROTOCOL_SWIFTSTACK) xent_set_layout_priority(ctx, child, (index % 3 == 0) ? 1.0f : 0.0f);
}

static XentNodeId build_many_children(XentContext *ctx, XentProtocol protocol, uint32_t node_count) {
	XentNodeId root = xent_create_node(ctx);
	xent_set_protocol(ctx, root, protocol);
	xent_set_size(ctx, root, (XentSize) {1200.0f, 720.0f});
	xent_set_gap(ctx, root, 1.0f);
	configure_protocol_axis(ctx, root, protocol);

	for (uint32_t i = 0; i < node_count; ++i) {
		XentNodeId child = xent_create_node(ctx);
		xent_set_size(ctx, child, (XentSize) {NAN, 18.0f});
		xent_set_text(ctx, child, "node");
		configure_child_behavior(ctx, child, protocol, i);
		xent_append_child(ctx, root, child);
	}
	return root;
}

static void run_case(char const *name, XentProtocol protocol, uint32_t nodes) {
	XentContext *ctx        = xent_create_context(NULL);
	XentNodeId   root       = build_many_children(ctx, protocol, nodes);

	int const    iterations = 20;
	xent_profile_reset(ctx);
	double start = now_ms();
	for (int i = 0; i < iterations; ++i) xent_layout(ctx, root, 1200.0f, 720.0f);
	double elapsed = now_ms() - start;
	printf(
	  "%s nodes=%u iterations=%d total_ms=%.3f avg_ms=%.3f\n", name, nodes, iterations, elapsed,
	  elapsed / ( double ) iterations
	);

	XentProfileStats p = xent_profile_get(ctx);
	printf(
	  "  profile swiftstack=%.3fms flex=%.3fms grid=%.3fms text_calls=%llu baseline_fallbacks=%llu\n",
	  p.swiftstack_total_ms, p.flex_total_ms, p.grid_total_ms, ( unsigned long long ) p.text_measure_calls,
	  ( unsigned long long ) p.text_baseline_fallbacks
	);

	xent_destroy_context(ctx);
}

int main(void) {
	printf("runtime simd=%s\n", xent_is_simd_enabled() ? "enabled" : "disabled");
	uint32_t sizes [] = {100u, 1000u, 10000u};
	for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes [0]); ++i) {
		run_case("xent_flex", XENT_PROTOCOL_FLEX, sizes [i]);
		run_case("xent_swiftstack", XENT_PROTOCOL_SWIFTSTACK, sizes [i]);
	}
	return 0;
}
