#include "../xent_internal.h"

#include "xent_layout_flex.h"
#include "xent_layout_util.h"

typedef struct XentIntrinsicInput {
	float               width;
	float               height;
	float               text_constraint;
	XentMeasureMode     width_mode;
	XentLineBreakPolicy line_break_policy;
} XentIntrinsicInput;

static XentMeasureMode resolve_intrinsic_width_mode(float width, float *text_constraint) {
	if (!isnan(width)) return XENT_MEASURE_EXACTLY;
	if (isfinite(*text_constraint) && *text_constraint > 0.0f) return XENT_MEASURE_AT_MOST;
	*text_constraint = INFINITY;
	return XENT_MEASURE_UNDEFINED;
}

static float resolve_style_axis(float absolute, float fraction, float available) {
	if (!isnan(absolute)) return absolute;
	if (isfinite(fraction) && fraction >= 0.0f && isfinite(available) && available >= 0.0f) return available * fraction;
	return NAN;
}

static XentIntrinsicInput
make_intrinsic_input(XentCtx const *ctx, XentNodeId node, float available_w, float available_h) {
	XentIntrinsicInput input = {
	  resolve_style_axis(
		ctx->nodes.layout.style_w [xent_node_index(node)], ctx->nodes.layout.style_w_percent [xent_node_index(node)],
		available_w
	  ),
	  resolve_style_axis(
		ctx->nodes.layout.style_h [xent_node_index(node)], ctx->nodes.layout.style_h_percent [xent_node_index(node)],
		available_h
	  ),
	  available_w,
	  XENT_MEASURE_UNDEFINED,
	  ( XentLineBreakPolicy ) ctx->nodes.text.line_break_policy [xent_node_index(node)],
	};

	if (!isnan(input.width)) input.text_constraint = input.width;
	input.width_mode = resolve_intrinsic_width_mode(input.width, &input.text_constraint);
	if (input.width_mode != XENT_MEASURE_EXACTLY && (!isfinite(input.text_constraint) || input.text_constraint <= 0.0f))
		input.text_constraint = INFINITY;
	( void ) available_h;
	return input;
}

static bool has_intrinsic_cache(XentCtx const *ctx, XentNodeId node, XentIntrinsicInput const *input) {
	return ctx->nodes.text.intrinsic_valid [xent_node_index(node)] != 0u
	    && ctx->nodes.text.intrinsic_font_size [xent_node_index(node)]
	         == ctx->nodes.text.font_size [xent_node_index(node)]
	    && ctx->nodes.text.intrinsic_font_weight [xent_node_index(node)]
	         == ctx->nodes.text.font_weight [xent_node_index(node)]
	    && ctx->nodes.text.intrinsic_constraint_w [xent_node_index(node)] == input->text_constraint
	    && ctx->nodes.text.intrinsic_line_break_policy [xent_node_index(node)] == ( uint8_t ) input->line_break_policy
	    && ctx->nodes.text.intrinsic_width_mode [xent_node_index(node)] == ( uint8_t ) input->width_mode;
}

static XentTextMetrics read_intrinsic_cache(XentCtx const *ctx, XentNodeId node) {
	return (XentTextMetrics) {
	  ctx->nodes.text.intrinsic_w [xent_node_index(node)],
	  ctx->nodes.text.intrinsic_h [xent_node_index(node)],
	  ctx->nodes.text.intrinsic_lines [xent_node_index(node)],
	};
}

static void
write_intrinsic_cache(XentCtx *ctx, XentNodeId node, XentIntrinsicInput const *input, XentTextMetrics const *metrics) {
	ctx->nodes.text.intrinsic_valid [xent_node_index(node)]        = 1u;
	ctx->nodes.text.intrinsic_constraint_w [xent_node_index(node)] = input->text_constraint;
	ctx->nodes.text.intrinsic_font_size [xent_node_index(node)]    = ctx->nodes.text.font_size [xent_node_index(node)];
	ctx->nodes.text.intrinsic_font_weight [xent_node_index(node)] = ctx->nodes.text.font_weight [xent_node_index(node)];
	ctx->nodes.text.intrinsic_line_break_policy [xent_node_index(node)] = ( uint8_t ) input->line_break_policy;
	ctx->nodes.text.intrinsic_width_mode [xent_node_index(node)]        = ( uint8_t ) input->width_mode;
	ctx->nodes.text.intrinsic_w [xent_node_index(node)]                 = metrics->width;
	ctx->nodes.text.intrinsic_h [xent_node_index(node)]                 = metrics->height;
	ctx->nodes.text.intrinsic_lines [xent_node_index(node)]             = metrics->line_count;
}

