#include "../xent_internal.h"

bool xent_set_semantic_role(XentContext *ctx, XentNodeId node, XentSemanticRole role) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.semantic_role[node] = (uint8_t)role;
    xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
    return true;
}

bool xent_set_semantic_label(XentContext *ctx, XentNodeId node, const char *label) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }

    char *copy = xent_strdup(label ? label : "");
    if (!copy) {
        return false;
    }

    free(ctx->nodes.semantic_label[node]);
    ctx->nodes.semantic_label[node] = copy;
    xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
    return true;
}

bool xent_set_semantic_flags(XentContext *ctx, XentNodeId node, uint32_t flags) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.semantic_flags[node] = flags;
    xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
    return true;
}

XentSemanticRole xent_get_semantic_role(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return XENT_SEMANTIC_NONE;
    }
    return (XentSemanticRole)ctx->nodes.semantic_role[node];
}

const char *xent_get_semantic_label(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return NULL;
    }
    return ctx->nodes.semantic_label[node];
}

bool xent_set_userdata(XentContext *ctx, XentNodeId node, void *data) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.userdata[node] = data;
    return true;
}

void *xent_get_userdata(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return NULL;
    }
    return ctx->nodes.userdata[node];
}

bool xent_set_control_type(XentContext *ctx, XentNodeId node, XentControlType type) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.control_type[node] = (uint8_t)type;
    return true;
}

XentControlType xent_get_control_type(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return XENT_CONTROL_CONTAINER;
    }
    return (XentControlType)ctx->nodes.control_type[node];
}

bool xent_set_semantic_checked(XentContext *ctx, XentNodeId node, uint8_t state) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.semantic_checked[node] = state;
    xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
    return true;
}

bool xent_set_semantic_enabled(XentContext *ctx, XentNodeId node, bool enabled) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.semantic_enabled[node] = enabled ? 1u : 0u;
    xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
    return true;
}

bool xent_set_semantic_expanded(XentContext *ctx, XentNodeId node, bool expanded) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.semantic_expanded[node] = expanded ? 1u : 0u;
    xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
    return true;
}

bool xent_set_semantic_selected(XentContext *ctx, XentNodeId node, bool selected) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.semantic_selected[node] = selected ? 1u : 0u;
    xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
    return true;
}

bool xent_set_semantic_value(XentContext *ctx, XentNodeId node,
                             float value, float min, float max) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    ctx->nodes.semantic_value_now[node] = value;
    ctx->nodes.semantic_value_min[node] = min;
    ctx->nodes.semantic_value_max[node] = max;
    xent_mark_dirty(ctx, node, XENT_DIRTY_SELF);
    return true;
}

uint8_t xent_get_semantic_checked(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return 0u;
    }
    return ctx->nodes.semantic_checked[node];
}

bool xent_get_semantic_enabled(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    return ctx->nodes.semantic_enabled[node] != 0u;
}

bool xent_get_semantic_expanded(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    return ctx->nodes.semantic_expanded[node] != 0u;
}

bool xent_get_semantic_selected(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    return ctx->nodes.semantic_selected[node] != 0u;
}

bool xent_get_semantic_value(const XentContext *ctx, XentNodeId node,
                             float *out_value, float *out_min, float *out_max) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }
    if (out_value) *out_value = ctx->nodes.semantic_value_now[node];
    if (out_min)   *out_min   = ctx->nodes.semantic_value_min[node];
    if (out_max)   *out_max   = ctx->nodes.semantic_value_max[node];
    return true;
}
