#include "../xent_internal.h"

static bool xent_is_valid_direction(XentDirection direction) {
	return direction == XENT_DIRECTION_INHERIT || direction == XENT_DIRECTION_LTR || direction == XENT_DIRECTION_RTL;
}

static XentDirection xent_resolve_direction_internal(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return XENT_DIRECTION_LTR;
	for (XentNodeId cursor = node; cursor != XENT_NODE_INVALID; cursor = ctx->nodes.topology.parent [cursor]) {
		XentDirection direction = ( XentDirection ) ctx->nodes.layout.direction [cursor];
		if (direction == XENT_DIRECTION_LTR || direction == XENT_DIRECTION_RTL) return direction;
	}
	return XENT_DIRECTION_LTR;
}

static XentResolvedInsets xent_resolve_logical_insets(XentInsets insets, XentDirection direction, XentAxis main_axis) {
	if (main_axis == XENT_AXIS_HORIZONTAL) {
		return (XentResolvedInsets) {
		  (direction == XENT_DIRECTION_RTL) ? insets.right : insets.left,
		  (direction == XENT_DIRECTION_RTL) ? insets.left : insets.right,
		  insets.top,
		  insets.bottom,
		};
	}

	return (XentResolvedInsets) {
	  insets.top,
	  insets.bottom,
	  (direction == XENT_DIRECTION_RTL) ? insets.right : insets.left,
	  (direction == XENT_DIRECTION_RTL) ? insets.left : insets.right,
	};
}

static XentInsets xent_layout_insets(float left, float top, float right, float bottom) {
	return (XentInsets) {left, top, right, bottom};
}

static bool xent_get_resolved_layout_insets(
  XentContext const *ctx, XentNodeId node, XentAxis main_axis, XentInsets insets, XentResolvedInsets *out_insets
) {
	if (!xent_is_valid_node(ctx, node) || !out_insets) return false;
	*out_insets = xent_resolve_logical_insets(insets, xent_resolve_direction_internal(ctx, node), main_axis);
	return true;
}