static XentIntrinsicInput resolve_intrinsic_mode(XentCtx const *ctx, XentIntrinsicInput input) {
	if (input.width_mode == XENT_MEASURE_EXACTLY) return input;
	if (ctx->sizing_mode == XENT_SIZING_MIN_CONTENT) input.width_mode = XENT_MEASURE_MIN_CONTENT;
	else if (ctx->sizing_mode == XENT_SIZING_MAX_CONTENT) input.width_mode = XENT_MEASURE_UNDEFINED;
	else return input;
	input.text_constraint = INFINITY;
	return input;
}

static bool
measure_intrinsic_text(XentCtx *ctx, XentNodeId node, XentIntrinsicInput const *input, XentTextMetrics *out_metrics) {
	/* CSS §4.1 intrinsic sizing for text: min-content = longest unbreakable run;
	 * max-content = no wrap (one line). An EXPLICIT width still wins (sizing modes
	 * only resolve an auto/content size). */
	XentIntrinsicInput resolved = resolve_intrinsic_mode(ctx, *input);
	input                       = &resolved;
	if (has_intrinsic_cache(ctx, node, input)) {
		*out_metrics = read_intrinsic_cache(ctx, node);
		return true;
	}

	XentTextMeasureReq request = {
	  ctx->nodes.text.content [xent_node_index(node)],
	  ctx->nodes.text.font_size [xent_node_index(node)],
	  ctx->nodes.text.font_weight [xent_node_index(node)],
	  input->text_constraint,
	  input->line_break_policy,
	  input->width_mode,
	};
	if (!xent_text_measure(ctx, &request, out_metrics)) return false;

	write_intrinsic_cache(ctx, node, input, out_metrics);
	return true;
}

/* Text-derived sizes are border-box: the text is measured against the content
 * box (constraint minus horizontal padding) and the node's padding is added
 * back to the resulting axes, so padded text leaves (labels, buttons) size to
 * label-plus-chrome without an explicit xent_setsize. */
static void
apply_text_intrinsic_size(XentCtx *ctx, XentNodeId node, XentIntrinsicInput const *input, float *width, float *height) {
	XentTextMetrics metrics = {0};
	char const     *content = ctx->nodes.text.content [xent_node_index(node)];
	if (!content || content [0] == '\0' || (!isnan(*width) && !isnan(*height))) return;

	float pad_w
	  = ctx->nodes.layout.padding_l [xent_node_index(node)] + ctx->nodes.layout.padding_r [xent_node_index(node)];
	float pad_h
	  = ctx->nodes.layout.padding_t [xent_node_index(node)] + ctx->nodes.layout.padding_b [xent_node_index(node)];

	XentIntrinsicInput padded = *input;
	if (isfinite(padded.text_constraint) && pad_w > 0.0f)
		padded.text_constraint = padded.text_constraint - pad_w > 1.0f ? padded.text_constraint - pad_w : 1.0f;

	if (!measure_intrinsic_text(ctx, node, &padded, &metrics)) return;
	if (metrics.line_count == 0u && metrics.width <= 0.0f && metrics.height <= 0.0f) return;

	if (isnan(*width)) *width = metrics.width + pad_w;
	if (isnan(*height)) *height = metrics.height + pad_h;
}

static void apply_aspect_ratio(XentCtx const *ctx, XentNodeId node, float *width, float *height) {
	float aspect = ctx->nodes.layout.aspect_ratio [xent_node_index(node)];
	if (!isfinite(aspect) || aspect <= 0.0f) return;
	if (!isnan(*width) && isnan(*height)) *height = *width / aspect;
	else if (isnan(*width) && !isnan(*height)) *width = *height * aspect;
}

static void resolve_auto_size(float available_w, float available_h, float *width, float *height) {
	if (isnan(*width)) *width = available_w;
	if (isnan(*height)) *height = available_h;
	if (!isfinite(*width) || *width < 0.0f) *width = 0.0f;
	if (!isfinite(*height) || *height < 0.0f) *height = 0.0f;
}

static void apply_size_constraints(XentCtx const *ctx, XentNodeId node, float *width, float *height) {
	*width = xent_clampf(
	  *width, ctx->nodes.layout.min_w [xent_node_index(node)], ctx->nodes.layout.max_w [xent_node_index(node)]
	);
	*height = xent_clampf(
	  *height, ctx->nodes.layout.min_h [xent_node_index(node)], ctx->nodes.layout.max_h [xent_node_index(node)]
	);
}

