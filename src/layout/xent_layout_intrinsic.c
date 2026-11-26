#include "../xent_internal.h"
#include "../core/xent_scratch.h"

#include "xent_layout_flex.h"
#include "xent_layout_util.h"

typedef struct XentIntrinsicInput {
	float               width;
	float               height;
	float               text_constraint;
	XentMeasureMode     width_mode;
	XentLineBreakPolicy line_break_policy;
} XentIntrinsicInput;

static uint32_t norm_f32_bits(float v) {
	if (isnan(v)) return 0x7fc00000u;
	if (v == 0.0f) return 0u;
	uint32_t b = 0u;
	memcpy(&b, &v, sizeof(b));
	return b;
}

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
	uint32_t            idx   = xent_node_index(node);
	XentTextComp const *tc    = xent_text_comp_c(&ctx->nodes, idx);
	XentIntrinsicInput  input = {
	  resolve_style_axis(ctx->nodes.layout.style_w [idx], ctx->nodes.layout.style_w_percent [idx], available_w),
	  resolve_style_axis(ctx->nodes.layout.style_h [idx], ctx->nodes.layout.style_h_percent [idx], available_h),
	  available_w,
	  XENT_MEASURE_UNDEFINED,
	  tc ? ( XentLineBreakPolicy ) tc->line_break_policy : XENT_LINEBREAK_CHARWRAP,
	};

	if (!isnan(input.width)) input.text_constraint = input.width;
	input.width_mode = resolve_intrinsic_width_mode(input.width, &input.text_constraint);
	if (input.width_mode != XENT_MEASURE_EXACTLY && (!isfinite(input.text_constraint) || input.text_constraint <= 0.0f))
		input.text_constraint = INFINITY;
	return input;
}

static bool has_intrinsic_cache(XentCtx const *ctx, XentNodeId node, XentIntrinsicInput const *input) {
	XentTextComp const *tc = xent_text_comp_c(&ctx->nodes, xent_node_index(node));
	if (!tc || tc->intrinsic_valid == 0u) return false;
	return tc->intrinsic_font_size == xent_fontsize(ctx, node)
	    && tc->intrinsic_font_weight == xent_fontweight(ctx, node)
	    && tc->intrinsic_constraint_w == input->text_constraint
	    && tc->intrinsic_line_break_policy == ( uint8_t ) input->line_break_policy
	    && tc->intrinsic_width_mode == ( uint8_t ) input->width_mode;
}

static XentTextMetrics read_intrinsic_cache(XentCtx const *ctx, XentNodeId node) {
	XentTextComp const *tc = xent_text_comp_c(&ctx->nodes, xent_node_index(node));
	return (XentTextMetrics) {
	  .width          = tc->intrinsic_w,
	  .height         = tc->intrinsic_h,
	  .line_count     = tc->intrinsic_lines,
	  .first_baseline = tc->intrinsic_baseline,
	};
}

static void
write_intrinsic_cache(XentCtx *ctx, XentNodeId node, XentIntrinsicInput const *input, XentTextMetrics const *metrics) {
	XentTextComp *tc = xent_text_comp_ensure(&ctx->nodes, xent_node_index(node));
	if (!tc) return;
	tc->intrinsic_valid             = 1u;
	tc->intrinsic_constraint_w      = input->text_constraint;
	tc->intrinsic_font_size         = xent_fontsize(ctx, node);
	tc->intrinsic_font_weight       = xent_fontweight(ctx, node);
	tc->intrinsic_line_break_policy = ( uint8_t ) input->line_break_policy;
	tc->intrinsic_width_mode        = ( uint8_t ) input->width_mode;
	tc->intrinsic_w                 = metrics->width;
	tc->intrinsic_h                 = metrics->height;
	tc->intrinsic_lines             = metrics->line_count;
	tc->intrinsic_baseline          = metrics->first_baseline;
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
	XentIntrinsicInput resolved = resolve_intrinsic_mode(ctx, *input);
	input                       = &resolved;
	if (has_intrinsic_cache(ctx, node, input)) {
		*out_metrics = read_intrinsic_cache(ctx, node);
		return true;
	}

	uint32_t            index   = xent_node_index(node);
	XentTextComp const *tc      = xent_text_comp_c(&ctx->nodes, index);
	XentTextMeasureReq  request = {
	  .text              = tc ? tc->content : NULL,
	  .runs              = tc ? tc->runs : NULL,
	  .run_count         = tc ? tc->run_count : 0u,
	  .font_family       = tc ? tc->font_family : NULL,
	  .font_size         = xent_fontsize(ctx, node),
	  .font_weight       = xent_fontweight(ctx, node),
	  .width_constraint  = input->text_constraint,
	  .max_lines         = tc ? tc->max_lines : 0u,
	  .trim              = tc ? ( XentTextTrim ) tc->trim : XENT_TEXT_TRIM_NONE,
	  .line_break_policy = input->line_break_policy,
	  .width_mode        = input->width_mode,
	};
	if (!xent_text_measure(ctx, &request, out_metrics)) return false;

	write_intrinsic_cache(ctx, node, input, out_metrics);
	return true;
}

static void
apply_text_intrinsic_size(XentCtx *ctx, XentNodeId node, XentIntrinsicInput const *input, float *width, float *height) {
	XentTextMetrics     metrics = {0};
	uint32_t            idx     = xent_node_index(node);
	XentTextComp const *tc      = xent_text_comp_c(&ctx->nodes, idx);
	char const         *content = tc ? tc->content : NULL;
	if (!content || content [0] == '\0' || (!isnan(*width) && !isnan(*height))) return;

	float              pad_w  = ctx->nodes.layout.padding_l [idx] + ctx->nodes.layout.padding_r [idx];
	float              pad_h  = ctx->nodes.layout.padding_t [idx] + ctx->nodes.layout.padding_b [idx];

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
	uint32_t idx = xent_node_index(node);
	*width       = xent_clampf(*width, ctx->nodes.layout.min_w [idx], ctx->nodes.layout.max_w [idx]);
	*height      = xent_clampf(*height, ctx->nodes.layout.min_h [idx], ctx->nodes.layout.max_h [idx]);
}

