#include "../xent_internal.h"

bool xent_set_semantic_role(XentContext *ctx, XentNodeId node, XentSemanticRole role) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.semantics.role [node] = ( uint8_t ) role;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
	return true;
}

bool xent_set_semantic_label(XentContext *ctx, XentNodeId node, char const *label) {
	if (!xent_is_valid_node(ctx, node)) return false;

	char *copy = xent_strdup(label ? label : "");
	if (!copy) return false;

	free(ctx->nodes.semantics.label [node]);
	ctx->nodes.semantics.label [node] = copy;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
	return true;
}

bool xent_set_semantic_flags(XentContext *ctx, XentNodeId node, uint32_t flags) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.semantics.flags [node] = flags;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
	return true;
}

XentSemanticRole xent_get_semantic_role(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return XENT_SEMANTIC_NONE;
	return ( XentSemanticRole ) ctx->nodes.semantics.role [node];
}

char const *xent_get_semantic_label(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return NULL;
	return ctx->nodes.semantics.label [node];
}

bool xent_set_node_lifecycle_callback(XentContext *ctx, XentNodeLifecycleFn callback, void *userdata) {
	if (!ctx) return false;
	ctx->node_lifecycle          = callback;
	ctx->node_lifecycle_userdata = userdata;
	return true;
}





bool xent_set_userdata(XentContext *ctx, XentNodeId node, void *data) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.external.userdata [node] = data;
	return true;
}

void *xent_get_userdata(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return NULL;
	return ctx->nodes.external.userdata [node];
}

bool xent_set_node_tag(XentContext *ctx, XentNodeId node, uint8_t tag) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.external.tag [node] = tag;
	return true;
}

uint8_t xent_get_node_tag(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return 0u;
	return ctx->nodes.external.tag [node];
}

bool xent_set_semantic_checked(XentContext *ctx, XentNodeId node, uint8_t state) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.semantics.checked [node] = state;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
	return true;
}

bool xent_set_semantic_enabled(XentContext *ctx, XentNodeId node, bool enabled) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.semantics.enabled [node] = enabled ? 1u : 0u;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
	return true;
}

bool xent_set_semantic_expanded(XentContext *ctx, XentNodeId node, bool expanded) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.semantics.expanded [node] = expanded ? 1u : 0u;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
	return true;
}

bool xent_set_semantic_selected(XentContext *ctx, XentNodeId node, bool selected) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.semantics.selected [node] = selected ? 1u : 0u;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
	return true;
}

bool xent_set_semantic_value(XentContext *ctx, XentNodeId node, float value, float min, float max) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.semantics.value_now [node] = value;
	ctx->nodes.semantics.value_min [node] = min;
	ctx->nodes.semantics.value_max [node] = max;
	xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
	return true;
}

uint8_t xent_get_semantic_checked(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return 0u;
	return ctx->nodes.semantics.checked [node];
}

bool xent_get_semantic_enabled(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return false;
	return ctx->nodes.semantics.enabled [node] != 0u;
}

bool xent_get_semantic_expanded(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return false;
	return ctx->nodes.semantics.expanded [node] != 0u;
}

bool xent_get_semantic_selected(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return false;
	return ctx->nodes.semantics.selected [node] != 0u;
}

bool xent_get_semantic_value(
  XentContext const *ctx, XentNodeId node, float *out_value, float *out_min, float *out_max
) {
	if (!xent_is_valid_node(ctx, node)) return false;
	if (out_value) *out_value = ctx->nodes.semantics.value_now [node];
	if (out_min) *out_min = ctx->nodes.semantics.value_min [node];
	if (out_max) *out_max = ctx->nodes.semantics.value_max [node];
	return true;
}
