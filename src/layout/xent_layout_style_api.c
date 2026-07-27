#include "../xent_internal.h"

static bool is_valid_direction(XentDirection direction) {
	return direction == XENT_DIRECTION_INHERIT || direction == XENT_DIRECTION_LTR || direction == XENT_DIRECTION_RTL;
}

static XentDirection resolve_direction_internal(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return XENT_DIRECTION_LTR;
	for (XentNodeId cursor = node; cursor != XENT_NODE_INVALID;
	  cursor               = ctx->nodes.topology.parent [xent_node_index(cursor)])
	{
		XentDirection direction = ( XentDirection ) ctx->nodes.layout.direction [xent_node_index(cursor)];
		if (direction == XENT_DIRECTION_LTR || direction == XENT_DIRECTION_RTL) return direction;
	}
	return XENT_DIRECTION_LTR;
}

static XentResolvedInsets resolve_logical_insets(XentInsets insets, XentDirection direction, XentAxis main_axis) {
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

static XentInsets layout_insets(float left, float top, float right, float bottom) {
	return (XentInsets) {left, top, right, bottom};
}

static bool get_resolved_layout_insets(
  XentCtx const *ctx, XentNodeId node, XentAxis main_axis, XentInsets insets, XentResolvedInsets *out_insets
) {
	if (!xent_node_valid(ctx, node) || !out_insets) return false;
	*out_insets = resolve_logical_insets(insets, resolve_direction_internal(ctx, node), main_axis);
	return true;
}

static bool set_layout_size_fields(XentCtx *ctx, XentNodeId node, XentSize size, float *width, float *height) {
	if (!xent_node_valid(ctx, node)) return false;
	*width  = size.w;
	*height = size.h;
	if (width == &ctx->nodes.layout.style_w [xent_node_index(node)])
		ctx->nodes.layout.style_w_percent [xent_node_index(node)] = NAN;
	if (height == &ctx->nodes.layout.style_h [xent_node_index(node)])
		ctx->nodes.layout.style_h_percent [xent_node_index(node)] = NAN;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

static bool set_layout_percent_field(XentCtx *ctx, XentNodeId node, float fraction, float *percent, float *absolute) {
	if (!xent_node_valid(ctx, node) || !isfinite(fraction) || fraction < 0.0f) return false;
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

static void write_insets(XentInsets insets, XentLayoutInsetArrays arrays, XentNodeId node) {
	uint32_t index   = xent_node_index(node);
	arrays.l [index] = insets.left;
	arrays.t [index] = insets.top;
	arrays.r [index] = insets.right;
	arrays.b [index] = insets.bottom;
}

bool xent_setproto(XentCtx *ctx, XentNodeId node, XentProtocol protocol) {
	if (!xent_node_valid(ctx, node)) return false;
	ctx->nodes.layout.protocol [xent_node_index(node)] = ( uint8_t ) protocol;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

XentProtocol xent_node_proto(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return XENT_PROTOCOL_ABSOLUTE;
	return ( XentProtocol ) ctx->nodes.layout.protocol [xent_node_index(node)];
}

bool xent_setdir(XentCtx *ctx, XentNodeId node, XentDirection direction) {
	if (!xent_node_valid(ctx, node) || !is_valid_direction(direction)) return false;
	if (ctx->nodes.layout.direction [xent_node_index(node)] != ( uint8_t ) direction) {
		ctx->nodes.layout.direction [xent_node_index(node)] = ( uint8_t ) direction;
		xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT | XENT_DIRTY_SUBTREE);
	}
	return true;
}

XentDirection xent_dir(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return XENT_DIRECTION_INHERIT;
	return ( XentDirection ) ctx->nodes.layout.direction [xent_node_index(node)];
}

XentDirection xent_resolved_dir(XentCtx const *ctx, XentNodeId node) { return resolve_direction_internal(ctx, node); }

bool          xent_resolved_m(XentCtx const *ctx, XentNodeId node, XentAxis main_axis, XentResolvedInsets *out_insets) {
	return get_resolved_layout_insets(
	  ctx, node, main_axis,
	  layout_insets(
		ctx->nodes.layout.margin_l [xent_node_index(node)], ctx->nodes.layout.margin_t [xent_node_index(node)],
		ctx->nodes.layout.margin_r [xent_node_index(node)], ctx->nodes.layout.margin_b [xent_node_index(node)]
	  ),
	  out_insets
	);
}

bool xent_resolved_p(XentCtx const *ctx, XentNodeId node, XentAxis main_axis, XentResolvedInsets *out_insets) {
	return get_resolved_layout_insets(
	  ctx, node, main_axis,
	  layout_insets(
		ctx->nodes.layout.padding_l [xent_node_index(node)], ctx->nodes.layout.padding_t [xent_node_index(node)],
		ctx->nodes.layout.padding_r [xent_node_index(node)], ctx->nodes.layout.padding_b [xent_node_index(node)]
	  ),
	  out_insets
	);
}

bool xent_setsize(XentCtx *ctx, XentNodeId node, XentSize size) {
	return set_layout_size_fields(
	  ctx, node, size, &ctx->nodes.layout.style_w [xent_node_index(node)],
	  &ctx->nodes.layout.style_h [xent_node_index(node)]
	);
}

bool xent_setw(XentCtx *ctx, XentNodeId node, float width) {
	if (!xent_node_valid(ctx, node)) return false;
	ctx->nodes.layout.style_w [xent_node_index(node)]         = width;
	ctx->nodes.layout.style_w_percent [xent_node_index(node)] = NAN;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_seth(XentCtx *ctx, XentNodeId node, float height) {
	if (!xent_node_valid(ctx, node)) return false;
	ctx->nodes.layout.style_h [xent_node_index(node)]         = height;
	ctx->nodes.layout.style_h_percent [xent_node_index(node)] = NAN;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_setwpct(XentCtx *ctx, XentNodeId node, float fraction) {
	if (!xent_node_valid(ctx, node)) return false;
	return set_layout_percent_field(
	  ctx, node, fraction, &ctx->nodes.layout.style_w_percent [xent_node_index(node)],
	  &ctx->nodes.layout.style_w [xent_node_index(node)]
	);
}

bool xent_sethpct(XentCtx *ctx, XentNodeId node, float fraction) {
	if (!xent_node_valid(ctx, node)) return false;
	return set_layout_percent_field(
	  ctx, node, fraction, &ctx->nodes.layout.style_h_percent [xent_node_index(node)],
	  &ctx->nodes.layout.style_h [xent_node_index(node)]
	);
}

bool xent_setsizepct(XentCtx *ctx, XentNodeId node, XentSize fraction) {
	if (!xent_node_valid(ctx, node)) return false;
	if (!isfinite(fraction.w) || fraction.w < 0.0f || !isfinite(fraction.h) || fraction.h < 0.0f) return false;
	return xent_setwpct(ctx, node, fraction.w) && xent_sethpct(ctx, node, fraction.h);
}

bool xent_setfit(XentCtx *ctx, XentNodeId node, bool wrap_width, bool wrap_height) {
	if (!xent_node_valid(ctx, node)) return false;
	ctx->nodes.layout.wrap_content_w [xent_node_index(node)] = wrap_width ? 1u : 0u;
	ctx->nodes.layout.wrap_content_h [xent_node_index(node)] = wrap_height ? 1u : 0u;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_setaspect(XentCtx *ctx, XentNodeId node, float aspect_ratio) {
	if (!xent_node_valid(ctx, node) || (!isnan(aspect_ratio) && (!isfinite(aspect_ratio) || aspect_ratio <= 0.0f)))
		return false;
	ctx->nodes.layout.aspect_ratio [xent_node_index(node)] = aspect_ratio;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_setminsize(XentCtx *ctx, XentNodeId node, XentSize size) {
	return set_layout_size_fields(
	  ctx, node, size, &ctx->nodes.layout.min_w [xent_node_index(node)],
	  &ctx->nodes.layout.min_h [xent_node_index(node)]
	);
}

bool xent_setmaxsize(XentCtx *ctx, XentNodeId node, XentSize size) {
	return set_layout_size_fields(
	  ctx, node, size, &ctx->nodes.layout.max_w [xent_node_index(node)],
	  &ctx->nodes.layout.max_h [xent_node_index(node)]
	);
}

bool xent_setm(XentCtx *ctx, XentNodeId node, XentInsets margin) {
	if (!xent_node_valid(ctx, node)) return false;
	write_insets(
	  margin,
	  (XentLayoutInsetArrays) {
		ctx->nodes.layout.margin_l, ctx->nodes.layout.margin_t, ctx->nodes.layout.margin_r, ctx->nodes.layout.margin_b},
	  node
	);
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_setp(XentCtx *ctx, XentNodeId node, XentInsets padding) {
	if (!xent_node_valid(ctx, node)) return false;
	write_insets(
	  padding,
	  (XentLayoutInsetArrays) {
		ctx->nodes.layout.padding_l, ctx->nodes.layout.padding_t, ctx->nodes.layout.padding_r,
		ctx->nodes.layout.padding_b},
	  node
	);
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_setgap(XentCtx *ctx, XentNodeId node, float gap) {
	if (!xent_node_valid(ctx, node)) return false;
	ctx->nodes.layout.gap [xent_node_index(node)] = gap < 0.0f ? 0.0f : gap;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_setz(XentCtx *ctx, XentNodeId node, int32_t z_index) {
	if (!xent_node_valid(ctx, node)) return false;
	ctx->nodes.layout.z_index [xent_node_index(node)] = z_index;
	return true;
}

int32_t xent_z(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return 0;
	return ctx->nodes.layout.z_index [xent_node_index(node)];
}

bool xent_layout_rect(XentCtx const *ctx, XentNodeId node, XentRect *out_rect) {
	if (!xent_node_valid(ctx, node) || !out_rect) return false;
	out_rect->x = ctx->nodes.layout.abs_x [xent_node_index(node)];
	out_rect->y = ctx->nodes.layout.abs_y [xent_node_index(node)];
	out_rect->w = ctx->nodes.layout.decided_w [xent_node_index(node)];
	out_rect->h = ctx->nodes.layout.decided_h [xent_node_index(node)];
	return true;
}

XentNodeId xent_layout_root(XentCtx const *ctx) {
	if (!ctx) return XENT_NODE_INVALID;
	return ctx->last_layout_root;
}

XentLayoutStrategy xent_layout_strategy(XentCtx const *ctx) {
	if (!ctx) return XENT_LAYOUT_STRATEGY_NONE;
	return ( XentLayoutStrategy ) ctx->last_layout_strategy;
}

bool xent_setpos(XentCtx *ctx, XentNodeId node, XentPoint position) {
	if (!xent_node_valid(ctx, node)) return false;
	ctx->nodes.layout.abs_pos_x [xent_node_index(node)] = position.x;
	ctx->nodes.layout.abs_pos_y [xent_node_index(node)] = position.y;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}
