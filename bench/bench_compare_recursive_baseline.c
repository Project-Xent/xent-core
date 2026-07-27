#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "xent/xent.h"

typedef struct BaselineNode {
	float                width;
	float                height;
	float                x;
	float                y;
	uint32_t             child_count;
	struct BaselineNode *children;
} BaselineNode;

static double now_ms(void) { return ( double ) clock() * 1000.0 / ( double ) CLOCKS_PER_SEC; }

static void   baseline_layout(BaselineNode *node, float x, float y, float w, float h) {
	node->x      = x;
	node->y      = y;
	node->width  = w;
	node->height = h;
	if (node->child_count == 0u) return;

	float each_h = h / ( float ) node->child_count;
	for (uint32_t i = 0; i < node->child_count; ++i)
		baseline_layout(&node->children [i], x, y + each_h * ( float ) i, w, each_h);
}

static BaselineNode make_baseline_tree(uint32_t node_count) {
	BaselineNode root = {0};
	root.child_count  = node_count;
	root.children     = ( BaselineNode * ) calloc(node_count, sizeof(BaselineNode));
	return root;
}

static void free_baseline_tree(BaselineNode *root) {
	free(root->children);
	root->children = NULL;
}

static XentNodeId make_xent_tree(XentCtx *ctx, uint32_t node_count) {
	XentNodeId root = xent_node_create(ctx);
	xent_setproto(ctx, root, XENT_PROTOCOL_FLEX);
	xent_setflexdir(ctx, root, XENT_FLEX_COLUMN);
	xent_setsize(ctx, root, (XentSize) {1000.0f, 800.0f});
	for (uint32_t i = 0; i < node_count; ++i) {
		XentNodeId child = xent_node_create(ctx);
		xent_setsize(ctx, child, (XentSize) {NAN, 20.0f});
		xent_node_append(ctx, root, child);
	}
	return root;
}

int main(void) {
	uint32_t const sizes []   = {100u, 1000u, 10000u};
	int const      iterations = 100;

	for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes [0]); ++i) {
		uint32_t     n              = sizes [i];

		BaselineNode baseline       = make_baseline_tree(n);
		double       baseline_start = now_ms();
		for (int it = 0; it < iterations; ++it) baseline_layout(&baseline, 0.0f, 0.0f, 1000.0f, 800.0f);
		double     baseline_elapsed = now_ms() - baseline_start;

		XentCtx   *ctx              = xent_ctx_create(NULL);
		XentNodeId root             = make_xent_tree(ctx, n);
		double     xent_start       = now_ms();
		for (int it = 0; it < iterations; ++it) xent_layout(ctx, root, 1000.0f, 800.0f);
		double xent_elapsed = now_ms() - xent_start;

		printf("nodes=%u baseline_ms=%.3f xent_ms=%.3f\n", n, baseline_elapsed, xent_elapsed);

		xent_ctx_destroy(ctx);
		free_baseline_tree(&baseline);
	}

	return 0;
}
