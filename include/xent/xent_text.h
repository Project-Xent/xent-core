#ifndef XENT_TEXT_H
#define XENT_TEXT_H

#include "xent_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef bool (*XentTextMeasureFn)(
  XentTextBackend const *backend, XentTextMeasureRequest const *request, XentTextMetrics *out_metrics
);

typedef bool (*XentTextShapeFn)(
  XentTextBackend const *backend, XentTextShapeRequest const *request, XentTextShapeOutput const *output
);

struct XentTextBackend {
	char const       *name;
	XentTextMeasureFn measure;
	XentTextShapeFn   shape;
	void             *userdata;
};

bool                   xent_validate_text_backend(XentTextBackend const *backend);

bool                   xent_set_text(XentContext *ctx, XentNodeId node, char const *text);
bool                   xent_set_font_size(XentContext *ctx, XentNodeId node, float font_size);
bool                   xent_set_text_line_break_policy(XentContext *ctx, XentNodeId node, XentLineBreakPolicy policy);
XentLineBreakPolicy    xent_get_text_line_break_policy(XentContext const *ctx, XentNodeId node);

bool                   xent_set_text_backend(XentContext *ctx, XentTextBackend const *backend);
XentTextBackend const *xent_get_text_backend(XentContext const *ctx);

bool xent_measure_text(XentContext *ctx, XentTextMeasureRequest const *request, XentTextMetrics *out_metrics);

bool xent_shape_text(XentContext *ctx, XentTextShapeRequest const *request, XentTextShapeOutput const *output);

XentTextCacheStats xent_get_text_cache_stats(XentContext const *ctx);
XentTextCacheStats xent_get_shape_cache_stats(XentContext const *ctx);

#ifdef __cplusplus
}
#endif

#endif
