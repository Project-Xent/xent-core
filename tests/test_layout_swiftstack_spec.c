#include "test_common.h"

#define SCHK_X 1u
#define SCHK_Y 2u
#define SCHK_W 4u
#define SCHK_H 8u

#define SREL_NONE             0
#define SREL_SECOND_WIDER     1
#define SREL_EQUAL_WIDTH      2
#define SREL_FIXED_VS_FLEX    3
#define SREL_FIRST_BELOW      4
#define SREL_BASELINE_LAST    5
#define SREL_SPACER_X         6

#define SWIFT_MAX_CHILD 4

typedef struct StackChildCase {
	XentSize    size;
	char const *text;
	float       priority;
	int         is_spacer;
	XentInsets  margin;
	float       expected_x;
	float       expected_y;
	float       expected_w;
	float       expected_h;
	uint32_t    check_mask;
} StackChildCase;

typedef struct StackCase {
	XentSize       root_size;
	XentAxis       axis;
	XentStackAlign alignment;
	uint32_t       child_count;
	StackChildCase children [SWIFT_MAX_CHILD];
	int            relative_assert;
	float          eps;
} StackCase;

static int stack_finite_size(XentSize s) { return isfinite(s.width) || isfinite(s.height); }
static int stack_finite_inset(XentInsets i) {
	return isfinite(i.top) || isfinite(i.right) || isfinite(i.bottom) || isfinite(i.left);
}

static XentNodeId stack_make_root(XentContext *ctx, StackCase const *spec) {
	XentNodeId root = xent_create_node(ctx);
	xent_set_protocol(ctx, root, XENT_PROTOCOL_SWIFTSTACK);
	xent_set_stack_axis(ctx, root, spec->axis);
	xent_set_size(ctx, root, spec->root_size);
	if (spec->alignment != XENT_STACK_ALIGN_START) xent_set_stack_alignment(ctx, root, spec->alignment);
	return root;
}

static XentNodeId stack_make_child(XentContext *ctx, XentNodeId root, StackChildCase const *spec) {
	XentNodeId node = xent_create_node(ctx);
	if (spec->text) xent_set_text(ctx, node, spec->text);
	if (stack_finite_size(spec->size)) xent_set_size(ctx, node, spec->size);
	if (isfinite(spec->priority)) xent_set_layout_priority(ctx, node, spec->priority);
	if (spec->is_spacer) xent_set_is_spacer(ctx, node, true);
	if (stack_finite_inset(spec->margin)) xent_set_margin(ctx, node, spec->margin);
	xent_append_child(ctx, root, node);
	return node;
}

static int stack_check_rect(XentContext *ctx, XentNodeId node, StackChildCase const *expect, float eps) {
	XentRect rect = {0};
	TEST_ASSERT(xent_get_layout_rect(ctx, node, &rect));
	if (expect->check_mask & SCHK_X) TEST_ASSERT(test_float_near(rect.x, expect->expected_x, eps));
	if (expect->check_mask & SCHK_Y) TEST_ASSERT(test_float_near(rect.y, expect->expected_y, eps));
	if (expect->check_mask & SCHK_W) TEST_ASSERT(test_float_near(rect.width, expect->expected_w, eps));
	if (expect->check_mask & SCHK_H) TEST_ASSERT(test_float_near(rect.height, expect->expected_h, eps));
	return 0;
}

