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

static void build_tree(XentCtx *ctx, BenchTree *tree) {
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

static double run_full(
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

static double run_dirty_scheduler(
  XentCtx *ctx, BenchTree const *tree, uint32_t groups_to_touch, uint32_t leaves_per_group, uint32_t iterations
) {
	double start = now_ms();
	for (uint32_t it = 0; it < iterations; ++it) {
		mutate_leafs(ctx, tree, groups_to_touch, leaves_per_group, it);
		xent_layout(ctx, tree->root, 1200.0f, 800.0f);
	}
	return (now_ms() - start) / ( double ) iterations;
}

static void run_scenario(char const *name, uint32_t groups_to_touch, uint32_t leaves_per_group) {
	XentCtx  *ctx  = xent_ctx_create(NULL);
	BenchTree tree = {0};
	build_tree(ctx, &tree);

	uint32_t const iterations  = 80u;
	double         full_avg    = run_full(ctx, &tree, groups_to_touch, leaves_per_group, iterations);
	double         dirty_avg   = run_dirty_scheduler(ctx, &tree, groups_to_touch, leaves_per_group, iterations);
	double         speedup     = (dirty_avg > 0.0) ? (full_avg / dirty_avg) : 0.0;
	uint32_t       dirty_nodes = groups_to_touch * leaves_per_group;

	printf(
	  "scenario=%s dirty_nodes=%u full_avg_ms=%.4f fixed_subtree_dirty_avg_ms=%.4f speedup=%.2fx\n", name, dirty_nodes,
	  full_avg, dirty_avg, speedup
	);

	xent_ctx_destroy(ctx);
}

int main(void) {
	printf("bench_dirty_vs_full fixed-subtree scheduler (100 groups x 100 leaves = 10k leaves)\n");
	run_scenario("leaf-1", 1u, 1u);
	run_scenario("group-100", 1u, 100u);
	run_scenario("ten-groups-1000", 10u, 100u);
	run_scenario("fifty-groups-5000", 50u, 100u);
	run_scenario("all-groups-10000", 100u, 100u);
	return 0;
}
