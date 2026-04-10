#include "../xent_internal.h"

static void json_escape(FILE *out, const char *text) {
    if (!text) {
        return;
    }
    while (*text) {
        char c = *text++;
        if (c == '"' || c == '\\') {
            fputc('\\', out);
            fputc(c, out);
        } else if (c == '\n') {
            fputs("\\n", out);
        } else {
            fputc(c, out);
        }
    }
}

static bool dump_json_node(const XentContext *ctx, XentNodeId node, FILE *out) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }

    fprintf(out,
            "{\"id\":%u,\"protocol\":%u,\"rect\":{\"x\":%.3f,\"y\":%.3f,\"w\":%.3f,\"h\":%.3f}",
            node,
            (unsigned)ctx->nodes.protocol[node],
            ctx->nodes.abs_x[node],
            ctx->nodes.abs_y[node],
            ctx->nodes.decided_w[node],
            ctx->nodes.decided_h[node]);

    const char *label = xent_get_semantic_label(ctx, node);
    const char *text = xent_get_text(ctx, node);

    if (label) {
        fputs(",\"label\":\"", out);
        json_escape(out, label);
        fputc('"', out);
    }
    if (text) {
        fputs(",\"text\":\"", out);
        json_escape(out, text);
        fputc('"', out);
    }

    fputs(",\"children\":[", out);
    XentNodeId child = ctx->nodes.first_child[node];
    bool first = true;
    while (child != XENT_NODE_INVALID) {
        if (!first) {
            fputc(',', out);
        }
        if (!dump_json_node(ctx, child, out)) {
            return false;
        }
        first = false;
        child = ctx->nodes.next_sibling[child];
    }
    fputs("]}", out);
    return true;
}

bool xent_dump_layout_json(const XentContext *ctx, XentNodeId root, FILE *out) {
    if (!xent_is_valid_node(ctx, root) || !out) {
        return false;
    }
    return dump_json_node(ctx, root, out);
}
