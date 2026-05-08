#include "../xent_internal.h"

static bool xent_set_flex_float(XentContext *ctx, XentNodeId node, float *slot, float value, bool clamp_nonnegative) {
	if (!xent_is_valid_node(ctx, node)) return false;
	*slot = clamp_nonnegative && value < 0.0f ? 0.0f : value;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

static bool xent_set_flex_u8(XentContext *ctx, XentNodeId node, uint8_t *slot, uint8_t value) {
	if (!xent_is_valid_node(ctx, node)) return false;
	*slot = value;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_set_flex_grow(XentContext *ctx, XentNodeId node, float grow) {
	return xent_set_flex_float(ctx, node, &ctx->nodes.flex.grow [node], grow, true);
}

bool xent_set_flex_shrink(XentContext *ctx, XentNodeId node, float shrink) {
	return xent_set_flex_float(ctx, node, &ctx->nodes.flex.shrink [node], shrink, true);
}

bool xent_set_flex_basis(XentContext *ctx, XentNodeId node, float basis) {
	return xent_set_flex_float(ctx, node, &ctx->nodes.flex.basis [node], basis, false);
}

bool xent_set_flex_direction(XentContext *ctx, XentNodeId node, XentFlexDirection direction) {
	return xent_set_flex_u8(ctx, node, &ctx->nodes.flex.direction [node], ( uint8_t ) direction);
}

bool xent_set_flex_wrap(XentContext *ctx, XentNodeId node, XentFlexWrap wrap) {
	return xent_set_flex_u8(ctx, node, &ctx->nodes.flex.wrap [node], ( uint8_t ) wrap);
}

bool xent_set_flex_justify_content(XentContext *ctx, XentNodeId node, XentFlexJustify justify) {
	return xent_set_flex_u8(ctx, node, &ctx->nodes.flex.justify_content [node], ( uint8_t ) justify);
}

bool xent_set_flex_align_items(XentContext *ctx, XentNodeId node, XentFlexAlign align_items) {
	return xent_set_flex_u8(ctx, node, &ctx->nodes.flex.align_items [node], ( uint8_t ) align_items);
}

bool xent_set_flex_align_self(XentContext *ctx, XentNodeId node, XentFlexAlign align_self) {
	return xent_set_flex_u8(ctx, node, &ctx->nodes.flex.align_self [node], ( uint8_t ) align_self);
}

bool xent_set_flex_align_content(XentContext *ctx, XentNodeId node, XentFlexAlignContent align_content) {
	return xent_set_flex_u8(ctx, node, &ctx->nodes.flex.align_content [node], ( uint8_t ) align_content);
}
