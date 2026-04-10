#include "../xent_internal.h"

XentContext *xent_create_context(const XentConfig *config) {
    XentContext *ctx = (XentContext *)calloc(1, sizeof(*ctx));
    if (!ctx) {
        return NULL;
    }

    ctx->config.initial_capacity = (config && config->initial_capacity) ? config->initial_capacity : 256u;
    ctx->config.mono_glyph_width = (config && config->mono_glyph_width > 0.0f) ? config->mono_glyph_width : 8.0f;
    ctx->config.mono_line_height = (config && config->mono_line_height > 0.0f) ? config->mono_line_height : 16.0f;
    ctx->config.enable_simd = config ? config->enable_simd : false;
    ctx->config.point_scale_factor = (config && config->point_scale_factor > 0.0f) ? config->point_scale_factor : 1.0f;
    ctx->config.enable_pixel_rounding = config ? config->enable_pixel_rounding : false;
    ctx->last_layout_root = XENT_NODE_INVALID;
    ctx->last_layout_available_w = NAN;
    ctx->last_layout_available_h = NAN;
    ctx->last_layout_strategy = (uint8_t)XENT_LAYOUT_STRATEGY_NONE;

    if (!xent_ensure_node_capacity(ctx, ctx->config.initial_capacity + 1u)) {
        xent_destroy_context(ctx);
        return NULL;
    }

    if (!xent_text_cache_init(&ctx->text_cache)) {
        xent_destroy_context(ctx);
        return NULL;
    }
    if (!xent_shape_cache_init(&ctx->shape_cache)) {
        xent_destroy_context(ctx);
        return NULL;
    }

    if (!xent_text_backend_mono_init(ctx)) {
        xent_destroy_context(ctx);
        return NULL;
    }

    return ctx;
}

static void xent_free_strings(XentContext *ctx) {
    for (uint32_t i = 0; i < ctx->nodes.capacity; ++i) {
        free(ctx->nodes.text[i]);
        ctx->nodes.text[i] = NULL;
        free(ctx->nodes.semantic_label[i]);
        ctx->nodes.semantic_label[i] = NULL;
    }
}

void xent_destroy_context(XentContext *ctx) {
    if (!ctx) {
        return;
    }

    xent_free_strings(ctx);
    xent_text_cache_destroy(&ctx->text_cache);
    xent_shape_cache_destroy(&ctx->shape_cache);

#define FREE_FIELD(field)                                                                                                 \
    free(ctx->nodes.field);                                                                                                \
    ctx->nodes.field = NULL

    FREE_FIELD(alive);
    FREE_FIELD(parent);
    FREE_FIELD(first_child);
    FREE_FIELD(next_sibling);
    FREE_FIELD(child_count);

    FREE_FIELD(protocol);
    FREE_FIELD(direction);
    FREE_FIELD(dirty_flags);

    FREE_FIELD(proposed_w);
    FREE_FIELD(proposed_h);
    FREE_FIELD(decided_w);
    FREE_FIELD(decided_h);
    FREE_FIELD(abs_x);
    FREE_FIELD(abs_y);

    FREE_FIELD(style_w);
    FREE_FIELD(style_h);
    FREE_FIELD(min_w);
    FREE_FIELD(min_h);
    FREE_FIELD(max_w);
    FREE_FIELD(max_h);

    FREE_FIELD(margin_l);
    FREE_FIELD(margin_t);
    FREE_FIELD(margin_r);
    FREE_FIELD(margin_b);

    FREE_FIELD(padding_l);
    FREE_FIELD(padding_t);
    FREE_FIELD(padding_r);
    FREE_FIELD(padding_b);

    FREE_FIELD(gap);

    FREE_FIELD(abs_pos_x);
    FREE_FIELD(abs_pos_y);

    FREE_FIELD(flex_grow);
    FREE_FIELD(flex_shrink);
    FREE_FIELD(flex_basis);
    FREE_FIELD(flex_direction);
    FREE_FIELD(flex_wrap);
    FREE_FIELD(flex_justify_content);
    FREE_FIELD(flex_align_items);
    FREE_FIELD(flex_align_self);
    FREE_FIELD(flex_align_content);

    FREE_FIELD(stack_axis);
    FREE_FIELD(stack_align);
    FREE_FIELD(layout_priority);
    FREE_FIELD(is_spacer);

    FREE_FIELD(text);
    FREE_FIELD(font_size);
    FREE_FIELD(text_line_break_policy);
    FREE_FIELD(text_intrinsic_valid);
    FREE_FIELD(text_intrinsic_constraint_w);
    FREE_FIELD(text_intrinsic_font_size);
    FREE_FIELD(text_intrinsic_line_break_policy);
    FREE_FIELD(text_intrinsic_width_mode);
    FREE_FIELD(text_intrinsic_w);
    FREE_FIELD(text_intrinsic_h);
    FREE_FIELD(text_intrinsic_lines);

    FREE_FIELD(semantic_role);
    FREE_FIELD(semantic_label);
    FREE_FIELD(semantic_flags);

    FREE_FIELD(userdata);
    FREE_FIELD(control_type);
    FREE_FIELD(semantic_checked);
    FREE_FIELD(semantic_enabled);
    FREE_FIELD(semantic_expanded);
    FREE_FIELD(semantic_selected);
    FREE_FIELD(semantic_value_now);
    FREE_FIELD(semantic_value_min);
    FREE_FIELD(semantic_value_max);

#undef FREE_FIELD

    free(ctx->free_ids);
    free(ctx->work_order);
    free(ctx->plugins);
    free(ctx->scratch);
    ctx->free_ids = NULL;
    ctx->work_order = NULL;
    ctx->plugins = NULL;
    ctx->scratch = NULL;

    free(ctx);
}
