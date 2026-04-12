#include "../xent_internal.h"

static bool xent_realloc_and_zero(void **ptr, size_t elem_size, uint32_t old_cap, uint32_t new_cap) {
    void *new_mem = realloc(*ptr, elem_size * (size_t)new_cap);
    if (!new_mem) {
        return false;
    }
    *ptr = new_mem;
    if (new_cap > old_cap) {
        memset((uint8_t *)*ptr + elem_size * (size_t)old_cap, 0, elem_size * (size_t)(new_cap - old_cap));
    }
    return true;
}

char *xent_strdup(const char *s) {
    if (!s) {
        return NULL;
    }
    size_t len = strlen(s);
    char *copy = (char *)malloc(len + 1u);
    if (!copy) {
        return NULL;
    }
    memcpy(copy, s, len + 1u);
    return copy;
}

bool xent_ensure_node_capacity(XentContext *ctx, uint32_t needed) {
    if (!ctx) {
        return false;
    }

    XentNodeStore *n = &ctx->nodes;
    if (needed <= n->capacity) {
        return true;
    }

    uint32_t old_cap = n->capacity;
    uint32_t new_cap = old_cap ? old_cap : 64u;
    while (new_cap <= needed) {
        new_cap *= 2u;
    }

#define GROW_ARRAY(field)                                                                                                 \
    if (!xent_realloc_and_zero((void **)&n->field, sizeof(*n->field), old_cap, new_cap)) {                              \
        return false;                                                                                                     \
    }

    GROW_ARRAY(alive);
    GROW_ARRAY(parent);
    GROW_ARRAY(first_child);
    GROW_ARRAY(next_sibling);
    GROW_ARRAY(child_count);

    GROW_ARRAY(protocol);
    GROW_ARRAY(direction);
    GROW_ARRAY(dirty_flags);

    GROW_ARRAY(proposed_w);
    GROW_ARRAY(proposed_h);
    GROW_ARRAY(decided_w);
    GROW_ARRAY(decided_h);
    GROW_ARRAY(abs_x);
    GROW_ARRAY(abs_y);

    GROW_ARRAY(style_w);
    GROW_ARRAY(style_h);
    GROW_ARRAY(min_w);
    GROW_ARRAY(min_h);
    GROW_ARRAY(max_w);
    GROW_ARRAY(max_h);

    GROW_ARRAY(margin_l);
    GROW_ARRAY(margin_t);
    GROW_ARRAY(margin_r);
    GROW_ARRAY(margin_b);

    GROW_ARRAY(padding_l);
    GROW_ARRAY(padding_t);
    GROW_ARRAY(padding_r);
    GROW_ARRAY(padding_b);

    GROW_ARRAY(gap);

    GROW_ARRAY(abs_pos_x);
    GROW_ARRAY(abs_pos_y);

    GROW_ARRAY(flex_grow);
    GROW_ARRAY(flex_shrink);
    GROW_ARRAY(flex_basis);
    GROW_ARRAY(flex_direction);
    GROW_ARRAY(flex_wrap);
    GROW_ARRAY(flex_justify_content);
    GROW_ARRAY(flex_align_items);
    GROW_ARRAY(flex_align_self);
    GROW_ARRAY(flex_align_content);

    GROW_ARRAY(stack_axis);
    GROW_ARRAY(stack_align);
    GROW_ARRAY(layout_priority);
    GROW_ARRAY(is_spacer);

    GROW_ARRAY(text);
    GROW_ARRAY(font_size);
    GROW_ARRAY(text_line_break_policy);
    GROW_ARRAY(text_intrinsic_valid);
    GROW_ARRAY(text_intrinsic_constraint_w);
    GROW_ARRAY(text_intrinsic_font_size);
    GROW_ARRAY(text_intrinsic_line_break_policy);
    GROW_ARRAY(text_intrinsic_width_mode);
    GROW_ARRAY(text_intrinsic_w);
    GROW_ARRAY(text_intrinsic_h);
    GROW_ARRAY(text_intrinsic_lines);

    GROW_ARRAY(semantic_role);
    GROW_ARRAY(semantic_label);
    GROW_ARRAY(semantic_flags);

    GROW_ARRAY(userdata);
    GROW_ARRAY(control_type);
    GROW_ARRAY(semantic_checked);
    GROW_ARRAY(semantic_enabled);
    GROW_ARRAY(semantic_expanded);
    GROW_ARRAY(semantic_selected);
    GROW_ARRAY(semantic_value_now);
    GROW_ARRAY(semantic_value_min);
    GROW_ARRAY(semantic_value_max);

    GROW_ARRAY(focusable);
    GROW_ARRAY(tab_index);

    GROW_ARRAY(grid_def);
    GROW_ARRAY(grid_row);
    GROW_ARRAY(grid_column);
    GROW_ARRAY(grid_row_span);
    GROW_ARRAY(grid_column_span);

#undef GROW_ARRAY

    for (uint32_t i = old_cap; i < new_cap; ++i) {
        n->style_w[i] = NAN;
        n->style_h[i] = NAN;
        n->min_w[i] = 0.0f;
        n->min_h[i] = 0.0f;
        n->max_w[i] = INFINITY;
        n->max_h[i] = INFINITY;
        n->flex_basis[i] = NAN;
        n->flex_shrink[i] = 1.0f;
        n->flex_direction[i] = (uint8_t)XENT_FLEX_ROW;
        n->flex_wrap[i] = (uint8_t)XENT_FLEX_NO_WRAP;
        n->flex_justify_content[i] = (uint8_t)XENT_FLEX_JUSTIFY_START;
        n->flex_align_items[i] = (uint8_t)XENT_FLEX_ALIGN_START;
        n->flex_align_self[i] = (uint8_t)XENT_FLEX_ALIGN_AUTO;
        n->flex_align_content[i] = (uint8_t)XENT_FLEX_ALIGN_CONTENT_START;
        n->stack_axis[i] = (uint8_t)XENT_AXIS_HORIZONTAL;
        n->stack_align[i] = (uint8_t)XENT_STACK_ALIGN_START;
        n->font_size[i] = 14.0f;
        n->text_line_break_policy[i] = (uint8_t)XENT_LINE_BREAK_CHAR_WRAP;
        n->text_intrinsic_valid[i] = 0u;
        n->text_intrinsic_constraint_w[i] = NAN;
        n->text_intrinsic_font_size[i] = 0.0f;
        n->text_intrinsic_line_break_policy[i] = (uint8_t)XENT_LINE_BREAK_CHAR_WRAP;
        n->text_intrinsic_width_mode[i] = (uint8_t)XENT_MEASURE_UNDEFINED;
        n->text_intrinsic_w[i] = 0.0f;
        n->text_intrinsic_h[i] = 0.0f;
        n->text_intrinsic_lines[i] = 0u;
        n->protocol[i] = (uint8_t)XENT_PROTOCOL_ABSOLUTE;
        n->direction[i] = (uint8_t)XENT_DIRECTION_INHERIT;
        n->semantic_enabled[i] = 1u;
        n->grid_def[i] = NULL;
        n->grid_row[i] = 0;
        n->grid_column[i] = 0;
        n->grid_row_span[i] = 1;
        n->grid_column_span[i] = 1;
    }

    n->capacity = new_cap;
    return true;
}
