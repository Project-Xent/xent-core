#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "test_common.h"

static uint32_t lcg_next(uint32_t *state) {
	*state = (*state * 1664525u) + 1013904223u;
	return *state;
}

typedef struct StressTree {
	XentCtx    *ctx;
	XentNodeId  root;
	XentNodeId *leaves;
	uint32_t    leaf_count;
} StressTree;

static void configure_root(XentCtx *ctx, XentNodeId root) {
	xent_setproto(ctx, root, XENT_PROTOCOL_FLEX);
	xent_setflexdir(ctx, root, XENT_FLEX_COLUMN);
	xent_setsize(ctx, root, (XentSize) {1920.0f, 1080.0f});
	xent_setgap(ctx, root, 1.0f);
}

static void configure_group(XentCtx *ctx, XentNodeId group, uint32_t index) {
	bool const is_flex = (index % 2u) == 0u;

	xent_setproto(ctx, group, is_flex ? XENT_PROTOCOL_FLEX : XENT_PROTOCOL_SWIFTSTACK);
	if (is_flex) xent_setflexdir(ctx, group, XENT_FLEX_ROW);
	else xent_stack_setaxis(ctx, group, XENT_AXIS_HORIZONTAL);
	xent_setsize(ctx, group, (XentSize) {NAN, 18.0f});
	xent_setgrow(ctx, group, 1.0f);
	xent_setgap(ctx, group, 1.0f);
}

static void configure_leaf_size(XentCtx *ctx, XentNodeId leaf, uint32_t value) {
	bool const is_text = (value & 3u) == 0u;

	if (is_text) {
		xent_settext(ctx, leaf, ((value & 1u) == 0u) ? "stress-text-alpha" : "stress-text-beta");
		xent_setsize(ctx, leaf, (XentSize) {NAN, 16.0f});
		return;
	}

	float const width = 8.0f + ( float ) (value % 48u);
	xent_setsize(ctx, leaf, (XentSize) {width, 16.0f});
}

static void configure_leaf_behavior(XentCtx *ctx, XentNodeId leaf, uint32_t group_index, uint32_t value) {
	bool const is_flex_group = (group_index % 2u) == 0u;

	if (is_flex_group) {
		xent_setshrink(ctx, leaf, 1.0f);
		xent_setgrow(ctx, leaf, (value % 5u == 0u) ? 1.0f : 0.0f);
		return;
	}

	xent_stack_setprio(ctx, leaf, ( float ) (value % 4u));
	xent_stack_setspacer(ctx, leaf, value % 17u == 0u);
}

static int
append_group(StressTree *tree, uint32_t group_index, uint32_t leaves_per_group, uint32_t *seed, uint32_t *write) {
	XentNodeId group = xent_node_create(tree->ctx);
	configure_group(tree->ctx, group, group_index);
	xent_node_append(tree->ctx, tree->root, group);

	for (uint32_t i = 0; i < leaves_per_group; ++i) {
		XentNodeId leaf        = xent_node_create(tree->ctx);
		uint32_t   value       = lcg_next(seed);
		tree->leaves [*write]  = leaf;
		*write                += 1u;
		configure_leaf_size(tree->ctx, leaf, value);
		configure_leaf_behavior(tree->ctx, leaf, group_index, value);
		xent_node_append(tree->ctx, group, leaf);
	}

	return 0;
}

static int build_tree(StressTree *tree, uint32_t groups, uint32_t leaves_per_group) {
	uint32_t seed  = 0xc0ffeeu;
	uint32_t write = 0u;

	for (uint32_t g = 0; g < groups; ++g) TEST_ASSERT(append_group(tree, g, leaves_per_group, &seed, &write) == 0);

	TEST_ASSERT(write == tree->leaf_count);
	return 0;
}

static void mutate_leaf(XentCtx *ctx, XentNodeId leaf, uint32_t index, uint32_t iteration) {
	if (((index + iteration) & 1u) == 0u) {
		xent_settext(ctx, leaf, (iteration & 1u) ? "mut-a" : "mut-b");
		xent_setsize(ctx, leaf, (XentSize) {NAN, 16.0f});
		return;
	}

	float const width = 10.0f + ( float ) ((index + iteration) % 64u);
	xent_setsize(ctx, leaf, (XentSize) {width, 16.0f});
}

static int validate_leaf_rect(XentCtx *ctx, XentNodeId leaf) {
	XentRect rect = {0};

	TEST_ASSERT(xent_layout_rect(ctx, leaf, &rect));
	TEST_ASSERT(isfinite(rect.x));
	TEST_ASSERT(isfinite(rect.y));
	TEST_ASSERT(isfinite(rect.w));
	TEST_ASSERT(isfinite(rect.h));
	TEST_ASSERT(rect.w >= 0.0f);
	TEST_ASSERT(rect.h >= 0.0f);
	return 0;
}

static int run_iteration(StressTree const *tree, uint32_t iteration) {
	for (uint32_t i = 0; i < tree->leaf_count; i += 97u) {
		XentNodeId leaf = tree->leaves [(i + iteration * 13u) % tree->leaf_count];
		mutate_leaf(tree->ctx, leaf, i, iteration);
	}

	TEST_ASSERT(xent_layout(tree->ctx, tree->root, 1920.0f, 1080.0f));

	for (uint32_t i = 0; i < tree->leaf_count; i += 503u)
		TEST_ASSERT(validate_leaf_rect(tree->ctx, tree->leaves [i]) == 0);

	return 0;
}

static int run_iterations(StressTree const *tree) {
	uint32_t const iterations = 24u;

	for (uint32_t it = 0; it < iterations; ++it) TEST_ASSERT(run_iteration(tree, it) == 0);

	return 0;
}

int main(void) {
	uint32_t const groups           = 120u;
	uint32_t const leaves_per_group = 120u;
	uint32_t const leaf_count       = groups * leaves_per_group;
	StressTree     tree             = {0};

	tree.ctx                        = xent_ctx_create(NULL);
	TEST_ASSERT(tree.ctx != NULL);

	tree.leaves = ( XentNodeId * ) malloc(sizeof(XentNodeId) * ( size_t ) leaf_count);
	TEST_ASSERT(tree.leaves != NULL);

	tree.leaf_count = leaf_count;
	tree.root       = xent_node_create(tree.ctx);
	configure_root(tree.ctx, tree.root);

	TEST_ASSERT(build_tree(&tree, groups, leaves_per_group) == 0);
	TEST_ASSERT(xent_layout(tree.ctx, tree.root, 1920.0f, 1080.0f));
	TEST_ASSERT(run_iterations(&tree) == 0);

	free(tree.leaves);
	xent_ctx_destroy(tree.ctx);
	return 0;
}
