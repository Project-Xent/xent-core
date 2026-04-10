#include "test_common.h"

static int test_priority_shrink_order(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId low = xent_create_node(ctx);
    XentNodeId high = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_SWIFTSTACK);
    xent_set_stack_axis(ctx, root, XENT_AXIS_HORIZONTAL);
    xent_set_size(ctx, root, 120.0f, 40.0f);

    xent_set_text(ctx, low, "abcdefghij12");
    xent_set_text(ctx, high, "abcdefghij12");
    xent_set_size(ctx, low, NAN, 20.0f);
    xent_set_size(ctx, high, NAN, 20.0f);
    xent_set_layout_priority(ctx, low, 0.0f);
    xent_set_layout_priority(ctx, high, 10.0f);

    xent_append_child(ctx, root, low);
    xent_append_child(ctx, root, high);
    TEST_ASSERT(xent_layout(ctx, root, 120.0f, 40.0f));

    XentRect low_r = {0};
    XentRect high_r = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, low, &low_r));
    TEST_ASSERT(xent_get_layout_rect(ctx, high, &high_r));
    TEST_ASSERT(high_r.width > low_r.width);

    xent_destroy_context(ctx);
    return 0;
}

static int test_equal_priority_proportional_shrink(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId a = xent_create_node(ctx);
    XentNodeId b = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_SWIFTSTACK);
    xent_set_stack_axis(ctx, root, XENT_AXIS_HORIZONTAL);
    xent_set_size(ctx, root, 120.0f, 40.0f);

    xent_set_text(ctx, a, "abcdefghij12");
    xent_set_text(ctx, b, "abcdefghij12");
    xent_set_size(ctx, a, NAN, 20.0f);
    xent_set_size(ctx, b, NAN, 20.0f);
    xent_set_layout_priority(ctx, a, 1.0f);
    xent_set_layout_priority(ctx, b, 1.0f);

    xent_append_child(ctx, root, a);
    xent_append_child(ctx, root, b);
    TEST_ASSERT(xent_layout(ctx, root, 120.0f, 40.0f));

    XentRect ar = {0};
    XentRect br = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, a, &ar));
    TEST_ASSERT(xent_get_layout_rect(ctx, b, &br));
    TEST_ASSERT(test_float_near(ar.width, br.width, 0.5f));

    xent_destroy_context(ctx);
    return 0;
}

static int test_spacer_absorbs_extra_space(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId left = xent_create_node(ctx);
    XentNodeId spacer = xent_create_node(ctx);
    XentNodeId right = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_SWIFTSTACK);
    xent_set_stack_axis(ctx, root, XENT_AXIS_HORIZONTAL);
    xent_set_size(ctx, root, 220.0f, 40.0f);

    xent_set_size(ctx, left, 40.0f, 20.0f);
    xent_set_size(ctx, right, 40.0f, 20.0f);
    xent_set_is_spacer(ctx, spacer, true);

    xent_append_child(ctx, root, left);
    xent_append_child(ctx, root, spacer);
    xent_append_child(ctx, root, right);
    TEST_ASSERT(xent_layout(ctx, root, 220.0f, 40.0f));

    XentRect sr = {0};
    XentRect rr = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, spacer, &sr));
    TEST_ASSERT(xent_get_layout_rect(ctx, right, &rr));
    TEST_ASSERT(sr.width > 120.0f);
    TEST_ASSERT(rr.x > sr.x);

    xent_destroy_context(ctx);
    return 0;
}

static int test_fixed_preserved_flexible_shrinks_first(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId fixed = xent_create_node(ctx);
    XentNodeId flexible = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_SWIFTSTACK);
    xent_set_stack_axis(ctx, root, XENT_AXIS_HORIZONTAL);
    xent_set_size(ctx, root, 120.0f, 40.0f);

    xent_set_size(ctx, fixed, 90.0f, 20.0f);
    xent_set_text(ctx, flexible, "abcdefghijkl");
    xent_set_size(ctx, flexible, NAN, 20.0f);
    xent_set_layout_priority(ctx, fixed, 0.0f);
    xent_set_layout_priority(ctx, flexible, 0.0f);

    xent_append_child(ctx, root, fixed);
    xent_append_child(ctx, root, flexible);
    TEST_ASSERT(xent_layout(ctx, root, 120.0f, 40.0f));

    XentRect fixed_r = {0};
    XentRect flex_r = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, fixed, &fixed_r));
    TEST_ASSERT(xent_get_layout_rect(ctx, flexible, &flex_r));
    TEST_ASSERT(fixed_r.width >= 85.0f);
    TEST_ASSERT(flex_r.width <= 40.0f);
    TEST_ASSERT(fixed_r.width > flex_r.width);

    xent_destroy_context(ctx);
    return 0;
}

static int test_cross_axis_margins_applied(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId child = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_SWIFTSTACK);
    xent_set_stack_axis(ctx, root, XENT_AXIS_HORIZONTAL);
    xent_set_size(ctx, root, 160.0f, 60.0f);
    xent_set_size(ctx, child, 40.0f, NAN);
    xent_set_margin(ctx, child, 0.0f, 6.0f, 0.0f, 8.0f);

    xent_append_child(ctx, root, child);
    TEST_ASSERT(xent_layout(ctx, root, 160.0f, 60.0f));

    XentRect r = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, child, &r));
    TEST_ASSERT(test_float_near(r.y, 6.0f, 0.2f));
    TEST_ASSERT(test_float_near(r.height, 46.0f, 0.2f));

    xent_destroy_context(ctx);
    return 0;
}

