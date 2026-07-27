#ifndef XENT_TEXT_H
#define XENT_TEXT_H

#include "xent_types.h"


typedef bool (*XentTextMeasureFn)(
  XentTextBackend const *backend, XentTextMeasureReq const *request, XentTextMetrics *out_metrics
);

struct XentTextBackend {
	char const       *name;
	XentTextMeasureFn measure;
	void             *userdata;
};

bool                   xent_text_backend_valid(XentTextBackend const *backend);

bool                   xent_settext(XentCtx *ctx, XentNodeId node, char const *text);
bool                   xent_setfontsize(XentCtx *ctx, XentNodeId node, float font_size);
bool                   xent_setfontweight(XentCtx *ctx, XentNodeId node, uint16_t weight);
bool                   xent_setlinebreak(XentCtx *ctx, XentNodeId node, XentLineBreakPolicy policy);
XentLineBreakPolicy    xent_linebreak(XentCtx const *ctx, XentNodeId node);

bool                   xent_text_setbackend(XentCtx *ctx, XentTextBackend const *backend);
XentTextBackend const *xent_text_backend(XentCtx const *ctx);

bool                   xent_text_measure(XentCtx *ctx, XentTextMeasureReq const *request, XentTextMetrics *out_metrics);

XentTextCacheStats     xent_text_stats(XentCtx const *ctx);


#endif