static bool xent_set_layout_size_fields(XentContext *ctx, XentNodeId node, XentSize size, float *width, float *height) {
	if (!xent_is_valid_node(ctx, node)) return false;
	*width  = size.w;
	*height = size.h;
	if (width == &ctx->nodes.layout.style_w [node]) ctx->nodes.layout.style_w_percent [node] = NAN;
	if (height == &ctx->nodes.layout.style_h [node]) ctx->nodes.layout.style_h_percent [node] = NAN;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

static bool
xent_set_layout_percent_field(XentContext *ctx, XentNodeId node, float fraction, float *percent, float *absolute) {
	if (!xent_is_valid_node(ctx, node) || !isfinite(fraction) || fraction < 0.0f) return false;
	*percent  = fraction;
	*absolute = NAN;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

typedef struct XentLayoutInsetArrays {
	float *l;
	float *t;
	float *r;
	float *b;
} XentLayoutInsetArrays;

static void xent_write_insets(XentInsets insets, XentLayoutInsetArrays arrays, XentNodeId node) {
	arrays.l [node] = insets.left;
	arrays.t [node] = insets.top;
	arrays.r [node] = insets.right;
	arrays.b [node] = insets.bottom;
}

bool xent_set_protocol(XentContext *ctx, XentNodeId node, XentProtocol protocol) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.layout.protocol [node] = ( uint8_t ) protocol;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

XentProtocol xent_get_protocol(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return XENT_PROTOCOL_ABSOLUTE;
	return ( XentProtocol ) ctx->nodes.layout.protocol [node];
}

bool xent_set_direction(XentContext *ctx, XentNodeId node, XentDirection direction) {
	if (!xent_is_valid_node(ctx, node) || !xent_is_valid_direction(direction)) return false;
	if (ctx->nodes.layout.direction [node] != ( uint8_t ) direction) {
		ctx->nodes.layout.direction [node] = ( uint8_t ) direction;
		xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT | XENT_DIRTY_SUBTREE);
	}
	return true;
}

XentDirection xent_get_direction(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return XENT_DIRECTION_INHERIT;
	return ( XentDirection ) ctx->nodes.layout.direction [node];
}

XentDirection xent_get_resolved_direction(XentContext const *ctx, XentNodeId node) {
	return xent_resolve_direction_internal(ctx, node);
}

bool xent_get_resolved_margin(
  XentContext const *ctx, XentNodeId node, XentAxis main_axis, XentResolvedInsets *out_insets
) {
	return xent_get_resolved_layout_insets(
	  ctx, node, main_axis,
	  xent_layout_insets(
		ctx->nodes.layout.margin_l [node], ctx->nodes.layout.margin_t [node], ctx->nodes.layout.margin_r [node],
		ctx->nodes.layout.margin_b [node]
	  ),
	  out_insets
	);
}

bool xent_get_resolved_padding(
  XentContext const *ctx, XentNodeId node, XentAxis main_axis, XentResolvedInsets *out_insets
) {
	return xent_get_resolved_layout_insets(
	  ctx, node, main_axis,
	  xent_layout_insets(
		ctx->nodes.layout.padding_l [node], ctx->nodes.layout.padding_t [node], ctx->nodes.layout.padding_r [node],
		ctx->nodes.layout.padding_b [node]
	  ),
	  out_insets
	);
}

bool xent_set_size(XentContext *ctx, XentNodeId node, XentSize size) {
	return xent_set_layout_size_fields(
	  ctx, node, size, &ctx->nodes.layout.style_w [node], &ctx->nodes.layout.style_h [node]
	);
}

bool xent_set_width(XentContext *ctx, XentNodeId node, float width) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.layout.style_w [node]         = width;
	ctx->nodes.layout.style_w_percent [node] = NAN;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_set_height(XentContext *ctx, XentNodeId node, float height) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.layout.style_h [node]         = height;
	ctx->nodes.layout.style_h_percent [node] = NAN;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_set_width_percent(XentContext *ctx, XentNodeId node, float fraction) {
	if (!xent_is_valid_node(ctx, node)) return false;
	return xent_set_layout_percent_field(
	  ctx, node, fraction, &ctx->nodes.layout.style_w_percent [node], &ctx->nodes.layout.style_w [node]
	);
}

bool xent_set_height_percent(XentContext *ctx, XentNodeId node, float fraction) {
	if (!xent_is_valid_node(ctx, node)) return false;
	return xent_set_layout_percent_field(
	  ctx, node, fraction, &ctx->nodes.layout.style_h_percent [node], &ctx->nodes.layout.style_h [node]
	);
}

bool xent_set_size_percent(XentContext *ctx, XentNodeId node, XentSize fraction) {
	if (!xent_is_valid_node(ctx, node)) return false;
	if (!isfinite(fraction.w) || fraction.w < 0.0f || !isfinite(fraction.h) || fraction.h < 0.0f) return false;
	return xent_set_width_percent(ctx, node, fraction.w) && xent_set_height_percent(ctx, node, fraction.h);
}

bool xent_set_wrap_content(XentContext *ctx, XentNodeId node, bool wrap_width, bool wrap_height) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.layout.wrap_content_w [node] = wrap_width ? 1u : 0u;
	ctx->nodes.layout.wrap_content_h [node] = wrap_height ? 1u : 0u;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_set_aspect_ratio(XentContext *ctx, XentNodeId node, float aspect_ratio) {
	if (!xent_is_valid_node(ctx, node) || (!isnan(aspect_ratio) && (!isfinite(aspect_ratio) || aspect_ratio <= 0.0f)))
		return false;
	ctx->nodes.layout.aspect_ratio [node] = aspect_ratio;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_set_min_size(XentContext *ctx, XentNodeId node, XentSize size) {
	return xent_set_layout_size_fields(
	  ctx, node, size, &ctx->nodes.layout.min_w [node], &ctx->nodes.layout.min_h [node]
	);
}

bool xent_set_max_size(XentContext *ctx, XentNodeId node, XentSize size) {
	return xent_set_layout_size_fields(
	  ctx, node, size, &ctx->nodes.layout.max_w [node], &ctx->nodes.layout.max_h [node]
	);
}

bool xent_set_margin(XentContext *ctx, XentNodeId node, XentInsets margin) {
	if (!xent_is_valid_node(ctx, node)) return false;
	xent_write_insets(
	  margin,
	  (XentLayoutInsetArrays) {
		ctx->nodes.layout.margin_l, ctx->nodes.layout.margin_t, ctx->nodes.layout.margin_r, ctx->nodes.layout.margin_b},
	  node
	);
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_set_padding(XentContext *ctx, XentNodeId node, XentInsets padding) {
	if (!xent_is_valid_node(ctx, node)) return false;
	xent_write_insets(
	  padding,
	  (XentLayoutInsetArrays) {
		ctx->nodes.layout.padding_l, ctx->nodes.layout.padding_t, ctx->nodes.layout.padding_r,
		ctx->nodes.layout.padding_b},
	  node
	);
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_set_gap(XentContext *ctx, XentNodeId node, float gap) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.layout.gap [node] = gap < 0.0f ? 0.0f : gap;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_set_z_index(XentContext *ctx, XentNodeId node, int32_t z_index) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.layout.z_index [node] = z_index;
	return true;
}

int32_t xent_get_z_index(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return 0;
	return ctx->nodes.layout.z_index [node];
}

bool xent_get_layout_rect(XentContext const *ctx, XentNodeId node, XentRect *out_rect) {
	if (!xent_is_valid_node(ctx, node) || !out_rect) return false;
	out_rect->x = ctx->nodes.layout.abs_x [node];
	out_rect->y = ctx->nodes.layout.abs_y [node];
	out_rect->w = ctx->nodes.layout.decided_w [node];
	out_rect->h = ctx->nodes.layout.decided_h [node];
	return true;
}

XentNodeId xent_get_last_layout_root(XentContext const *ctx) {
	if (!ctx) return XENT_NODE_INVALID;
	return ctx->last_layout_root;
}

XentLayoutStrategy xent_get_last_layout_strategy(XentContext const *ctx) {
	if (!ctx) return XENT_LAYOUT_STRATEGY_NONE;
	return ( XentLayoutStrategy ) ctx->last_layout_strategy;
}
