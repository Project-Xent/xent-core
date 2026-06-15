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

static void configure_protocol_axis(XentContext *ctx, XentNodeId node, XentProtocol protocol) {
	if (protocol == XENT_PROTOCOL_FLEX) xent_set_flex_direction(ctx, node, XENT_FLEX_COLUMN);
	else if (protocol == XENT_PROTOCOL_SWIFTSTACK) xent_set_stack_axis(ctx, node, XENT_AXIS_VERTICAL);
}

static void configure_child_behavior(XentContext *ctx, XentNodeId child, XentProtocol protocol, uint32_t index) {
	if (protocol == XENT_PROTOCOL_FLEX) {
		xent_set_flex_shrink(ctx, child, 1.0f);
		return;
	}

	if (protocol == XENT_PROTOCOL_SWIFTSTACK) xent_set_layout_priority(ctx, child, (index % 3u == 0u) ? 1.0f : 0.0f);
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
		xent_set_text_line_break_policy(ctx, child, XENT_LINE_BREAK_CHAR_WRAP);
		configure_child_behavior(ctx, child, protocol, i);
		xent_append_child(ctx, root, child);
	}
	return root;
}

static double run_layout_case_avg_ms(XentProtocol protocol, uint32_t nodes, uint32_t iterations) {
	XentContext *ctx   = xent_create_context(NULL);
	XentNodeId   root  = build_many_children(ctx, protocol, nodes);

	xent_layout(ctx, root, 1200.0f, 720.0f);
	double       start = now_ms();
	for (uint32_t i = 0; i < iterations; ++i) {
		float width = 1200.0f + ( float ) (i & 1u);
		xent_set_size(ctx, root, (XentSize) {width, 720.0f});
		xent_layout(ctx, root, width, 720.0f);
	}
	double avg = (now_ms() - start) / ( double ) iterations;

	xent_destroy_context(ctx);
	return avg;
}

static void build_dirty_tree(XentContext *ctx, BenchTree *tree) {
	tree->root = xent_create_node(ctx);
	xent_set_protocol(ctx, tree->root, XENT_PROTOCOL_FLEX);
	xent_set_flex_direction(ctx, tree->root, XENT_FLEX_COLUMN);
	xent_set_size(ctx, tree->root, (XentSize) {1200.0f, 800.0f});
	xent_set_gap(ctx, tree->root, 1.0f);

	for (uint32_t g = 0; g < 100u; ++g) {
		XentNodeId group = xent_create_node(ctx);
		tree->groups [g] = group;
		xent_set_protocol(ctx, group, XENT_PROTOCOL_FLEX);
		xent_set_flex_direction(ctx, group, XENT_FLEX_ROW);
		xent_set_size(ctx, group, (XentSize) {1200.0f, 16.0f});
		xent_set_gap(ctx, group, 1.0f);
		xent_append_child(ctx, tree->root, group);

		for (uint32_t i = 0; i < 100u; ++i) {
			XentNodeId leaf     = xent_create_node(ctx);
			tree->leaves [g][i] = leaf;
			xent_set_text(ctx, leaf, "node");
			xent_set_text_line_break_policy(ctx, leaf, XENT_LINE_BREAK_CHAR_WRAP);
			xent_set_size(ctx, leaf, (XentSize) {NAN, 16.0f});
			xent_append_child(ctx, group, leaf);
		}
	}

	xent_layout(ctx, tree->root, 1200.0f, 800.0f);
}

static void mutate_leafs(
  XentContext *ctx, BenchTree const *tree, uint32_t groups_to_touch, uint32_t leaves_per_group, uint32_t tick
) {
	char const *text = (tick & 1u) ? "node" : "node_updated";
	for (uint32_t g = 0; g < groups_to_touch; ++g)
		for (uint32_t i = 0; i < leaves_per_group; ++i) xent_set_text(ctx, tree->leaves [g][i], text);
}

