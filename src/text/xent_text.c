#include <math.h>
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

static bool is_valid_trim(XentTextTrim trim) { return trim <= XENT_TEXT_TRIM_PATH_ELLIPSIS; }

static bool utf8_boundary(char const *text, size_t text_len, uint32_t at) {
	return at == text_len || (( uint8_t ) text [at] & 0xc0u) != 0x80u;
}

static bool runs_tile_text(char const *text, XentTextRun const *runs, uint32_t count) {
	size_t   text_len = strlen(text);
	uint32_t consumed = 0u;
	for (uint32_t i = 0u; i < count; ++i) {
		if (runs [i].offset != consumed || runs [i].length == 0u) return false;
		if (!(runs [i].font_size > 0.0f)) return false;
		if (runs [i].length > text_len - consumed) return false;
		consumed += runs [i].length;
		if (!utf8_boundary(text, text_len, consumed)) return false;
	}
	return consumed == text_len;
}

static bool are_valid_runs(char const *text, XentTextRun const *runs, uint32_t count) {
	if (count == 0u) return true;
	return runs && runs_tile_text(text, runs, count);
}

static uint32_t blob_put(uint8_t *out, uint32_t at, void const *src, uint32_t size) {
	if (out) memcpy(out + at, src, size);
	return at + size;
}

static uint32_t blob_put_str(uint8_t *out, uint32_t at, char const *text) {
	uint32_t len = text ? ( uint32_t ) strlen(text) : 0u;
	at           = blob_put(out, at, &len, sizeof(len));
	return len ? blob_put(out, at, text, len) : at;
}

static uint32_t blob_put_run(uint8_t *out, uint32_t at, XentTextRun const *run) {
	uint8_t narrow [2] = {run->font_stretch, ( uint8_t ) run->slant};
	at                 = blob_put(out, at, &run->offset, sizeof(run->offset));
	at                 = blob_put(out, at, &run->length, sizeof(run->length));
	at                 = blob_put(out, at, &run->font_size, sizeof(run->font_size));
	at                 = blob_put(out, at, &run->font_weight, sizeof(run->font_weight));
	at                 = blob_put(out, at, narrow, sizeof(narrow));
	return blob_put_str(out, at, run->font_family);
}

static uint32_t style_blob_build(uint8_t *out, XentTextMeasureReq const *request) {
	if (request->run_count == 0u && !request->font_family) return 0u;

	uint32_t at = blob_put(out, 0u, &request->run_count, sizeof(request->run_count));
	if (request->run_count == 0u) return blob_put_str(out, at, request->font_family);
	for (uint32_t i = 0u; i < request->run_count; ++i) at = blob_put_run(out, at, &request->runs [i]);
	return at;
}

static bool style_key_stage(XentCtx *ctx, XentTextMeasureReq const *request, XentTextStyleBlob *out) {
	*out          = (XentTextStyleBlob) {NULL, 0u};

	uint32_t size = style_blob_build(NULL, request);
	if (size == 0u) return true;

	if (size > ctx->style_key_cap) {
		uint8_t *grown = ( uint8_t * ) xent_realloc_internal(XENT_ALLOC_TEXT, ctx->style_key_buf, size);
		if (!grown) return false;
		ctx->style_key_buf = grown;
		ctx->style_key_cap = size;
	}

	( void ) style_blob_build(ctx->style_key_buf, request);
	*out = (XentTextStyleBlob) {ctx->style_key_buf, size};
	return true;
}

static void normalize_text_cache_key_for_backend(XentCtx const *ctx, XentTextCacheKey *key) {
	if (ctx->text_backend != &ctx->mono_backend) return;
	key->font_size   = 0.0f;
	key->font_weight = 0u;
	key->style       = (XentTextStyleBlob) {NULL, 0u};
}

static XentTextCacheKey
text_cache_key_from_measure_request(XentCtx const *ctx, XentTextMeasureReq const *request, XentTextStyleBlob style) {
	XentTextCacheKey key = {
	  request->text,
	  style,
	  request->font_size,
	  request->font_weight,
	  request->width_constraint,
	  request->max_lines,
	  request->trim,
	  request->line_break_policy,
	  request->width_mode,
	};
	normalize_text_cache_key_for_backend(ctx, &key);
	return key;
}

bool xent_text_backendvalid(XentTextBackend const *backend) {
	if (!backend || !backend->measure) return false;
	if (!backend->name || backend->name [0] == '\0') return false;
	return true;
}

