#include "../xent_internal.h"

static uint32_t xent_config_initial_capacity(XentConfig const *config) {
	if (!config || config->initial_capacity == 0u) return 256u;
	return config->initial_capacity;
}

static float xent_config_positive_float(float value, float fallback) {
	if (value <= 0.0f) return fallback;
	return value;
}

static void xent_apply_config_defaults(XentContext *ctx, XentConfig const *config) {
	ctx->config.initial_capacity      = xent_config_initial_capacity(config);
	ctx->config.mono_glyph_width      = config ? xent_config_positive_float(config->mono_glyph_width, 8.0f) : 8.0f;
	ctx->config.mono_line_height      = config ? xent_config_positive_float(config->mono_line_height, 16.0f) : 16.0f;
	ctx->config.enable_simd           = config ? config->enable_simd : false;
	ctx->config.point_scale_factor    = config ? xent_config_positive_float(config->point_scale_factor, 1.0f) : 1.0f;
	ctx->config.enable_pixel_rounding = config ? config->enable_pixel_rounding : false;
}

static void xent_reset_last_layout(XentContext *ctx) {
	ctx->last_layout_root        = XENT_NODE_INVALID;
	ctx->last_layout_available_w = NAN;
	ctx->last_layout_available_h = NAN;
	ctx->last_layout_strategy    = ( uint8_t ) XENT_LAYOUT_STRATEGY_NONE;
	ctx->last_layout_node_count  = 0u;
}

static bool xent_init_context_storage(XentContext *ctx) {
	if (!xent_ensure_node_capacity(ctx, ctx->config.initial_capacity + 1u)) return false;
	if (!xent_text_cache_init(&ctx->text_cache)) return false;
	if (!xent_shape_cache_init(&ctx->shape_cache)) return false;
	return xent_text_backend_mono_init(ctx);
}

XentContext *xent_create_context(XentConfig const *config) {
	XentContext *ctx = ( XentContext * ) calloc(1, sizeof(*ctx));
	if (!ctx) return NULL;

	xent_apply_config_defaults(ctx, config);
	xent_reset_last_layout(ctx);
	if (!xent_init_context_storage(ctx)) {
		xent_destroy_context(ctx);
		return NULL;
	}

	return ctx;
}

static void xent_free_strings(XentContext *ctx) {
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

static void xent_free_payloads(XentContext *ctx) {
	for (uint32_t i = 0; i < ctx->nodes.capacity; ++i) {
		if (ctx->nodes.external.payload_destroy [i] && ctx->nodes.external.payload [i])
			ctx->nodes.external.payload_destroy [i](
			  ctx->nodes.external.payload [i], ctx->nodes.external.payload_destroy_userdata [i]
			);
		ctx->nodes.external.payload [i]                  = NULL;
		ctx->nodes.external.payload_type [i]             = 0u;
		ctx->nodes.external.payload_destroy [i]          = NULL;
		ctx->nodes.external.payload_destroy_userdata [i] = NULL;
	}
}

void xent_destroy_context(XentContext *ctx) {
	if (!ctx) return;

	xent_free_payloads(ctx);
	xent_free_strings(ctx);
	xent_text_cache_destroy(&ctx->text_cache);
	xent_shape_cache_destroy(&ctx->shape_cache);

#define FREE_FIELD(field)   \
	free(ctx->nodes.field); \
	ctx->nodes.field = NULL

	FREE_FIELD(lifetime.alive);

	FREE_FIELD(topology.parent);
	FREE_FIELD(topology.first_child);
	FREE_FIELD(topology.last_child);
	FREE_FIELD(topology.next_sibling);
	FREE_FIELD(topology.prev_sibling);
	FREE_FIELD(topology.child_count);

	FREE_FIELD(layout.protocol);
	FREE_FIELD(layout.direction);
	FREE_FIELD(layout.dirty_flags);
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
	FREE_FIELD(text.line_break_policy);
	FREE_FIELD(text.intrinsic_valid);
	FREE_FIELD(text.intrinsic_constraint_w);
	FREE_FIELD(text.intrinsic_font_size);
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

	FREE_FIELD(external.userdata);
	FREE_FIELD(external.payload);
	FREE_FIELD(external.payload_type);
	FREE_FIELD(external.payload_destroy);
	FREE_FIELD(external.payload_destroy_userdata);
	FREE_FIELD(external.control_type);

	FREE_FIELD(focus.focusable);
	FREE_FIELD(focus.tab_index);

	FREE_FIELD(grid.def);
	FREE_FIELD(grid.row);
	FREE_FIELD(grid.column);
	FREE_FIELD(grid.row_span);
	FREE_FIELD(grid.column_span);

#undef FREE_FIELD

	free(ctx->free_ids);
	free(ctx->work_order);
	free(ctx->dirty_nodes);
	free(ctx->plugins);
	free(ctx->scratch);
	ctx->free_ids    = NULL;
	ctx->work_order  = NULL;
	ctx->dirty_nodes = NULL;
	ctx->plugins     = NULL;
	ctx->scratch     = NULL;

	free(ctx);
}

bool xent_set_focusable(XentContext *ctx, XentNodeId node, bool focusable) {
	if (!ctx || node == XENT_NODE_INVALID || node >= ctx->nodes.capacity) return false;
	if (!ctx->nodes.lifetime.alive [node]) return false;
	ctx->nodes.focus.focusable [node] = focusable ? 1 : 0;
	return true;
}

bool xent_get_focusable(XentContext const *ctx, XentNodeId node) {
	if (!ctx || node == XENT_NODE_INVALID || node >= ctx->nodes.capacity) return false;
	if (!ctx->nodes.lifetime.alive [node]) return false;
	return ctx->nodes.focus.focusable [node] != 0;
}

bool xent_set_tab_index(XentContext *ctx, XentNodeId node, int32_t tab_index) {
	if (!ctx || node == XENT_NODE_INVALID || node >= ctx->nodes.capacity) return false;
	if (!ctx->nodes.lifetime.alive [node]) return false;
	ctx->nodes.focus.tab_index [node] = tab_index;
	return true;
}

int32_t xent_get_tab_index(XentContext const *ctx, XentNodeId node) {
	if (!ctx || node == XENT_NODE_INVALID || node >= ctx->nodes.capacity) return 0;
	if (!ctx->nodes.lifetime.alive [node]) return 0;
	return ctx->nodes.focus.tab_index [node];
}
