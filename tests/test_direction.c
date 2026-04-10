#include "test_common.h"

static int test_direction_inheritance(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId child = xent_create_node(ctx);
    XentNodeId grandchild = xent_create_node(ctx);
    TEST_ASSERT(root != XENT_NODE_INVALID && child != XENT_NODE_INVALID && grandchild != XENT_NODE_INVALID);
    TEST_ASSERT(xent_append_child(ctx, root, child));
    TEST_ASSERT(xent_append_child(ctx, child, grandchild));

    TEST_ASSERT(xent_get_direction(ctx, child) == XENT_DIRECTION_INHERIT);
    TEST_ASSERT(xent_get_resolved_direction(ctx, grandchild) == XENT_DIRECTION_LTR);

    TEST_ASSERT(xent_set_direction(ctx, root, XENT_DIRECTION_RTL));
    TEST_ASSERT(xent_get_resolved_direction(ctx, root) == XENT_DIRECTION_RTL);
    TEST_ASSERT(xent_get_resolved_direction(ctx, child) == XENT_DIRECTION_RTL);
    TEST_ASSERT(xent_get_resolved_direction(ctx, grandchild) == XENT_DIRECTION_RTL);

    TEST_ASSERT(xent_set_direction(ctx, child, XENT_DIRECTION_LTR));
    TEST_ASSERT(xent_get_resolved_direction(ctx, child) == XENT_DIRECTION_LTR);
    TEST_ASSERT(xent_get_resolved_direction(ctx, grandchild) == XENT_DIRECTION_LTR);

    xent_destroy_context(ctx);
    return 0;
}

static int test_logical_insets_resolution(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId node = xent_create_node(ctx);
    TEST_ASSERT(root != XENT_NODE_INVALID && node != XENT_NODE_INVALID);
    TEST_ASSERT(xent_append_child(ctx, root, node));

    TEST_ASSERT(xent_set_direction(ctx, root, XENT_DIRECTION_RTL));
    TEST_ASSERT(xent_set_margin(ctx, node, 1.0f, 2.0f, 3.0f, 4.0f));
    TEST_ASSERT(xent_set_padding(ctx, node, 5.0f, 6.0f, 7.0f, 8.0f));

    float ms = 0.0f, me = 0.0f, cs = 0.0f, ce = 0.0f;
    TEST_ASSERT(xent_get_resolved_margin(ctx, node, XENT_AXIS_HORIZONTAL, &ms, &me, &cs, &ce));
    TEST_ASSERT(test_float_near(ms, 3.0f, 0.001f));
    TEST_ASSERT(test_float_near(me, 1.0f, 0.001f));
    TEST_ASSERT(test_float_near(cs, 2.0f, 0.001f));
    TEST_ASSERT(test_float_near(ce, 4.0f, 0.001f));

    TEST_ASSERT(xent_get_resolved_padding(ctx, node, XENT_AXIS_VERTICAL, &ms, &me, &cs, &ce));
    TEST_ASSERT(test_float_near(ms, 6.0f, 0.001f));
    TEST_ASSERT(test_float_near(me, 8.0f, 0.001f));
    TEST_ASSERT(test_float_near(cs, 7.0f, 0.001f));
    TEST_ASSERT(test_float_near(ce, 5.0f, 0.001f));

    xent_destroy_context(ctx);
    return 0;
}

int main(void) {
    if (test_direction_inheritance() != 0) {
        return 1;
    }
    if (test_logical_insets_resolution() != 0) {
        return 1;
    }
    return 0;
}