/* Core intrinsic sizing. forced_w/forced_h, when not NaN, pin that axis to a
 * definite size (as if it were a style size) before measuring the other axis —
 * used to compute a flex item's hypothetical cross size at its used main size
 * (CSS Flexbox §9.4 "algo-cross-item": lay out with the used main size). */
static void compute_intrinsic_size_ex(
  XentCtx *ctx, XentNodeId node, float available_w, float available_h, float forced_w, float forced_h, float *out_w,
  float *out_h
) {
	XentIntrinsicInput input = make_intrinsic_input(ctx, node, available_w, available_h);
	if (!isnan(forced_w)) {
		input.width           = forced_w;
		input.text_constraint = forced_w;
		input.width_mode      = XENT_MEASURE_EXACTLY;
	}
	if (!isnan(forced_h)) input.height = forced_h;

	float width  = input.width;
	float height = input.height;

	/* External content (image / HtmlView): call whenever any axis is auto, with
	 * resolved modes so width-fixed/height-auto (and the reverse) still measure. */
	( void ) xent_resolve_external_measure(ctx, node, available_w, available_h, &width, &height);

	apply_text_intrinsic_size(ctx, node, &input, &width, &height);
	apply_aspect_ratio(ctx, node, &width, &height);
	xent_flex_wrap_content(ctx, node, available_w, available_h, &width, &height);
	resolve_auto_size(available_w, available_h, &width, &height);
	apply_size_constraints(ctx, node, &width, &height);

	*out_w = width;
	*out_h = height;
}

void xent_compute_intrinsic_size(
  XentCtx *ctx, XentNodeId node, float available_w, float available_h, float *out_w, float *out_h
) {
	compute_intrinsic_size_ex(ctx, node, available_w, available_h, NAN, NAN, out_w, out_h);
}

static float
measure_hypothetical_cross(XentCtx *ctx, XentNodeId node, bool row, float used_main, float available_cross) {
	float w = 0.0f;
	float h = 0.0f;
	if (row) compute_intrinsic_size_ex(ctx, node, used_main, available_cross, used_main, NAN, &w, &h);
	else compute_intrinsic_size_ex(ctx, node, available_cross, used_main, NAN, used_main, &w, &h);
	return row ? h : w;
}

static float measure_cross_mode(XentCtx *ctx, XentNodeId node, bool row, float used_main, uint8_t mode) {
	uint8_t saved    = ctx->sizing_mode;
	ctx->sizing_mode = mode;
	float cross      = measure_hypothetical_cross(ctx, node, row, used_main, INFINITY);
	ctx->sizing_mode = saved;
	return cross;
}

float xent_compute_hypothetical_cross(XentCtx *ctx, XentNodeId node, bool row, float used_main, float available_cross) {
	uint32_t idx     = xent_node_index(node);
	float    percent = row ? ctx->nodes.layout.style_h_percent [idx] : ctx->nodes.layout.style_w_percent [idx];
	if (isfinite(percent)) return measure_hypothetical_cross(ctx, node, row, used_main, available_cross);

	float content = measure_hypothetical_cross(ctx, node, row, used_main, INFINITY);
	if (row || !isfinite(available_cross) || ctx->sizing_mode != XENT_SIZING_NORMAL) return content;

	float min_cross = measure_cross_mode(ctx, node, row, used_main, XENT_SIZING_MIN_CONTENT);
	float max_cross = measure_cross_mode(ctx, node, row, used_main, XENT_SIZING_MAX_CONTENT);
	return xent_clampf(available_cross, min_cross, max_cross);
}

static float decide_axis(float available, bool definite, float intrinsic, float min_v, float max_v) {
	float size = definite ? available : intrinsic;
	if (definite) size = xent_clampf(size, min_v, max_v);
	return size < 0.0f ? 0.0f : size;
}

void xent_decide_node_box(
  XentCtx *ctx, XentNodeId node, float available_w, float available_h, bool definite_w, bool definite_h, float *out_w,
  float *out_h
) {
	if (!definite_w && !definite_h) {
		xent_compute_intrinsic_size(ctx, node, available_w, available_h, out_w, out_h);
		return;
	}

	float intrinsic_w = available_w;
	float intrinsic_h = available_h;
	if (!definite_w || !definite_h)
		xent_compute_intrinsic_size(ctx, node, available_w, available_h, &intrinsic_w, &intrinsic_h);

	uint32_t idx = xent_node_index(node);
	*out_w
	  = decide_axis(available_w, definite_w, intrinsic_w, ctx->nodes.layout.min_w [idx], ctx->nodes.layout.max_w [idx]);
	*out_h
	  = decide_axis(available_h, definite_h, intrinsic_h, ctx->nodes.layout.min_h [idx], ctx->nodes.layout.max_h [idx]);
}