static XentMeasureCacheSlot
make_measure_key(uint8_t sizing_mode, float available_w, float available_h, float forced_w, float forced_h) {
	return (XentMeasureCacheSlot) {
	  .avail_w_bits  = norm_f32_bits(available_w),
	  .avail_h_bits  = norm_f32_bits(available_h),
	  .forced_w_bits = norm_f32_bits(forced_w),
	  .forced_h_bits = norm_f32_bits(forced_h),
	  .sizing_mode   = sizing_mode,
	};
}

static bool
lookup_container_measure(XentCtx *ctx, uint32_t idx, XentMeasureCacheSlot const *key, float *out_w, float *out_h) {
	XentMeasureCache *cache = xent_measure_cache(&ctx->nodes, idx);
	if (!cache) return false;
	for (uint8_t i = 0u; i < cache->count; ++i) {
		XentMeasureCacheSlot const *s = &cache->slots [i];
		if (s->avail_w_bits == key->avail_w_bits
		    && s->avail_h_bits == key->avail_h_bits
		    && s->forced_w_bits == key->forced_w_bits
		    && s->forced_h_bits == key->forced_h_bits
		    && s->sizing_mode == key->sizing_mode)
		{
			*out_w = s->out_w;
			*out_h = s->out_h;
			return true;
		}
	}
	return false;
}

static void
store_container_measure(XentCtx *ctx, uint32_t idx, XentMeasureCacheSlot const *key, float out_w, float out_h) {
	XentMeasureCache *cache = xent_measure_cache_ensure(&ctx->nodes, idx);
	if (!cache) return;
	uint8_t slot = cache->count < XENT_MEASURE_CACHE_SLOTS ? cache->count++ : cache->next_evict;
	if (cache->count == XENT_MEASURE_CACHE_SLOTS)
		cache->next_evict = ( uint8_t ) ((cache->next_evict + 1u) % XENT_MEASURE_CACHE_SLOTS);
	cache->slots [slot]       = *key;
	cache->slots [slot].out_w = out_w;
	cache->slots [slot].out_h = out_h;
}

static bool should_cache_container(XentCtx const *ctx, XentNodeId node, uint32_t idx) {
	if (ctx->nodes.topology.child_count [idx] == 0u) return false;
	if (ctx->nodes.layout.protocol [idx] != ( uint8_t ) XENT_PROTOCOL_FLEX) return false;
	bool auto_axis = isnan(ctx->nodes.layout.style_w [idx])
	              || isnan(ctx->nodes.layout.style_h [idx])
	              || ctx->nodes.layout.wrap_content_w [idx]
	              || ctx->nodes.layout.wrap_content_h [idx];
	if (!auto_axis) return false;
	return ctx->external_measure_cap == 0u || !xent_node_hasmeasure(ctx, node);
}

static void compute_intrinsic_size_ex(
  XentCtx *ctx, XentNodeId node, float available_w, float available_h, float forced_w, float forced_h, float *out_w,
  float *out_h
) {
	uint32_t             idx       = xent_node_index(node);
	bool                 can_cache = should_cache_container(ctx, node, idx);
	XentMeasureCacheSlot key       = {0};
	if (can_cache) {
		key = make_measure_key(ctx->sizing_mode, available_w, available_h, forced_w, forced_h);
		if (lookup_container_measure(ctx, idx, &key, out_w, out_h)) return;
	}

	XentIntrinsicInput input = make_intrinsic_input(ctx, node, available_w, available_h);
	if (!isnan(forced_w)) {
		input.width           = forced_w;
		input.text_constraint = forced_w;
		input.width_mode      = XENT_MEASURE_EXACTLY;
	}
	if (!isnan(forced_h)) input.height = forced_h;

	float width  = input.width;
	float height = input.height;

	if (ctx->external_measure_cap > 0u)
		( void ) xent_resolve_external_measure(ctx, node, available_w, available_h, &width, &height);

	if (ctx->nodes.cold.text [idx] != 0u) apply_text_intrinsic_size(ctx, node, &input, &width, &height);
	apply_aspect_ratio(ctx, node, &width, &height);
	xent_flex_wrap_content(ctx, node, available_w, available_h, &width, &height);
	resolve_auto_size(available_w, available_h, &width, &height);
	apply_size_constraints(ctx, node, &width, &height);

	*out_w = width;
	*out_h = height;
	if (can_cache) store_container_measure(ctx, idx, &key, width, height);
}

void xent_compute_intrinsic_size(
  XentCtx *ctx, XentNodeId node, float available_w, float available_h, float *out_w, float *out_h
) {
	compute_intrinsic_size_ex(ctx, node, available_w, available_h, NAN, NAN, out_w, out_h);
}

bool xent_layout_desired_size(XentCtx *ctx, XentNodeId node, XentSize available, XentSize *out_size) {
	if (!out_size || !xent_node_valid(ctx, node)) return false;
	*out_size = (XentSize) {0};
	if (xent_collapsed(ctx, node)) return true;
	XentScratchMark mark = xent_scratch_mark(ctx);
	xent_compute_intrinsic_size(ctx, node, available.w, available.h, &out_size->w, &out_size->h);
	xent_scratch_rewind(ctx, mark);
	return true;
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
