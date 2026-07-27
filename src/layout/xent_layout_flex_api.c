#include "../xent_internal.h"

static bool set_flex_float(XentCtx *ctx, XentNodeId node, float *slot, float value, bool clamp_nonnegative) {
	if (!xent_node_valid(ctx, node)) return false;
	*slot = clamp_nonnegative && value < 0.0f ? 0.0f : value;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

static bool set_flex_u8(XentCtx *ctx, XentNodeId node, uint8_t *slot, uint8_t value) {
	if (!xent_node_valid(ctx, node)) return false;
	*slot = value;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_setgrow(XentCtx *ctx, XentNodeId node, float grow) {
	return set_flex_float(ctx, node, &ctx->nodes.flex.grow [xent_node_index(node)], grow, true);
}

bool xent_setshrink(XentCtx *ctx, XentNodeId node, float shrink) {
	return set_flex_float(ctx, node, &ctx->nodes.flex.shrink [xent_node_index(node)], shrink, true);
}

bool xent_setbasis(XentCtx *ctx, XentNodeId node, float basis) {
	return set_flex_float(ctx, node, &ctx->nodes.flex.basis [xent_node_index(node)], basis, false);
}

bool xent_setflexdir(XentCtx *ctx, XentNodeId node, XentFlexDirection direction) {
	return set_flex_u8(ctx, node, &ctx->nodes.flex.direction [xent_node_index(node)], ( uint8_t ) direction);
}

bool xent_setflexwrap(XentCtx *ctx, XentNodeId node, XentFlexWrap wrap) {
	return set_flex_u8(ctx, node, &ctx->nodes.flex.wrap [xent_node_index(node)], ( uint8_t ) wrap);
}

bool xent_setjustify(XentCtx *ctx, XentNodeId node, XentFlexJustify justify) {
	return set_flex_u8(ctx, node, &ctx->nodes.flex.justify_content [xent_node_index(node)], ( uint8_t ) justify);
}

bool xent_setitems(XentCtx *ctx, XentNodeId node, XentFlexAlign align_items) {
	return set_flex_u8(ctx, node, &ctx->nodes.flex.align_items [xent_node_index(node)], ( uint8_t ) align_items);
}

bool xent_setself(XentCtx *ctx, XentNodeId node, XentFlexAlign align_self) {
	return set_flex_u8(ctx, node, &ctx->nodes.flex.align_self [xent_node_index(node)], ( uint8_t ) align_self);
}

bool xent_setcontent(XentCtx *ctx, XentNodeId node, XentFlexAlignContent align_content) {
	return set_flex_u8(ctx, node, &ctx->nodes.flex.align_content [xent_node_index(node)], ( uint8_t ) align_content);
}
