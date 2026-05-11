#include "../xent_internal.h"

static bool xent_is_valid_line_break_policy(XentLineBreakPolicy policy) {
	return policy == XENT_LINE_BREAK_NO_WRAP
	    || policy == XENT_LINE_BREAK_WORD_WRAP
	    || policy == XENT_LINE_BREAK_CHAR_WRAP;
}

static bool xent_is_valid_measure_mode(XentMeasureMode mode) {
	return mode == XENT_MEASURE_UNDEFINED || mode == XENT_MEASURE_AT_MOST || mode == XENT_MEASURE_EXACTLY;
}

static XentTextCacheKey xent_text_cache_key_from_measure_request(XentTextMeasureRequest const *request) {
	return (XentTextCacheKey) {
	  request->text,
	  request->font_size,
	  request->width_constraint,
	  request->line_break_policy,
	  request->width_mode,
	};
}

static XentTextCacheKey xent_text_cache_key_from_shape_request(XentTextShapeRequest const *request) {
	return (XentTextCacheKey) {
	  request->text,
	  request->font_size,
	  request->width_constraint,
	  request->line_break_policy,
	  request->width_mode,
	};
}

static void xent_normalize_text_cache_key_for_backend(XentContext const *ctx, XentTextCacheKey *key) {
	if (ctx->text_backend == &ctx->mono_backend) key->font_size = 0.0f;
}

bool xent_validate_text_backend(XentTextBackend const *backend) {
	if (!backend || !backend->measure || !backend->shape) return false;
	if (!backend->name || backend->name [0] == '\0') return false;
	return true;
}

static bool xent_is_valid_width_constraint(XentMeasureMode mode, float width_constraint) {
	if (mode != XENT_MEASURE_EXACTLY) return true;
	return isfinite(width_constraint) && width_constraint >= 0.0f;
}

static bool xent_is_valid_text_measure_request(
  XentContext const *ctx, XentTextMeasureRequest const *request, XentTextMetrics const *out_metrics
) {
	if (!ctx || !ctx->text_backend || !ctx->text_backend->measure) return false;
	if (!request || !request->text || !out_metrics) return false;
	if (!xent_is_valid_line_break_policy(request->line_break_policy)) return false;
	if (!xent_is_valid_measure_mode(request->width_mode)) return false;
	return xent_is_valid_width_constraint(request->width_mode, request->width_constraint);
}

static double xent_begin_text_measure_profile(XentContext *ctx) {
	ctx->profile.text_measure_calls += 1u;
	return xent_now_ms();
}

static void xent_end_text_measure_profile(XentContext *ctx, double measure_start_ms) {
	double elapsed_ms = xent_now_ms() - measure_start_ms;
	if (ctx->swiftstack_scope_depth > 0u) ctx->profile.swiftstack_text_ms += elapsed_ms;
	if (ctx->flex_scope_depth > 0u) ctx->profile.flex_text_ms += elapsed_ms;
	if (ctx->grid_scope_depth > 0u) ctx->profile.grid_text_ms += elapsed_ms;
}

