#include "test_common.h"

static int test_justify_center(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId a = xent_create_node(ctx);
    XentNodeId b = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
    xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
    xent_set_flex_justify_content(ctx, root, XENT_FLEX_JUSTIFY_CENTER);
    xent_set_size(ctx, root, 300.0f, 100.0f);

    xent_set_size(ctx, a, 50.0f, 20.0f);
    xent_set_size(ctx, b, 50.0f, 20.0f);
    xent_append_child(ctx, root, a);
    xent_append_child(ctx, root, b);

    TEST_ASSERT(xent_layout(ctx, root, 300.0f, 100.0f));

    XentRect ar = {0};
    XentRect br = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, a, &ar));
    TEST_ASSERT(xent_get_layout_rect(ctx, b, &br));
    TEST_ASSERT(test_float_near(ar.x, 100.0f, 0.1f));
    TEST_ASSERT(test_float_near(br.x, 150.0f, 0.1f));

    xent_destroy_context(ctx);
    return 0;
}

static int test_justify_space_between(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId a = xent_create_node(ctx);
    XentNodeId b = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
    xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
    xent_set_flex_justify_content(ctx, root, XENT_FLEX_JUSTIFY_SPACE_BETWEEN);
    xent_set_size(ctx, root, 300.0f, 100.0f);

    xent_set_size(ctx, a, 50.0f, 20.0f);
    xent_set_size(ctx, b, 50.0f, 20.0f);
    xent_append_child(ctx, root, a);
    xent_append_child(ctx, root, b);

    TEST_ASSERT(xent_layout(ctx, root, 300.0f, 100.0f));

    XentRect ar = {0};
    XentRect br = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, a, &ar));
    TEST_ASSERT(xent_get_layout_rect(ctx, b, &br));
    TEST_ASSERT(test_float_near(ar.x, 0.0f, 0.1f));
    TEST_ASSERT(test_float_near(br.x, 250.0f, 0.2f));

    xent_destroy_context(ctx);
    return 0;
}

static int test_align_items_and_self(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId centered = xent_create_node(ctx);
    XentNodeId ended = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
    xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
    xent_set_flex_align_items(ctx, root, XENT_FLEX_ALIGN_CENTER);
    xent_set_size(ctx, root, 300.0f, 100.0f);

    xent_set_size(ctx, centered, 50.0f, 20.0f);
    xent_set_size(ctx, ended, 50.0f, 20.0f);
    xent_set_flex_align_self(ctx, ended, XENT_FLEX_ALIGN_END);

    xent_append_child(ctx, root, centered);
    xent_append_child(ctx, root, ended);

    TEST_ASSERT(xent_layout(ctx, root, 300.0f, 100.0f));

    XentRect cr = {0};
    XentRect er = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, centered, &cr));
    TEST_ASSERT(xent_get_layout_rect(ctx, ended, &er));

    TEST_ASSERT(test_float_near(cr.y, 40.0f, 0.2f));
    TEST_ASSERT(test_float_near(er.y, 80.0f, 0.2f));

    xent_destroy_context(ctx);
    return 0;
}

static int test_wrap_multiline(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId a = xent_create_node(ctx);
    XentNodeId b = xent_create_node(ctx);
    XentNodeId c = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
    xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
    xent_set_flex_wrap(ctx, root, XENT_FLEX_WRAP);
    xent_set_size(ctx, root, 120.0f, 100.0f);

    xent_set_size(ctx, a, 50.0f, 20.0f);
    xent_set_size(ctx, b, 50.0f, 20.0f);
    xent_set_size(ctx, c, 50.0f, 20.0f);
    xent_append_child(ctx, root, a);
    xent_append_child(ctx, root, b);
    xent_append_child(ctx, root, c);

    TEST_ASSERT(xent_layout(ctx, root, 120.0f, 100.0f));

    XentRect ar = {0};
    XentRect br = {0};
    XentRect cr = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, a, &ar));
    TEST_ASSERT(xent_get_layout_rect(ctx, b, &br));
    TEST_ASSERT(xent_get_layout_rect(ctx, c, &cr));

    TEST_ASSERT(test_float_near(ar.y, 0.0f, 0.1f));
    TEST_ASSERT(test_float_near(br.y, 0.0f, 0.1f));
    TEST_ASSERT(test_float_near(cr.x, 0.0f, 0.1f));
    TEST_ASSERT(test_float_near(cr.y, 20.0f, 0.2f));

    xent_destroy_context(ctx);
    return 0;
}

