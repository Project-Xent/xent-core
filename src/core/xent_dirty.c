#include "../xent_internal.h"

void xent_mark_dirty(XentContext *ctx, XentNodeId node, uint32_t flags) {
    if (!xent_is_valid_node(ctx, node)) {
        return;
    }

    ctx->nodes.dirty_flags[node] |= (flags | XENT_DIRTY_SELF);

    XentNodeId parent = ctx->nodes.parent[node];
    while (parent != XENT_NODE_INVALID) {
        ctx->nodes.dirty_flags[parent] |= XENT_DIRTY_SUBTREE;
        parent = ctx->nodes.parent[parent];
    }
}

uint32_t xent_get_dirty_flags(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return XENT_DIRTY_NONE;
    }
    return ctx->nodes.dirty_flags[node];
}

void xent_clear_dirty_in_work_order(XentContext *ctx) {
    for (uint32_t i = 0; i < ctx->work_count; ++i) {
        XentNodeId node = ctx->work_order[i];
        if (xent_is_valid_node(ctx, node)) {
            ctx->nodes.dirty_flags[node] = XENT_DIRTY_NONE;
        }
    }
}