static int test_baseline_alignment_for_text(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId small = xent_create_node(ctx);
    XentNodeId large = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_SWIFTSTACK);
    xent_set_stack_axis(ctx, root, XENT_AXIS_HORIZONTAL);
    xent_set_stack_alignment(ctx, root, XENT_STACK_ALIGN_BASELINE);
    xent_set_size(ctx, root, 240.0f, 80.0f);

    xent_set_text(ctx, small, "small");
    xent_set_size(ctx, small, 60.0f, 20.0f);
    xent_set_text(ctx, large, "large");
    xent_set_size(ctx, large, 60.0f, 40.0f);

    xent_append_child(ctx, root, small);
    xent_append_child(ctx, root, large);
    TEST_ASSERT(xent_layout(ctx, root, 240.0f, 80.0f));

    XentRect sr = {0};
    XentRect lr = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, small, &sr));
    TEST_ASSERT(xent_get_layout_rect(ctx, large, &lr));
    TEST_ASSERT(sr.y > lr.y);

    xent_destroy_context(ctx);
    return 0;
}

static int test_baseline_fallback_for_non_text(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId tall = xent_create_node(ctx);
    XentNodeId short_box = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_SWIFTSTACK);
    xent_set_stack_axis(ctx, root, XENT_AXIS_HORIZONTAL);
    xent_set_stack_alignment(ctx, root, XENT_STACK_ALIGN_BASELINE);
    xent_set_size(ctx, root, 240.0f, 100.0f);

    xent_set_size(ctx, tall, 60.0f, 60.0f);
    xent_set_size(ctx, short_box, 60.0f, 20.0f);

    xent_append_child(ctx, root, tall);
    xent_append_child(ctx, root, short_box);
    TEST_ASSERT(xent_layout(ctx, root, 240.0f, 100.0f));

    XentRect tr = {0};
    XentRect sr = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, tall, &tr));
    TEST_ASSERT(xent_get_layout_rect(ctx, short_box, &sr));
    TEST_ASSERT(test_float_near(tr.y + tr.height, sr.y + sr.height, 0.2f));

    xent_destroy_context(ctx);
    return 0;
}

static int test_baseline_mode_keeps_spacer_cross_fill(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId text_node = xent_create_node(ctx);
    XentNodeId spacer = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_SWIFTSTACK);
    xent_set_stack_axis(ctx, root, XENT_AXIS_HORIZONTAL);
    xent_set_stack_alignment(ctx, root, XENT_STACK_ALIGN_BASELINE);
    xent_set_size(ctx, root, 260.0f, 80.0f);

    xent_set_text(ctx, text_node, "x");
    xent_set_size(ctx, text_node, 20.0f, 20.0f);
    xent_set_is_spacer(ctx, spacer, true);

    xent_append_child(ctx, root, text_node);
    xent_append_child(ctx, root, spacer);
    TEST_ASSERT(xent_layout(ctx, root, 260.0f, 80.0f));

    XentRect sr = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, spacer, &sr));
    TEST_ASSERT(test_float_near(sr.height, 80.0f, 0.2f));

    xent_destroy_context(ctx);
    return 0;
}

static int test_vertical_axis_ignores_baseline_mode(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId child = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_SWIFTSTACK);
    xent_set_stack_axis(ctx, root, XENT_AXIS_VERTICAL);
    xent_set_stack_alignment(ctx, root, XENT_STACK_ALIGN_BASELINE);
    xent_set_size(ctx, root, 100.0f, 160.0f);

    xent_set_size(ctx, child, NAN, 30.0f);
    xent_set_margin(ctx, child, 5.0f, 0.0f, 7.0f, 0.0f);
    xent_append_child(ctx, root, child);
    TEST_ASSERT(xent_layout(ctx, root, 100.0f, 160.0f));

    XentRect r = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, child, &r));
    TEST_ASSERT(test_float_near(r.x, 5.0f, 0.2f));
    TEST_ASSERT(test_float_near(r.width, 88.0f, 0.2f));

    xent_destroy_context(ctx);
    return 0;
}

int main(void) {
    if (test_priority_shrink_order() != 0) {
        return 1;
    }
    if (test_equal_priority_proportional_shrink() != 0) {
        return 1;
    }
    if (test_spacer_absorbs_extra_space() != 0) {
        return 1;
    }
    if (test_fixed_preserved_flexible_shrinks_first() != 0) {
        return 1;
    }
    if (test_cross_axis_margins_applied() != 0) {
        return 1;
    }
    if (test_baseline_alignment_for_text() != 0) {
        return 1;
    }
    if (test_baseline_fallback_for_non_text() != 0) {
        return 1;
    }
    if (test_baseline_mode_keeps_spacer_cross_fill() != 0) {
        return 1;
    }
    if (test_vertical_axis_ignores_baseline_mode() != 0) {
        return 1;
    }
    return 0;
}
