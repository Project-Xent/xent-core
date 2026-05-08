#include "../xent_internal.h"

static bool xent_set_stack_u8(XentContext *ctx, XentNodeId node, uint8_t *slot, uint8_t value) {
	if (!xent_is_valid_node(ctx, node)) return false;
	*slot = value;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_set_stack_axis(XentContext *ctx, XentNodeId node, XentAxis axis) {
	return xent_set_stack_u8(ctx, node, &ctx->nodes.stack.axis [node], ( uint8_t ) axis);
}

bool xent_set_stack_alignment(XentContext *ctx, XentNodeId node, XentStackAlign alignment) {
	if (alignment != XENT_STACK_ALIGN_START && alignment != XENT_STACK_ALIGN_BASELINE) return false;
	return xent_set_stack_u8(ctx, node, &ctx->nodes.stack.align [node], ( uint8_t ) alignment);
}

XentStackAlign xent_get_stack_alignment(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return XENT_STACK_ALIGN_START;
	return ( XentStackAlign ) ctx->nodes.stack.align [node];
}

bool xent_set_layout_priority(XentContext *ctx, XentNodeId node, float priority) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.stack.priority [node] = priority;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

float xent_get_layout_priority(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return 0.0f;
	return ctx->nodes.stack.priority [node];
}

bool xent_set_is_spacer(XentContext *ctx, XentNodeId node, bool is_spacer) {
	return xent_set_stack_u8(ctx, node, &ctx->nodes.stack.spacer [node], is_spacer ? 1u : 0u);
}