static bool is_valid_width_constraint(XentMeasureMode mode, float width_constraint) {
	if (mode != XENT_MEASURE_EXACTLY && mode != XENT_MEASURE_AT_MOST) return true;
	return isfinite(width_constraint) && width_constraint >= 0.0f;
}

static bool is_valid_text_measure_request(XentCtx const *ctx, XentTextMeasureReq const *request) {
	if (!ctx || !ctx->text_backend || !ctx->text_backend->measure) return false;
	if (!request || !request->text) return false;
	if (!is_valid_line_break_policy(request->line_break_policy)) return false;
	if (!is_valid_measure_mode(request->width_mode)) return false;
	if (!is_valid_trim(request->trim)) return false;
	if (!are_valid_runs(request->text, request->runs, request->run_count)) return false;
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

static void text_node_restyled(XentCtx *ctx, XentNodeId node) {
	ctx->nodes.text.intrinsic_valid [xent_node_index(node)] = 0u;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
}

void xent_text_release_node(XentNodeStore *nodes, uint32_t index) {
	free(nodes->text.content [index]);
	free(nodes->text.font_family [index]);
	free(nodes->text.runs [index]);
	nodes->text.content [index]     = NULL;
	nodes->text.font_family [index] = NULL;
	nodes->text.runs [index]        = NULL;
	nodes->text.run_count [index]   = 0u;
}

static void clear_node_runs(XentCtx *ctx, uint32_t index) {
	free(ctx->nodes.text.runs [index]);
	ctx->nodes.text.runs [index]      = NULL;
	ctx->nodes.text.run_count [index] = 0u;
}

bool xent_settext(XentCtx *ctx, XentNodeId node, char const *text) {
	if (!xent_node_valid(ctx, node)) return false;

	char *copy = xent_strdup(XENT_ALLOC_TEXT, text ? text : "");
	if (!copy) return false;

	uint32_t index = xent_node_index(node);
	free(ctx->nodes.text.content [index]);
	ctx->nodes.text.content [index] = copy;
	clear_node_runs(ctx, index);
	text_node_restyled(ctx, node);
	return true;
}

char const *xent_node_text(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return NULL;
	return ctx->nodes.text.content [xent_node_index(node)];
}

static void text_subtree_restyled(XentCtx *ctx, XentNodeId root) {
	XentNodeId node = root;
	while (node != XENT_NODE_INVALID) {
		text_node_restyled(ctx, node);
		XentNodeId next = xent_node_first(ctx, node);
		while (next == XENT_NODE_INVALID && node != root) {
			next = xent_node_next(ctx, node);
			node = xent_node_parent(ctx, node);
		}
		node = next;
	}
}

bool xent_setfontsize(XentCtx *ctx, XentNodeId node, float font_size) {
	if (!xent_node_valid(ctx, node) || !(isnan(font_size) || font_size > 0.0f)) return false;
	ctx->nodes.text.font_size [xent_node_index(node)] = font_size;
	text_subtree_restyled(ctx, node);
	return true;
}

float xent_fontsize(XentCtx const *ctx, XentNodeId node) {
	for (; xent_node_valid(ctx, node); node = xent_node_parent(ctx, node)) {
		float size = ctx->nodes.text.font_size [xent_node_index(node)];
		if (!isnan(size)) return size;
	}
	return XENT_FONT_SIZE_DEFAULT;
}

uint16_t xent_fontweight(XentCtx const *ctx, XentNodeId node) {
	for (; xent_node_valid(ctx, node); node = xent_node_parent(ctx, node)) {
		uint16_t weight = ctx->nodes.text.font_weight [xent_node_index(node)];
		if (weight) return weight;
	}
	return XENT_FONT_WEIGHT_DEFAULT;
}

bool xent_setfontweight(XentCtx *ctx, XentNodeId node, uint16_t weight) {
	if (!xent_node_valid(ctx, node)) return false;
	ctx->nodes.text.font_weight [xent_node_index(node)] = weight;
	text_subtree_restyled(ctx, node);
	return true;
}

bool xent_setfontfamily(XentCtx *ctx, XentNodeId node, char const *family) {
	if (!xent_node_valid(ctx, node)) return false;

	char *copy = NULL;
	if (family && family [0] != '\0') {
		copy = xent_strdup(XENT_ALLOC_TEXT, family);
		if (!copy) return false;
	}

	uint32_t index = xent_node_index(node);
	free(ctx->nodes.text.font_family [index]);
	ctx->nodes.text.font_family [index] = copy;
	text_node_restyled(ctx, node);
	return true;
}

bool xent_setlinebreak(XentCtx *ctx, XentNodeId node, XentLineBreakPolicy policy) {
	if (!xent_node_valid(ctx, node) || !is_valid_line_break_policy(policy)) return false;
	if (ctx->nodes.text.line_break_policy [xent_node_index(node)] != ( uint8_t ) policy) {
		ctx->nodes.text.line_break_policy [xent_node_index(node)] = ( uint8_t ) policy;
		text_node_restyled(ctx, node);
	}
	return true;
}

XentLineBreakPolicy xent_linebreak(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return XENT_LINEBREAK_CHARWRAP;
	return ( XentLineBreakPolicy ) ctx->nodes.text.line_break_policy [xent_node_index(node)];
}

bool xent_settexttrim(XentCtx *ctx, XentNodeId node, XentTextTrim trim) {
	if (!xent_node_valid(ctx, node) || !is_valid_trim(trim)) return false;
	if (ctx->nodes.text.trim [xent_node_index(node)] != ( uint8_t ) trim) {
		ctx->nodes.text.trim [xent_node_index(node)] = ( uint8_t ) trim;
		text_node_restyled(ctx, node);
	}
	return true;
}

bool xent_setmaxlines(XentCtx *ctx, XentNodeId node, uint32_t max_lines) {
	if (!xent_node_valid(ctx, node)) return false;
	if (ctx->nodes.text.max_lines [xent_node_index(node)] != max_lines) {
		ctx->nodes.text.max_lines [xent_node_index(node)] = max_lines;
		text_node_restyled(ctx, node);
	}
	return true;
}

static size_t runs_block_size(XentTextRun const *runs, uint32_t count) {
	size_t size = sizeof(XentTextRun) * ( size_t ) count;
	for (uint32_t i = 0u; i < count; ++i)
		if (runs [i].font_family) size += strlen(runs [i].font_family) + 1u;
	return size;
}

static XentTextRun *runs_clone(XentTextRun const *runs, uint32_t count) {
	XentTextRun *block = ( XentTextRun * ) xent_alloc_internal(XENT_ALLOC_TEXT, runs_block_size(runs, count));
	if (!block) return NULL;

	char *strings = ( char * ) (block + count);
	for (uint32_t i = 0u; i < count; ++i) {
		block [i] = runs [i];
		if (!runs [i].font_family) continue;
		size_t size = strlen(runs [i].font_family) + 1u;
		memcpy(strings, runs [i].font_family, size);
		block [i].font_family  = strings;
		strings               += size;
	}
	return block;
}

bool xent_settextruns(XentCtx *ctx, XentNodeId node, XentTextRun const *runs, uint32_t count) {
	if (!xent_node_valid(ctx, node)) return false;

	uint32_t    index = xent_node_index(node);
	char const *text  = ctx->nodes.text.content [index];
	if (!are_valid_runs(text ? text : "", runs, count)) return false;

	XentTextRun *block = count ? runs_clone(runs, count) : NULL;
	if (count && !block) return false;

	clear_node_runs(ctx, index);
	ctx->nodes.text.runs [index]      = block;
	ctx->nodes.text.run_count [index] = count;
	text_node_restyled(ctx, node);
	return true;
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
	if (backend && !xent_text_backendvalid(backend)) return false;
	XentTextBackend const *next_backend = backend ? backend : &ctx->mono_backend;
	if (ctx->text_backend != next_backend) replace_text_backend(ctx, next_backend);
	return true;
}

XentTextBackend const *xent_text_backend(XentCtx const *ctx) {
	if (!ctx) return NULL;
	return ctx->text_backend;
}

bool xent_text_measure(XentCtx *ctx, XentTextMeasureReq const *request, XentTextMetrics *out_metrics) {
	if (!out_metrics || !is_valid_text_measure_request(ctx, request)) return false;

	XentTextStyleBlob style = {NULL, 0u};
	if (!style_key_stage(ctx, request, &style)) return false;

	double           measure_start_ms = begin_text_measure_profile(ctx);
	XentTextCacheKey cache_key        = text_cache_key_from_measure_request(ctx, request, style);
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

bool xent_text_request_valid(XentCtx const *ctx, XentTextMeasureReq const *request) {
	return is_valid_text_measure_request(ctx, request);
}

XentTextCacheStats xent_text_stats(XentCtx const *ctx) {
	XentTextCacheStats zero = {0};
	if (!ctx) return zero;
	return ctx->text_cache.stats;
}
