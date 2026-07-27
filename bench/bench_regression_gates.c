#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "xent/xent.h"

typedef struct BenchTree {
	XentNodeId root;
	XentNodeId groups [100];
	XentNodeId leaves [100][100];
} BenchTree;

static double now_ms(void) {
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	return ( double ) ts.tv_sec * 1000.0 + ( double ) ts.tv_nsec / 1000000.0;
}

static void configure_protocol_axis(XentCtx *ctx, XentNodeId node, XentProtocol protocol) {
	if (protocol == XENT_PROTOCOL_FLEX) xent_setflexdir(ctx, node, XENT_FLEX_COLUMN);
	else if (protocol == XENT_PROTOCOL_SWIFTSTACK) xent_stack_setaxis(ctx, node, XENT_AXIS_VERTICAL);
}

static void configure_child_behavior(XentCtx *ctx, XentNodeId child, XentProtocol protocol, uint32_t index) {
	if (protocol == XENT_PROTOCOL_FLEX) {
		xent_setshrink(ctx, child, 1.0f);
		return;
	}

	if (protocol == XENT_PROTOCOL_SWIFTSTACK) xent_stack_setprio(ctx, child, (index % 3u == 0u) ? 1.0f : 0.0f);
}

static XentNodeId build_many_children(XentCtx *ctx, XentProtocol protocol, uint32_t node_count) {
	XentNodeId root = xent_node_create(ctx);
	xent_setproto(ctx, root, protocol);
	xent_setsize(ctx, root, (XentSize) {1200.0f, 720.0f});
	xent_setgap(ctx, root, 1.0f);
	configure_protocol_axis(ctx, root, protocol);

	for (uint32_t i = 0; i < node_count; ++i) {
		XentNodeId child = xent_node_create(ctx);
		xent_setsize(ctx, child, (XentSize) {NAN, 18.0f});
		xent_settext(ctx, child, "node");
		xent_setlinebreak(ctx, child, XENT_LINEBREAK_CHARWRAP);
		configure_child_behavior(ctx, child, protocol, i);
		xent_node_append(ctx, root, child);
	}
	return root;
}

static double run_layout_case_avg_ms(XentProtocol protocol, uint32_t nodes, uint32_t iterations) {
	XentCtx   *ctx  = xent_ctx_create(NULL);
	XentNodeId root = build_many_children(ctx, protocol, nodes);

	xent_layout(ctx, root, 1200.0f, 720.0f);
	double start = now_ms();
	for (uint32_t i = 0; i < iterations; ++i) {
		float width = 1200.0f + ( float ) (i & 1u);
		xent_setsize(ctx, root, (XentSize) {width, 720.0f});
		xent_layout(ctx, root, width, 720.0f);
	}
	double avg = (now_ms() - start) / ( double ) iterations;

	xent_ctx_destroy(ctx);
	return avg;
}

static void build_dirty_tree(XentCtx *ctx, BenchTree *tree) {
	tree->root = xent_node_create(ctx);
	xent_setproto(ctx, tree->root, XENT_PROTOCOL_FLEX);
	xent_setflexdir(ctx, tree->root, XENT_FLEX_COLUMN);
	xent_setsize(ctx, tree->root, (XentSize) {1200.0f, 800.0f});
	xent_setgap(ctx, tree->root, 1.0f);

	for (uint32_t g = 0; g < 100u; ++g) {
		XentNodeId group = xent_node_create(ctx);
		tree->groups [g] = group;
		xent_setproto(ctx, group, XENT_PROTOCOL_FLEX);
		xent_setflexdir(ctx, group, XENT_FLEX_ROW);
		xent_setsize(ctx, group, (XentSize) {1200.0f, 16.0f});
		xent_setgap(ctx, group, 1.0f);
		xent_node_append(ctx, tree->root, group);

		for (uint32_t i = 0; i < 100u; ++i) {
			XentNodeId leaf     = xent_node_create(ctx);
			tree->leaves [g][i] = leaf;
			xent_settext(ctx, leaf, "node");
			xent_setlinebreak(ctx, leaf, XENT_LINEBREAK_CHARWRAP);
			xent_setsize(ctx, leaf, (XentSize) {NAN, 16.0f});
			xent_node_append(ctx, group, leaf);
		}
	}

	xent_layout(ctx, tree->root, 1200.0f, 800.0f);
}

static void
mutate_leafs(XentCtx *ctx, BenchTree const *tree, uint32_t groups_to_touch, uint32_t leaves_per_group, uint32_t tick) {
	char const *text = (tick & 1u) ? "node" : "node_updated";
	for (uint32_t g = 0; g < groups_to_touch; ++g)
		for (uint32_t i = 0; i < leaves_per_group; ++i) xent_settext(ctx, tree->leaves [g][i], text);
}

static double run_dirty_full_avg(
  XentCtx *ctx, BenchTree const *tree, uint32_t groups_to_touch, uint32_t leaves_per_group, uint32_t iterations
) {
	double start = now_ms();
	for (uint32_t it = 0; it < iterations; ++it) {
		mutate_leafs(ctx, tree, groups_to_touch, leaves_per_group, it);
		xent_setsize(ctx, tree->root, (XentSize) {1200.0f, 800.0f});
		xent_layout(ctx, tree->root, 1200.0f, 800.0f);
	}
	return (now_ms() - start) / ( double ) iterations;
}