static double run_dirty_full_avg(
  XentContext *ctx, BenchTree const *tree, uint32_t groups_to_touch, uint32_t leaves_per_group, uint32_t iterations
) {
	double start = now_ms();
	for (uint32_t it = 0; it < iterations; ++it) {
		mutate_leafs(ctx, tree, groups_to_touch, leaves_per_group, it);
		xent_set_size(ctx, tree->root, (XentSize) {1200.0f, 800.0f});
		xent_layout(ctx, tree->root, 1200.0f, 800.0f);
	}
	return (now_ms() - start) / ( double ) iterations;
}

static double run_dirty_subtree_avg(
  XentContext *ctx, BenchTree const *tree, uint32_t groups_to_touch, uint32_t leaves_per_group, uint32_t iterations
) {
	double start = now_ms();
	for (uint32_t it = 0; it < iterations; ++it) {
		mutate_leafs(ctx, tree, groups_to_touch, leaves_per_group, it);
		xent_layout(ctx, tree->root, 1200.0f, 800.0f);
	}
	return (now_ms() - start) / ( double ) iterations;
}

static double measure_dirty_case(
  bool force_full, uint32_t groups_to_touch, uint32_t leaves_per_group, uint32_t iterations
) {
	XentContext *ctx  = xent_create_context(NULL);
	BenchTree    tree = {0};
	build_dirty_tree(ctx, &tree);
	double avg = force_full
	           ? run_dirty_full_avg(ctx, &tree, groups_to_touch, leaves_per_group, iterations)
	           : run_dirty_subtree_avg(ctx, &tree, groups_to_touch, leaves_per_group, iterations);
	xent_destroy_context(ctx);
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
	XentContext *ctx  = xent_create_context(NULL);
	XentNodeId   root = xent_create_node(ctx);
	XentNodeId   group = xent_create_node(ctx);
	XentNodeId   a     = xent_create_node(ctx);
	XentNodeId   b     = xent_create_node(ctx);
	XentNodeId   tail  = xent_create_node(ctx);

	xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
	xent_set_flex_direction(ctx, root, XENT_FLEX_COLUMN);
	xent_set_size(ctx, root, (XentSize) {200.0f, 80.0f});
	xent_set_protocol(ctx, group, XENT_PROTOCOL_FLEX);
	xent_set_flex_direction(ctx, group, XENT_FLEX_ROW);
	xent_set_size(ctx, group, (XentSize) {200.0f, 40.0f});
	xent_set_size(ctx, a, (XentSize) {50.0f, 20.0f});
	xent_set_size(ctx, b, (XentSize) {NAN, 20.0f});
	xent_set_size(ctx, tail, (XentSize) {200.0f, 20.0f});
	xent_set_text(ctx, b, "node");
	xent_set_text_line_break_policy(ctx, b, XENT_LINE_BREAK_CHAR_WRAP);
	xent_append_child(ctx, root, group);
	xent_append_child(ctx, root, tail);
	xent_append_child(ctx, group, a);
	xent_append_child(ctx, group, b);

	xent_layout(ctx, root, 200.0f, 80.0f);
	bool pass_full = xent_get_last_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_FULL_TREE;

	xent_layout(ctx, root, 200.0f, 80.0f);
	bool pass_none = xent_get_last_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_NONE;

	xent_set_text(ctx, b, "node2");
	xent_layout(ctx, root, 200.0f, 80.0f);
	bool pass_subtree = xent_get_last_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_DIRTY_SUBTREE;

	xent_set_size(ctx, root, (XentSize) {220.0f, 80.0f});
	xent_layout(ctx, root, 220.0f, 80.0f);
	bool pass_full_again = xent_get_last_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_FULL_TREE;

	bool pass            = pass_full && pass_none && pass_subtree && pass_full_again;
	printf("[gate] strategy-transition full->none->subtree->full => %s\n", pass ? "PASS" : "FAIL");

	xent_destroy_context(ctx);
	return pass;
}

int main(void) {
	bool ok = true;
	ok      = gate_layout_relative_perf() && ok;
	ok      = gate_dirty_scheduler() && ok;
	ok      = gate_layout_strategy_transitions() && ok;
	return ok ? 0 : 1;
}
