#include "../xent_internal.h"

bool xent_is_valid_node(const XentContext *ctx, XentNodeId node) {
    if (!ctx || node == XENT_NODE_INVALID || node >= ctx->nodes.capacity) {
        return false;
    }
    return ctx->nodes.alive[node] != 0u;
}

static bool xent_push_free_id(XentContext *ctx, XentNodeId node) {
    if (ctx->free_count == ctx->free_capacity) {
        uint32_t new_cap = ctx->free_capacity ? ctx->free_capacity * 2u : 64u;
        XentNodeId *new_mem = (XentNodeId *)realloc(ctx->free_ids, sizeof(XentNodeId) * (size_t)new_cap);
        if (!new_mem) {
            return false;
        }
        ctx->free_ids = new_mem;
        ctx->free_capacity = new_cap;
    }
    ctx->free_ids[ctx->free_count++] = node;
    return true;
}

XentNodeId xent_create_node(XentContext *ctx) {
    if (!ctx) {
        return XENT_NODE_INVALID;
    }

    XentNodeId id = XENT_NODE_INVALID;
    if (ctx->free_count > 0u) {
        id = ctx->free_ids[--ctx->free_count];
    } else {
        id = ++ctx->nodes.count;
        if (!xent_ensure_node_capacity(ctx, id + 1u)) {
            ctx->nodes.count--;
            return XENT_NODE_INVALID;
        }
    }

    ctx->nodes.alive[id] = 1u;
    ctx->nodes.parent[id] = XENT_NODE_INVALID;
    ctx->nodes.first_child[id] = XENT_NODE_INVALID;
    ctx->nodes.next_sibling[id] = XENT_NODE_INVALID;
    ctx->nodes.child_count[id] = 0u;

    ctx->nodes.protocol[id] = (uint8_t)XENT_PROTOCOL_ABSOLUTE;
    ctx->nodes.direction[id] = (uint8_t)XENT_DIRECTION_INHERIT;
    ctx->nodes.dirty_flags[id] = XENT_DIRTY_SELF | XENT_DIRTY_SUBTREE | XENT_DIRTY_LAYOUT;

    ctx->nodes.style_w[id] = NAN;
    ctx->nodes.style_h[id] = NAN;
    ctx->nodes.min_w[id] = 0.0f;
    ctx->nodes.min_h[id] = 0.0f;
    ctx->nodes.max_w[id] = INFINITY;
    ctx->nodes.max_h[id] = INFINITY;

    ctx->nodes.flex_grow[id] = 0.0f;
    ctx->nodes.flex_shrink[id] = 1.0f;
    ctx->nodes.flex_basis[id] = NAN;
    ctx->nodes.flex_direction[id] = (uint8_t)XENT_FLEX_ROW;
    ctx->nodes.flex_wrap[id] = (uint8_t)XENT_FLEX_NO_WRAP;
    ctx->nodes.flex_justify_content[id] = (uint8_t)XENT_FLEX_JUSTIFY_START;
    ctx->nodes.flex_align_items[id] = (uint8_t)XENT_FLEX_ALIGN_START;
    ctx->nodes.flex_align_self[id] = (uint8_t)XENT_FLEX_ALIGN_AUTO;
    ctx->nodes.flex_align_content[id] = (uint8_t)XENT_FLEX_ALIGN_CONTENT_START;

    ctx->nodes.stack_axis[id] = (uint8_t)XENT_AXIS_HORIZONTAL;
    ctx->nodes.stack_align[id] = (uint8_t)XENT_STACK_ALIGN_START;
    ctx->nodes.layout_priority[id] = 0.0f;
    ctx->nodes.is_spacer[id] = 0u;

    ctx->nodes.abs_pos_x[id] = 0.0f;
    ctx->nodes.abs_pos_y[id] = 0.0f;
    ctx->nodes.gap[id] = 0.0f;

    if (ctx->nodes.text[id]) {
        free(ctx->nodes.text[id]);
        ctx->nodes.text[id] = NULL;
    }
    ctx->nodes.font_size[id] = 14.0f;
    ctx->nodes.text_line_break_policy[id] = (uint8_t)XENT_LINE_BREAK_CHAR_WRAP;
    ctx->nodes.text_intrinsic_valid[id] = 0u;
    ctx->nodes.text_intrinsic_constraint_w[id] = NAN;
    ctx->nodes.text_intrinsic_font_size[id] = 0.0f;
    ctx->nodes.text_intrinsic_line_break_policy[id] = (uint8_t)XENT_LINE_BREAK_CHAR_WRAP;
    ctx->nodes.text_intrinsic_width_mode[id] = (uint8_t)XENT_MEASURE_UNDEFINED;
    ctx->nodes.text_intrinsic_w[id] = 0.0f;
    ctx->nodes.text_intrinsic_h[id] = 0.0f;
    ctx->nodes.text_intrinsic_lines[id] = 0u;

    ctx->nodes.semantic_role[id] = (uint8_t)XENT_SEMANTIC_NONE;
    if (ctx->nodes.semantic_label[id]) {
        free(ctx->nodes.semantic_label[id]);
        ctx->nodes.semantic_label[id] = NULL;
    }
    ctx->nodes.semantic_flags[id] = 0u;

    ctx->nodes.userdata[id] = NULL;
    ctx->nodes.control_type[id] = (uint8_t)XENT_CONTROL_CONTAINER;
    ctx->nodes.semantic_checked[id] = 0u;
    ctx->nodes.semantic_enabled[id] = 1u;
    ctx->nodes.semantic_expanded[id] = 0u;
    ctx->nodes.semantic_selected[id] = 0u;
    ctx->nodes.semantic_value_now[id] = 0.0f;
    ctx->nodes.semantic_value_min[id] = 0.0f;
    ctx->nodes.semantic_value_max[id] = 0.0f;

    return id;
}

