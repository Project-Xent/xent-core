#include "../xent_internal.h"

static bool node_has_text(XentCtx const *ctx, uint32_t index) {
	char const *text = ctx->nodes.text.content [index];
	return text && text [0] != '\0';
}

static float node_height(XentCtx const *ctx, XentNodeId node, float fallback) {
	uint32_t index  = xent_node_index(node);
	float    height = ctx->nodes.layout.style_h [index];
	if (isfinite(height)) return height;
	height = ctx->nodes.layout.decided_h [index];
	return height > 0.0f && isfinite(height) ? height : fallback;
}

static float child_outer_width(XentCtx const *ctx, XentNodeId child) {
	uint32_t index = xent_node_index(child);
	float    width = ctx->nodes.layout.style_w [index];
	if (!isfinite(width)) width = ctx->nodes.layout.decided_w [index];
	if (!isfinite(width) || width < 0.0f) width = 0.0f;
	return width + ctx->nodes.layout.margin_l [index] + ctx->nodes.layout.margin_r [index];
}

static bool child_uses_baseline(XentCtx const *ctx, uint32_t parent, XentNodeId child) {
	uint8_t align = ctx->nodes.flex.align_self [xent_node_index(child)];
	if (align == XENT_FLEX_ALIGN_AUTO) align = ctx->nodes.flex.align_items [parent];
	return align == XENT_FLEX_ALIGN_BASELINE;
}

static float child_baseline(XentCtx *ctx, XentNodeId child, float fallback) {
	uint32_t index  = xent_node_index(child);
	float    height = node_height(ctx, child, fallback);
	return ctx->nodes.layout.margin_t [index] + xent_node_baseline(ctx, child, height);
}

typedef struct RowBaselineScan {
	uint32_t parent;
	float    available;
	float    used;
	float    shared;
	bool     has_item;
	bool     has_baseline;
} RowBaselineScan;

static bool beyond_first_line(XentCtx const *ctx, RowBaselineScan const *scan, XentNodeId child) {
	if (!ctx->nodes.flex.wrap [scan->parent] || !scan->has_item || !isfinite(scan->available)) return false;
	float gap = ctx->nodes.layout.gap [scan->parent];
	if (gap < 0.0f) gap = 0.0f;
	return scan->used + gap + child_outer_width(ctx, child) > scan->available;
}

static bool scan_row_child(XentCtx *ctx, RowBaselineScan *scan, XentNodeId child, float fallback) {
	if (beyond_first_line(ctx, scan, child)) return false;
	float gap = ctx->nodes.layout.gap [scan->parent];
	if (gap < 0.0f) gap = 0.0f;
	if (scan->has_item) scan->used += gap;
	scan->used     += child_outer_width(ctx, child);
	scan->has_item  = true;
	if (!child_uses_baseline(ctx, scan->parent, child)) return true;
	float baseline = child_baseline(ctx, child, fallback);
	if (!scan->has_baseline || baseline > scan->shared) scan->shared = baseline;
	scan->has_baseline = true;
	return true;
}

static float flex_row_baseline(XentCtx *ctx, XentNodeId node, float fallback) {
	uint32_t        parent = xent_node_index(node);
	RowBaselineScan scan   = {
	  .parent    = parent,
	  .available = ctx->nodes.layout.style_w [parent]
	             - ctx->nodes.layout.padding_l [parent]
	             - ctx->nodes.layout.padding_r [parent],
	};
	XentNodeId first = ctx->nodes.topology.first_child [parent];

	for (XentNodeId child = first; child != XENT_NODE_INVALID;
	  child               = ctx->nodes.topology.next_sibling [xent_node_index(child)])
		if (!scan_row_child(ctx, &scan, child, fallback)) break;

	float top = ctx->nodes.layout.padding_t [parent];
	if (scan.has_baseline) return top + scan.shared;
	if (first != XENT_NODE_INVALID) return top + child_baseline(ctx, first, fallback);
	return fallback;
}

static float flex_column_baseline(XentCtx *ctx, XentNodeId node, float fallback) {
	uint32_t   parent = xent_node_index(node);
	XentNodeId first  = ctx->nodes.topology.first_child [parent];
	if (first == XENT_NODE_INVALID) return fallback;
	return ctx->nodes.layout.padding_t [parent] + child_baseline(ctx, first, fallback);
}

float xent_node_baseline(XentCtx *ctx, XentNodeId node, float cross_size) {
	if (cross_size <= 0.0f) return 0.0f;
	uint32_t index = xent_node_index(node);
	if (node_has_text(ctx, index)) {
		ctx->profile.text_baseline_fallbacks += 1u;
		return cross_size * 0.8f;
	}
	if (ctx->nodes.layout.protocol [index] != XENT_PROTOCOL_FLEX) return cross_size;
	if (ctx->nodes.topology.first_child [index] == XENT_NODE_INVALID) return cross_size;
	if (ctx->nodes.flex.direction [index] == XENT_FLEX_ROW) return flex_row_baseline(ctx, node, cross_size);
	return flex_column_baseline(ctx, node, cross_size);
}
