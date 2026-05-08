#include "../xent_internal.h"

typedef struct XentIntrinsicInput {
	float               width;
	float               height;
	float               text_constraint;
	XentMeasureMode     width_mode;
	XentLineBreakPolicy line_break_policy;
} XentIntrinsicInput;

static float xent_clampf(float value, float min_v, float max_v) {
	if (value < min_v) value = min_v;
	if (value > max_v) value = max_v;
	return value;
}

static XentMeasureMode xent_resolve_intrinsic_width_mode(float width, float *text_constraint) {
	if (!isnan(width)) return XENT_MEASURE_EXACTLY;
	if (isfinite(*text_constraint) && *text_constraint > 0.0f) return XENT_MEASURE_AT_MOST;
	*text_constraint = INFINITY;
	return XENT_MEASURE_UNDEFINED;
}

static XentIntrinsicInput
xent_make_intrinsic_input(XentContext const *ctx, XentNodeId node, float available_w, float available_h) {
	XentIntrinsicInput input = {
	  ctx->nodes.layout.style_w [node],
	  ctx->nodes.layout.style_h [node],
	  available_w,
	  XENT_MEASURE_UNDEFINED,
	  ( XentLineBreakPolicy ) ctx->nodes.text.line_break_policy [node],
	};

	if (!isnan(input.width)) input.text_constraint = input.width;
	input.width_mode = xent_resolve_intrinsic_width_mode(input.width, &input.text_constraint);
	if (input.width_mode != XENT_MEASURE_EXACTLY && (!isfinite(input.text_constraint) || input.text_constraint <= 0.0f))
		input.text_constraint = INFINITY;
	( void ) available_h;
	return input;
}

static bool xent_has_intrinsic_cache(XentContext const *ctx, XentNodeId node, XentIntrinsicInput const *input) {
	return ctx->nodes.text.intrinsic_valid [node] != 0u
	    && ctx->nodes.text.intrinsic_font_size [node] == ctx->nodes.text.font_size [node]
	    && ctx->nodes.text.intrinsic_constraint_w [node] == input->text_constraint
	    && ctx->nodes.text.intrinsic_line_break_policy [node] == ( uint8_t ) input->line_break_policy
	    && ctx->nodes.text.intrinsic_width_mode [node] == ( uint8_t ) input->width_mode;
}

static XentTextMetrics xent_read_intrinsic_cache(XentContext const *ctx, XentNodeId node) {
	return (XentTextMetrics) {
	  ctx->nodes.text.intrinsic_w [node],
	  ctx->nodes.text.intrinsic_h [node],
	  ctx->nodes.text.intrinsic_lines [node],
	};
}

static void xent_write_intrinsic_cache(
  XentContext *ctx, XentNodeId node, XentIntrinsicInput const *input, XentTextMetrics const *metrics
) {
	ctx->nodes.text.intrinsic_valid [node]             = 1u;
	ctx->nodes.text.intrinsic_constraint_w [node]      = input->text_constraint;
	ctx->nodes.text.intrinsic_font_size [node]         = ctx->nodes.text.font_size [node];
	ctx->nodes.text.intrinsic_line_break_policy [node] = ( uint8_t ) input->line_break_policy;
	ctx->nodes.text.intrinsic_width_mode [node]        = ( uint8_t ) input->width_mode;
	ctx->nodes.text.intrinsic_w [node]                 = metrics->width;
	ctx->nodes.text.intrinsic_h [node]                 = metrics->height;
	ctx->nodes.text.intrinsic_lines [node]             = metrics->line_count;
}

static bool xent_measure_intrinsic_text(
  XentContext *ctx, XentNodeId node, XentIntrinsicInput const *input, XentTextMetrics *out_metrics
) {
	if (xent_has_intrinsic_cache(ctx, node, input)) {
		*out_metrics = xent_read_intrinsic_cache(ctx, node);
		return true;
	}

	XentTextMeasureRequest request = {
	  ctx->nodes.text.content [node],
	  ctx->nodes.text.font_size [node],
	  input->text_constraint,
	  input->line_break_policy,
	  input->width_mode,
	};
	if (!xent_measure_text(ctx, &request, out_metrics)) return false;

	xent_write_intrinsic_cache(ctx, node, input, out_metrics);
	return true;
}

static void xent_apply_text_intrinsic_size(
  XentContext *ctx, XentNodeId node, XentIntrinsicInput const *input, float *width, float *height
) {
	XentTextMetrics metrics = {0};
	if (!ctx->nodes.text.content [node] || (!isnan(*width) && !isnan(*height))) return;
	if (!xent_measure_intrinsic_text(ctx, node, input, &metrics)) return;
	if (metrics.line_count == 0u && metrics.width <= 0.0f && metrics.height <= 0.0f) return;

	if (isnan(*width)) *width = metrics.width;
	if (isnan(*height)) *height = metrics.height;
}

static void xent_resolve_auto_size(float available_w, float available_h, float *width, float *height) {
	if (isnan(*width)) *width = available_w;
	if (isnan(*height)) *height = available_h;
	if (!isfinite(*width) || *width < 0.0f) *width = 0.0f;
	if (!isfinite(*height) || *height < 0.0f) *height = 0.0f;
}

static void xent_apply_size_constraints(XentContext const *ctx, XentNodeId node, float *width, float *height) {
	*width  = xent_clampf(*width, ctx->nodes.layout.min_w [node], ctx->nodes.layout.max_w [node]);
	*height = xent_clampf(*height, ctx->nodes.layout.min_h [node], ctx->nodes.layout.max_h [node]);
}

void xent_compute_intrinsic_size(
  XentContext *ctx, XentNodeId node, float available_w, float available_h, float *out_w, float *out_h
) {
	XentIntrinsicInput input  = xent_make_intrinsic_input(ctx, node, available_w, available_h);
	float              width  = input.width;
	float              height = input.height;

	xent_apply_text_intrinsic_size(ctx, node, &input, &width, &height);
	xent_resolve_auto_size(available_w, available_h, &width, &height);
	xent_apply_size_constraints(ctx, node, &width, &height);

	*out_w = width;
	*out_h = height;
}