static double run_dirty_subtree_avg(
  XentCtx *ctx, BenchTree const *tree, uint32_t groups_to_touch, uint32_t leaves_per_group, uint32_t iterations
) {
	double start = now_ms();
	for (uint32_t it = 0; it < iterations; ++it) {
		mutate_leafs(ctx, tree, groups_to_touch, leaves_per_group, it);
		xent_layout(ctx, tree->root, 1200.0f, 800.0f);
	}
	return (now_ms() - start) / ( double ) iterations;
}

static double
measure_dirty_case(bool force_full, uint32_t groups_to_touch, uint32_t leaves_per_group, uint32_t iterations) {
	XentCtx  *ctx  = xent_ctx_create(NULL);
	BenchTree tree = {0};
	build_dirty_tree(ctx, &tree);
	double avg = force_full ? run_dirty_full_avg(ctx, &tree, groups_to_touch, leaves_per_group, iterations)
	                        : run_dirty_subtree_avg(ctx, &tree, groups_to_touch, leaves_per_group, iterations);
	xent_ctx_destroy(ctx);
	return avg;
}

static bool gate_layout_relative_perf(void) {
	double flex_10k  = run_layout_case_avg_ms(XENT_PROTOCOL_FLEX, 10000u, 20u);
	double swift_10k = run_layout_case_avg_ms(XENT_PROTOCOL_SWIFTSTACK, 10000u, 20u);

	double limit     = (flex_10k * 4.0) + 0.25;
	bool   pass      = swift_10k <= limit;
	printf(
	  "[gate] layout-relative swift<=4x-flex+0.25: flex=%.4f swift=%.4f limit=%.4f => %s\n", flex_10k, swift_10k, limit,
	  pass ? "PASS" : "FAIL"
	);
	return pass;
}

static bool gate_dirty_scheduler(void) {
	double leaf_full    = measure_dirty_case(true, 1u, 1u, 60u);
	double leaf_subtree = measure_dirty_case(false, 1u, 1u, 60u);
	double all_full     = measure_dirty_case(true, 100u, 100u, 60u);
	double all_subtree  = measure_dirty_case(false, 100u, 100u, 60u);

	bool   leaf_pass    = leaf_subtree < (leaf_full * 0.80);
	bool   all_pass     = all_subtree < (all_full * 1.25);
	printf(
	  "[gate] dirty-leaf subtree<0.8x full: full=%.4f subtree=%.4f => %s\n", leaf_full, leaf_subtree,
	  leaf_pass ? "PASS" : "FAIL"
	);
	printf(
	  "[gate] dirty-all scheduler<1.25x full: full=%.4f scheduler=%.4f => %s\n", all_full, all_subtree,
	  all_pass ? "PASS" : "FAIL"
	);

	return leaf_pass && all_pass;
}

static bool gate_layout_strategy_transitions(void) {
	XentCtx   *ctx   = xent_ctx_create(NULL);
	XentNodeId root  = xent_node_create(ctx);
	XentNodeId group = xent_node_create(ctx);
	XentNodeId a     = xent_node_create(ctx);
	XentNodeId b     = xent_node_create(ctx);
	XentNodeId tail  = xent_node_create(ctx);

	xent_setproto(ctx, root, XENT_PROTOCOL_FLEX);
	xent_setflexdir(ctx, root, XENT_FLEX_COLUMN);
	xent_setsize(ctx, root, (XentSize) {200.0f, 80.0f});
	xent_setproto(ctx, group, XENT_PROTOCOL_FLEX);
	xent_setflexdir(ctx, group, XENT_FLEX_ROW);
	xent_setsize(ctx, group, (XentSize) {200.0f, 40.0f});
	xent_setsize(ctx, a, (XentSize) {50.0f, 20.0f});
	xent_setsize(ctx, b, (XentSize) {NAN, 20.0f});
	xent_setsize(ctx, tail, (XentSize) {200.0f, 20.0f});
	xent_settext(ctx, b, "node");
	xent_setlinebreak(ctx, b, XENT_LINEBREAK_CHARWRAP);
	xent_node_append(ctx, root, group);
	xent_node_append(ctx, root, tail);
	xent_node_append(ctx, group, a);
	xent_node_append(ctx, group, b);

	xent_layout(ctx, root, 200.0f, 80.0f);
	bool pass_full = xent_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_FULL;

	xent_layout(ctx, root, 200.0f, 80.0f);
	bool pass_none = xent_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_NONE;

	xent_settext(ctx, b, "node2");
	xent_layout(ctx, root, 200.0f, 80.0f);
	bool pass_subtree = xent_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_DIRTY;

	xent_setsize(ctx, root, (XentSize) {220.0f, 80.0f});
	xent_layout(ctx, root, 220.0f, 80.0f);
	bool pass_full_again = xent_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_FULL;

	bool pass            = pass_full && pass_none && pass_subtree && pass_full_again;
	printf("[gate] strategy-transition full->none->subtree->full => %s\n", pass ? "PASS" : "FAIL");

	xent_ctx_destroy(ctx);
	return pass;
}

int main(void) {
	bool ok = true;
	ok      = gate_layout_relative_perf() && ok;
	ok      = gate_dirty_scheduler() && ok;
	ok      = gate_layout_strategy_transitions() && ok;
	return ok ? 0 : 1;
}
