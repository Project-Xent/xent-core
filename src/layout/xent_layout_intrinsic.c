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

float xent_estimate_text_baseline(XentContext *ctx, XentNodeId node, float cross_size) {
	if (cross_size <= 0.0f) return 0.0f;
	if (ctx->nodes.text.content [node] && ctx->nodes.text.content [node][0] != '\0') {
		/* Mono fallback until text backends expose real ascender/baseline metrics. */
		ctx->profile.text_baseline_fallbacks += 1u;
		return cross_size * 0.8f;
	}
	return cross_size;
}

static XentMeasureMode xent_resolve_intrinsic_width_mode(float width, float *text_constraint) {
	if (!isnan(width)) return XENT_MEASURE_EXACTLY;
	if (isfinite(*text_constraint) && *text_constraint > 0.0f) return XENT_MEASURE_AT_MOST;
	*text_constraint = INFINITY;
	return XENT_MEASURE_UNDEFINED;
}

static float xent_resolve_style_axis(float absolute, float fraction, float available) {
	if (!isnan(absolute)) return absolute;
	if (isfinite(fraction) && fraction >= 0.0f && isfinite(available) && available >= 0.0f) return available * fraction;
	return NAN;
}

static XentIntrinsicInput
xent_make_intrinsic_input(XentContext const *ctx, XentNodeId node, float available_w, float available_h) {
	XentIntrinsicInput input = {
	  xent_resolve_style_axis(ctx->nodes.layout.style_w [node], ctx->nodes.layout.style_w_percent [node], available_w),
	  xent_resolve_style_axis(ctx->nodes.layout.style_h [node], ctx->nodes.layout.style_h_percent [node], available_h),
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
	    && ctx->nodes.text.intrinsic_font_weight [node] == ctx->nodes.text.font_weight [node]
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
	ctx->nodes.text.intrinsic_font_weight [node]       = ctx->nodes.text.font_weight [node];
	ctx->nodes.text.intrinsic_line_break_policy [node] = ( uint8_t ) input->line_break_policy;
	ctx->nodes.text.intrinsic_width_mode [node]        = ( uint8_t ) input->width_mode;
	ctx->nodes.text.intrinsic_w [node]                 = metrics->width;
	ctx->nodes.text.intrinsic_h [node]                 = metrics->height;
	ctx->nodes.text.intrinsic_lines [node]             = metrics->line_count;
}

static bool xent_measure_intrinsic_text(
  XentContext *ctx, XentNodeId node, XentIntrinsicInput const *input, XentTextMetrics *out_metrics
) {
	/* CSS §4.1 intrinsic sizing for text: min-content = longest unbreakable run;
	 * max-content = no wrap (one line). An EXPLICIT width still wins (sizing modes
	 * only resolve an auto/content size). */
	XentIntrinsicInput resolved = *input;
	if (resolved.width_mode != XENT_MEASURE_EXACTLY) {
		if (ctx->sizing_mode == XENT_SIZING_MIN_CONTENT) {
			resolved.width_mode      = XENT_MEASURE_MIN_CONTENT;
			resolved.text_constraint = INFINITY;
		} else if (ctx->sizing_mode == XENT_SIZING_MAX_CONTENT) {
			resolved.width_mode      = XENT_MEASURE_UNDEFINED;
			resolved.text_constraint = INFINITY;
		}
	}
	input = &resolved;
	if (xent_has_intrinsic_cache(ctx, node, input)) {
		*out_metrics = xent_read_intrinsic_cache(ctx, node);
		return true;
	}

	XentTextMeasureRequest request = {
	  ctx->nodes.text.content [node],
	  ctx->nodes.text.font_size [node],
	  ctx->nodes.text.font_weight [node],
	  input->text_constraint,
	  input->line_break_policy,
	  input->width_mode,
	};
	if (!xent_measure_text(ctx, &request, out_metrics)) return false;

	xent_write_intrinsic_cache(ctx, node, input, out_metrics);
	return true;
}

/* Text-derived sizes are border-box: the text is measured against the content
 * box (constraint minus horizontal padding) and the node's padding is added
 * back to the resulting axes, so padded text leaves (labels, buttons) size to
 * label-plus-chrome without an explicit xent_set_size. */
static void xent_apply_text_intrinsic_size(
  XentContext *ctx, XentNodeId node, XentIntrinsicInput const *input, float *width, float *height
) {
	XentTextMetrics metrics = {0};
	if (!ctx->nodes.text.content [node] || (!isnan(*width) && !isnan(*height))) return;

	float              pad_w  = ctx->nodes.layout.padding_l [node] + ctx->nodes.layout.padding_r [node];
	float              pad_h  = ctx->nodes.layout.padding_t [node] + ctx->nodes.layout.padding_b [node];

	XentIntrinsicInput padded = *input;
	if (isfinite(padded.text_constraint) && pad_w > 0.0f)
		padded.text_constraint = padded.text_constraint - pad_w > 1.0f ? padded.text_constraint - pad_w : 1.0f;

	if (!xent_measure_intrinsic_text(ctx, node, &padded, &metrics)) return;
	if (metrics.line_count == 0u && metrics.width <= 0.0f && metrics.height <= 0.0f) return;

	if (isnan(*width)) *width = metrics.width + pad_w;
	if (isnan(*height)) *height = metrics.height + pad_h;
}

static void xent_apply_aspect_ratio(XentContext const *ctx, XentNodeId node, float *width, float *height) {
	float aspect = ctx->nodes.layout.aspect_ratio [node];
	if (!isfinite(aspect) || aspect <= 0.0f) return;
	if (!isnan(*width) && isnan(*height)) *height = *width / aspect;
	else if (isnan(*width) && !isnan(*height)) *width = *height * aspect;
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

/* Opt-in fit-content: size a FLEX container's auto axis to the extent of its
 * children, instead of filling available space. Delegates to the spec content
 * sizer (Flexbox §9.4/§9.9), which resolves used main sizes and then measures
 * each item's cross at its used main — so a wrapped/grow item contributes its
 * real (wrapped) cross extent rather than a max-content single line. The wrapped
 * axis is measured unconstrained (INFINITY) as its available space. */
static void xent_apply_flex_wrap_content(
  XentContext *ctx, XentNodeId node, float available_w, float available_h, float *width, float *height
) {
	/* An auto axis sizes to content when wrap_content is opted in OR whenever the
	 * available space on that axis is INDEFINITE — a flex container with an auto
	 * size and no definite space to fill must size to its content, per CSS; only a
	 * definite available is filled. */
	bool wrap_w = isnan(*width) && (ctx->nodes.layout.wrap_content_w [node] != 0u || !isfinite(available_w));
	bool wrap_h = isnan(*height) && (ctx->nodes.layout.wrap_content_h [node] != 0u || !isfinite(available_h));
	if ((!wrap_w && !wrap_h) || ctx->nodes.layout.protocol [node] != ( uint8_t ) XENT_PROTOCOL_FLEX) return;

	bool  row   = ctx->nodes.flex.direction [node] == ( uint8_t ) XENT_FLEX_ROW;
	float pad_w = ctx->nodes.layout.padding_l [node] + ctx->nodes.layout.padding_r [node];
	float pad_h = ctx->nodes.layout.padding_t [node] + ctx->nodes.layout.padding_b [node];

	/* Main axis = width (row) / height (column); cross axis is the other.
	 * The hugged MAIN axis is measured unconstrained (max-content sum). The
	 * CROSS axis instead uses fit-content — bounded by the finite available
	 * space — so each item's base main size is measured at the cross extent it
	 * will actually get (Flexbox §9.4 algo-main-item E). Measuring the cross at
	 * INFINITY here is the bug that made a stretched hug-width column compute
	 * its height with all text on one line, then overflow when laid out narrow. */
	bool  main_hug    = row ? wrap_w : wrap_h;
	float main_dim    = row ? *width : *height;
	float main_avail  = row ? available_w : available_h;
	float main_pad    = row ? pad_w : pad_h;
	float cross_dim   = row ? *height : *width;
	float cross_avail = row ? available_h : available_w;
	float cross_pad   = row ? pad_h : pad_w;

	float avail_main  = isnan(main_dim) ? (main_hug ? INFINITY : main_avail - main_pad) : (main_dim - main_pad);
	float avail_cross = isnan(cross_dim) ? (isfinite(cross_avail) ? cross_avail - cross_pad : INFINITY) : (cross_dim - cross_pad);

	float main_extent = 0.0f, cross_extent = 0.0f;
	xent_flex_intrinsic_content(ctx, node, avail_main, avail_cross, row, &main_extent, &cross_extent);

	if (wrap_w) *width = (row ? main_extent : cross_extent) + pad_w;
	if (wrap_h) *height = (row ? cross_extent : main_extent) + pad_h;
}

/* Core intrinsic sizing. forced_w/forced_h, when not NaN, pin that axis to a
 * definite size (as if it were a style size) before measuring the other axis —
 * used to compute a flex item's hypothetical cross size at its used main size
 * (CSS Flexbox §9.4 "algo-cross-item": lay out with the used main size). */
static void xent_compute_intrinsic_size_ex(
  XentContext *ctx, XentNodeId node, float available_w, float available_h, float forced_w, float forced_h, float *out_w,
  float *out_h
) {
	XentIntrinsicInput input = xent_make_intrinsic_input(ctx, node, available_w, available_h);
	if (!isnan(forced_w)) {
		input.width           = forced_w;
		input.text_constraint = forced_w;
		input.width_mode      = XENT_MEASURE_EXACTLY;
	}
	if (!isnan(forced_h)) input.height = forced_h;

	float width  = input.width;
	float height = input.height;

	xent_apply_text_intrinsic_size(ctx, node, &input, &width, &height);
	xent_apply_aspect_ratio(ctx, node, &width, &height);
	xent_apply_flex_wrap_content(ctx, node, available_w, available_h, &width, &height);
	xent_resolve_auto_size(available_w, available_h, &width, &height);
	xent_apply_size_constraints(ctx, node, &width, &height);

	*out_w = width;
	*out_h = height;
}

void xent_compute_intrinsic_size(
  XentContext *ctx, XentNodeId node, float available_w, float available_h, float *out_w, float *out_h
) {
	xent_compute_intrinsic_size_ex(ctx, node, available_w, available_h, NAN, NAN, out_w, out_h);
}

float xent_compute_hypothetical_cross(
  XentContext *ctx, XentNodeId node, bool row, float used_main, float available_cross
) {
	float w = 0.0f;
	float h = 0.0f;
	if (row)
		xent_compute_intrinsic_size_ex(ctx, node, used_main, available_cross, used_main, NAN, &w, &h);
	else
		xent_compute_intrinsic_size_ex(ctx, node, available_cross, used_main, NAN, used_main, &w, &h);
	return row ? h : w;
}

void xent_decide_node_box(
  XentContext *ctx, XentNodeId node, float available_w, float available_h, bool definite_w, bool definite_h,
  float *out_w, float *out_h
) {
	if (!definite_w && !definite_h) {
		xent_compute_intrinsic_size(ctx, node, available_w, available_h, out_w, out_h);
		return;
	}

	float width  = available_w;
	float height = available_h;
	if (!definite_w || !definite_h) {
		float intrinsic_w = 0.0f;
		float intrinsic_h = 0.0f;
		xent_compute_intrinsic_size(ctx, node, available_w, available_h, &intrinsic_w, &intrinsic_h);
		if (!definite_w) width = intrinsic_w;
		if (!definite_h) height = intrinsic_h;
	}

	/* On a definite axis the node honors the parent's allocation as its used
	 * size — the parent (flex distribution / cross stretch / grid cell) already
	 * resolved the item's flex base size from its style/basis, so the used size
	 * is authoritative and must NOT be re-overridden by the style here (that
	 * would defeat grow/shrink). Just clamp to the node's own min/max. */
	if (definite_w) width = xent_clampf(width, ctx->nodes.layout.min_w [node], ctx->nodes.layout.max_w [node]);
	if (definite_h) height = xent_clampf(height, ctx->nodes.layout.min_h [node], ctx->nodes.layout.max_h [node]);
	if (width < 0.0f) width = 0.0f;
	if (height < 0.0f) height = 0.0f;

	*out_w = width;
	*out_h = height;
}
