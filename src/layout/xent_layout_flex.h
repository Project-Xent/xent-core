#ifndef XENT_LAYOUT_FLEX_H
#define XENT_LAYOUT_FLEX_H

#include "../xent_internal.h"

void xent_layout_node_flex(XentLayoutRequest const *request);

/* Content-box main/cross extents of a flex container, per Flexbox §9.4/§9.9:
 * resolve used main sizes, measure each item's cross at its used main, sum the
 * line cross sizes. Used for content-based (wrap-content) container sizing. */
void xent_flex_intrinsic_content(
  XentCtx *ctx, XentNodeId node, float avail_main, float avail_cross, bool row, float *out_main, float *out_cross
);

/* Opt-in fit-content: size a FLEX container's auto axis to the extent of its
 * children, instead of filling available space. Delegates to the spec content
 * sizer (Flexbox §9.4/§9.9), which resolves used main sizes and then measures
 * each item's cross at its used main — so a wrapped/grow item contributes its
 * real (wrapped) cross extent rather than a max-content single line. The wrapped
 * axis is measured unconstrained (INFINITY) as its available space. */
void xent_flex_wrap_content(
  XentCtx *ctx, XentNodeId node, float available_w, float available_h, float *width, float *height
);

#endif
