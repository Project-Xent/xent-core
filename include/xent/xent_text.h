#ifndef XENT_TEXT_H
#define XENT_TEXT_H

#include "xent_types.h"

typedef struct XentShapeSink XentShapeSink;

bool xent_shape_emit(XentShapeSink *sink, XentGlyphRunDesc const *desc, XentGlyph const *glyphs, uint32_t count);

typedef bool (*XentTextMeasureFn)(
  XentTextBackend const *backend, XentTextMeasureReq const *request, XentTextMetrics *out_metrics
);

typedef bool (*XentTextShapeFn)(
  XentTextBackend const *backend, XentTextMeasureReq const *request, XentShapeSink *sink, XentTextMetrics *out_metrics
);

struct XentTextBackend {
	char const       *name;
	XentTextMeasureFn measure;
	XentTextShapeFn   shape;
	void             *userdata;
};

bool xent_text_backendvalid(XentTextBackend const *backend);

/** Copies `text`. NULL is stored as an empty string. The caller may free it. */
bool xent_settext(XentCtx *ctx, XentNodeId node, char const *text);
#define XENT_FONT_SIZE_DEFAULT   14.0f
#define XENT_FONT_WEIGHT_DEFAULT 400u

bool                   xent_setfontsize(XentCtx *ctx, XentNodeId node, float font_size);
bool                   xent_setfontweight(XentCtx *ctx, XentNodeId node, uint16_t weight);
float                  xent_fontsize(XentCtx const *ctx, XentNodeId node);
uint16_t               xent_fontweight(XentCtx const *ctx, XentNodeId node);
/** Copies `family`. NULL or "" clears it. The caller may free it. */
bool                   xent_setfontfamily(XentCtx *ctx, XentNodeId node, char const *family);
bool                   xent_setlinebreak(XentCtx *ctx, XentNodeId node, XentLineBreakPolicy policy);
XentLineBreakPolicy    xent_linebreak(XentCtx const *ctx, XentNodeId node);
bool                   xent_settexttrim(XentCtx *ctx, XentNodeId node, XentTextTrim trim);
bool                   xent_setmaxlines(XentCtx *ctx, XentNodeId node, uint32_t max_lines);

/** Copies `runs` and each `font_family`. The caller may free the input. */
bool                   xent_settextruns(XentCtx *ctx, XentNodeId node, XentTextRun const *runs, uint32_t count);

bool                   xent_text_setbackend(XentCtx *ctx, XentTextBackend const *backend);
XentTextBackend const *xent_text_backend(XentCtx const *ctx);

bool                   xent_text_measure(XentCtx *ctx, XentTextMeasureReq const *request, XentTextMetrics *out_metrics);

XENT_NODISCARD XentTextShape *xent_text_shape(XentCtx *ctx, XentTextMeasureReq const *request);
void                          xent_text_freeshape(XentTextShape *shape);

XentTextCacheStats            xent_text_stats(XentCtx const *ctx);

#endif
