#include "test_common.h"

int main(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId left = xent_create_node(ctx);
    XentNodeId right = xent_create_node(ctx);
    XentNodeId right_spacer = xent_create_node(ctx);
    XentNodeId right_tail = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
    xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
    xent_set_size(ctx, root, 400.0f, 100.0f);
    xent_set_gap(ctx, root, 4.0f);

    xent_set_protocol(ctx, left, XENT_PROTOCOL_ABSOLUTE);
    xent_set_size(ctx, left, 120.0f, 100.0f);
    xent_set_flex_shrink(ctx, left, 0.0f);

    xent_set_protocol(ctx, right, XENT_PROTOCOL_SWIFTSTACK);
    xent_set_stack_axis(ctx, right, XENT_AXIS_HORIZONTAL);
    xent_set_flex_grow(ctx, right, 1.0f);
    xent_set_size(ctx, right, NAN, 100.0f);

    xent_set_is_spacer(ctx, right_spacer, true);
    xent_set_size(ctx, right_tail, 30.0f, 30.0f);

    xent_append_child(ctx, right, right_spacer);
    xent_append_child(ctx, right, right_tail);

    xent_append_child(ctx, root, left);
    xent_append_child(ctx, root, right);

    TEST_ASSERT(xent_layout(ctx, root, 400.0f, 100.0f));

    XentRect left_rect = {0};
    XentRect right_rect = {0};
    XentRect tail_rect = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, left, &left_rect));
    TEST_ASSERT(xent_get_layout_rect(ctx, right, &right_rect));
    TEST_ASSERT(xent_get_layout_rect(ctx, right_tail, &tail_rect));

    TEST_ASSERT(left_rect.width > 0.0f);
    TEST_ASSERT(right_rect.x + 0.01f >= left_rect.x + left_rect.width);
    TEST_ASSERT(tail_rect.x > right_rect.x);

    xent_destroy_context(ctx);
    return 0;
}
