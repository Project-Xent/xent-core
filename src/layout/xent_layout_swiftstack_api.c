#include "../xent_internal.h"

static bool set_stack_u8(XentCtx *ctx, XentNodeId node, uint8_t *slot, uint8_t value) {
	if (!xent_node_valid(ctx, node)) return false;
	*slot = value;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_stack_setaxis(XentCtx *ctx, XentNodeId node, XentAxis axis) {
	return set_stack_u8(ctx, node, &ctx->nodes.stack.axis [xent_node_index(node)], ( uint8_t ) axis);
}

bool xent_stack_setalign(XentCtx *ctx, XentNodeId node, XentStackAlign alignment) {
	if (alignment != XENT_STACK_ALIGN_START && alignment != XENT_STACK_ALIGN_BASELINE) return false;
	return set_stack_u8(ctx, node, &ctx->nodes.stack.align [xent_node_index(node)], ( uint8_t ) alignment);
}

XentStackAlign xent_stack_align(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return XENT_STACK_ALIGN_START;
	return ( XentStackAlign ) ctx->nodes.stack.align [xent_node_index(node)];
}

bool xent_stack_setprio(XentCtx *ctx, XentNodeId node, float priority) {
	if (!xent_node_valid(ctx, node)) return false;
	ctx->nodes.stack.priority [xent_node_index(node)] = priority;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

float xent_node_priority(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return 0.0f;
	return ctx->nodes.stack.priority [xent_node_index(node)];
}

bool xent_stack_setspacer(XentCtx *ctx, XentNodeId node, bool is_spacer) {
	return set_stack_u8(ctx, node, &ctx->nodes.stack.spacer [xent_node_index(node)], is_spacer ? 1u : 0u);
}
