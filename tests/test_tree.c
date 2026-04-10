#include "test_common.h"

int main(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId a = xent_create_node(ctx);
    XentNodeId b = xent_create_node(ctx);
    XentNodeId c = xent_create_node(ctx);
    TEST_ASSERT(root != XENT_NODE_INVALID && a != XENT_NODE_INVALID && b != XENT_NODE_INVALID && c != XENT_NODE_INVALID);

    TEST_ASSERT(xent_append_child(ctx, root, a));
    TEST_ASSERT(xent_append_child(ctx, root, b));
    TEST_ASSERT(xent_get_child_count(ctx, root) == 2u);
    TEST_ASSERT(xent_get_parent(ctx, a) == root);
    TEST_ASSERT(xent_get_parent(ctx, b) == root);

    TEST_ASSERT(xent_append_child(ctx, a, c));
    TEST_ASSERT(xent_get_parent(ctx, c) == a);

    TEST_ASSERT(xent_append_child(ctx, root, c));
    TEST_ASSERT(xent_get_parent(ctx, c) == root);
    TEST_ASSERT(xent_get_child_count(ctx, a) == 0u);
    TEST_ASSERT(xent_get_child_count(ctx, root) == 3u);

    TEST_ASSERT(xent_remove_child(ctx, root, b));
    TEST_ASSERT(xent_get_parent(ctx, b) == XENT_NODE_INVALID);
    TEST_ASSERT(xent_get_child_count(ctx, root) == 2u);

    int payload = 42;
    TEST_ASSERT(xent_get_userdata(ctx, a) == NULL);
    TEST_ASSERT(xent_set_userdata(ctx, a, &payload));
    TEST_ASSERT(xent_get_userdata(ctx, a) == &payload);
    TEST_ASSERT(*(int *)xent_get_userdata(ctx, a) == 42);
    TEST_ASSERT(xent_set_userdata(ctx, a, NULL));
    TEST_ASSERT(xent_get_userdata(ctx, a) == NULL);
    TEST_ASSERT(!xent_set_userdata(ctx, XENT_NODE_INVALID, &payload));
    TEST_ASSERT(xent_get_userdata(ctx, XENT_NODE_INVALID) == NULL);

    TEST_ASSERT(xent_get_control_type(ctx, a) == XENT_CONTROL_CONTAINER);
    TEST_ASSERT(xent_set_control_type(ctx, a, XENT_CONTROL_BUTTON));
    TEST_ASSERT(xent_get_control_type(ctx, a) == XENT_CONTROL_BUTTON);
    TEST_ASSERT(xent_set_control_type(ctx, a, XENT_CONTROL_SLIDER));
    TEST_ASSERT(xent_get_control_type(ctx, a) == XENT_CONTROL_SLIDER);
    TEST_ASSERT(xent_set_control_type(ctx, a, XENT_CONTROL_CUSTOM));
    TEST_ASSERT(xent_get_control_type(ctx, a) == XENT_CONTROL_CUSTOM);
    TEST_ASSERT(!xent_set_control_type(ctx, XENT_NODE_INVALID, XENT_CONTROL_TEXT));
    TEST_ASSERT(xent_get_control_type(ctx, XENT_NODE_INVALID) == XENT_CONTROL_CONTAINER);

    TEST_ASSERT(xent_get_semantic_checked(ctx, a) == 0u);
    TEST_ASSERT(xent_set_semantic_checked(ctx, a, 1));
    TEST_ASSERT(xent_get_semantic_checked(ctx, a) == 1u);
    TEST_ASSERT(xent_set_semantic_checked(ctx, a, 2));
    TEST_ASSERT(xent_get_semantic_checked(ctx, a) == 2u);

    TEST_ASSERT(xent_get_semantic_enabled(ctx, a) == true);
    TEST_ASSERT(xent_set_semantic_enabled(ctx, a, false));
    TEST_ASSERT(xent_get_semantic_enabled(ctx, a) == false);
    TEST_ASSERT(xent_set_semantic_enabled(ctx, a, true));
    TEST_ASSERT(xent_get_semantic_enabled(ctx, a) == true);

    TEST_ASSERT(xent_get_semantic_expanded(ctx, a) == false);
    TEST_ASSERT(xent_set_semantic_expanded(ctx, a, true));
    TEST_ASSERT(xent_get_semantic_expanded(ctx, a) == true);

    TEST_ASSERT(xent_get_semantic_selected(ctx, a) == false);
    TEST_ASSERT(xent_set_semantic_selected(ctx, a, true));
    TEST_ASSERT(xent_get_semantic_selected(ctx, a) == true);

    float val, vmin, vmax;
    TEST_ASSERT(xent_get_semantic_value(ctx, a, &val, &vmin, &vmax));
    TEST_ASSERT(val == 0.0f && vmin == 0.0f && vmax == 0.0f);
    TEST_ASSERT(xent_set_semantic_value(ctx, a, 0.5f, 0.0f, 1.0f));
    TEST_ASSERT(xent_get_semantic_value(ctx, a, &val, &vmin, &vmax));
    TEST_ASSERT(test_float_near(val, 0.5f, 1e-6f));
    TEST_ASSERT(test_float_near(vmin, 0.0f, 1e-6f));
    TEST_ASSERT(test_float_near(vmax, 1.0f, 1e-6f));
    TEST_ASSERT(!xent_get_semantic_value(ctx, XENT_NODE_INVALID, &val, &vmin, &vmax));
    TEST_ASSERT(xent_get_semantic_value(ctx, a, NULL, NULL, NULL));

    TEST_ASSERT(xent_set_userdata(ctx, c, &payload));
    TEST_ASSERT(xent_set_control_type(ctx, c, XENT_CONTROL_CHECKBOX));
    TEST_ASSERT(xent_set_semantic_checked(ctx, c, 1));
    TEST_ASSERT(xent_destroy_node(ctx, c));
    XentNodeId d = xent_create_node(ctx);
    TEST_ASSERT(d != XENT_NODE_INVALID);
    TEST_ASSERT(xent_get_userdata(ctx, d) == NULL);
    TEST_ASSERT(xent_get_control_type(ctx, d) == XENT_CONTROL_CONTAINER);
    TEST_ASSERT(xent_get_semantic_checked(ctx, d) == 0u);
    TEST_ASSERT(xent_get_semantic_enabled(ctx, d) == true);

    xent_destroy_context(ctx);
    return 0;
}
