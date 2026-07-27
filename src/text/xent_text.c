#include "../xent_internal.h"

static bool is_valid_line_break_policy(XentLineBreakPolicy policy) {
	return policy == XENT_LINEBREAK_NOWRAP || policy == XENT_LINEBREAK_WORDWRAP || policy == XENT_LINEBREAK_CHARWRAP;
}

static bool is_valid_measure_mode(XentMeasureMode mode) {
	return mode == XENT_MEASURE_UNDEFINED
	    || mode == XENT_MEASURE_AT_MOST
	    || mode == XENT_MEASURE_EXACTLY
	    || mode == XENT_MEASURE_MIN_CONTENT;
}

static XentTextCacheKey text_cache_key_from_measure_request(XentTextMeasureReq const *request) {
	return (XentTextCacheKey) {
	  request->text,
	  request->font_size,
	  request->font_weight,
	  request->width_constraint,
	  request->line_break_policy,
	  request->width_mode,
	};
}

static void normalize_text_cache_key_for_backend(XentCtx const *ctx, XentTextCacheKey *key) {
	if (ctx->text_backend == &ctx->mono_backend) {
		key->font_size   = 0.0f;
		key->font_weight = 0u;
	}
}

bool xent_text_backend_valid(XentTextBackend const *backend) {
	if (!backend || !backend->measure) return false;
	if (!backend->name || backend->name [0] == '\0') return false;
	return true;
}

static bool is_valid_width_constraint(XentMeasureMode mode, float width_constraint) {
	if (mode != XENT_MEASURE_EXACTLY && mode != XENT_MEASURE_AT_MOST) return true;
	return isfinite(width_constraint) && width_constraint >= 0.0f;
}

static bool is_valid_text_measure_request(
  XentCtx const *ctx, XentTextMeasureReq const *request, XentTextMetrics const *out_metrics
) {
	if (!ctx || !ctx->text_backend || !ctx->text_backend->measure) return false;
	if (!request || !request->text || !out_metrics) return false;
	if (!is_valid_line_break_policy(request->line_break_policy)) return false;
	if (!is_valid_measure_mode(request->width_mode)) return false;
	return is_valid_width_constraint(request->width_mode, request->width_constraint);
}

static double begin_text_measure_profile(XentCtx *ctx) {
	ctx->profile.text_measure_calls += 1u;
	return xent_now_ms();
}

static void end_text_measure_profile(XentCtx *ctx, double measure_start_ms) {
	double elapsed_ms = xent_now_ms() - measure_start_ms;
	if (ctx->swiftstack_scope_depth > 0u) ctx->profile.swiftstack_text_ms += elapsed_ms;
	if (ctx->flex_scope_depth > 0u) ctx->profile.flex_text_ms += elapsed_ms;
	if (ctx->grid_scope_depth > 0u) ctx->profile.grid_text_ms += elapsed_ms;
}

bool xent_settext(XentCtx *ctx, XentNodeId node, char const *text) {
	if (!xent_node_valid(ctx, node)) return false;

	char *copy = xent_strdup(text ? text : "");
	if (!copy) return false;

	free(ctx->nodes.text.content [xent_node_index(node)]);
	ctx->nodes.text.content [xent_node_index(node)]         = copy;
	ctx->nodes.text.intrinsic_valid [xent_node_index(node)] = 0u;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

char const *xent_node_text(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return NULL;
	return ctx->nodes.text.content [xent_node_index(node)];
}

bool xent_setfontsize(XentCtx *ctx, XentNodeId node, float font_size) {
	if (!xent_node_valid(ctx, node) || font_size <= 0.0f) return false;
	ctx->nodes.text.font_size [xent_node_index(node)]       = font_size;
	ctx->nodes.text.intrinsic_valid [xent_node_index(node)] = 0u;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_setfontweight(XentCtx *ctx, XentNodeId node, uint16_t weight) {
	if (!xent_node_valid(ctx, node)) return false;
	ctx->nodes.text.font_weight [xent_node_index(node)]     = weight;
	ctx->nodes.text.intrinsic_valid [xent_node_index(node)] = 0u;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_setlinebreak(XentCtx *ctx, XentNodeId node, XentLineBreakPolicy policy) {
	if (!xent_node_valid(ctx, node) || !is_valid_line_break_policy(policy)) return false;
	if (ctx->nodes.text.line_break_policy [xent_node_index(node)] != ( uint8_t ) policy) {
		ctx->nodes.text.line_break_policy [xent_node_index(node)] = ( uint8_t ) policy;
		ctx->nodes.text.intrinsic_valid [xent_node_index(node)]   = 0u;
		xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	}
	return true;
}

XentLineBreakPolicy xent_linebreak(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return XENT_LINEBREAK_CHARWRAP;
	return ( XentLineBreakPolicy ) ctx->nodes.text.line_break_policy [xent_node_index(node)];
}

static void invalidate_text_intrinsics(XentCtx *ctx) {
	for (uint32_t i = 1u; i <= ctx->nodes.count; ++i)
		if (ctx->nodes.lifetime.alive [i]) ctx->nodes.text.intrinsic_valid [i] = 0u;
}

static void replace_text_backend(XentCtx *ctx, XentTextBackend const *next_backend) {
	ctx->text_backend = next_backend;
	xent_text_cache_destroy(&ctx->text_cache);
	( void ) xent_text_cache_init(&ctx->text_cache);
	invalidate_text_intrinsics(ctx);
}

bool xent_text_setbackend(XentCtx *ctx, XentTextBackend const *backend) {
	if (!ctx) return false;
	if (backend && !xent_text_backend_valid(backend)) return false;
	XentTextBackend const *next_backend = backend ? backend : &ctx->mono_backend;
	if (ctx->text_backend != next_backend) replace_text_backend(ctx, next_backend);
	return true;
}

XentTextBackend const *xent_text_backend(XentCtx const *ctx) {
	if (!ctx) return NULL;
	return ctx->text_backend;
}

bool xent_text_measure(XentCtx *ctx, XentTextMeasureReq const *request, XentTextMetrics *out_metrics) {
	if (!is_valid_text_measure_request(ctx, request, out_metrics)) return false;

	double           measure_start_ms = begin_text_measure_profile(ctx);

	XentTextCacheKey cache_key        = text_cache_key_from_measure_request(request);
	normalize_text_cache_key_for_backend(ctx, &cache_key);
	if (xent_text_cache_lookup(&ctx->text_cache, &cache_key, out_metrics)) {
		end_text_measure_profile(ctx, measure_start_ms);
		return true;
	}

	if (!ctx->text_backend->measure(ctx->text_backend, request, out_metrics)) {
		end_text_measure_profile(ctx, measure_start_ms);
		return false;
	}

	xent_text_cache_insert(&ctx->text_cache, &cache_key, out_metrics);
	end_text_measure_profile(ctx, measure_start_ms);
	return true;
}

XentTextCacheStats xent_text_stats(XentCtx const *ctx) {
	XentTextCacheStats zero = {0};
	if (!ctx) return zero;
	return ctx->text_cache.stats;
}
