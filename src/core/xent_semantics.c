#include "../xent_internal.h"

bool xent_sem_setrole(XentCtx *ctx, XentNodeId node, XentSemRole role) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	ctx->nodes.semantics.role [index] = ( uint8_t ) role;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
	return true;
}

bool xent_sem_setlabel(XentCtx *ctx, XentNodeId node, char const *label) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;

	char *copy = xent_strdup(label ? label : "");
	if (!copy) return false;

	free(ctx->nodes.semantics.label [index]);
	ctx->nodes.semantics.label [index] = copy;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
	return true;
}

bool xent_sem_setflags(XentCtx *ctx, XentNodeId node, uint32_t flags) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	ctx->nodes.semantics.flags [index] = flags;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
	return true;
}

XentSemRole xent_sem_role(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return XENT_SEM_ROLE_NONE;
	return ( XentSemRole ) ctx->nodes.semantics.role [index];
}

char const *xent_sem_label(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return NULL;
	return ctx->nodes.semantics.label [index];
}

bool xent_sem_setchecked(XentCtx *ctx, XentNodeId node, uint8_t state) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	ctx->nodes.semantics.checked [index] = state;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
	return true;
}

bool xent_sem_setenabled(XentCtx *ctx, XentNodeId node, bool enabled) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	ctx->nodes.semantics.enabled [index] = enabled ? 1u : 0u;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
	return true;
}

bool xent_sem_setexpanded(XentCtx *ctx, XentNodeId node, bool expanded) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	ctx->nodes.semantics.expanded [index] = expanded ? 1u : 0u;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
	return true;
}

bool xent_sem_setselected(XentCtx *ctx, XentNodeId node, bool selected) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	ctx->nodes.semantics.selected [index] = selected ? 1u : 0u;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
	return true;
}

bool xent_sem_setvalue(XentCtx *ctx, XentNodeId node, float value, float min, float max) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	ctx->nodes.semantics.value_now [index] = value;
	ctx->nodes.semantics.value_min [index] = min;
	ctx->nodes.semantics.value_max [index] = max;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
	return true;
}

uint8_t xent_sem_checked(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return 0u;
	return ctx->nodes.semantics.checked [index];
}

bool xent_sem_enabled(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	return ctx->nodes.semantics.enabled [index] != 0u;
}

bool xent_sem_expanded(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	return ctx->nodes.semantics.expanded [index] != 0u;
}

bool xent_sem_selected(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	return ctx->nodes.semantics.selected [index] != 0u;
}

bool xent_sem_value(XentCtx const *ctx, XentNodeId node, float *out_value, float *out_min, float *out_max) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	if (out_value) *out_value = ctx->nodes.semantics.value_now [index];
	if (out_min) *out_min = ctx->nodes.semantics.value_min [index];
	if (out_max) *out_max = ctx->nodes.semantics.value_max [index];
	return true;
}
