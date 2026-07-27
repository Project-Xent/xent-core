#ifndef XENT_LAYOUT_H
#define XENT_LAYOUT_H

#include "xent_types.h"


bool           xent_setproto(XentCtx *ctx, XentNodeId node, XentProtocol protocol);

/**
 * @brief Copy style inputs (protocol, size, flex, stack, grid template/placement,
 * measure text/font) from @p src onto @p dst. Computed layout rects are not copied.
 * Used when absorbing a seeded orphan onto a live Host node.
 */
bool           xent_copystyle(XentCtx *ctx, XentNodeId dst, XentNodeId src);

bool           xent_setsize(XentCtx *ctx, XentNodeId node, XentSize size);
/** @brief Set one style axis without touching the other (NAN = auto). */
bool           xent_setw(XentCtx *ctx, XentNodeId node, float width);
bool           xent_seth(XentCtx *ctx, XentNodeId node, float height);
bool           xent_setwpct(XentCtx *ctx, XentNodeId node, float fraction);
bool           xent_sethpct(XentCtx *ctx, XentNodeId node, float fraction);
bool           xent_setsizepct(XentCtx *ctx, XentNodeId node, XentSize fraction);
bool           xent_setaspect(XentCtx *ctx, XentNodeId node, float aspect_ratio);
/**
 * @brief Size an axis to its children's extent (fit-content / wrap-content) when that
 * axis is otherwise auto. Opt-in: default is auto = fill available space. Currently
 * applies to FLEX containers; the wrapped axis should hold children whose size on that
 * axis is definite or text-derived (a child that itself fills the parent is circular).
 */
bool           xent_setfit(XentCtx *ctx, XentNodeId node, bool wrap_width, bool wrap_height);
bool           xent_setminsize(XentCtx *ctx, XentNodeId node, XentSize size);
bool           xent_setmaxsize(XentCtx *ctx, XentNodeId node, XentSize size);
bool           xent_setm(XentCtx *ctx, XentNodeId node, XentInsets margin);
bool           xent_setp(XentCtx *ctx, XentNodeId node, XentInsets padding);
bool           xent_setpos(XentCtx *ctx, XentNodeId node, XentPoint position);
bool           xent_setgap(XentCtx *ctx, XentNodeId node, float gap);
bool           xent_setz(XentCtx *ctx, XentNodeId node, int32_t z_index);
int32_t        xent_z(XentCtx const *ctx, XentNodeId node);

bool           xent_setgrow(XentCtx *ctx, XentNodeId node, float grow);
bool           xent_setshrink(XentCtx *ctx, XentNodeId node, float shrink);
bool           xent_setbasis(XentCtx *ctx, XentNodeId node, float basis);
bool           xent_setflexdir(XentCtx *ctx, XentNodeId node, XentFlexDirection direction);
bool           xent_setflexwrap(XentCtx *ctx, XentNodeId node, XentFlexWrap wrap);
bool           xent_setjustify(XentCtx *ctx, XentNodeId node, XentFlexJustify justify);
bool           xent_setitems(XentCtx *ctx, XentNodeId node, XentFlexAlign align_items);
bool           xent_setself(XentCtx *ctx, XentNodeId node, XentFlexAlign align_self);
bool           xent_setcontent(XentCtx *ctx, XentNodeId node, XentFlexAlignContent align_content);

bool           xent_stack_setaxis(XentCtx *ctx, XentNodeId node, XentAxis axis);
bool           xent_stack_setalign(XentCtx *ctx, XentNodeId node, XentStackAlign alignment);
XentStackAlign xent_stack_align(XentCtx const *ctx, XentNodeId node);
bool           xent_stack_setprio(XentCtx *ctx, XentNodeId node, float priority);
bool           xent_stack_setspacer(XentCtx *ctx, XentNodeId node, bool is_spacer);
bool           xent_setdir(XentCtx *ctx, XentNodeId node, XentDirection direction);
XentDirection  xent_dir(XentCtx const *ctx, XentNodeId node);
XentDirection  xent_resolved_dir(XentCtx const *ctx, XentNodeId node);
bool           xent_resolved_m(XentCtx const *ctx, XentNodeId node, XentAxis main_axis, XentResolvedInsets *out_insets);
bool           xent_resolved_p(XentCtx const *ctx, XentNodeId node, XentAxis main_axis, XentResolvedInsets *out_insets);

bool           xent_grid_setrows(
  XentCtx *ctx, XentNodeId node, XentGridSizeMode const *modes, float const *values, uint32_t count
);
bool xent_grid_setcols(
  XentCtx *ctx, XentNodeId node, XentGridSizeMode const *modes, float const *values, uint32_t count
);
bool               xent_grid_setrow(XentCtx *ctx, XentNodeId node, uint32_t row);
bool               xent_grid_setcol(XentCtx *ctx, XentNodeId node, uint32_t column);
bool               xent_grid_setrowspan(XentCtx *ctx, XentNodeId node, uint32_t span);
bool               xent_grid_setcolspan(XentCtx *ctx, XentNodeId node, uint32_t span);
bool               xent_grid_setrowgap(XentCtx *ctx, XentNodeId node, float gap);
bool               xent_grid_setcolgap(XentCtx *ctx, XentNodeId node, float gap);

uint32_t           xent_grid_row(XentCtx const *ctx, XentNodeId node);
uint32_t           xent_grid_col(XentCtx const *ctx, XentNodeId node);
uint32_t           xent_grid_rowspan(XentCtx const *ctx, XentNodeId node);
uint32_t           xent_grid_colspan(XentCtx const *ctx, XentNodeId node);

bool               xent_setscale(XentCtx *ctx, float point_scale_factor);
float              xent_scale(XentCtx const *ctx);
bool               xent_setrounding(XentCtx *ctx, bool enabled);
bool               xent_rounding(XentCtx const *ctx);

bool               xent_layout(XentCtx *ctx, XentNodeId root, float available_width, float available_height);
bool               xent_layout_rect(XentCtx const *ctx, XentNodeId node, XentRect *out_rect);
XentLayoutStrategy xent_layout_strategy(XentCtx const *ctx);


#endif
