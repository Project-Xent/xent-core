#include "../xent_internal.h"

bool xent_set_absolute_position(XentContext *ctx, XentNodeId node, XentPoint position) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.layout.abs_pos_x [node] = position.x;
	ctx->nodes.layout.abs_pos_y [node] = position.y;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT);
	return true;
}
