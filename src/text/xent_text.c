#include "../xent_internal.h"

static bool xent_is_valid_line_break_policy(XentLineBreakPolicy policy) {
    return policy == XENT_LINE_BREAK_NO_WRAP || policy == XENT_LINE_BREAK_WORD_WRAP ||
           policy == XENT_LINE_BREAK_CHAR_WRAP;
}

static bool xent_is_valid_measure_mode(XentMeasureMode mode) {
    return mode == XENT_MEASURE_UNDEFINED || mode == XENT_MEASURE_AT_MOST || mode == XENT_MEASURE_EXACTLY;
}

bool xent_validate_text_backend(const XentTextBackend *backend) {
    if (!backend || !backend->measure || !backend->shape) {
        return false;
    }
    if (!backend->name || backend->name[0] == '\0') {
        return false;
    }
    return true;
}

bool xent_set_text(XentContext *ctx, XentNodeId node, const char *text) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }

    char *copy = xent_strdup(text ? text : "");
    if (!copy) {
        return false;
    }

    free(ctx->nodes.text[node]);
    ctx->nodes.text[node] = copy;
    ctx->nodes.text_intrinsic_valid[node] = 0u;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

const char *xent_get_text(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return NULL;
    }
    return ctx->nodes.text[node];
}

bool xent_set_font_size(XentContext *ctx, XentNodeId node, float font_size) {
    if (!xent_is_valid_node(ctx, node) || font_size <= 0.0f) {
        return false;
    }
    ctx->nodes.font_size[node] = font_size;
    ctx->nodes.text_intrinsic_valid[node] = 0u;
    xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_set_text_line_break_policy(XentContext *ctx, XentNodeId node, XentLineBreakPolicy policy) {
    if (!xent_is_valid_node(ctx, node) || !xent_is_valid_line_break_policy(policy)) {
        return false;
    }
    if (ctx->nodes.text_line_break_policy[node] != (uint8_t)policy) {
        ctx->nodes.text_line_break_policy[node] = (uint8_t)policy;
        ctx->nodes.text_intrinsic_valid[node] = 0u;
        xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
    }
    return true;
}

XentLineBreakPolicy xent_get_text_line_break_policy(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return XENT_LINE_BREAK_CHAR_WRAP;
    }
    return (XentLineBreakPolicy)ctx->nodes.text_line_break_policy[node];
}

bool xent_set_text_backend(XentContext *ctx, const XentTextBackend *backend) {
    if (!ctx) {
        return false;
    }
    if (backend && !xent_validate_text_backend(backend)) {
        return false;
    }
    const XentTextBackend *next_backend = backend ? backend : &ctx->mono_backend;
    if (ctx->text_backend != next_backend) {
        ctx->text_backend = next_backend;
        xent_text_cache_destroy(&ctx->text_cache);
        (void)xent_text_cache_init(&ctx->text_cache);
        xent_shape_cache_destroy(&ctx->shape_cache);
        (void)xent_shape_cache_init(&ctx->shape_cache);
        for (uint32_t i = 1u; i <= ctx->nodes.count; ++i) {
            if (ctx->nodes.alive[i]) {
                ctx->nodes.text_intrinsic_valid[i] = 0u;
            }
        }
    }
    return true;
}

const XentTextBackend *xent_get_text_backend(const XentContext *ctx) {
    if (!ctx) {
        return NULL;
    }
    return ctx->text_backend;
}

bool xent_measure_text(XentContext *ctx,
                       const char *text,
                       float font_size,
                       float width_constraint,
                       XentLineBreakPolicy line_break_policy,
                       XentMeasureMode width_mode,
                       XentTextMetrics *out_metrics) {
    if (!ctx || !ctx->text_backend || !ctx->text_backend->measure || !text || !out_metrics) {
        return false;
    }
    if (!xent_is_valid_line_break_policy(line_break_policy)) {
        return false;
    }
    if (!xent_is_valid_measure_mode(width_mode)) {
        return false;
    }
    if (width_mode == XENT_MEASURE_EXACTLY && (!isfinite(width_constraint) || width_constraint < 0.0f)) {
        return false;
    }

    bool track_swiftstack = ctx->swiftstack_scope_depth > 0u;
    double measure_start_ms = 0.0;
    if (track_swiftstack) {
        ctx->profile.text_measure_calls += 1u;
        measure_start_ms = xent_now_ms();
    }

    if (xent_text_cache_lookup(
            &ctx->text_cache, text, font_size, width_constraint, line_break_policy, width_mode, out_metrics)) {
        if (track_swiftstack) {
            ctx->profile.swiftstack_text_ms += (xent_now_ms() - measure_start_ms);
        }
        return true;
    }

    if (!ctx->text_backend->measure(
            ctx->text_backend, text, font_size, width_constraint, line_break_policy, width_mode, out_metrics)) {
        if (track_swiftstack) {
            ctx->profile.swiftstack_text_ms += (xent_now_ms() - measure_start_ms);
        }
        return false;
    }

    xent_text_cache_insert(
        &ctx->text_cache, text, font_size, width_constraint, line_break_policy, width_mode, out_metrics);
    if (track_swiftstack) {
        ctx->profile.swiftstack_text_ms += (xent_now_ms() - measure_start_ms);
    }
    return true;
}

bool xent_shape_text(XentContext *ctx,
                     const char *text,
                     float font_size,
                     float width_constraint,
                     XentLineBreakPolicy line_break_policy,
                     XentMeasureMode width_mode,
                     XentShapedGlyph *out_glyphs,
                     uint32_t glyph_capacity,
                     XentShapedRun *out_runs,
                     uint32_t run_capacity,
                     XentShapedLine *out_lines,
                     uint32_t line_capacity,
                     XentShapingResult *out_result) {
    if (!ctx || !ctx->text_backend || !ctx->text_backend->shape || !text || !out_result) {
        return false;
    }
    if (!xent_is_valid_line_break_policy(line_break_policy)) {
        return false;
    }
    if (!xent_is_valid_measure_mode(width_mode)) {
        return false;
    }
    if (width_mode == XENT_MEASURE_EXACTLY && (!isfinite(width_constraint) || width_constraint < 0.0f)) {
        return false;
    }

    bool summary_only = (out_glyphs == NULL && glyph_capacity == 0u && out_runs == NULL && run_capacity == 0u &&
                         out_lines == NULL && line_capacity == 0u);
    if (summary_only && xent_shape_cache_lookup(&ctx->shape_cache,
                                                text,
                                                font_size,
                                                width_constraint,
                                                line_break_policy,
                                                width_mode,
                                                out_result)) {
        return true;
    }

    if (!ctx->text_backend->shape(ctx->text_backend,
                                  text,
                                  font_size,
                                  width_constraint,
                                  line_break_policy,
                                  width_mode,
                                  out_glyphs,
                                  glyph_capacity,
                                  out_runs,
                                  run_capacity,
                                  out_lines,
                                  line_capacity,
                                  out_result)) {
        return false;
    }

    xent_shape_cache_insert(
        &ctx->shape_cache, text, font_size, width_constraint, line_break_policy, width_mode, out_result);
    return true;
}

XentTextCacheStats xent_get_text_cache_stats(const XentContext *ctx) {
    XentTextCacheStats zero = {0};
    if (!ctx) {
        return zero;
    }
    return ctx->text_cache.stats;
}

XentTextCacheStats xent_get_shape_cache_stats(const XentContext *ctx) {
    XentTextCacheStats zero = {0};
    if (!ctx) {
        return zero;
    }
    return ctx->shape_cache.stats;
}
