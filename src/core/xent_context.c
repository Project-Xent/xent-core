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
	ctx->last_layout_root        = XENT_NODE_INVALID;
	ctx->last_layout_available_w = NAN;
	ctx->last_layout_available_h = NAN;
	ctx->last_layout_strategy    = ( uint8_t ) XENT_LAYOUT_STRATEGY_NONE;
	ctx->last_layout_node_count  = 0u;
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
	ctx->scratch_chunk_size = XENT_SCRATCH_CHUNK_SIZE;
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

static void free_strings(XentCtx *ctx) {
	for (uint32_t i = 0; i < ctx->nodes.capacity; ++i) {
		free(ctx->nodes.text.content [i]);
		ctx->nodes.text.content [i] = NULL;
		free(ctx->nodes.semantics.label [i]);
		ctx->nodes.semantics.label [i] = NULL;
		if (ctx->nodes.grid.def [i]) {
			free(ctx->nodes.grid.def [i]->row_modes);
			free(ctx->nodes.grid.def [i]->row_values);
			free(ctx->nodes.grid.def [i]->col_modes);
			free(ctx->nodes.grid.def [i]->col_values);
			free(ctx->nodes.grid.def [i]);
		}
		ctx->nodes.grid.def [i] = NULL;
	}
}

void xent_ctx_destroy(XentCtx *ctx) {
	if (!ctx) return;
	if (ctx->node_observer_dispatch_depth) return;

	xent_extmeasure_clear(ctx);
	free_strings(ctx);
	xent_text_cache_destroy(&ctx->text_cache);

#define FREE_FIELD(field)   \
	free(ctx->nodes.field); \
	ctx->nodes.field = NULL

	FREE_FIELD(lifetime.alive);
	FREE_FIELD(lifetime.generation);

	FREE_FIELD(topology.parent);
	FREE_FIELD(topology.first_child);
	FREE_FIELD(topology.last_child);
	FREE_FIELD(topology.next_sibling);
	FREE_FIELD(topology.prev_sibling);
	FREE_FIELD(topology.child_count);

	FREE_FIELD(layout.protocol);
	FREE_FIELD(layout.direction);
	FREE_FIELD(layout.wrap_content_w);
	FREE_FIELD(layout.wrap_content_h);
	FREE_FIELD(layout.dirty_flags);
	FREE_FIELD(layout.dirty_queued);
	FREE_FIELD(layout.proposed_w);
	FREE_FIELD(layout.proposed_h);
	FREE_FIELD(layout.decided_w);
	FREE_FIELD(layout.decided_h);
	FREE_FIELD(layout.abs_x);
	FREE_FIELD(layout.abs_y);
	FREE_FIELD(layout.style_w);
	FREE_FIELD(layout.style_h);
	FREE_FIELD(layout.style_w_percent);
	FREE_FIELD(layout.style_h_percent);
	FREE_FIELD(layout.aspect_ratio);
	FREE_FIELD(layout.min_w);
	FREE_FIELD(layout.min_h);
	FREE_FIELD(layout.max_w);
	FREE_FIELD(layout.max_h);
	FREE_FIELD(layout.margin_l);
	FREE_FIELD(layout.margin_t);
	FREE_FIELD(layout.margin_r);
	FREE_FIELD(layout.margin_b);
	FREE_FIELD(layout.padding_l);
	FREE_FIELD(layout.padding_t);
	FREE_FIELD(layout.padding_r);
	FREE_FIELD(layout.padding_b);
	FREE_FIELD(layout.gap);
	FREE_FIELD(layout.abs_pos_x);
	FREE_FIELD(layout.abs_pos_y);
	FREE_FIELD(layout.z_index);

	FREE_FIELD(flex.grow);
	FREE_FIELD(flex.shrink);
	FREE_FIELD(flex.basis);
	FREE_FIELD(flex.direction);
	FREE_FIELD(flex.wrap);
	FREE_FIELD(flex.justify_content);
	FREE_FIELD(flex.align_items);
	FREE_FIELD(flex.align_self);
	FREE_FIELD(flex.align_content);

	FREE_FIELD(stack.axis);
	FREE_FIELD(stack.align);
	FREE_FIELD(stack.priority);
	FREE_FIELD(stack.spacer);

	FREE_FIELD(text.content);
	FREE_FIELD(text.font_size);
	FREE_FIELD(text.font_weight);
	FREE_FIELD(text.line_break_policy);
	FREE_FIELD(text.intrinsic_valid);
	FREE_FIELD(text.intrinsic_constraint_w);
	FREE_FIELD(text.intrinsic_font_size);
	FREE_FIELD(text.intrinsic_font_weight);
	FREE_FIELD(text.intrinsic_line_break_policy);
	FREE_FIELD(text.intrinsic_width_mode);
	FREE_FIELD(text.intrinsic_w);
	FREE_FIELD(text.intrinsic_h);
	FREE_FIELD(text.intrinsic_lines);

	FREE_FIELD(semantics.role);
	FREE_FIELD(semantics.label);
	FREE_FIELD(semantics.flags);
	FREE_FIELD(semantics.checked);
	FREE_FIELD(semantics.enabled);
	FREE_FIELD(semantics.expanded);
	FREE_FIELD(semantics.selected);
	FREE_FIELD(semantics.value_now);
	FREE_FIELD(semantics.value_min);
	FREE_FIELD(semantics.value_max);

	FREE_FIELD(focus.focusable);
	FREE_FIELD(focus.tab_index);

	FREE_FIELD(grid.def);
	FREE_FIELD(grid.row);
	FREE_FIELD(grid.column);
	FREE_FIELD(grid.row_span);
	FREE_FIELD(grid.column_span);

#undef FREE_FIELD

	free(ctx->free_indices);
	free(ctx->work_order);
	free(ctx->dirty_nodes);
	free(ctx->node_observers);
	xent_free_scratch(ctx);
	ctx->free_indices   = NULL;
	ctx->work_order     = NULL;
	ctx->dirty_nodes    = NULL;
	ctx->node_observers = NULL;

	free(ctx);
}