static int stack_check_relative(XentContext *ctx, XentNodeId const *nodes, uint32_t count, int kind, float eps) {
	if (kind == SREL_NONE) return 0;
	if (count < 2) return 0;
	XentRect r0 = {0};
	XentRect r1 = {0};
	TEST_ASSERT(xent_get_layout_rect(ctx, nodes [0], &r0));
	TEST_ASSERT(xent_get_layout_rect(ctx, nodes [1], &r1));
	if (kind == SREL_SECOND_WIDER) TEST_ASSERT(r1.width > r0.width);
	if (kind == SREL_EQUAL_WIDTH) TEST_ASSERT(test_float_near(r0.width, r1.width, eps));
	if (kind == SREL_FIXED_VS_FLEX) {
		TEST_ASSERT(r0.width >= 85.0f);
		TEST_ASSERT(r1.width <= 40.0f);
		TEST_ASSERT(r0.width > r1.width);
	}
	if (kind == SREL_FIRST_BELOW) TEST_ASSERT(r0.y > r1.y);
	if (kind == SREL_BASELINE_LAST) TEST_ASSERT(test_float_near(r0.y + r0.height, r1.y + r1.height, eps));
	if (kind == SREL_SPACER_X && count >= 3) {
		XentRect r2 = {0};
		TEST_ASSERT(xent_get_layout_rect(ctx, nodes [2], &r2));
		TEST_ASSERT(r1.width > 120.0f);
		TEST_ASSERT(r2.x > r1.x);
	}
	return 0;
}

static int run_stack_case(StackCase const *spec) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root                     = stack_make_root(ctx, spec);
	XentNodeId nodes [SWIFT_MAX_CHILD]  = {XENT_NODE_INVALID};
	for (uint32_t i = 0; i < spec->child_count; ++i) nodes [i] = stack_make_child(ctx, root, &spec->children [i]);

	TEST_ASSERT(xent_layout(ctx, root, spec->root_size.width, spec->root_size.height));

	for (uint32_t i = 0; i < spec->child_count; ++i)
		TEST_ASSERT(stack_check_rect(ctx, nodes [i], &spec->children [i], spec->eps) == 0);

	TEST_ASSERT(stack_check_relative(ctx, nodes, spec->child_count, spec->relative_assert, spec->eps) == 0);

	xent_destroy_context(ctx);
	return 0;
}

