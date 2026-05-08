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

bool xent_set_node_payload(
  XentContext *ctx, XentNodeId node, uint32_t payload_type, void *payload, XentNodePayloadDestroyFn destroy,
  void *destroy_userdata
) {
	if (!xent_is_valid_node(ctx, node)) return false;
	if (ctx->nodes.external.payload_destroy [node]
		&& ctx->nodes.external.payload [node]
		&& ctx->nodes.external.payload [node] != payload)
		ctx->nodes.external.payload_destroy [node](
		  ctx->nodes.external.payload [node], ctx->nodes.external.payload_destroy_userdata [node]
		);
	ctx->nodes.external.payload [node]                  = payload;
	ctx->nodes.external.payload_type [node]             = payload ? payload_type : 0u;
	ctx->nodes.external.payload_destroy [node]          = destroy;
	ctx->nodes.external.payload_destroy_userdata [node] = destroy_userdata;
	ctx->nodes.external.userdata [node]                 = payload;
	return true;
}

bool xent_clear_node_payload(XentContext *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return false;
	if (ctx->nodes.external.payload_destroy [node] && ctx->nodes.external.payload [node])
		ctx->nodes.external.payload_destroy [node](
		  ctx->nodes.external.payload [node], ctx->nodes.external.payload_destroy_userdata [node]
		);
	ctx->nodes.external.payload [node]                  = NULL;
	ctx->nodes.external.payload_type [node]             = 0u;
	ctx->nodes.external.payload_destroy [node]          = NULL;
	ctx->nodes.external.payload_destroy_userdata [node] = NULL;
	ctx->nodes.external.userdata [node]                 = NULL;
	return true;
}

void *xent_get_node_payload(XentContext const *ctx, XentNodeId node, uint32_t expected_payload_type) {
	if (!xent_is_valid_node(ctx, node)) return NULL;
	if (expected_payload_type && ctx->nodes.external.payload_type [node] != expected_payload_type) return NULL;
	return ctx->nodes.external.payload [node];
}

uint32_t xent_get_node_payload_type(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return 0u;
	return ctx->nodes.external.payload_type [node];
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

bool xent_set_control_type(XentContext *ctx, XentNodeId node, XentControlType type) {
	if (!xent_is_valid_node(ctx, node)) return false;
	ctx->nodes.external.control_type [node] = ( uint8_t ) type;
	return true;
}

XentControlType xent_get_control_type(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return XENT_CONTROL_CONTAINER;
	return ( XentControlType ) ctx->nodes.external.control_type [node];
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
