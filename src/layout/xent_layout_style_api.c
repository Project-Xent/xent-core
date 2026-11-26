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

static XentResolvedInsets resolve_logical_insets(XentInsets insets, XentAxis main_axis) {
	if (main_axis == XENT_AXIS_HORIZONTAL)
		return (XentResolvedInsets) {insets.left, insets.right, insets.top, insets.bottom};
	return (XentResolvedInsets) {insets.top, insets.bottom, insets.left, insets.right};
}

static XentInsets layout_insets(float left, float top, float right, float bottom) {
	return (XentInsets) {left, top, right, bottom};
}

static bool get_resolved_layout_insets(
  XentCtx const *ctx, XentNodeId node, XentAxis main_axis, XentInsets insets, XentResolvedInsets *out_insets
) {
	if (!xent_node_valid(ctx, node) || !out_insets) return false;
	*out_insets = resolve_logical_insets(insets, main_axis);
	return true;
}

static bool set_layout_size_fields(XentCtx *ctx, XentNodeId node, XentSize size, float *width, float *height) {
	if (!xent_node_valid(ctx, node)) return false;
	uint32_t i       = xent_node_index(node);
	bool     is_main = width == &ctx->nodes.layout.style_w [i];
	bool     changed = !xent_f32_same(*width, size.w) || !xent_f32_same(*height, size.h);
	if (is_main)
		changed
		  = changed || !isnan(ctx->nodes.layout.style_w_percent [i]) || !isnan(ctx->nodes.layout.style_h_percent [i]);
	if (!changed) return true;
	*width  = size.w;
	*height = size.h;
	if (is_main) {
		ctx->nodes.layout.style_w_percent [i] = NAN;
		ctx->nodes.layout.style_h_percent [i] = NAN;
	}
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

static bool set_layout_percent_field(XentCtx *ctx, XentNodeId node, float fraction, float *percent, float *absolute) {
	if (!xent_node_valid(ctx, node) || !isfinite(fraction) || fraction < 0.0f) return false;
	if (xent_f32_same(*percent, fraction) && isnan(*absolute)) return true;
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

static bool insets_equal(XentInsets insets, XentLayoutInsetArrays arrays, uint32_t index) {
	return xent_f32_same(arrays.l [index], insets.left)
	    && xent_f32_same(arrays.t [index], insets.top)
	    && xent_f32_same(arrays.r [index], insets.right)
	    && xent_f32_same(arrays.b [index], insets.bottom);
}

static void write_insets(XentInsets insets, XentLayoutInsetArrays arrays, uint32_t index) {
	arrays.l [index] = insets.left;
	arrays.t [index] = insets.top;
	arrays.r [index] = insets.right;
	arrays.b [index] = insets.bottom;
}

bool xent_setproto(XentCtx *ctx, XentNodeId node, XentProtocol protocol) {
	if (!xent_node_valid(ctx, node)) return false;
	uint32_t i = xent_node_index(node);
	if (ctx->nodes.layout.protocol [i] == ( uint8_t ) protocol) return true;
	ctx->nodes.layout.protocol [i] = ( uint8_t ) protocol;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

XentProtocol xent_node_proto(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return XENT_PROTOCOL_ABSOLUTE;
	return ( XentProtocol ) ctx->nodes.layout.protocol [xent_node_index(node)];
}

bool xent_setdir(XentCtx *ctx, XentNodeId node, XentDirection direction) {
	if (!xent_node_valid(ctx, node) || !is_valid_direction(direction)) return false;
	uint32_t i = xent_node_index(node);
	if (ctx->nodes.layout.direction [i] != ( uint8_t ) direction) {
		ctx->nodes.layout.direction [i] = ( uint8_t ) direction;
		xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT | XENT_DIRTY_SUBTREE);
	}
	return true;
}

bool xent_setcollapsed(XentCtx *ctx, XentNodeId node, bool collapsed) {
	if (!xent_node_valid(ctx, node)) return false;
	uint32_t i = xent_node_index(node);
	if (ctx->nodes.layout.collapsed [i] == ( uint8_t ) collapsed) return true;
	ctx->nodes.layout.collapsed [i] = ( uint8_t ) collapsed;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT | XENT_DIRTY_SUBTREE);
	XentNodeId parent = ctx->nodes.topology.parent [i];
	if (parent != XENT_NODE_INVALID) xent_mark_dirty(ctx, parent, XENT_DIRTY_LAYOUT);
	XentNodeEvent ev = {node, parent, parent, XENT_NODE_EVENT_COLLAPSED};
	xent_notify_node_observers(ctx, &ev);
	return true;
}

bool xent_collapsed(XentCtx const *ctx, XentNodeId node) {
	return xent_node_valid(ctx, node) && ctx->nodes.layout.collapsed [xent_node_index(node)] != 0u;
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
	uint32_t i = xent_node_index(node);
	return set_layout_size_fields(ctx, node, size, &ctx->nodes.layout.style_w [i], &ctx->nodes.layout.style_h [i]);
}

bool xent_setw(XentCtx *ctx, XentNodeId node, float width) {
	if (!xent_node_valid(ctx, node)) return false;
	uint32_t i = xent_node_index(node);
	if (xent_f32_same(ctx->nodes.layout.style_w [i], width) && isnan(ctx->nodes.layout.style_w_percent [i]))
		return true;
	ctx->nodes.layout.style_w [i]         = width;
	ctx->nodes.layout.style_w_percent [i] = NAN;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_seth(XentCtx *ctx, XentNodeId node, float height) {
	if (!xent_node_valid(ctx, node)) return false;
	uint32_t i = xent_node_index(node);
	if (xent_f32_same(ctx->nodes.layout.style_h [i], height) && isnan(ctx->nodes.layout.style_h_percent [i]))
		return true;
	ctx->nodes.layout.style_h [i]         = height;
	ctx->nodes.layout.style_h_percent [i] = NAN;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_setwpct(XentCtx *ctx, XentNodeId node, float fraction) {
	if (!xent_node_valid(ctx, node)) return false;
	uint32_t i = xent_node_index(node);
	return set_layout_percent_field(
	  ctx, node, fraction, &ctx->nodes.layout.style_w_percent [i], &ctx->nodes.layout.style_w [i]
	);
}

bool xent_sethpct(XentCtx *ctx, XentNodeId node, float fraction) {
	if (!xent_node_valid(ctx, node)) return false;
	uint32_t i = xent_node_index(node);
	return set_layout_percent_field(
	  ctx, node, fraction, &ctx->nodes.layout.style_h_percent [i], &ctx->nodes.layout.style_h [i]
	);
}

bool xent_setsizepct(XentCtx *ctx, XentNodeId node, XentSize fraction) {
	if (!xent_node_valid(ctx, node)) return false;
	if (!isfinite(fraction.w) || fraction.w < 0.0f || !isfinite(fraction.h) || fraction.h < 0.0f) return false;
	return xent_setwpct(ctx, node, fraction.w) && xent_sethpct(ctx, node, fraction.h);
}

bool xent_setfit(XentCtx *ctx, XentNodeId node, bool wrap_width, bool wrap_height) {
	if (!xent_node_valid(ctx, node)) return false;
	uint32_t i  = xent_node_index(node);
	uint8_t  ww = wrap_width ? 1u : 0u;
	uint8_t  wh = wrap_height ? 1u : 0u;
	if (ctx->nodes.layout.wrap_content_w [i] == ww && ctx->nodes.layout.wrap_content_h [i] == wh) return true;
	ctx->nodes.layout.wrap_content_w [i] = ww;
	ctx->nodes.layout.wrap_content_h [i] = wh;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_setscroller(XentCtx *ctx, XentNodeId node, bool scroller) {
	if (!xent_node_valid(ctx, node)) return false;
	uint32_t i = xent_node_index(node);
	uint8_t  v = scroller ? 1u : 0u;
	if (ctx->nodes.layout.scroller [i] == v) return true;
	ctx->nodes.layout.scroller [i] = v;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_setaspect(XentCtx *ctx, XentNodeId node, float aspect_ratio) {
	if (!xent_node_valid(ctx, node) || (!isnan(aspect_ratio) && (!isfinite(aspect_ratio) || aspect_ratio <= 0.0f)))
		return false;
	uint32_t i = xent_node_index(node);
	if (xent_f32_same(ctx->nodes.layout.aspect_ratio [i], aspect_ratio)) return true;
	ctx->nodes.layout.aspect_ratio [i] = aspect_ratio;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_setminsize(XentCtx *ctx, XentNodeId node, XentSize size) {
	uint32_t i = xent_node_index(node);
	return set_layout_size_fields(ctx, node, size, &ctx->nodes.layout.min_w [i], &ctx->nodes.layout.min_h [i]);
}

bool xent_setmaxsize(XentCtx *ctx, XentNodeId node, XentSize size) {
	uint32_t i = xent_node_index(node);
	return set_layout_size_fields(ctx, node, size, &ctx->nodes.layout.max_w [i], &ctx->nodes.layout.max_h [i]);
}

bool xent_setm(XentCtx *ctx, XentNodeId node, XentInsets margin) {
	if (!xent_node_valid(ctx, node)) return false;
	uint32_t              i      = xent_node_index(node);
	XentLayoutInsetArrays arrays = {
	  ctx->nodes.layout.margin_l, ctx->nodes.layout.margin_t, ctx->nodes.layout.margin_r, ctx->nodes.layout.margin_b};
	if (insets_equal(margin, arrays, i)) return true;
	write_insets(margin, arrays, i);
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_setp(XentCtx *ctx, XentNodeId node, XentInsets padding) {
	if (!xent_node_valid(ctx, node)) return false;
	uint32_t              i      = xent_node_index(node);
	XentLayoutInsetArrays arrays = {
	  ctx->nodes.layout.padding_l, ctx->nodes.layout.padding_t, ctx->nodes.layout.padding_r,
	  ctx->nodes.layout.padding_b};
	if (insets_equal(padding, arrays, i)) return true;
	write_insets(padding, arrays, i);
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_setgap(XentCtx *ctx, XentNodeId node, float gap) {
	if (!xent_node_valid(ctx, node)) return false;
	uint32_t i = xent_node_index(node);
	float    v = gap < 0.0f ? 0.0f : gap;
	if (xent_f32_same(ctx->nodes.layout.gap [i], v)) return true;
	ctx->nodes.layout.gap [i] = v;
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
	uint32_t i  = xent_node_index(node);
	out_rect->x = ctx->nodes.layout.abs_x [i];
	out_rect->y = ctx->nodes.layout.abs_y [i];
	out_rect->w = ctx->nodes.layout.decided_w [i];
	out_rect->h = ctx->nodes.layout.decided_h [i];
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
	uint32_t i = xent_node_index(node);
	if (xent_f32_same(ctx->nodes.layout.abs_pos_x [i], position.x)
	    && xent_f32_same(ctx->nodes.layout.abs_pos_y [i], position.y))
		return true;
	ctx->nodes.layout.abs_pos_x [i] = position.x;
	ctx->nodes.layout.abs_pos_y [i] = position.y;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}