static int test_stack_table_cases(void) {
	static StackCase const cases [] = {
	    {
	        .root_size   = {120.0f, 40.0f},
	        .axis        = XENT_AXIS_HORIZONTAL,
	        .alignment   = XENT_STACK_ALIGN_START,
	        .child_count = 2,
	        .children    = {
	            {.size = {NAN, 20.0f}, .text = "abcdefghij12", .priority = 0.0f, .margin = {NAN, NAN, NAN, NAN}},
	            {.size = {NAN, 20.0f}, .text = "abcdefghij12", .priority = 10.0f, .margin = {NAN, NAN, NAN, NAN}},
	        },
	        .relative_assert = SREL_SECOND_WIDER,
	        .eps             = 0.5f,
	    },
	    {
	        .root_size   = {120.0f, 40.0f},
	        .axis        = XENT_AXIS_HORIZONTAL,
	        .alignment   = XENT_STACK_ALIGN_START,
	        .child_count = 2,
	        .children    = {
	            {.size = {NAN, 20.0f}, .text = "abcdefghij12", .priority = 1.0f, .margin = {NAN, NAN, NAN, NAN}},
	            {.size = {NAN, 20.0f}, .text = "abcdefghij12", .priority = 1.0f, .margin = {NAN, NAN, NAN, NAN}},
	        },
	        .relative_assert = SREL_EQUAL_WIDTH,
	        .eps             = 0.5f,
	    },
	    {
	        .root_size   = {220.0f, 40.0f},
	        .axis        = XENT_AXIS_HORIZONTAL,
	        .alignment   = XENT_STACK_ALIGN_START,
	        .child_count = 3,
	        .children    = {
	            {.size = {40.0f, 20.0f}, .priority = NAN, .margin = {NAN, NAN, NAN, NAN}},
	            {.size = {NAN, NAN}, .is_spacer = 1, .priority = NAN, .margin = {NAN, NAN, NAN, NAN}},
	            {.size = {40.0f, 20.0f}, .priority = NAN, .margin = {NAN, NAN, NAN, NAN}},
	        },
	        .relative_assert = SREL_SPACER_X,
	        .eps             = 0.5f,
	    },
	    {
	        .root_size   = {120.0f, 40.0f},
	        .axis        = XENT_AXIS_HORIZONTAL,
	        .alignment   = XENT_STACK_ALIGN_START,
	        .child_count = 2,
	        .children    = {
	            {.size = {90.0f, 20.0f}, .priority = 0.0f, .margin = {NAN, NAN, NAN, NAN}},
	            {.size = {NAN, 20.0f}, .text = "abcdefghijkl", .priority = 0.0f, .margin = {NAN, NAN, NAN, NAN}},
	        },
	        .relative_assert = SREL_FIXED_VS_FLEX,
	        .eps             = 0.5f,
	    },
	    {
	        .root_size   = {160.0f, 60.0f},
	        .axis        = XENT_AXIS_HORIZONTAL,
	        .alignment   = XENT_STACK_ALIGN_START,
	        .child_count = 1,
	        .children    = {
	            {.size = {40.0f, NAN}, .priority = NAN, .margin = {0.0f, 6.0f, 0.0f, 8.0f}, .expected_y = 6.0f, .expected_h = 46.0f, .check_mask = SCHK_Y | SCHK_H},
	        },
	        .eps = 0.2f,
	    },
	    {
	        .root_size   = {240.0f, 80.0f},
	        .axis        = XENT_AXIS_HORIZONTAL,
	        .alignment   = XENT_STACK_ALIGN_BASELINE,
	        .child_count = 2,
	        .children    = {
	            {.size = {60.0f, 20.0f}, .text = "small", .priority = NAN, .margin = {NAN, NAN, NAN, NAN}},
	            {.size = {60.0f, 40.0f}, .text = "large", .priority = NAN, .margin = {NAN, NAN, NAN, NAN}},
	        },
	        .relative_assert = SREL_FIRST_BELOW,
	        .eps             = 0.2f,
	    },
	    {
	        .root_size   = {240.0f, 100.0f},
	        .axis        = XENT_AXIS_HORIZONTAL,
	        .alignment   = XENT_STACK_ALIGN_BASELINE,
	        .child_count = 2,
	        .children    = {
	            {.size = {60.0f, 60.0f}, .priority = NAN, .margin = {NAN, NAN, NAN, NAN}},
	            {.size = {60.0f, 20.0f}, .priority = NAN, .margin = {NAN, NAN, NAN, NAN}},
	        },
	        .relative_assert = SREL_BASELINE_LAST,
	        .eps             = 0.2f,
	    },
	    {
	        .root_size   = {260.0f, 80.0f},
	        .axis        = XENT_AXIS_HORIZONTAL,
	        .alignment   = XENT_STACK_ALIGN_BASELINE,
	        .child_count = 2,
	        .children    = {
	            {.size = {20.0f, 20.0f}, .text = "x", .priority = NAN, .margin = {NAN, NAN, NAN, NAN}},
	            {.size = {NAN, NAN}, .is_spacer = 1, .priority = NAN, .margin = {NAN, NAN, NAN, NAN}, .expected_h = 80.0f, .check_mask = SCHK_H},
	        },
	        .eps = 0.2f,
	    },
	    {
	        .root_size   = {100.0f, 160.0f},
	        .axis        = XENT_AXIS_VERTICAL,
	        .alignment   = XENT_STACK_ALIGN_BASELINE,
	        .child_count = 1,
	        .children    = {
	            {.size = {NAN, 30.0f}, .priority = NAN, .margin = {5.0f, 0.0f, 7.0f, 0.0f}, .expected_x = 5.0f, .expected_w = 88.0f, .check_mask = SCHK_X | SCHK_W},
	        },
	        .eps = 0.2f,
	    },
	};

	for (size_t i = 0; i < sizeof(cases) / sizeof(cases [0]); ++i)
		TEST_ASSERT(run_stack_case(&cases [i]) == 0);

	return 0;
}

int main(void) {
	XentTestFn const tests[] = {
	    test_stack_table_cases,
	};

	return test_run_all(tests, sizeof(tests) / sizeof(tests [0]));
}
