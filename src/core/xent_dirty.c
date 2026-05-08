#include "../xent_internal.h"

#if XENT_ISPC_ENABLED
  #include "xent_ispc_kernels_ispc.h"
#endif

void xent_mark_dirty(XentContext *ctx, XentNodeId node, uint32_t flags) {
	if (!xent_is_valid_node(ctx, node)) return;

	ctx->nodes.layout.dirty_flags [node] |= (flags | XENT_DIRTY_SELF);

	XentNodeId parent                     = ctx->nodes.topology.parent [node];
	while (parent != XENT_NODE_INVALID) {
		ctx->nodes.layout.dirty_flags [parent] |= XENT_DIRTY_SUBTREE;
		parent                                  = ctx->nodes.topology.parent [parent];
	}
}

uint32_t xent_get_dirty_flags(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return XENT_DIRTY_NONE;
	return ctx->nodes.layout.dirty_flags [node];
}

void xent_clear_dirty_in_work_order(XentContext *ctx) {
#if XENT_ISPC_ENABLED
	if (ctx->work_count >= 64u) {
		xent_ispc_scatter_zero_u32(ctx->nodes.layout.dirty_flags, ctx->work_order, ctx->work_count);
		return;
	}
#endif
	for (uint32_t i = 0; i < ctx->work_count; ++i) {
		XentNodeId node = ctx->work_order [i];
		if (xent_is_valid_node(ctx, node)) ctx->nodes.layout.dirty_flags [node] = XENT_DIRTY_NONE;
	}
}
