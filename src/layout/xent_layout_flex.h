#ifndef XENT_LAYOUT_FLEX_H
#define XENT_LAYOUT_FLEX_H

#include "../xent_internal.h"

void xent_layout_node_flex(XentLayoutRequest const *request);

void xent_flex_intrinsic_content(
  XentCtx *ctx, XentNodeId node, float avail_main, float avail_cross, bool row, float *out_main, float *out_cross
);

void xent_flex_wrap_content(
  XentCtx *ctx, XentNodeId node, float available_w, float available_h, float *width, float *height
);

#endif
