#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "test_common.h"

static uint32_t lcg_next(uint32_t *state) {
	*state = (*state * 1664525u) + 1013904223u;
	return *state;
}

typedef struct StressTree {
	XentContext *ctx;
	XentNodeId  root;
	XentNodeId *leaves;
	uint32_t    leaf_count;
} StressTree;

static void configure_root(XentContext *ctx, XentNodeId root) {
	xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
	xent_set_flex_direction(ctx, root, XENT_FLEX_COLUMN);
	xent_set_size(ctx, root, (XentSize) {1920.0f, 1080.0f});
	xent_set_gap(ctx, root, 1.0f);
}

static void configure_group(XentContext *ctx, XentNodeId group, uint32_t index) {
	bool const is_flex = (index % 2u) == 0u;

	xent_set_protocol(ctx, group, is_flex ? XENT_PROTOCOL_FLEX : XENT_PROTOCOL_SWIFTSTACK);
	if (is_flex) xent_set_flex_direction(ctx, group, XENT_FLEX_ROW);
	else xent_set_stack_axis(ctx, group, XENT_AXIS_HORIZONTAL);
	xent_set_size(ctx, group, (XentSize) {NAN, 18.0f});
	xent_set_flex_grow(ctx, group, 1.0f);
	xent_set_gap(ctx, group, 1.0f);
}

static void configure_leaf_size(XentContext *ctx, XentNodeId leaf, uint32_t value) {
	bool const is_text = (value & 3u) == 0u;

	if (is_text) {
		xent_set_text(ctx, leaf, ((value & 1u) == 0u) ? "stress-text-alpha" : "stress-text-beta");
		xent_set_size(ctx, leaf, (XentSize) {NAN, 16.0f});
		return;
	}

	float const width = 8.0f + ( float ) (value % 48u);
	xent_set_size(ctx, leaf, (XentSize) {width, 16.0f});
}

static void configure_leaf_behavior(XentContext *ctx, XentNodeId leaf, uint32_t group_index, uint32_t value) {
	bool const is_flex_group = (group_index % 2u) == 0u;

	if (is_flex_group) {
		xent_set_flex_shrink(ctx, leaf, 1.0f);
		xent_set_flex_grow(ctx, leaf, (value % 5u == 0u) ? 1.0f : 0.0f);
		return;
	}

	xent_set_layout_priority(ctx, leaf, ( float ) (value % 4u));
	xent_set_is_spacer(ctx, leaf, value % 17u == 0u);
}

static int append_group(StressTree *tree, uint32_t group_index, uint32_t leaves_per_group, uint32_t *seed, uint32_t *write) {
	XentNodeId group = xent_create_node(tree->ctx);
	configure_group(tree->ctx, group, group_index);
	xent_append_child(tree->ctx, tree->root, group);

	for (uint32_t i = 0; i < leaves_per_group; ++i) {
		XentNodeId leaf      = xent_create_node(tree->ctx);
		uint32_t   value     = lcg_next(seed);
		tree->leaves [*write] = leaf;
		*write += 1u;
		configure_leaf_size(tree->ctx, leaf, value);
		configure_leaf_behavior(tree->ctx, leaf, group_index, value);
		xent_append_child(tree->ctx, group, leaf);
	}

	return 0;
}

static int build_tree(StressTree *tree, uint32_t groups, uint32_t leaves_per_group) {
	uint32_t seed  = 0xc0ffeeu;
	uint32_t write = 0u;

	for (uint32_t g = 0; g < groups; ++g) {
		TEST_ASSERT(append_group(tree, g, leaves_per_group, &seed, &write) == 0);
	}

	TEST_ASSERT(write == tree->leaf_count);
	return 0;
}

static void mutate_leaf(XentContext *ctx, XentNodeId leaf, uint32_t index, uint32_t iteration) {
	if (((index + iteration) & 1u) == 0u) {
		xent_set_text(ctx, leaf, (iteration & 1u) ? "mut-a" : "mut-b");
		xent_set_size(ctx, leaf, (XentSize) {NAN, 16.0f});
		return;
	}

	float const width = 10.0f + ( float ) ((index + iteration) % 64u);
	xent_set_size(ctx, leaf, (XentSize) {width, 16.0f});
}

static int validate_leaf_rect(XentContext *ctx, XentNodeId leaf) {
	XentRect rect = {0};

	TEST_ASSERT(xent_get_layout_rect(ctx, leaf, &rect));
	TEST_ASSERT(isfinite(rect.x));
	TEST_ASSERT(isfinite(rect.y));
	TEST_ASSERT(isfinite(rect.width));
	TEST_ASSERT(isfinite(rect.height));
	TEST_ASSERT(rect.width >= 0.0f);
	TEST_ASSERT(rect.height >= 0.0f);
	return 0;
}

static int run_iteration(StressTree const *tree, uint32_t iteration) {
	for (uint32_t i = 0; i < tree->leaf_count; i += 97u) {
		XentNodeId leaf = tree->leaves [(i + iteration * 13u) % tree->leaf_count];
		mutate_leaf(tree->ctx, leaf, i, iteration);
	}

	TEST_ASSERT(xent_layout(tree->ctx, tree->root, 1920.0f, 1080.0f));

	for (uint32_t i = 0; i < tree->leaf_count; i += 503u) {
		TEST_ASSERT(validate_leaf_rect(tree->ctx, tree->leaves [i]) == 0);
	}

	return 0;
}

static int run_iterations(StressTree const *tree) {
	uint32_t const iterations = 24u;

	for (uint32_t it = 0; it < iterations; ++it) {
		TEST_ASSERT(run_iteration(tree, it) == 0);
	}

	return 0;
}

int main(void) {
	uint32_t const groups           = 120u;
	uint32_t const leaves_per_group = 120u;
	uint32_t const leaf_count       = groups * leaves_per_group;
	StressTree     tree            = {0};

	tree.ctx = xent_create_context(NULL);
	TEST_ASSERT(tree.ctx != NULL);

	tree.leaves = ( XentNodeId * ) malloc(sizeof(XentNodeId) * ( size_t ) leaf_count);
	TEST_ASSERT(tree.leaves != NULL);

	tree.leaf_count = leaf_count;
	tree.root       = xent_create_node(tree.ctx);
	configure_root(tree.ctx, tree.root);

	TEST_ASSERT(build_tree(&tree, groups, leaves_per_group) == 0);
	TEST_ASSERT(xent_layout(tree.ctx, tree.root, 1920.0f, 1080.0f));
	TEST_ASSERT(run_iterations(&tree) == 0);

	free(tree.leaves);
	xent_destroy_context(tree.ctx);
	return 0;
}
