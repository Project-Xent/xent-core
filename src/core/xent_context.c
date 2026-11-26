#include "../xent_internal.h"

#ifndef XENT_ENABLE_SIMD
  #define XENT_ENABLE_SIMD 0
#endif

static uint32_t config_initial_capacity(XentCfg const *config) {
	if (!config) return 0u;
	return config->initial_capacity;
}

static float config_positive_float(float value, float fallback) {
	if (value <= 0.0f) return fallback;
	return value;
}

static void apply_config_defaults(XentCtx *ctx, XentCfg const *config) {
	ctx->config.initial_capacity      = config_initial_capacity(config);
	ctx->config.mono_glyph_width      = config ? config_positive_float(config->mono_glyph_width, 8.0f) : 8.0f;
	ctx->config.mono_line_height      = config ? config_positive_float(config->mono_line_height, 16.0f) : 16.0f;
	ctx->config.point_scale_factor    = config ? config_positive_float(config->point_scale_factor, 1.0f) : 1.0f;
	ctx->config.enable_pixel_rounding = config ? config->enable_pixel_rounding : false;
	ctx->config.enable_simd           = config ? config->enable_simd : XENT_ENABLE_SIMD != 0;
}

static void reset_last_layout(XentCtx *ctx) {
	for (uint32_t i = 0u; i < XENT_ROOT_SNAPSHOT_SLOTS; ++i) {
		ctx->root_snapshots [i] = (XentRootSnapshot) {
		  .root        = XENT_NODE_INVALID,
		  .available_w = NAN,
		  .available_h = NAN,
		};
	}
	ctx->root_snapshot_clock  = 0u;
	ctx->last_layout_root     = XENT_NODE_INVALID;
	ctx->last_layout_strategy = ( uint8_t ) XENT_LAYOUT_STRATEGY_NONE;
}

static bool init_context_storage(XentCtx *ctx) {
	uint32_t capacity = ctx->config.initial_capacity;
	if (capacity >= UINT32_MAX - 1u) return false;
	if (capacity > 0u && !xent_ensure_node_capacity(ctx, capacity + 1u)) return false;
	if (!xent_text_cache_init(&ctx->text_cache)) return false;
	return xent_text_mono_init(ctx);
}

XentCtx *xent_ctx_create(XentCfg const *config) {
	XentCtx *ctx = ( XentCtx * ) calloc(1, sizeof(*ctx));
	if (!ctx) return NULL;

	apply_config_defaults(ctx, config);
	reset_last_layout(ctx);
	ctx->nodes.min_generation = 1u;
	ctx->scratch_chunk_size   = XENT_SCRATCH_CHUNK_SIZE;
	if (!init_context_storage(ctx)) {
		xent_ctx_destroy(ctx);
		return NULL;
	}

	return ctx;
}

bool xent_node_reserve(XentCtx *ctx, uint32_t capacity) {
	if (!ctx) return false;
	if (capacity >= UINT32_MAX - 1u) return false;
	return xent_ensure_node_capacity(ctx, capacity + 1u);
}

static void free_cold_resources(XentCtx *ctx) {
	XentNodeStore *n = &ctx->nodes;
	for (uint32_t s = 1u; s <= n->text_pool.count; ++s) {
		XentTextComp *t = ( XentTextComp * ) xent_pool_get(&n->text_pool, sizeof(*t), s);
		if (!t || !t->active) continue;
		free(t->content);
		free(t->font_family);
		free(t->runs);
		t->content     = NULL;
		t->font_family = NULL;
		t->runs        = NULL;
	}
	for (uint32_t s = 1u; s <= n->sem_pool.count; ++s) {
		XentSemComp *sem = ( XentSemComp * ) xent_pool_get(&n->sem_pool, sizeof(*sem), s);
		if (!sem || !sem->active) continue;
		free(sem->label);
		sem->label = NULL;
	}
	for (uint32_t s = 1u; s <= n->grid_pool.count; ++s) {
		XentGridComp *g = ( XentGridComp * ) xent_pool_get(&n->grid_pool, sizeof(*g), s);
		if (!g || !g->active) continue;
		xent_grid_def_free(g->def);
		g->def = NULL;
	}
}

void xent_ctx_destroy(XentCtx *ctx) {
	if (!ctx) return;
	if (ctx->node_observer_dispatch_depth) return;

	xent_extmeasure_clear(ctx);
	free_cold_resources(ctx);
	xent_text_cache_destroy(&ctx->text_cache);
	xent_arena_free(&ctx->nodes);

	free(ctx->free_indices);
	free(ctx->work_order);
	free(ctx->dirty_nodes);
	free(ctx->sem_dirty_nodes);
	free(ctx->node_observers);
	free(ctx->style_key_buf);
	xent_free_scratch(ctx);
	ctx->free_indices    = NULL;
	ctx->work_order      = NULL;
	ctx->dirty_nodes     = NULL;
	ctx->sem_dirty_nodes = NULL;
	ctx->node_observers  = NULL;
	ctx->style_key_buf   = NULL;
	ctx->style_key_cap   = 0u;

	free(ctx);
}
