#ifndef XENT_LAYOUT_H
#define XENT_LAYOUT_H

#include "xent_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

bool           xent_set_protocol(XentContext *ctx, XentNodeId node, XentProtocol protocol);

bool           xent_set_size(XentContext *ctx, XentNodeId node, XentSize size);
/** @brief Set one style axis without touching the other (NAN = auto). */
bool           xent_set_width(XentContext *ctx, XentNodeId node, float width);
bool           xent_set_height(XentContext *ctx, XentNodeId node, float height);
bool           xent_set_width_percent(XentContext *ctx, XentNodeId node, float fraction);
bool           xent_set_height_percent(XentContext *ctx, XentNodeId node, float fraction);
bool           xent_set_size_percent(XentContext *ctx, XentNodeId node, XentSize fraction);
bool           xent_set_aspect_ratio(XentContext *ctx, XentNodeId node, float aspect_ratio);
/**
 * @brief Size an axis to its children's extent (fit-content / wrap-content) when that
 * axis is otherwise auto. Opt-in: default is auto = fill available space. Currently
 * applies to FLEX containers; the wrapped axis should hold children whose size on that
 * axis is definite or text-derived (a child that itself fills the parent is circular).
 */
bool           xent_set_wrap_content(XentContext *ctx, XentNodeId node, bool wrap_width, bool wrap_height);
bool           xent_set_min_size(XentContext *ctx, XentNodeId node, XentSize size);
bool           xent_set_max_size(XentContext *ctx, XentNodeId node, XentSize size);
bool           xent_set_margin(XentContext *ctx, XentNodeId node, XentInsets margin);
bool           xent_set_padding(XentContext *ctx, XentNodeId node, XentInsets padding);
bool           xent_set_absolute_position(XentContext *ctx, XentNodeId node, XentPoint position);
bool           xent_set_gap(XentContext *ctx, XentNodeId node, float gap);
bool           xent_set_z_index(XentContext *ctx, XentNodeId node, int32_t z_index);
int32_t        xent_get_z_index(XentContext const *ctx, XentNodeId node);

bool           xent_set_flex_grow(XentContext *ctx, XentNodeId node, float grow);
bool           xent_set_flex_shrink(XentContext *ctx, XentNodeId node, float shrink);
bool           xent_set_flex_basis(XentContext *ctx, XentNodeId node, float basis);
bool           xent_set_flex_direction(XentContext *ctx, XentNodeId node, XentFlexDirection direction);
bool           xent_set_flex_wrap(XentContext *ctx, XentNodeId node, XentFlexWrap wrap);
bool           xent_set_flex_justify_content(XentContext *ctx, XentNodeId node, XentFlexJustify justify);
bool           xent_set_flex_align_items(XentContext *ctx, XentNodeId node, XentFlexAlign align_items);
bool           xent_set_flex_align_self(XentContext *ctx, XentNodeId node, XentFlexAlign align_self);
bool           xent_set_flex_align_content(XentContext *ctx, XentNodeId node, XentFlexAlignContent align_content);

bool           xent_set_stack_axis(XentContext *ctx, XentNodeId node, XentAxis axis);
bool           xent_set_stack_alignment(XentContext *ctx, XentNodeId node, XentStackAlign alignment);
XentStackAlign xent_get_stack_alignment(XentContext const *ctx, XentNodeId node);
bool           xent_set_layout_priority(XentContext *ctx, XentNodeId node, float priority);
bool           xent_set_is_spacer(XentContext *ctx, XentNodeId node, bool is_spacer);
bool           xent_set_direction(XentContext *ctx, XentNodeId node, XentDirection direction);
XentDirection  xent_get_direction(XentContext const *ctx, XentNodeId node);
XentDirection  xent_get_resolved_direction(XentContext const *ctx, XentNodeId node);
bool           xent_get_resolved_margin(
  XentContext const *ctx, XentNodeId node, XentAxis main_axis, XentResolvedInsets *out_insets
);
bool xent_get_resolved_padding(
  XentContext const *ctx, XentNodeId node, XentAxis main_axis, XentResolvedInsets *out_insets
);

bool xent_set_grid_rows(
  XentContext *ctx, XentNodeId node, XentGridSizeMode const *modes, float const *values, uint32_t count
);
bool xent_set_grid_columns(
  XentContext *ctx, XentNodeId node, XentGridSizeMode const *modes, float const *values, uint32_t count
);
bool               xent_set_grid_row(XentContext *ctx, XentNodeId node, uint32_t row);
bool               xent_set_grid_column(XentContext *ctx, XentNodeId node, uint32_t column);
bool               xent_set_grid_row_span(XentContext *ctx, XentNodeId node, uint32_t span);
bool               xent_set_grid_column_span(XentContext *ctx, XentNodeId node, uint32_t span);
bool               xent_set_grid_row_gap(XentContext *ctx, XentNodeId node, float gap);
bool               xent_set_grid_column_gap(XentContext *ctx, XentNodeId node, float gap);

uint32_t           xent_get_grid_row(XentContext const *ctx, XentNodeId node);
uint32_t           xent_get_grid_column(XentContext const *ctx, XentNodeId node);
uint32_t           xent_get_grid_row_span(XentContext const *ctx, XentNodeId node);
uint32_t           xent_get_grid_column_span(XentContext const *ctx, XentNodeId node);

bool               xent_set_point_scale_factor(XentContext *ctx, float point_scale_factor);
float              xent_get_point_scale_factor(XentContext const *ctx);
bool               xent_set_pixel_rounding_enabled(XentContext *ctx, bool enabled);
bool               xent_is_pixel_rounding_enabled(XentContext const *ctx);

bool               xent_layout(XentContext *ctx, XentNodeId root, float available_width, float available_height);
bool               xent_get_layout_rect(XentContext const *ctx, XentNodeId node, XentRect *out_rect);
XentLayoutStrategy xent_get_last_layout_strategy(XentContext const *ctx);
/** Traverse laid-out nodes with accumulated scroll, effective clip, and configurable child order.
 * Returning XENT_TRAVERSAL_SKIP_CHILDREN from enter still invokes leave for the skipped node.
 */

#ifdef __cplusplus
}
#endif

#endif
