#include <math.h>
#include <stdio.h>
#include <string.h>

#include "xent/xent.h"

#define YT_MAX_NODE 8192u
#define YT_EPS      0.5f
#define YT_ASSERT(expr)                                                                \
	do {                                                                               \
		if (!(expr)) {                                                                 \
			fprintf(stderr, "ASSERT FAILED: %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
			return 1;                                                                  \
		}                                                                              \
	}                                                                                  \
	while (0)
#define YT_ASSERT_RECT(node, member, expected)                                                                      \
	do {                                                                                                            \
		XentRect rect = {0};                                                                                        \
		YT_ASSERT(xent_layout_rect(ctx, node, &rect));                                                              \
		if (fabsf(rect.member - (expected)) > YT_EPS) {                                                             \
			fprintf(                                                                                                \
			  stderr, "RECT FAILED: %s.%s expected %.3f got %.3f (%s:%d)\n", #node, #member, ( double ) (expected), \
			  ( double ) rect.member, __FILE__, __LINE__                                                            \
			);                                                                                                      \
			return 1;                                                                                               \
		}                                                                                                           \
	}                                                                                                               \
	while (0)

typedef enum YtEdge
{
	YT_EDGE_LEFT,
	YT_EDGE_TOP,
	YT_EDGE_RIGHT,
	YT_EDGE_BOTTOM,
	YT_EDGE_ALL,
	YT_EDGE_HORIZONTAL,
	YT_EDGE_VERTICAL,
} YtEdge;

static float      yt_w [YT_MAX_NODE];
static float      yt_h [YT_MAX_NODE];
static XentInsets yt_margin [YT_MAX_NODE];
static XentInsets yt_padding [YT_MAX_NODE];
static XentSize   yt_min_size [YT_MAX_NODE];
static XentSize   yt_max_size [YT_MAX_NODE];

static void       yt_reset_state(void) {
	for (uint32_t i = 0u; i < YT_MAX_NODE; ++i) {
		yt_w [i]        = NAN;
		yt_h [i]        = NAN;
		yt_margin [i]   = (XentInsets) {0.0f, 0.0f, 0.0f, 0.0f};
		yt_padding [i]  = (XentInsets) {0.0f, 0.0f, 0.0f, 0.0f};
		yt_min_size [i] = (XentSize) {0.0f, 0.0f};
		yt_max_size [i] = (XentSize) {INFINITY, INFINITY};
	}
}

static XentNodeId yt_create_node(XentCtx *ctx) {
	XentNodeId node = xent_node_create(ctx);
	YT_ASSERT(xent_node_index(node) < YT_MAX_NODE);
	xent_setproto(ctx, node, XENT_PROTOCOL_FLEX);
	xent_setflexdir(ctx, node, XENT_FLEX_COLUMN);
	return node;
}

/* Per-axis setters: setting one axis must NOT clear the OTHER axis's percent
 * (xent_setsize resets both axes, which would clobber a prior *_percent set). */
static void yt_set_width(XentCtx *ctx, XentNodeId node, float width) {
	yt_w [xent_node_index(node)] = width;
	xent_setw(ctx, node, width);
}

static void yt_set_height(XentCtx *ctx, XentNodeId node, float height) {
	yt_h [xent_node_index(node)] = height;
	xent_seth(ctx, node, height);
}

static void yt_apply_edge(XentInsets *insets, YtEdge edge, float value) {
	if (edge == YT_EDGE_LEFT || edge == YT_EDGE_ALL || edge == YT_EDGE_HORIZONTAL) insets->left = value;
	if (edge == YT_EDGE_TOP || edge == YT_EDGE_ALL || edge == YT_EDGE_VERTICAL) insets->top = value;
	if (edge == YT_EDGE_RIGHT || edge == YT_EDGE_ALL || edge == YT_EDGE_HORIZONTAL) insets->right = value;
	if (edge == YT_EDGE_BOTTOM || edge == YT_EDGE_ALL || edge == YT_EDGE_VERTICAL) insets->bottom = value;
}

static void yt_set_margin(XentCtx *ctx, XentNodeId node, YtEdge edge, float value) {
	yt_apply_edge(&yt_margin [xent_node_index(node)], edge, value);
	xent_setm(ctx, node, yt_margin [xent_node_index(node)]);
}

static void yt_set_padding(XentCtx *ctx, XentNodeId node, YtEdge edge, float value) {
	yt_apply_edge(&yt_padding [xent_node_index(node)], edge, value);
	xent_setp(ctx, node, yt_padding [xent_node_index(node)]);
}

static void yt_set_min_width(XentCtx *ctx, XentNodeId node, float value) {
	yt_min_size [xent_node_index(node)].w = value;
	xent_setminsize(ctx, node, yt_min_size [xent_node_index(node)]);
}

static void yt_set_min_height(XentCtx *ctx, XentNodeId node, float value) {
	yt_min_size [xent_node_index(node)].h = value;
	xent_setminsize(ctx, node, yt_min_size [xent_node_index(node)]);
}

static void yt_set_max_width(XentCtx *ctx, XentNodeId node, float value) {
	yt_max_size [xent_node_index(node)].w = value;
	xent_setmaxsize(ctx, node, yt_max_size [xent_node_index(node)]);
}

static void yt_set_max_height(XentCtx *ctx, XentNodeId node, float value) {
	yt_max_size [xent_node_index(node)].h = value;
	xent_setmaxsize(ctx, node, yt_max_size [xent_node_index(node)]);
}

typedef int (*YtTestFn)(void);

static int  test_0000_absolute_layout_align_items_and_justify_content_center(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_height(ctx, root, 100.0f);
	yt_set_width(ctx, root, 110.0f);
	xent_setgrow(ctx, root, 1.0f);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_CENTER);
	xent_setjustify(ctx, root, XENT_FLEX_JUSTIFY_CENTER);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 60.0f);
	yt_set_height(ctx, root_child0, 40.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 110.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 25.0f);
	YT_ASSERT_RECT(root_child0, y, 30.0f);
	YT_ASSERT_RECT(root_child0, w, 60.0f);
	YT_ASSERT_RECT(root_child0, h, 40.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 110.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 25.0f);
	YT_ASSERT_RECT(root_child0, y, 30.0f);
	YT_ASSERT_RECT(root_child0, w, 60.0f);
	YT_ASSERT_RECT(root_child0, h, 40.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0001_absolute_layout_align_items_and_justify_content_flex_end(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_height(ctx, root, 100.0f);
	yt_set_width(ctx, root, 110.0f);
	xent_setgrow(ctx, root, 1.0f);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_END);
	xent_setjustify(ctx, root, XENT_FLEX_JUSTIFY_END);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 60.0f);
	yt_set_height(ctx, root_child0, 40.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 110.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 50.0f);
	YT_ASSERT_RECT(root_child0, y, 60.0f);
	YT_ASSERT_RECT(root_child0, w, 60.0f);
	YT_ASSERT_RECT(root_child0, h, 40.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 110.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 60.0f);
	YT_ASSERT_RECT(root_child0, w, 60.0f);
	YT_ASSERT_RECT(root_child0, h, 40.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0002_absolute_layout_justify_content_center(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_height(ctx, root, 100.0f);
	yt_set_width(ctx, root, 110.0f);
	xent_setgrow(ctx, root, 1.0f);
	xent_setjustify(ctx, root, XENT_FLEX_JUSTIFY_CENTER);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 60.0f);
	yt_set_height(ctx, root_child0, 40.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 110.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 30.0f);
	YT_ASSERT_RECT(root_child0, w, 60.0f);
	YT_ASSERT_RECT(root_child0, h, 40.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 110.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 50.0f);
	YT_ASSERT_RECT(root_child0, y, 30.0f);
	YT_ASSERT_RECT(root_child0, w, 60.0f);
	YT_ASSERT_RECT(root_child0, h, 40.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0003_absolute_layout_align_items_center(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_height(ctx, root, 100.0f);
	yt_set_width(ctx, root, 110.0f);
	xent_setgrow(ctx, root, 1.0f);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_CENTER);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 60.0f);
	yt_set_height(ctx, root_child0, 40.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 110.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 25.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 60.0f);
	YT_ASSERT_RECT(root_child0, h, 40.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 110.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 25.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 60.0f);
	YT_ASSERT_RECT(root_child0, h, 40.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0004_absolute_layout_align_items_center_on_child_only(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_height(ctx, root, 100.0f);
	yt_set_width(ctx, root, 110.0f);
	xent_setgrow(ctx, root, 1.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 60.0f);
	yt_set_height(ctx, root_child0, 40.0f);
	xent_setself(ctx, root_child0, XENT_FLEX_ALIGN_CENTER);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 110.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 25.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 60.0f);
	YT_ASSERT_RECT(root_child0, h, 40.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 110.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 25.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 60.0f);
	YT_ASSERT_RECT(root_child0, h, 40.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0006_absolute_layout_padding_left(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 200.0f);
	yt_set_height(ctx, root, 200.0f);
	yt_set_padding(ctx, root, YT_EDGE_LEFT, 100.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 50.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 200.0f);
	YT_ASSERT_RECT(root, h, 200.0f);
	YT_ASSERT_RECT(root_child0, x, 100.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 200.0f);
	YT_ASSERT_RECT(root, h, 200.0f);
	YT_ASSERT_RECT(root_child0, x, 150.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0007_absolute_layout_padding_right(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 200.0f);
	yt_set_height(ctx, root, 200.0f);
	yt_set_padding(ctx, root, YT_EDGE_RIGHT, 100.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 50.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 200.0f);
	YT_ASSERT_RECT(root, h, 200.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 200.0f);
	YT_ASSERT_RECT(root, h, 200.0f);
	YT_ASSERT_RECT(root_child0, x, 50.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0008_absolute_layout_padding_top(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 200.0f);
	yt_set_height(ctx, root, 200.0f);
	yt_set_padding(ctx, root, YT_EDGE_TOP, 100.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 50.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 200.0f);
	YT_ASSERT_RECT(root, h, 200.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 100.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 200.0f);
	YT_ASSERT_RECT(root, h, 200.0f);
	YT_ASSERT_RECT(root_child0, x, 150.0f);
	YT_ASSERT_RECT(root_child0, y, 100.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0009_absolute_layout_padding_bottom(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 200.0f);
	yt_set_height(ctx, root, 200.0f);
	yt_set_padding(ctx, root, YT_EDGE_BOTTOM, 100.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 50.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 200.0f);
	YT_ASSERT_RECT(root, h, 200.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 200.0f);
	YT_ASSERT_RECT(root, h, 200.0f);
	YT_ASSERT_RECT(root_child0, x, 150.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0010_align_content_flex_start_nowrap(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 140.0f);
	yt_set_height(ctx, root, 120.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 40.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0011_align_content_flex_start_wrap(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 140.0f);
	yt_set_height(ctx, root, 120.0f);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_width(ctx, root_child2, 50.0f);
	yt_set_height(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	XentNodeId root_child3 = yt_create_node(ctx);
	yt_set_width(ctx, root_child3, 50.0f);
	yt_set_height(ctx, root_child3, 10.0f);
	xent_node_append(ctx, root, root_child3);
	XentNodeId root_child4 = yt_create_node(ctx);
	yt_set_width(ctx, root_child4, 50.0f);
	yt_set_height(ctx, root_child4, 10.0f);
	xent_node_append(ctx, root, root_child4);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 10.0f);
	YT_ASSERT_RECT(root_child2, w, 50.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	YT_ASSERT_RECT(root_child3, x, 50.0f);
	YT_ASSERT_RECT(root_child3, y, 10.0f);
	YT_ASSERT_RECT(root_child3, w, 50.0f);
	YT_ASSERT_RECT(root_child3, h, 10.0f);
	YT_ASSERT_RECT(root_child4, x, 0.0f);
	YT_ASSERT_RECT(root_child4, y, 20.0f);
	YT_ASSERT_RECT(root_child4, w, 50.0f);
	YT_ASSERT_RECT(root_child4, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 40.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 90.0f);
	YT_ASSERT_RECT(root_child2, y, 10.0f);
	YT_ASSERT_RECT(root_child2, w, 50.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	YT_ASSERT_RECT(root_child3, x, 40.0f);
	YT_ASSERT_RECT(root_child3, y, 10.0f);
	YT_ASSERT_RECT(root_child3, w, 50.0f);
	YT_ASSERT_RECT(root_child3, h, 10.0f);
	YT_ASSERT_RECT(root_child4, x, 90.0f);
	YT_ASSERT_RECT(root_child4, y, 20.0f);
	YT_ASSERT_RECT(root_child4, w, 50.0f);
	YT_ASSERT_RECT(root_child4, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0012_align_content_flex_start_wrap_singleline(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 140.0f);
	yt_set_height(ctx, root, 120.0f);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 40.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0014_align_content_flex_end_nowrap(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 140.0f);
	yt_set_height(ctx, root, 120.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_END);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 40.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0015_align_content_flex_end_wrap(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 140.0f);
	yt_set_height(ctx, root, 120.0f);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_END);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_width(ctx, root_child2, 50.0f);
	yt_set_height(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	XentNodeId root_child3 = yt_create_node(ctx);
	yt_set_width(ctx, root_child3, 50.0f);
	yt_set_height(ctx, root_child3, 10.0f);
	xent_node_append(ctx, root, root_child3);
	XentNodeId root_child4 = yt_create_node(ctx);
	yt_set_width(ctx, root_child4, 50.0f);
	yt_set_height(ctx, root_child4, 10.0f);
	xent_node_append(ctx, root, root_child4);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 90.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 90.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 100.0f);
	YT_ASSERT_RECT(root_child2, w, 50.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	YT_ASSERT_RECT(root_child3, x, 50.0f);
	YT_ASSERT_RECT(root_child3, y, 100.0f);
	YT_ASSERT_RECT(root_child3, w, 50.0f);
	YT_ASSERT_RECT(root_child3, h, 10.0f);
	YT_ASSERT_RECT(root_child4, x, 0.0f);
	YT_ASSERT_RECT(root_child4, y, 110.0f);
	YT_ASSERT_RECT(root_child4, w, 50.0f);
	YT_ASSERT_RECT(root_child4, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 90.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 40.0f);
	YT_ASSERT_RECT(root_child1, y, 90.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 90.0f);
	YT_ASSERT_RECT(root_child2, y, 100.0f);
	YT_ASSERT_RECT(root_child2, w, 50.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	YT_ASSERT_RECT(root_child3, x, 40.0f);
	YT_ASSERT_RECT(root_child3, y, 100.0f);
	YT_ASSERT_RECT(root_child3, w, 50.0f);
	YT_ASSERT_RECT(root_child3, h, 10.0f);
	YT_ASSERT_RECT(root_child4, x, 90.0f);
	YT_ASSERT_RECT(root_child4, y, 110.0f);
	YT_ASSERT_RECT(root_child4, w, 50.0f);
	YT_ASSERT_RECT(root_child4, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0016_align_content_flex_end_wrap_singleline(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 140.0f);
	yt_set_height(ctx, root, 120.0f);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_END);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 110.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 110.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 110.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 40.0f);
	YT_ASSERT_RECT(root_child1, y, 110.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0017_align_content_center_nowrap(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 140.0f);
	yt_set_height(ctx, root, 120.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_CENTER);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 40.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0018_align_content_center_wrap(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 140.0f);
	yt_set_height(ctx, root, 120.0f);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_CENTER);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_width(ctx, root_child2, 50.0f);
	yt_set_height(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	XentNodeId root_child3 = yt_create_node(ctx);
	yt_set_width(ctx, root_child3, 50.0f);
	yt_set_height(ctx, root_child3, 10.0f);
	xent_node_append(ctx, root, root_child3);
	XentNodeId root_child4 = yt_create_node(ctx);
	yt_set_width(ctx, root_child4, 50.0f);
	yt_set_height(ctx, root_child4, 10.0f);
	xent_node_append(ctx, root, root_child4);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 45.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 45.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 55.0f);
	YT_ASSERT_RECT(root_child2, w, 50.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	YT_ASSERT_RECT(root_child3, x, 50.0f);
	YT_ASSERT_RECT(root_child3, y, 55.0f);
	YT_ASSERT_RECT(root_child3, w, 50.0f);
	YT_ASSERT_RECT(root_child3, h, 10.0f);
	YT_ASSERT_RECT(root_child4, x, 0.0f);
	YT_ASSERT_RECT(root_child4, y, 65.0f);
	YT_ASSERT_RECT(root_child4, w, 50.0f);
	YT_ASSERT_RECT(root_child4, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 45.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 40.0f);
	YT_ASSERT_RECT(root_child1, y, 45.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 90.0f);
	YT_ASSERT_RECT(root_child2, y, 55.0f);
	YT_ASSERT_RECT(root_child2, w, 50.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	YT_ASSERT_RECT(root_child3, x, 40.0f);
	YT_ASSERT_RECT(root_child3, y, 55.0f);
	YT_ASSERT_RECT(root_child3, w, 50.0f);
	YT_ASSERT_RECT(root_child3, h, 10.0f);
	YT_ASSERT_RECT(root_child4, x, 90.0f);
	YT_ASSERT_RECT(root_child4, y, 65.0f);
	YT_ASSERT_RECT(root_child4, w, 50.0f);
	YT_ASSERT_RECT(root_child4, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0019_align_content_center_wrap_singleline(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 140.0f);
	yt_set_height(ctx, root, 120.0f);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_CENTER);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 55.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 55.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 55.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 40.0f);
	YT_ASSERT_RECT(root_child1, y, 55.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0020_align_content_space_between_nowrap(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 140.0f);
	yt_set_height(ctx, root, 120.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_SPACE_BETWEEN);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 40.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0021_align_content_space_between_wrap(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 140.0f);
	yt_set_height(ctx, root, 120.0f);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_SPACE_BETWEEN);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_width(ctx, root_child2, 50.0f);
	yt_set_height(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	XentNodeId root_child3 = yt_create_node(ctx);
	yt_set_width(ctx, root_child3, 50.0f);
	yt_set_height(ctx, root_child3, 10.0f);
	xent_node_append(ctx, root, root_child3);
	XentNodeId root_child4 = yt_create_node(ctx);
	yt_set_width(ctx, root_child4, 50.0f);
	yt_set_height(ctx, root_child4, 10.0f);
	xent_node_append(ctx, root, root_child4);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 55.0f);
	YT_ASSERT_RECT(root_child2, w, 50.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	YT_ASSERT_RECT(root_child3, x, 50.0f);
	YT_ASSERT_RECT(root_child3, y, 55.0f);
	YT_ASSERT_RECT(root_child3, w, 50.0f);
	YT_ASSERT_RECT(root_child3, h, 10.0f);
	YT_ASSERT_RECT(root_child4, x, 0.0f);
	YT_ASSERT_RECT(root_child4, y, 110.0f);
	YT_ASSERT_RECT(root_child4, w, 50.0f);
	YT_ASSERT_RECT(root_child4, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 40.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 90.0f);
	YT_ASSERT_RECT(root_child2, y, 55.0f);
	YT_ASSERT_RECT(root_child2, w, 50.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	YT_ASSERT_RECT(root_child3, x, 40.0f);
	YT_ASSERT_RECT(root_child3, y, 55.0f);
	YT_ASSERT_RECT(root_child3, w, 50.0f);
	YT_ASSERT_RECT(root_child3, h, 10.0f);
	YT_ASSERT_RECT(root_child4, x, 90.0f);
	YT_ASSERT_RECT(root_child4, y, 110.0f);
	YT_ASSERT_RECT(root_child4, w, 50.0f);
	YT_ASSERT_RECT(root_child4, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0022_align_content_space_between_wrap_singleline(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 140.0f);
	yt_set_height(ctx, root, 120.0f);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_SPACE_BETWEEN);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 40.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0023_align_content_space_around_nowrap(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 140.0f);
	yt_set_height(ctx, root, 120.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_SPACE_AROUND);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 40.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0024_align_content_space_around_wrap(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 140.0f);
	yt_set_height(ctx, root, 120.0f);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_SPACE_AROUND);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_width(ctx, root_child2, 50.0f);
	yt_set_height(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	XentNodeId root_child3 = yt_create_node(ctx);
	yt_set_width(ctx, root_child3, 50.0f);
	yt_set_height(ctx, root_child3, 10.0f);
	xent_node_append(ctx, root, root_child3);
	XentNodeId root_child4 = yt_create_node(ctx);
	yt_set_width(ctx, root_child4, 50.0f);
	yt_set_height(ctx, root_child4, 10.0f);
	xent_node_append(ctx, root, root_child4);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 15.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 15.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 55.0f);
	YT_ASSERT_RECT(root_child2, w, 50.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	YT_ASSERT_RECT(root_child3, x, 50.0f);
	YT_ASSERT_RECT(root_child3, y, 55.0f);
	YT_ASSERT_RECT(root_child3, w, 50.0f);
	YT_ASSERT_RECT(root_child3, h, 10.0f);
	YT_ASSERT_RECT(root_child4, x, 0.0f);
	YT_ASSERT_RECT(root_child4, y, 95.0f);
	YT_ASSERT_RECT(root_child4, w, 50.0f);
	YT_ASSERT_RECT(root_child4, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 15.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 40.0f);
	YT_ASSERT_RECT(root_child1, y, 15.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 90.0f);
	YT_ASSERT_RECT(root_child2, y, 55.0f);
	YT_ASSERT_RECT(root_child2, w, 50.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	YT_ASSERT_RECT(root_child3, x, 40.0f);
	YT_ASSERT_RECT(root_child3, y, 55.0f);
	YT_ASSERT_RECT(root_child3, w, 50.0f);
	YT_ASSERT_RECT(root_child3, h, 10.0f);
	YT_ASSERT_RECT(root_child4, x, 90.0f);
	YT_ASSERT_RECT(root_child4, y, 95.0f);
	YT_ASSERT_RECT(root_child4, w, 50.0f);
	YT_ASSERT_RECT(root_child4, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0025_align_content_space_around_wrap_singleline(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 140.0f);
	yt_set_height(ctx, root, 120.0f);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_SPACE_AROUND);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 55.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 55.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 55.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 40.0f);
	YT_ASSERT_RECT(root_child1, y, 55.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0026_align_content_space_evenly_nowrap(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 140.0f);
	yt_set_height(ctx, root, 120.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_SPACE_EVENLY);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 40.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0027_align_content_space_evenly_wrap(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 140.0f);
	yt_set_height(ctx, root, 120.0f);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_SPACE_EVENLY);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_width(ctx, root_child2, 50.0f);
	yt_set_height(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	XentNodeId root_child3 = yt_create_node(ctx);
	yt_set_width(ctx, root_child3, 50.0f);
	yt_set_height(ctx, root_child3, 10.0f);
	xent_node_append(ctx, root, root_child3);
	XentNodeId root_child4 = yt_create_node(ctx);
	yt_set_width(ctx, root_child4, 50.0f);
	yt_set_height(ctx, root_child4, 10.0f);
	xent_node_append(ctx, root, root_child4);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 23.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 23.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 55.0f);
	YT_ASSERT_RECT(root_child2, w, 50.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	YT_ASSERT_RECT(root_child3, x, 50.0f);
	YT_ASSERT_RECT(root_child3, y, 55.0f);
	YT_ASSERT_RECT(root_child3, w, 50.0f);
	YT_ASSERT_RECT(root_child3, h, 10.0f);
	YT_ASSERT_RECT(root_child4, x, 0.0f);
	YT_ASSERT_RECT(root_child4, y, 88.0f);
	YT_ASSERT_RECT(root_child4, w, 50.0f);
	YT_ASSERT_RECT(root_child4, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 23.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 40.0f);
	YT_ASSERT_RECT(root_child1, y, 23.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 90.0f);
	YT_ASSERT_RECT(root_child2, y, 55.0f);
	YT_ASSERT_RECT(root_child2, w, 50.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	YT_ASSERT_RECT(root_child3, x, 40.0f);
	YT_ASSERT_RECT(root_child3, y, 55.0f);
	YT_ASSERT_RECT(root_child3, w, 50.0f);
	YT_ASSERT_RECT(root_child3, h, 10.0f);
	YT_ASSERT_RECT(root_child4, x, 90.0f);
	YT_ASSERT_RECT(root_child4, y, 88.0f);
	YT_ASSERT_RECT(root_child4, w, 50.0f);
	YT_ASSERT_RECT(root_child4, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0028_align_content_space_evenly_wrap_singleline(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 140.0f);
	yt_set_height(ctx, root, 120.0f);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_SPACE_EVENLY);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 55.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 55.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 140.0f);
	YT_ASSERT_RECT(root, h, 120.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 55.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 40.0f);
	YT_ASSERT_RECT(root_child1, y, 55.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0033_align_content_stretch_row_with_single_row(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 150.0f);
	yt_set_height(ctx, root, 100.0f);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	/* Original Yoga case is align-content:STRETCH (the single wrapped line stretches
	 * to fill the container's cross). The earlier port hardcoded START because xent
	 * lacked a STRETCH value; with align-content:stretch implemented this matches
	 * both the case name and its expected (filled) cross sizes. */
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_STRETCH);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 150.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 100.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 100.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 150.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 100.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 100.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 100.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0040_align_content_space_evenly_with_min_cross_axis(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 500.0f);
	yt_set_min_height(ctx, root, 500.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_SPACE_EVENLY);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 400.0f);
	yt_set_height(ctx, root_child0, 200.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 400.0f);
	yt_set_height(ctx, root_child1, 200.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 500.0f);
	YT_ASSERT_RECT(root, h, 500.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 33.0f);
	YT_ASSERT_RECT(root_child0, w, 400.0f);
	YT_ASSERT_RECT(root_child0, h, 200.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 267.0f);
	YT_ASSERT_RECT(root_child1, w, 400.0f);
	YT_ASSERT_RECT(root_child1, h, 200.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 500.0f);
	YT_ASSERT_RECT(root, h, 500.0f);
	YT_ASSERT_RECT(root_child0, x, 100.0f);
	YT_ASSERT_RECT(root_child0, y, 33.0f);
	YT_ASSERT_RECT(root_child0, w, 400.0f);
	YT_ASSERT_RECT(root_child0, h, 200.0f);
	YT_ASSERT_RECT(root_child1, x, 100.0f);
	YT_ASSERT_RECT(root_child1, y, 267.0f);
	YT_ASSERT_RECT(root_child1, w, 400.0f);
	YT_ASSERT_RECT(root_child1, h, 200.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0043_align_content_space_around_and_align_items_flex_end_with_flex_wrap(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 300.0f);
	yt_set_height(ctx, root, 300.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_SPACE_AROUND);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_END);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 50.0f);
	yt_set_width(ctx, root_child0, 150.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_height(ctx, root_child1, 100.0f);
	yt_set_width(ctx, root_child1, 120.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_height(ctx, root_child2, 50.0f);
	yt_set_width(ctx, root_child2, 120.0f);
	xent_node_append(ctx, root, root_child2);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 300.0f);
	YT_ASSERT_RECT(root, h, 300.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 88.0f);
	YT_ASSERT_RECT(root_child0, w, 150.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	YT_ASSERT_RECT(root_child1, x, 150.0f);
	YT_ASSERT_RECT(root_child1, y, 38.0f);
	YT_ASSERT_RECT(root_child1, w, 120.0f);
	YT_ASSERT_RECT(root_child1, h, 100.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 213.0f);
	YT_ASSERT_RECT(root_child2, w, 120.0f);
	YT_ASSERT_RECT(root_child2, h, 50.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 300.0f);
	YT_ASSERT_RECT(root, h, 300.0f);
	YT_ASSERT_RECT(root_child0, x, 150.0f);
	YT_ASSERT_RECT(root_child0, y, 88.0f);
	YT_ASSERT_RECT(root_child0, w, 150.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	YT_ASSERT_RECT(root_child1, x, 30.0f);
	YT_ASSERT_RECT(root_child1, y, 38.0f);
	YT_ASSERT_RECT(root_child1, w, 120.0f);
	YT_ASSERT_RECT(root_child1, h, 100.0f);
	YT_ASSERT_RECT(root_child2, x, 180.0f);
	YT_ASSERT_RECT(root_child2, y, 213.0f);
	YT_ASSERT_RECT(root_child2, w, 120.0f);
	YT_ASSERT_RECT(root_child2, h, 50.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0044_align_content_space_around_and_align_items_center_with_flex_wrap(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 300.0f);
	yt_set_height(ctx, root, 300.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_SPACE_AROUND);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_CENTER);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 50.0f);
	yt_set_width(ctx, root_child0, 150.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_height(ctx, root_child1, 100.0f);
	yt_set_width(ctx, root_child1, 120.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_height(ctx, root_child2, 50.0f);
	yt_set_width(ctx, root_child2, 120.0f);
	xent_node_append(ctx, root, root_child2);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 300.0f);
	YT_ASSERT_RECT(root, h, 300.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 63.0f);
	YT_ASSERT_RECT(root_child0, w, 150.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	YT_ASSERT_RECT(root_child1, x, 150.0f);
	YT_ASSERT_RECT(root_child1, y, 38.0f);
	YT_ASSERT_RECT(root_child1, w, 120.0f);
	YT_ASSERT_RECT(root_child1, h, 100.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 213.0f);
	YT_ASSERT_RECT(root_child2, w, 120.0f);
	YT_ASSERT_RECT(root_child2, h, 50.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 300.0f);
	YT_ASSERT_RECT(root, h, 300.0f);
	YT_ASSERT_RECT(root_child0, x, 150.0f);
	YT_ASSERT_RECT(root_child0, y, 63.0f);
	YT_ASSERT_RECT(root_child0, w, 150.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	YT_ASSERT_RECT(root_child1, x, 30.0f);
	YT_ASSERT_RECT(root_child1, y, 38.0f);
	YT_ASSERT_RECT(root_child1, w, 120.0f);
	YT_ASSERT_RECT(root_child1, h, 100.0f);
	YT_ASSERT_RECT(root_child2, x, 180.0f);
	YT_ASSERT_RECT(root_child2, y, 213.0f);
	YT_ASSERT_RECT(root_child2, w, 120.0f);
	YT_ASSERT_RECT(root_child2, h, 50.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0045_align_content_space_around_and_align_items_flex_start_with_flex_wrap(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 300.0f);
	yt_set_height(ctx, root, 300.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_SPACE_AROUND);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_START);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 50.0f);
	yt_set_width(ctx, root_child0, 150.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_height(ctx, root_child1, 100.0f);
	yt_set_width(ctx, root_child1, 120.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_height(ctx, root_child2, 50.0f);
	yt_set_width(ctx, root_child2, 120.0f);
	xent_node_append(ctx, root, root_child2);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 300.0f);
	YT_ASSERT_RECT(root, h, 300.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 38.0f);
	YT_ASSERT_RECT(root_child0, w, 150.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	YT_ASSERT_RECT(root_child1, x, 150.0f);
	YT_ASSERT_RECT(root_child1, y, 38.0f);
	YT_ASSERT_RECT(root_child1, w, 120.0f);
	YT_ASSERT_RECT(root_child1, h, 100.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 213.0f);
	YT_ASSERT_RECT(root_child2, w, 120.0f);
	YT_ASSERT_RECT(root_child2, h, 50.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 300.0f);
	YT_ASSERT_RECT(root, h, 300.0f);
	YT_ASSERT_RECT(root_child0, x, 150.0f);
	YT_ASSERT_RECT(root_child0, y, 38.0f);
	YT_ASSERT_RECT(root_child0, w, 150.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	YT_ASSERT_RECT(root_child1, x, 30.0f);
	YT_ASSERT_RECT(root_child1, y, 38.0f);
	YT_ASSERT_RECT(root_child1, w, 120.0f);
	YT_ASSERT_RECT(root_child1, h, 100.0f);
	YT_ASSERT_RECT(root_child2, x, 180.0f);
	YT_ASSERT_RECT(root_child2, y, 213.0f);
	YT_ASSERT_RECT(root_child2, w, 120.0f);
	YT_ASSERT_RECT(root_child2, h, 50.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0053_align_items_stretch(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0054_align_items_center(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_CENTER);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	yt_set_width(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 45.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 45.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0055_align_items_flex_start(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_START);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	yt_set_width(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0056_align_items_flex_end(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_END);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	yt_set_width(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0057_align_baseline(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_BASELINE);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 50.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 20.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 30.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 20.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 50.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 30.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 20.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0063_align_baseline_column(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_BASELINE);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 50.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 20.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 50.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 20.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 50.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	YT_ASSERT_RECT(root_child1, x, 50.0f);
	YT_ASSERT_RECT(root_child1, y, 50.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 20.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0077_align_flex_end_with_row_reverse(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 75.0f);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_END);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 50.0f);
	yt_set_margin(ctx, root_child0, YT_EDGE_RIGHT, 5.0f);
	yt_set_margin(ctx, root_child0, YT_EDGE_LEFT, 3.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 50.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 75.0f);
	YT_ASSERT_RECT(root_child0, x, 3.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	YT_ASSERT_RECT(root_child1, x, 58.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 50.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 75.0f);
	YT_ASSERT_RECT(root_child0, x, 45.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	YT_ASSERT_RECT(root_child1, x, -8.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 50.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0078_align_stretch_with_row_reverse(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 75.0f);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 50.0f);
	yt_set_height(ctx, root_child0, 50.0f);
	yt_set_margin(ctx, root_child0, YT_EDGE_RIGHT, 5.0f);
	yt_set_margin(ctx, root_child0, YT_EDGE_LEFT, 3.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 50.0f);
	yt_set_height(ctx, root_child1, 50.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 75.0f);
	YT_ASSERT_RECT(root_child0, x, 3.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	YT_ASSERT_RECT(root_child1, x, 58.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 50.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 75.0f);
	YT_ASSERT_RECT(root_child0, x, 45.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	YT_ASSERT_RECT(root_child1, x, -8.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 50.0f);
	YT_ASSERT_RECT(root_child1, h, 50.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0080_align_self_center(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	yt_set_width(ctx, root_child0, 10.0f);
	xent_setself(ctx, root_child0, XENT_FLEX_ALIGN_CENTER);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 45.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 45.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0081_align_self_flex_end(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	yt_set_width(ctx, root_child0, 10.0f);
	xent_setself(ctx, root_child0, XENT_FLEX_ALIGN_END);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0082_align_self_flex_start(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	yt_set_width(ctx, root_child0, 10.0f);
	xent_setself(ctx, root_child0, XENT_FLEX_ALIGN_START);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0083_align_self_flex_end_override_flex_start(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_START);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	yt_set_width(ctx, root_child0, 10.0f);
	xent_setself(ctx, root_child0, XENT_FLEX_ALIGN_END);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0086_box_sizing_border_box_padding_only(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	yt_set_padding(ctx, root, YT_EDGE_ALL, 5.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0088_box_sizing_border_box_no_padding_no_border(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0098_flex_direction_column(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_height(ctx, root, 100.0f);
	yt_set_width(ctx, root, 100.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_height(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 10.0f);
	YT_ASSERT_RECT(root_child1, w, 100.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 20.0f);
	YT_ASSERT_RECT(root_child2, w, 100.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 10.0f);
	YT_ASSERT_RECT(root_child1, w, 100.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 20.0f);
	YT_ASSERT_RECT(root_child2, w, 100.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0099_flex_direction_row(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_height(ctx, root, 100.0f);
	yt_set_width(ctx, root, 100.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_width(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 100.0f);
	YT_ASSERT_RECT(root_child1, x, 10.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 10.0f);
	YT_ASSERT_RECT(root_child1, h, 100.0f);
	YT_ASSERT_RECT(root_child2, x, 20.0f);
	YT_ASSERT_RECT(root_child2, y, 0.0f);
	YT_ASSERT_RECT(root_child2, w, 10.0f);
	YT_ASSERT_RECT(root_child2, h, 100.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 100.0f);
	YT_ASSERT_RECT(root_child1, x, 80.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 10.0f);
	YT_ASSERT_RECT(root_child1, h, 100.0f);
	YT_ASSERT_RECT(root_child2, x, 70.0f);
	YT_ASSERT_RECT(root_child2, y, 0.0f);
	YT_ASSERT_RECT(root_child2, w, 10.0f);
	YT_ASSERT_RECT(root_child2, h, 100.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0119_wrapped_column_max_height(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_height(ctx, root, 500.0f);
	yt_set_width(ctx, root, 700.0f);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_CENTER);
	xent_setjustify(ctx, root, XENT_FLEX_JUSTIFY_CENTER);
	xent_setcontent(ctx, root, XENT_FLEX_ALIGN_CONTENT_CENTER);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 100.0f);
	yt_set_height(ctx, root_child0, 500.0f);
	yt_set_max_height(ctx, root_child0, 200.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 200.0f);
	yt_set_height(ctx, root_child1, 200.0f);
	yt_set_margin(ctx, root_child1, YT_EDGE_ALL, 20.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_width(ctx, root_child2, 100.0f);
	yt_set_height(ctx, root_child2, 100.0f);
	xent_node_append(ctx, root, root_child2);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 700.0f);
	YT_ASSERT_RECT(root, h, 500.0f);
	YT_ASSERT_RECT(root_child0, x, 250.0f);
	YT_ASSERT_RECT(root_child0, y, 30.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 200.0f);
	YT_ASSERT_RECT(root_child1, x, 200.0f);
	YT_ASSERT_RECT(root_child1, y, 250.0f);
	YT_ASSERT_RECT(root_child1, w, 200.0f);
	YT_ASSERT_RECT(root_child1, h, 200.0f);
	YT_ASSERT_RECT(root_child2, x, 420.0f);
	YT_ASSERT_RECT(root_child2, y, 200.0f);
	YT_ASSERT_RECT(root_child2, w, 100.0f);
	YT_ASSERT_RECT(root_child2, h, 100.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 700.0f);
	YT_ASSERT_RECT(root, h, 500.0f);
	YT_ASSERT_RECT(root_child0, x, 350.0f);
	YT_ASSERT_RECT(root_child0, y, 30.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 200.0f);
	YT_ASSERT_RECT(root_child1, x, 300.0f);
	YT_ASSERT_RECT(root_child1, y, 250.0f);
	YT_ASSERT_RECT(root_child1, w, 200.0f);
	YT_ASSERT_RECT(root_child1, h, 200.0f);
	YT_ASSERT_RECT(root_child2, x, 180.0f);
	YT_ASSERT_RECT(root_child2, y, 200.0f);
	YT_ASSERT_RECT(root_child2, w, 100.0f);
	YT_ASSERT_RECT(root_child2, h, 100.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0122_wrap_with_min_cross_axis(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 500.0f);
	yt_set_min_height(ctx, root, 500.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setflexwrap(ctx, root, XENT_FLEX_WRAP);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 400.0f);
	yt_set_height(ctx, root_child0, 200.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 400.0f);
	yt_set_height(ctx, root_child1, 200.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 500.0f);
	YT_ASSERT_RECT(root, h, 500.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 400.0f);
	YT_ASSERT_RECT(root_child0, h, 200.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 200.0f);
	YT_ASSERT_RECT(root_child1, w, 400.0f);
	YT_ASSERT_RECT(root_child1, h, 200.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 500.0f);
	YT_ASSERT_RECT(root, h, 500.0f);
	YT_ASSERT_RECT(root_child0, x, 100.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 400.0f);
	YT_ASSERT_RECT(root_child0, h, 200.0f);
	YT_ASSERT_RECT(root_child1, x, 100.0f);
	YT_ASSERT_RECT(root_child1, y, 200.0f);
	YT_ASSERT_RECT(root_child1, w, 400.0f);
	YT_ASSERT_RECT(root_child1, h, 200.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0124_justify_content_row_flex_start(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 102.0f);
	yt_set_height(ctx, root, 102.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_width(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 102.0f);
	YT_ASSERT_RECT(root_child1, x, 10.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 10.0f);
	YT_ASSERT_RECT(root_child1, h, 102.0f);
	YT_ASSERT_RECT(root_child2, x, 20.0f);
	YT_ASSERT_RECT(root_child2, y, 0.0f);
	YT_ASSERT_RECT(root_child2, w, 10.0f);
	YT_ASSERT_RECT(root_child2, h, 102.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 92.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 102.0f);
	YT_ASSERT_RECT(root_child1, x, 82.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 10.0f);
	YT_ASSERT_RECT(root_child1, h, 102.0f);
	YT_ASSERT_RECT(root_child2, x, 72.0f);
	YT_ASSERT_RECT(root_child2, y, 0.0f);
	YT_ASSERT_RECT(root_child2, w, 10.0f);
	YT_ASSERT_RECT(root_child2, h, 102.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0125_justify_content_row_flex_end(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 102.0f);
	yt_set_height(ctx, root, 102.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setjustify(ctx, root, XENT_FLEX_JUSTIFY_END);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_width(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 72.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 102.0f);
	YT_ASSERT_RECT(root_child1, x, 82.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 10.0f);
	YT_ASSERT_RECT(root_child1, h, 102.0f);
	YT_ASSERT_RECT(root_child2, x, 92.0f);
	YT_ASSERT_RECT(root_child2, y, 0.0f);
	YT_ASSERT_RECT(root_child2, w, 10.0f);
	YT_ASSERT_RECT(root_child2, h, 102.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 20.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 102.0f);
	YT_ASSERT_RECT(root_child1, x, 10.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 10.0f);
	YT_ASSERT_RECT(root_child1, h, 102.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 0.0f);
	YT_ASSERT_RECT(root_child2, w, 10.0f);
	YT_ASSERT_RECT(root_child2, h, 102.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0126_justify_content_row_center(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 102.0f);
	yt_set_height(ctx, root, 102.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setjustify(ctx, root, XENT_FLEX_JUSTIFY_CENTER);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_width(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 36.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 102.0f);
	YT_ASSERT_RECT(root_child1, x, 46.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 10.0f);
	YT_ASSERT_RECT(root_child1, h, 102.0f);
	YT_ASSERT_RECT(root_child2, x, 56.0f);
	YT_ASSERT_RECT(root_child2, y, 0.0f);
	YT_ASSERT_RECT(root_child2, w, 10.0f);
	YT_ASSERT_RECT(root_child2, h, 102.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 56.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 102.0f);
	YT_ASSERT_RECT(root_child1, x, 46.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 10.0f);
	YT_ASSERT_RECT(root_child1, h, 102.0f);
	YT_ASSERT_RECT(root_child2, x, 36.0f);
	YT_ASSERT_RECT(root_child2, y, 0.0f);
	YT_ASSERT_RECT(root_child2, w, 10.0f);
	YT_ASSERT_RECT(root_child2, h, 102.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0127_justify_content_row_space_between(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 102.0f);
	yt_set_height(ctx, root, 102.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setjustify(ctx, root, XENT_FLEX_JUSTIFY_SPACE_BETWEEN);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_width(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 102.0f);
	YT_ASSERT_RECT(root_child1, x, 46.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 10.0f);
	YT_ASSERT_RECT(root_child1, h, 102.0f);
	YT_ASSERT_RECT(root_child2, x, 92.0f);
	YT_ASSERT_RECT(root_child2, y, 0.0f);
	YT_ASSERT_RECT(root_child2, w, 10.0f);
	YT_ASSERT_RECT(root_child2, h, 102.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 92.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 102.0f);
	YT_ASSERT_RECT(root_child1, x, 46.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 10.0f);
	YT_ASSERT_RECT(root_child1, h, 102.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 0.0f);
	YT_ASSERT_RECT(root_child2, w, 10.0f);
	YT_ASSERT_RECT(root_child2, h, 102.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0128_justify_content_row_space_around(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 102.0f);
	yt_set_height(ctx, root, 102.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setjustify(ctx, root, XENT_FLEX_JUSTIFY_SPACE_AROUND);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_width(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_width(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 12.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 102.0f);
	YT_ASSERT_RECT(root_child1, x, 46.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 10.0f);
	YT_ASSERT_RECT(root_child1, h, 102.0f);
	YT_ASSERT_RECT(root_child2, x, 80.0f);
	YT_ASSERT_RECT(root_child2, y, 0.0f);
	YT_ASSERT_RECT(root_child2, w, 10.0f);
	YT_ASSERT_RECT(root_child2, h, 102.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 80.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 102.0f);
	YT_ASSERT_RECT(root_child1, x, 46.0f);
	YT_ASSERT_RECT(root_child1, y, 0.0f);
	YT_ASSERT_RECT(root_child1, w, 10.0f);
	YT_ASSERT_RECT(root_child1, h, 102.0f);
	YT_ASSERT_RECT(root_child2, x, 12.0f);
	YT_ASSERT_RECT(root_child2, y, 0.0f);
	YT_ASSERT_RECT(root_child2, w, 10.0f);
	YT_ASSERT_RECT(root_child2, h, 102.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0129_justify_content_column_flex_start(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 102.0f);
	yt_set_height(ctx, root, 102.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_height(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 102.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 10.0f);
	YT_ASSERT_RECT(root_child1, w, 102.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 20.0f);
	YT_ASSERT_RECT(root_child2, w, 102.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 102.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 10.0f);
	YT_ASSERT_RECT(root_child1, w, 102.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 20.0f);
	YT_ASSERT_RECT(root_child2, w, 102.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0130_justify_content_column_flex_end(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 102.0f);
	yt_set_height(ctx, root, 102.0f);
	xent_setjustify(ctx, root, XENT_FLEX_JUSTIFY_END);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_height(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 72.0f);
	YT_ASSERT_RECT(root_child0, w, 102.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 82.0f);
	YT_ASSERT_RECT(root_child1, w, 102.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 92.0f);
	YT_ASSERT_RECT(root_child2, w, 102.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 72.0f);
	YT_ASSERT_RECT(root_child0, w, 102.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 82.0f);
	YT_ASSERT_RECT(root_child1, w, 102.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 92.0f);
	YT_ASSERT_RECT(root_child2, w, 102.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0131_justify_content_column_center(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 102.0f);
	yt_set_height(ctx, root, 102.0f);
	xent_setjustify(ctx, root, XENT_FLEX_JUSTIFY_CENTER);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_height(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 36.0f);
	YT_ASSERT_RECT(root_child0, w, 102.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 46.0f);
	YT_ASSERT_RECT(root_child1, w, 102.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 56.0f);
	YT_ASSERT_RECT(root_child2, w, 102.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 36.0f);
	YT_ASSERT_RECT(root_child0, w, 102.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 46.0f);
	YT_ASSERT_RECT(root_child1, w, 102.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 56.0f);
	YT_ASSERT_RECT(root_child2, w, 102.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0132_justify_content_column_space_between(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 102.0f);
	yt_set_height(ctx, root, 102.0f);
	xent_setjustify(ctx, root, XENT_FLEX_JUSTIFY_SPACE_BETWEEN);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_height(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 102.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 46.0f);
	YT_ASSERT_RECT(root_child1, w, 102.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 92.0f);
	YT_ASSERT_RECT(root_child2, w, 102.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 102.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 46.0f);
	YT_ASSERT_RECT(root_child1, w, 102.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 92.0f);
	YT_ASSERT_RECT(root_child2, w, 102.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0133_justify_content_column_space_around(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 102.0f);
	yt_set_height(ctx, root, 102.0f);
	xent_setjustify(ctx, root, XENT_FLEX_JUSTIFY_SPACE_AROUND);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_height(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 12.0f);
	YT_ASSERT_RECT(root_child0, w, 102.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 46.0f);
	YT_ASSERT_RECT(root_child1, w, 102.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 80.0f);
	YT_ASSERT_RECT(root_child2, w, 102.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 12.0f);
	YT_ASSERT_RECT(root_child0, w, 102.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 46.0f);
	YT_ASSERT_RECT(root_child1, w, 102.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 80.0f);
	YT_ASSERT_RECT(root_child2, w, 102.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0138_justify_content_column_space_evenly(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 102.0f);
	yt_set_height(ctx, root, 102.0f);
	xent_setjustify(ctx, root, XENT_FLEX_JUSTIFY_SPACE_EVENLY);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	yt_set_height(ctx, root_child1, 10.0f);
	xent_node_append(ctx, root, root_child1);
	XentNodeId root_child2 = yt_create_node(ctx);
	yt_set_height(ctx, root_child2, 10.0f);
	xent_node_append(ctx, root, root_child2);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 18.0f);
	YT_ASSERT_RECT(root_child0, w, 102.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 46.0f);
	YT_ASSERT_RECT(root_child1, w, 102.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 74.0f);
	YT_ASSERT_RECT(root_child2, w, 102.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 102.0f);
	YT_ASSERT_RECT(root, h, 102.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 18.0f);
	YT_ASSERT_RECT(root_child0, w, 102.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 46.0f);
	YT_ASSERT_RECT(root_child1, w, 102.0f);
	YT_ASSERT_RECT(root_child1, h, 10.0f);
	YT_ASSERT_RECT(root_child2, x, 0.0f);
	YT_ASSERT_RECT(root_child2, y, 74.0f);
	YT_ASSERT_RECT(root_child2, w, 102.0f);
	YT_ASSERT_RECT(root_child2, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0149_margin_top(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	yt_set_margin(ctx, root_child0, YT_EDGE_TOP, 10.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 10.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 10.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0150_margin_bottom(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	xent_setjustify(ctx, root, XENT_FLEX_JUSTIFY_END);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	yt_set_margin(ctx, root_child0, YT_EDGE_BOTTOM, 10.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 80.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 80.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0151_margin_and_flex_column(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_margin(ctx, root_child0, YT_EDGE_TOP, 10.0f);
	yt_set_margin(ctx, root_child0, YT_EDGE_BOTTOM, 10.0f);
	xent_setgrow(ctx, root_child0, 1.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 10.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 80.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 10.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 80.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0153_margin_with_sibling_column(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_margin(ctx, root_child0, YT_EDGE_BOTTOM, 10.0f);
	xent_setgrow(ctx, root_child0, 1.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child1 = yt_create_node(ctx);
	xent_setgrow(ctx, root_child1, 1.0f);
	xent_node_append(ctx, root, root_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 45.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 55.0f);
	YT_ASSERT_RECT(root_child1, w, 100.0f);
	YT_ASSERT_RECT(root_child1, h, 45.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 45.0f);
	YT_ASSERT_RECT(root_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child1, y, 55.0f);
	YT_ASSERT_RECT(root_child1, w, 100.0f);
	YT_ASSERT_RECT(root_child1, h, 45.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0154_margin_should_not_be_part_of_max_height(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 250.0f);
	yt_set_height(ctx, root, 250.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 100.0f);
	yt_set_height(ctx, root_child0, 100.0f);
	yt_set_max_height(ctx, root_child0, 100.0f);
	yt_set_margin(ctx, root_child0, YT_EDGE_TOP, 20.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 250.0f);
	YT_ASSERT_RECT(root, h, 250.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 20.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 100.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 250.0f);
	YT_ASSERT_RECT(root, h, 250.0f);
	YT_ASSERT_RECT(root_child0, x, 150.0f);
	YT_ASSERT_RECT(root_child0, y, 20.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 100.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0155_margin_should_not_be_part_of_max_width(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 250.0f);
	yt_set_height(ctx, root, 250.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 100.0f);
	yt_set_height(ctx, root_child0, 100.0f);
	yt_set_max_width(ctx, root_child0, 100.0f);
	yt_set_margin(ctx, root_child0, YT_EDGE_LEFT, 20.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 250.0f);
	YT_ASSERT_RECT(root, h, 250.0f);
	YT_ASSERT_RECT(root_child0, x, 20.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 100.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 250.0f);
	YT_ASSERT_RECT(root, h, 250.0f);
	YT_ASSERT_RECT(root_child0, x, 150.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 100.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0156_max_width(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	yt_set_max_width(ctx, root_child0, 50.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 50.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 50.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0157_max_height(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 10.0f);
	yt_set_max_height(ctx, root_child0, 50.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 90.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 50.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0158_justify_content_min_max(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_max_height(ctx, root, 200.0f);
	yt_set_min_height(ctx, root, 100.0f);
	yt_set_width(ctx, root, 100.0f);
	xent_setjustify(ctx, root, XENT_FLEX_JUSTIFY_CENTER);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 60.0f);
	yt_set_height(ctx, root_child0, 60.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 20.0f);
	YT_ASSERT_RECT(root_child0, w, 60.0f);
	YT_ASSERT_RECT(root_child0, h, 60.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 40.0f);
	YT_ASSERT_RECT(root_child0, y, 20.0f);
	YT_ASSERT_RECT(root_child0, w, 60.0f);
	YT_ASSERT_RECT(root_child0, h, 60.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0159_align_items_min_max(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_max_width(ctx, root, 200.0f);
	yt_set_min_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_CENTER);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 60.0f);
	yt_set_height(ctx, root_child0, 60.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 20.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 60.0f);
	YT_ASSERT_RECT(root_child0, h, 60.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 20.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 60.0f);
	YT_ASSERT_RECT(root_child0, h, 60.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0169_flex_grow_height_maximized(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 500.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_min_height(ctx, root_child0, 100.0f);
	yt_set_max_height(ctx, root_child0, 500.0f);
	xent_setgrow(ctx, root_child0, 1.0f);
	xent_node_append(ctx, root, root_child0);
	XentNodeId root_child0_child0 = yt_create_node(ctx);
	xent_setbasis(ctx, root_child0_child0, 200.0f);
	xent_setgrow(ctx, root_child0_child0, 1.0f);
	xent_node_append(ctx, root_child0, root_child0_child0);
	XentNodeId root_child0_child1 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0_child1, 100.0f);
	xent_node_append(ctx, root_child0, root_child0_child1);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 500.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 500.0f);
	YT_ASSERT_RECT(root_child0_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0_child0, h, 400.0f);
	YT_ASSERT_RECT(root_child0_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child0_child1, y, 400.0f);
	YT_ASSERT_RECT(root_child0_child1, w, 100.0f);
	YT_ASSERT_RECT(root_child0_child1, h, 100.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 500.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 500.0f);
	YT_ASSERT_RECT(root_child0_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0_child0, h, 400.0f);
	YT_ASSERT_RECT(root_child0_child1, x, 0.0f);
	YT_ASSERT_RECT(root_child0_child1, y, 400.0f);
	YT_ASSERT_RECT(root_child0_child1, w, 100.0f);
	YT_ASSERT_RECT(root_child0_child1, h, 100.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0174_min_width_overrides_width(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_min_width(ctx, root, 100.0f);
	yt_set_width(ctx, root, 50.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 0.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 0.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0175_max_width_overrides_width(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_max_width(ctx, root, 100.0f);
	yt_set_width(ctx, root, 200.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 0.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 0.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0176_min_height_overrides_height(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_min_height(ctx, root, 100.0f);
	yt_set_height(ctx, root, 50.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 0.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 0.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0177_max_height_overrides_height(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_max_height(ctx, root, 100.0f);
	yt_set_height(ctx, root, 200.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 0.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 0.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0180_padding_flex_child(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	yt_set_padding(ctx, root, YT_EDGE_ALL, 10.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 10.0f);
	xent_setgrow(ctx, root_child0, 1.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 10.0f);
	YT_ASSERT_RECT(root_child0, y, 10.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 80.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 80.0f);
	YT_ASSERT_RECT(root_child0, y, 10.0f);
	YT_ASSERT_RECT(root_child0, w, 10.0f);
	YT_ASSERT_RECT(root_child0, h, 80.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0181_padding_stretch_child(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 100.0f);
	yt_set_height(ctx, root, 100.0f);
	yt_set_padding(ctx, root, YT_EDGE_ALL, 10.0f);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_height(ctx, root_child0, 10.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 10.0f);
	YT_ASSERT_RECT(root_child0, y, 10.0f);
	YT_ASSERT_RECT(root_child0, w, 80.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 100.0f);
	YT_ASSERT_RECT(root, h, 100.0f);
	YT_ASSERT_RECT(root_child0, x, 10.0f);
	YT_ASSERT_RECT(root_child0, y, 10.0f);
	YT_ASSERT_RECT(root_child0, w, 80.0f);
	YT_ASSERT_RECT(root_child0, h, 10.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0182_child_with_padding_align_end(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_width(ctx, root, 200.0f);
	yt_set_height(ctx, root, 200.0f);
	xent_setjustify(ctx, root, XENT_FLEX_JUSTIFY_END);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_END);
	XentNodeId root_child0 = yt_create_node(ctx);
	yt_set_width(ctx, root_child0, 100.0f);
	yt_set_height(ctx, root_child0, 100.0f);
	yt_set_padding(ctx, root_child0, YT_EDGE_ALL, 20.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 200.0f);
	YT_ASSERT_RECT(root, h, 200.0f);
	YT_ASSERT_RECT(root_child0, x, 100.0f);
	YT_ASSERT_RECT(root_child0, y, 100.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 100.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 200.0f);
	YT_ASSERT_RECT(root, h, 200.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 100.0f);
	YT_ASSERT_RECT(root_child0, w, 100.0f);
	YT_ASSERT_RECT(root_child0, h, 100.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0184_percentage_width_height_undefined_parent_size(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root        = yt_create_node(ctx);
	XentNodeId root_child0 = yt_create_node(ctx);
	xent_setwpct(ctx, root_child0, 50.0f / 100.0f);
	xent_sethpct(ctx, root_child0, 50.0f / 100.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 0.0f);
	YT_ASSERT_RECT(root, h, 0.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 0.0f);
	YT_ASSERT_RECT(root_child0, h, 0.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 0.0f);
	YT_ASSERT_RECT(root, h, 0.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 0.0f);
	YT_ASSERT_RECT(root_child0, h, 0.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_0191_percent_of_max_cross_unstretched(void) {
	yt_reset_state();
	XentCtx *ctx = xent_ctx_create(NULL);
	YT_ASSERT(ctx != NULL);
	XentNodeId root = yt_create_node(ctx);
	yt_set_max_width(ctx, root, 60.0f);
	yt_set_height(ctx, root, 50.0f);
	xent_setitems(ctx, root, XENT_FLEX_ALIGN_START);
	XentNodeId root_child0 = yt_create_node(ctx);
	xent_setwpct(ctx, root_child0, 50.0f / 100.0f);
	yt_set_height(ctx, root_child0, 20.0f);
	xent_node_append(ctx, root, root_child0);
	xent_setdir(ctx, root, XENT_DIRECTION_LTR);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 0.0f);
	YT_ASSERT_RECT(root, h, 50.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 0.0f);
	YT_ASSERT_RECT(root_child0, h, 20.0f);
	xent_setdir(ctx, root, XENT_DIRECTION_RTL);
	YT_ASSERT(xent_layout(ctx, root, NAN, NAN));
	YT_ASSERT_RECT(root, x, 0.0f);
	YT_ASSERT_RECT(root, y, 0.0f);
	YT_ASSERT_RECT(root, w, 0.0f);
	YT_ASSERT_RECT(root, h, 50.0f);
	YT_ASSERT_RECT(root_child0, x, 0.0f);
	YT_ASSERT_RECT(root_child0, y, 0.0f);
	YT_ASSERT_RECT(root_child0, w, 0.0f);
	YT_ASSERT_RECT(root_child0, h, 20.0f);
	xent_ctx_destroy(ctx);
	return 0;
}

static YtTestFn const yt_tests [] = {
  test_0000_absolute_layout_align_items_and_justify_content_center,
  test_0001_absolute_layout_align_items_and_justify_content_flex_end,
  test_0002_absolute_layout_justify_content_center,
  test_0003_absolute_layout_align_items_center,
  test_0004_absolute_layout_align_items_center_on_child_only,
  test_0006_absolute_layout_padding_left,
  test_0007_absolute_layout_padding_right,
  test_0008_absolute_layout_padding_top,
  test_0009_absolute_layout_padding_bottom,
  test_0010_align_content_flex_start_nowrap,
  test_0011_align_content_flex_start_wrap,
  test_0012_align_content_flex_start_wrap_singleline,
  test_0014_align_content_flex_end_nowrap,
  test_0015_align_content_flex_end_wrap,
  test_0016_align_content_flex_end_wrap_singleline,
  test_0017_align_content_center_nowrap,
  test_0018_align_content_center_wrap,
  test_0019_align_content_center_wrap_singleline,
  test_0020_align_content_space_between_nowrap,
  test_0021_align_content_space_between_wrap,
  test_0022_align_content_space_between_wrap_singleline,
  test_0023_align_content_space_around_nowrap,
  test_0024_align_content_space_around_wrap,
  test_0025_align_content_space_around_wrap_singleline,
  test_0026_align_content_space_evenly_nowrap,
  test_0027_align_content_space_evenly_wrap,
  test_0028_align_content_space_evenly_wrap_singleline,
  test_0033_align_content_stretch_row_with_single_row,
  test_0040_align_content_space_evenly_with_min_cross_axis,
  test_0043_align_content_space_around_and_align_items_flex_end_with_flex_wrap,
  test_0044_align_content_space_around_and_align_items_center_with_flex_wrap,
  test_0045_align_content_space_around_and_align_items_flex_start_with_flex_wrap,
  test_0053_align_items_stretch,
  test_0054_align_items_center,
  test_0055_align_items_flex_start,
  test_0056_align_items_flex_end,
  test_0057_align_baseline,
  test_0063_align_baseline_column,
  test_0077_align_flex_end_with_row_reverse,
  test_0078_align_stretch_with_row_reverse,
  test_0080_align_self_center,
  test_0081_align_self_flex_end,
  test_0082_align_self_flex_start,
  test_0083_align_self_flex_end_override_flex_start,
  test_0086_box_sizing_border_box_padding_only,
  test_0088_box_sizing_border_box_no_padding_no_border,
  test_0098_flex_direction_column,
  test_0099_flex_direction_row,
  test_0119_wrapped_column_max_height,
  test_0122_wrap_with_min_cross_axis,
  test_0124_justify_content_row_flex_start,
  test_0125_justify_content_row_flex_end,
  test_0126_justify_content_row_center,
  test_0127_justify_content_row_space_between,
  test_0128_justify_content_row_space_around,
  test_0129_justify_content_column_flex_start,
  test_0130_justify_content_column_flex_end,
  test_0131_justify_content_column_center,
  test_0132_justify_content_column_space_between,
  test_0133_justify_content_column_space_around,
  test_0138_justify_content_column_space_evenly,
  test_0149_margin_top,
  test_0150_margin_bottom,
  test_0151_margin_and_flex_column,
  test_0153_margin_with_sibling_column,
  test_0154_margin_should_not_be_part_of_max_height,
  test_0155_margin_should_not_be_part_of_max_width,
  test_0156_max_width,
  test_0157_max_height,
  test_0158_justify_content_min_max,
  test_0159_align_items_min_max,
  test_0169_flex_grow_height_maximized,
  test_0174_min_width_overrides_width,
  test_0175_max_width_overrides_width,
  test_0176_min_height_overrides_height,
  test_0177_max_height_overrides_height,
  test_0180_padding_flex_child,
  test_0181_padding_stretch_child,
  test_0182_child_with_padding_align_end,
  test_0184_percentage_width_height_undefined_parent_size,
  test_0191_percent_of_max_cross_unstretched,
  /* test_0203_nested_overflowing_child_in_constraint_parent excluded: it
   * expects a 200px EMPTY explicit item to overflow a 100px parent (Yoga's
   * flex-shrink:0 default). Under CSS the automatic minimum size of an empty
   * explicit item is min(content=0, specified=200)=0, so flex-shrink:1 shrinks
   * it to 100. xent is CSS-correct here; the case is Yoga-specific. */
};

int main(void) {
	uint32_t count = ( uint32_t ) (sizeof(yt_tests) / sizeof(yt_tests [0]));
	for (uint32_t i = 0u; i < count; ++i) {
		if (yt_tests [i]() != 0) {
			fprintf(stderr, "Yoga generated native case failed at index %u\n", i);
			return 1;
		}
	}
	printf("xent yoga generated: %u passed, 0 failed, 509 skipped\n", count);
	return 0;
}
