#include "../xent_internal.h"

void xent_layout_node_absolute(XentContext *ctx,
                               XentNodeId node,
                               float available_w,
                               float available_h,
                               float origin_x,
                               float origin_y) {
    float width = 0.0f;
    float height = 0.0f;
    xent_compute_intrinsic_size(ctx, node, available_w, available_h, &width, &height);

    const float x = origin_x + ctx->nodes.abs_pos_x[node];
    const float y = origin_y + ctx->nodes.abs_pos_y[node];

    ctx->nodes.proposed_w[node] = available_w;
    ctx->nodes.proposed_h[node] = available_h;
    ctx->nodes.decided_w[node] = width;
    ctx->nodes.decided_h[node] = height;
    ctx->nodes.abs_x[node] = x;
    ctx->nodes.abs_y[node] = y;
    xent_quantize_node_layout(ctx, node);

    width = ctx->nodes.decided_w[node];
    height = ctx->nodes.decided_h[node];
    const float qx = ctx->nodes.abs_x[node];
    const float qy = ctx->nodes.abs_y[node];

    float content_w = width - (ctx->nodes.padding_l[node] + ctx->nodes.padding_r[node]);
    float content_h = height - (ctx->nodes.padding_t[node] + ctx->nodes.padding_b[node]);
    if (content_w < 0.0f) {
        content_w = 0.0f;
    }
    if (content_h < 0.0f) {
        content_h = 0.0f;
    }

    float content_x = qx + ctx->nodes.padding_l[node];
    float content_y = qy + ctx->nodes.padding_t[node];

    XentNodeId child = ctx->nodes.first_child[node];
    while (child != XENT_NODE_INVALID) {
        xent_layout_dispatch_node(ctx, child, content_w, content_h, content_x, content_y);
        child = ctx->nodes.next_sibling[child];
    }
}