static int test_align_content_distribution(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId n1 = xent_create_node(ctx);
    XentNodeId n2 = xent_create_node(ctx);
    XentNodeId n3 = xent_create_node(ctx);
    XentNodeId n4 = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
    xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
    xent_set_flex_wrap(ctx, root, XENT_FLEX_WRAP);
    xent_set_flex_align_content(ctx, root, XENT_FLEX_ALIGN_CONTENT_SPACE_BETWEEN);
    xent_set_size(ctx, root, 120.0f, 100.0f);

    xent_set_size(ctx, n1, 50.0f, 20.0f);
    xent_set_size(ctx, n2, 50.0f, 20.0f);
    xent_set_size(ctx, n3, 50.0f, 20.0f);
    xent_set_size(ctx, n4, 50.0f, 20.0f);
    xent_append_child(ctx, root, n1);
    xent_append_child(ctx, root, n2);
    xent_append_child(ctx, root, n3);
    xent_append_child(ctx, root, n4);

    TEST_ASSERT(xent_layout(ctx, root, 120.0f, 100.0f));

    XentRect r1 = {0};
    XentRect r3 = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, n1, &r1));
    TEST_ASSERT(xent_get_layout_rect(ctx, n3, &r3));

    TEST_ASSERT(test_float_near(r1.y, 0.0f, 0.2f));
    TEST_ASSERT(test_float_near(r3.y, 80.0f, 0.5f));

    xent_destroy_context(ctx);
    return 0;
}

static int test_rtl_row_start_order(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId a = xent_create_node(ctx);
    XentNodeId b = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
    xent_set_direction(ctx, root, XENT_DIRECTION_RTL);
    xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
    xent_set_size(ctx, root, 300.0f, 100.0f);

    xent_set_size(ctx, a, 50.0f, 20.0f);
    xent_set_size(ctx, b, 50.0f, 20.0f);
    xent_append_child(ctx, root, a);
    xent_append_child(ctx, root, b);

    TEST_ASSERT(xent_layout(ctx, root, 300.0f, 100.0f));

    XentRect ar = {0};
    XentRect br = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, a, &ar));
    TEST_ASSERT(xent_get_layout_rect(ctx, b, &br));
    TEST_ASSERT(test_float_near(ar.x, 250.0f, 0.2f));
    TEST_ASSERT(test_float_near(br.x, 200.0f, 0.2f));

    xent_destroy_context(ctx);
    return 0;
}

static int test_rtl_column_cross_start_alignment(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId child = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
    xent_set_direction(ctx, root, XENT_DIRECTION_RTL);
    xent_set_flex_direction(ctx, root, XENT_FLEX_COLUMN);
    xent_set_flex_align_items(ctx, root, XENT_FLEX_ALIGN_START);
    xent_set_size(ctx, root, 120.0f, 100.0f);

    xent_set_size(ctx, child, 30.0f, 20.0f);
    xent_append_child(ctx, root, child);

    TEST_ASSERT(xent_layout(ctx, root, 120.0f, 100.0f));

    XentRect r = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, child, &r));
    TEST_ASSERT(test_float_near(r.x, 90.0f, 0.2f));

    xent_destroy_context(ctx);
    return 0;
}