bool xent_set_text(XentContext *ctx, XentNodeId node, char const *text) {
	if (!xent_is_valid_node(ctx, node)) return false;

	char *copy = xent_strdup(text ? text : "");
	if (!copy) return false;

	free(ctx->nodes.text.content [node]);
	ctx->nodes.text.content [node]         = copy;
	ctx->nodes.text.intrinsic_valid [node] = 0u;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

char const *xent_get_text(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return NULL;
	return ctx->nodes.text.content [node];
}

bool xent_set_font_size(XentContext *ctx, XentNodeId node, float font_size) {
	if (!xent_is_valid_node(ctx, node) || font_size <= 0.0f) return false;
	ctx->nodes.text.font_size [node]       = font_size;
	ctx->nodes.text.intrinsic_valid [node] = 0u;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_set_text_line_break_policy(XentContext *ctx, XentNodeId node, XentLineBreakPolicy policy) {
	if (!xent_is_valid_node(ctx, node) || !xent_is_valid_line_break_policy(policy)) return false;
	if (ctx->nodes.text.line_break_policy [node] != ( uint8_t ) policy) {
		ctx->nodes.text.line_break_policy [node] = ( uint8_t ) policy;
		ctx->nodes.text.intrinsic_valid [node]   = 0u;
		xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	}
	return true;
}

XentLineBreakPolicy xent_get_text_line_break_policy(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return XENT_LINE_BREAK_CHAR_WRAP;
	return ( XentLineBreakPolicy ) ctx->nodes.text.line_break_policy [node];
}

static void xent_invalidate_text_intrinsics(XentContext *ctx) {
	for (uint32_t i = 1u; i <= ctx->nodes.count; ++i)
		if (ctx->nodes.lifetime.alive [i]) ctx->nodes.text.intrinsic_valid [i] = 0u;
}

static void xent_replace_text_backend(XentContext *ctx, XentTextBackend const *next_backend) {
	ctx->text_backend = next_backend;
	xent_text_cache_destroy(&ctx->text_cache);
	( void ) xent_text_cache_init(&ctx->text_cache);
	xent_shape_cache_destroy(&ctx->shape_cache);
	( void ) xent_shape_cache_init(&ctx->shape_cache);
	xent_invalidate_text_intrinsics(ctx);
}

bool xent_set_text_backend(XentContext *ctx, XentTextBackend const *backend) {
	if (!ctx) return false;
	if (backend && !xent_validate_text_backend(backend)) return false;
	XentTextBackend const *next_backend = backend ? backend : &ctx->mono_backend;
	if (ctx->text_backend != next_backend) xent_replace_text_backend(ctx, next_backend);
	return true;
}

XentTextBackend const *xent_get_text_backend(XentContext const *ctx) {
	if (!ctx) return NULL;
	return ctx->text_backend;
}

bool xent_measure_text(XentContext *ctx, XentTextMeasureRequest const *request, XentTextMetrics *out_metrics) {
	if (!xent_is_valid_text_measure_request(ctx, request, out_metrics)) return false;

	double           measure_start_ms = xent_begin_text_measure_profile(ctx);

	XentTextCacheKey cache_key        = xent_text_cache_key_from_measure_request(request);
	xent_normalize_text_cache_key_for_backend(ctx, &cache_key);
	if (xent_text_cache_lookup(&ctx->text_cache, &cache_key, out_metrics)) {
		xent_end_text_measure_profile(ctx, measure_start_ms);
		return true;
	}

	if (!ctx->text_backend->measure(ctx->text_backend, request, out_metrics)) {
		xent_end_text_measure_profile(ctx, measure_start_ms);
		return false;
	}

	xent_text_cache_insert(&ctx->text_cache, &cache_key, out_metrics);
	xent_end_text_measure_profile(ctx, measure_start_ms);
	return true;
}

bool xent_shape_text(XentContext *ctx, XentTextShapeRequest const *request, XentTextShapeOutput const *output) {
	if (!ctx
		|| !ctx->text_backend
		|| !ctx->text_backend->shape
		|| !request
		|| !request->text
		|| !output
		|| !output->result)
	{
		return false;
	}
	if (!xent_is_valid_line_break_policy(request->line_break_policy)) return false;
	if (!xent_is_valid_measure_mode(request->width_mode)) return false;
	if (request->width_mode == XENT_MEASURE_EXACTLY
		&& (!isfinite(request->width_constraint) || request->width_constraint < 0.0f))
	{
		return false;
	}

	bool summary_only = (output->glyphs == NULL && output->glyph_capacity == 0u && output->runs == NULL &&
	                     output->run_capacity == 0u && output->lines == NULL && output->line_capacity == 0u);
	XentTextCacheKey cache_key = xent_text_cache_key_from_shape_request(request);
	xent_normalize_text_cache_key_for_backend(ctx, &cache_key);
	if (summary_only && xent_shape_cache_lookup(&ctx->shape_cache, &cache_key, output->result)) return true;

	if (!ctx->text_backend->shape(ctx->text_backend, request, output)) return false;

	xent_shape_cache_insert(&ctx->shape_cache, &cache_key, output->result);
	return true;
}

XentTextCacheStats xent_get_text_cache_stats(XentContext const *ctx) {
	XentTextCacheStats zero = {0};
	if (!ctx) return zero;
	return ctx->text_cache.stats;
}

XentTextCacheStats xent_get_shape_cache_stats(XentContext const *ctx) {
	XentTextCacheStats zero = {0};
	if (!ctx) return zero;
	return ctx->shape_cache.stats;
}
