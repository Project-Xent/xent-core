#include "../xent_internal.h"

bool xent_stack_setaxis(XentCtx *ctx, XentNodeId node, XentAxis axis) {
	if (!xent_node_valid(ctx, node)) return false;
	uint32_t             i   = xent_node_index(node);
	XentStackComp const *cur = xent_stack_comp_c(&ctx->nodes, i);
	if ((cur ? cur->axis : ( uint8_t ) XENT_AXIS_HORIZONTAL) == ( uint8_t ) axis) return true;
	XentStackComp *s = xent_stack_comp_ensure(&ctx->nodes, i);
	if (!s) return false;
	s->axis = ( uint8_t ) axis;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_stack_setalign(XentCtx *ctx, XentNodeId node, XentStackAlign alignment) {
	if (alignment != XENT_STACK_ALIGN_START && alignment != XENT_STACK_ALIGN_BASELINE) return false;
	if (!xent_node_valid(ctx, node)) return false;
	uint32_t             i   = xent_node_index(node);
	XentStackComp const *cur = xent_stack_comp_c(&ctx->nodes, i);
	if ((cur ? cur->align : ( uint8_t ) XENT_STACK_ALIGN_START) == ( uint8_t ) alignment) return true;
	XentStackComp *s = xent_stack_comp_ensure(&ctx->nodes, i);
	if (!s) return false;
	s->align = ( uint8_t ) alignment;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

XentStackAlign xent_stack_align(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return XENT_STACK_ALIGN_START;
	XentStackComp const *s = xent_stack_comp_c(&ctx->nodes, xent_node_index(node));
	return s ? ( XentStackAlign ) s->align : XENT_STACK_ALIGN_START;
}

bool xent_stack_setprio(XentCtx *ctx, XentNodeId node, float priority) {
	if (!xent_node_valid(ctx, node)) return false;
	uint32_t             i   = xent_node_index(node);
	XentStackComp const *cur = xent_stack_comp_c(&ctx->nodes, i);
	if (xent_f32_same(cur ? cur->priority : 0.0f, priority)) return true;
	XentStackComp *s = xent_stack_comp_ensure(&ctx->nodes, i);
	if (!s) return false;
	s->priority = priority;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}

float xent_node_priority(XentCtx const *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return 0.0f;
	XentStackComp const *s = xent_stack_comp_c(&ctx->nodes, xent_node_index(node));
	return s ? s->priority : 0.0f;
}

bool xent_stack_setspacer(XentCtx *ctx, XentNodeId node, bool is_spacer) {
	if (!xent_node_valid(ctx, node)) return false;
	uint32_t             i   = xent_node_index(node);
	uint8_t              val = is_spacer ? 1u : 0u;
	XentStackComp const *cur = xent_stack_comp_c(&ctx->nodes, i);
	if ((cur ? cur->spacer : 0u) == val) return true;
	XentStackComp *s = xent_stack_comp_ensure(&ctx->nodes, i);
	if (!s) return false;
	s->spacer = val;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}
