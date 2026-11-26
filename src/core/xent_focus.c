#include "xent_focus.h"

#include "../xent_internal.h"

bool xent_set_focusable(XentCtx *ctx, XentNodeId node, bool focusable) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	uint8_t              val = focusable ? 1u : 0u;
	XentFocusComp const *cur = xent_focus_comp_c(&ctx->nodes, index);
	if ((cur ? cur->focusable : 0u) == val) return true;
	XentFocusComp *f = xent_focus_comp_ensure(&ctx->nodes, index);
	if (!f) return false;
	f->focusable = val;
	return true;
}

bool xent_get_focusable(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	XentFocusComp const *f = xent_focus_comp_c(&ctx->nodes, index);
	return f ? f->focusable != 0u : false;
}

bool xent_set_tabindex(XentCtx *ctx, XentNodeId node, int32_t tab_index) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	XentFocusComp const *cur = xent_focus_comp_c(&ctx->nodes, index);
	if ((cur ? cur->tab_index : 0) == tab_index) return true;
	XentFocusComp *f = xent_focus_comp_ensure(&ctx->nodes, index);
	if (!f) return false;
	f->tab_index = tab_index;
	return true;
}

int32_t xent_get_tabindex(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return 0;
	XentFocusComp const *f = xent_focus_comp_c(&ctx->nodes, index);
	return f ? f->tab_index : 0;
}