static int test_baseline_alignment_for_text(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId small = xent_create_node(ctx);
    XentNodeId large = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
    xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
    xent_set_flex_align_items(ctx, root, XENT_FLEX_ALIGN_BASELINE);
    xent_set_size(ctx, root, 400.0f, 120.0f);

    xent_set_text(ctx, small, "small");
    xent_set_size(ctx, small, 80.0f, 20.0f);

    xent_set_text(ctx, large, "large");
    xent_set_size(ctx, large, 80.0f, 40.0f);

    xent_append_child(ctx, root, small);
    xent_append_child(ctx, root, large);

    TEST_ASSERT(xent_layout(ctx, root, 400.0f, 120.0f));

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

    xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
    xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
    xent_set_flex_align_items(ctx, root, XENT_FLEX_ALIGN_BASELINE);
    xent_set_size(ctx, root, 300.0f, 120.0f);

    xent_set_size(ctx, tall, 50.0f, 60.0f);
    xent_set_size(ctx, short_box, 50.0f, 20.0f);
    xent_append_child(ctx, root, tall);
    xent_append_child(ctx, root, short_box);

    TEST_ASSERT(xent_layout(ctx, root, 300.0f, 120.0f));

    XentRect tr = {0};
    XentRect sr = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, tall, &tr));
    TEST_ASSERT(xent_get_layout_rect(ctx, short_box, &sr));
    TEST_ASSERT(test_float_near(tr.y + tr.height, sr.y + sr.height, 0.2f));

    xent_destroy_context(ctx);
    return 0;
}

static int test_basis_min_max_clamping(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId child = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
    xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
    xent_set_size(ctx, root, 200.0f, 60.0f);

    xent_set_size(ctx, child, NAN, 20.0f);
    xent_set_flex_basis(ctx, child, 10.0f);
    xent_set_min_size(ctx, child, 30.0f, 0.0f);
    xent_set_max_size(ctx, child, 40.0f, 100.0f);
    xent_append_child(ctx, root, child);

    TEST_ASSERT(xent_layout(ctx, root, 200.0f, 60.0f));

    XentRect r = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, child, &r));
    TEST_ASSERT(test_float_near(r.width, 30.0f, 0.2f));

    xent_set_flex_basis(ctx, child, 90.0f);
    TEST_ASSERT(xent_layout(ctx, root, 200.0f, 60.0f));
    TEST_ASSERT(xent_get_layout_rect(ctx, child, &r));
    TEST_ASSERT(test_float_near(r.width, 40.0f, 0.2f));

    xent_destroy_context(ctx);
    return 0;
}

static int test_stretch_respects_cross_margins(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    XentNodeId root = xent_create_node(ctx);
    XentNodeId child = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
    xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
    xent_set_flex_align_items(ctx, root, XENT_FLEX_ALIGN_STRETCH);
    xent_set_size(ctx, root, 200.0f, 100.0f);

    xent_set_size(ctx, child, 50.0f, NAN);
    xent_set_margin(ctx, child, 0.0f, 10.0f, 0.0f, 15.0f);
    xent_append_child(ctx, root, child);

    TEST_ASSERT(xent_layout(ctx, root, 200.0f, 100.0f));

    XentRect r = {0};
    TEST_ASSERT(xent_get_layout_rect(ctx, child, &r));
    TEST_ASSERT(test_float_near(r.y, 10.0f, 0.2f));
    TEST_ASSERT(test_float_near(r.height, 75.0f, 0.2f));

    xent_destroy_context(ctx);
    return 0;
}

int main(void) {
    if (test_justify_center() != 0) {
        return 1;
    }
    if (test_justify_space_between() != 0) {
        return 1;
    }
    if (test_align_items_and_self() != 0) {
        return 1;
    }
    if (test_wrap_multiline() != 0) {
        return 1;
    }
    if (test_align_content_distribution() != 0) {
        return 1;
    }
    if (test_rtl_row_start_order() != 0) {
        return 1;
    }
    if (test_rtl_column_cross_start_alignment() != 0) {
        return 1;
    }
    if (test_baseline_alignment_for_text() != 0) {
        return 1;
    }
    if (test_baseline_fallback_for_non_text() != 0) {
        return 1;
    }
    if (test_basis_min_max_clamping() != 0) {
        return 1;
    }
    if (test_stretch_respects_cross_margins() != 0) {
        return 1;
    }
    return 0;
}
