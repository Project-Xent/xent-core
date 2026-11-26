#include "../xent_internal.h"

bool xent_sem_setrole(XentCtx *ctx, XentNodeId node, XentSemRole role) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	XentSemComp const *cur = xent_sem_comp_c(&ctx->nodes, index);
	uint8_t            old = cur ? cur->role : ( uint8_t ) XENT_SEM_ROLE_NONE;
	if (old == ( uint8_t ) role) return true;
	XentSemComp *s = xent_sem_comp_ensure(&ctx->nodes, index);
	if (!s) return false;
	s->role = ( uint8_t ) role;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SEMANTICS);
	return true;
}

bool xent_sem_setlabel(XentCtx *ctx, XentNodeId node, char const *label) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	char const        *next = label ? label : "";
	XentSemComp const *cur  = xent_sem_comp_c(&ctx->nodes, index);
	if (cur && cur->label && strcmp(cur->label, next) == 0) return true;
	if (!cur && next [0] == '\0') return true;

	XentSemComp *s = xent_sem_comp_ensure(&ctx->nodes, index);
	if (!s) return false;
	char *copy = xent_strdup(XENT_ALLOC_SEMANTICS, next);
	if (!copy) return false;

	free(s->label);
	s->label = copy;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SEMANTICS);
	return true;
}

bool xent_sem_setflags(XentCtx *ctx, XentNodeId node, uint32_t flags) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	XentSemComp const *cur = xent_sem_comp_c(&ctx->nodes, index);
	if ((cur ? cur->flags : 0u) == flags) return true;
	XentSemComp *s = xent_sem_comp_ensure(&ctx->nodes, index);
	if (!s) return false;
	s->flags = flags;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SEMANTICS);
	return true;
}

XentSemRole xent_sem_role(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return XENT_SEM_ROLE_NONE;
	XentSemComp const *s = xent_sem_comp_c(&ctx->nodes, index);
	return s ? ( XentSemRole ) s->role : XENT_SEM_ROLE_NONE;
}

char const *xent_sem_label(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return NULL;
	XentSemComp const *s = xent_sem_comp_c(&ctx->nodes, index);
	return s ? s->label : NULL;
}

bool xent_sem_setchecked(XentCtx *ctx, XentNodeId node, uint8_t state) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	XentSemComp const *cur = xent_sem_comp_c(&ctx->nodes, index);
	if ((cur ? cur->checked : 0u) == state) return true;
	XentSemComp *s = xent_sem_comp_ensure(&ctx->nodes, index);
	if (!s) return false;
	s->checked = state;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SEMANTICS);
	return true;
}

bool xent_sem_setenabled(XentCtx *ctx, XentNodeId node, bool enabled) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	uint8_t            val = enabled ? 1u : 0u;
	XentSemComp const *cur = xent_sem_comp_c(&ctx->nodes, index);
	if ((cur ? cur->enabled : 1u) == val) return true;
	XentSemComp *s = xent_sem_comp_ensure(&ctx->nodes, index);
	if (!s) return false;
	s->enabled = val;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SEMANTICS);
	return true;
}

bool xent_sem_setexpanded(XentCtx *ctx, XentNodeId node, bool expanded) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	uint8_t            val = expanded ? 1u : 0u;
	XentSemComp const *cur = xent_sem_comp_c(&ctx->nodes, index);
	if ((cur ? cur->expanded : 0u) == val) return true;
	XentSemComp *s = xent_sem_comp_ensure(&ctx->nodes, index);
	if (!s) return false;
	s->expanded = val;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SEMANTICS);
	return true;
}

bool xent_sem_setselected(XentCtx *ctx, XentNodeId node, bool selected) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	uint8_t            val = selected ? 1u : 0u;
	XentSemComp const *cur = xent_sem_comp_c(&ctx->nodes, index);
	if ((cur ? cur->selected : 0u) == val) return true;
	XentSemComp *s = xent_sem_comp_ensure(&ctx->nodes, index);
	if (!s) return false;
	s->selected = val;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SEMANTICS);
	return true;
}

bool xent_sem_setvalue(XentCtx *ctx, XentNodeId node, float value, float min, float max) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	XentSemComp const *cur = xent_sem_comp_c(&ctx->nodes, index);
	if (cur
	    && xent_f32_same(cur->value_now, value)
	    && xent_f32_same(cur->value_min, min)
	    && xent_f32_same(cur->value_max, max))
		return true;
	if (!cur && value == 0.0f && min == 0.0f && max == 0.0f) return true;
	XentSemComp *s = xent_sem_comp_ensure(&ctx->nodes, index);
	if (!s) return false;
	s->value_now = value;
	s->value_min = min;
	s->value_max = max;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SEMANTICS);
	return true;
}

uint8_t xent_sem_checked(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return 0u;
	XentSemComp const *s = xent_sem_comp_c(&ctx->nodes, index);
	return s ? s->checked : 0u;
}

bool xent_sem_enabled(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	XentSemComp const *s = xent_sem_comp_c(&ctx->nodes, index);
	return s ? s->enabled != 0u : true;
}

bool xent_sem_expanded(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	XentSemComp const *s = xent_sem_comp_c(&ctx->nodes, index);
	return s ? s->expanded != 0u : false;
}

bool xent_sem_selected(XentCtx const *ctx, XentNodeId node) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	XentSemComp const *s = xent_sem_comp_c(&ctx->nodes, index);
	return s ? s->selected != 0u : false;
}

bool xent_sem_value(XentCtx const *ctx, XentNodeId node, float *out_value, float *out_min, float *out_max) {
	uint32_t index = xent_live_index(ctx, node);
	if (!index) return false;
	XentSemComp const *s = xent_sem_comp_c(&ctx->nodes, index);
	if (out_value) *out_value = s ? s->value_now : 0.0f;
	if (out_min) *out_min = s ? s->value_min : 0.0f;
	if (out_max) *out_max = s ? s->value_max : 0.0f;
	return true;
}
