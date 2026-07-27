#include "xent_focus.h"

#include "../xent_internal.h"

bool xent_set_focusable(XentCtx *ctx, XentNodeId node, bool focusable) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	ctx->nodes.focus.focusable [index] = focusable ? 1 : 0;
	return true;
}

bool xent_get_focusable(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	return ctx->nodes.focus.focusable [index] != 0;
}

bool xent_set_tab_index(XentCtx *ctx, XentNodeId node, int32_t tab_index) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	ctx->nodes.focus.tab_index [index] = tab_index;
	return true;
}

int32_t xent_get_tab_index(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return 0;
	return ctx->nodes.focus.tab_index [index];
}