static void xent_destroy_subtree(XentContext *ctx, XentNodeId node) {
    XentNodeId child = ctx->nodes.first_child[node];
    while (child != XENT_NODE_INVALID) {
        XentNodeId next = ctx->nodes.next_sibling[child];
        xent_destroy_subtree(ctx, child);
        child = next;
    }

    free(ctx->nodes.text[node]);
    ctx->nodes.text[node] = NULL;
    ctx->nodes.text_intrinsic_valid[node] = 0u;
    free(ctx->nodes.semantic_label[node]);
    ctx->nodes.semantic_label[node] = NULL;
    ctx->nodes.userdata[node] = NULL;

    ctx->nodes.alive[node] = 0u;
    ctx->nodes.parent[node] = XENT_NODE_INVALID;
    ctx->nodes.first_child[node] = XENT_NODE_INVALID;
    ctx->nodes.next_sibling[node] = XENT_NODE_INVALID;
    ctx->nodes.child_count[node] = 0u;
    ctx->nodes.dirty_flags[node] = XENT_DIRTY_NONE;
    (void)xent_push_free_id(ctx, node);
}

bool xent_destroy_node(XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return false;
    }

    XentNodeId parent = ctx->nodes.parent[node];
    if (parent != XENT_NODE_INVALID) {
        (void)xent_remove_child(ctx, parent, node);
    }

    xent_destroy_subtree(ctx, node);
    return true;
}

bool xent_append_child(XentContext *ctx, XentNodeId parent, XentNodeId child) {
    if (!xent_is_valid_node(ctx, parent) || !xent_is_valid_node(ctx, child) || parent == child) {
        return false;
    }

    XentNodeId cursor = parent;
    while (cursor != XENT_NODE_INVALID) {
        if (cursor == child) {
            return false;
        }
        cursor = ctx->nodes.parent[cursor];
    }

    XentNodeId old_parent = ctx->nodes.parent[child];
    if (old_parent != XENT_NODE_INVALID) {
        (void)xent_remove_child(ctx, old_parent, child);
    }

    if (ctx->nodes.first_child[parent] == XENT_NODE_INVALID) {
        ctx->nodes.first_child[parent] = child;
    } else {
        XentNodeId tail = ctx->nodes.first_child[parent];
        while (ctx->nodes.next_sibling[tail] != XENT_NODE_INVALID) {
            tail = ctx->nodes.next_sibling[tail];
        }
        ctx->nodes.next_sibling[tail] = child;
    }

    ctx->nodes.parent[child] = parent;
    ctx->nodes.next_sibling[child] = XENT_NODE_INVALID;
    ctx->nodes.child_count[parent] += 1u;

    xent_mark_dirty(ctx, parent, XENT_DIRTY_SUBTREE | XENT_DIRTY_LAYOUT);
    return true;
}

bool xent_remove_child(XentContext *ctx, XentNodeId parent, XentNodeId child) {
    if (!xent_is_valid_node(ctx, parent) || !xent_is_valid_node(ctx, child)) {
        return false;
    }

    XentNodeId prev = XENT_NODE_INVALID;
    XentNodeId cur = ctx->nodes.first_child[parent];
    while (cur != XENT_NODE_INVALID) {
        if (cur == child) {
            break;
        }
        prev = cur;
        cur = ctx->nodes.next_sibling[cur];
    }

    if (cur == XENT_NODE_INVALID) {
        return false;
    }

    if (prev == XENT_NODE_INVALID) {
        ctx->nodes.first_child[parent] = ctx->nodes.next_sibling[cur];
    } else {
        ctx->nodes.next_sibling[prev] = ctx->nodes.next_sibling[cur];
    }

    ctx->nodes.next_sibling[cur] = XENT_NODE_INVALID;
    ctx->nodes.parent[cur] = XENT_NODE_INVALID;
    if (ctx->nodes.child_count[parent] > 0u) {
        ctx->nodes.child_count[parent] -= 1u;
    }

    xent_mark_dirty(ctx, parent, XENT_DIRTY_SUBTREE | XENT_DIRTY_LAYOUT);
    return true;
}

XentNodeId xent_get_parent(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return XENT_NODE_INVALID;
    }
    return ctx->nodes.parent[node];
}

XentNodeId xent_get_first_child(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return XENT_NODE_INVALID;
    }
    return ctx->nodes.first_child[node];
}

XentNodeId xent_get_next_sibling(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return XENT_NODE_INVALID;
    }
    return ctx->nodes.next_sibling[node];
}

uint32_t xent_get_child_count(const XentContext *ctx, XentNodeId node) {
    if (!xent_is_valid_node(ctx, node)) {
        return 0u;
    }
    return ctx->nodes.child_count[node];
}
