#ifndef XENT_TEXT_H
#define XENT_TEXT_H

#include "xent_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*XentTextMeasureFn)(const XentTextBackend *backend,
                                  const char *text,
                                  float font_size,
                                  float width_constraint,
                                  XentLineBreakPolicy line_break_policy,
                                  XentMeasureMode width_mode,
                                  XentTextMetrics *out_metrics);

typedef bool (*XentTextShapeFn)(const XentTextBackend *backend,
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
                                XentShapingResult *out_result);

struct XentTextBackend {
    const char *name;
    XentTextMeasureFn measure;
    XentTextShapeFn shape;
    void *userdata;
};

bool xent_validate_text_backend(const XentTextBackend *backend);

bool xent_set_text(XentContext *ctx, XentNodeId node, const char *text);
bool xent_set_font_size(XentContext *ctx, XentNodeId node, float font_size);
bool xent_set_text_line_break_policy(XentContext *ctx, XentNodeId node, XentLineBreakPolicy policy);
XentLineBreakPolicy xent_get_text_line_break_policy(const XentContext *ctx, XentNodeId node);

bool xent_set_text_backend(XentContext *ctx, const XentTextBackend *backend);
const XentTextBackend *xent_get_text_backend(const XentContext *ctx);

bool xent_measure_text(XentContext *ctx,
                       const char *text,
                       float font_size,
                       float width_constraint,
                       XentLineBreakPolicy line_break_policy,
                       XentMeasureMode width_mode,
                       XentTextMetrics *out_metrics);

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
                     XentShapingResult *out_result);

XentTextCacheStats xent_get_text_cache_stats(const XentContext *ctx);
XentTextCacheStats xent_get_shape_cache_stats(const XentContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
