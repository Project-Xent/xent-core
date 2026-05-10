#include "test_common.h"

#define SCHK_X             1u
#define SCHK_Y             2u
#define SCHK_W             4u
#define SCHK_H             8u

#define SREL_NONE          0
#define SREL_SECOND_WIDER  1
#define SREL_EQUAL_WIDTH   2
#define SREL_FIXED_VS_FLEX 3
#define SREL_FIRST_BELOW   4
#define SREL_BASELINE_LAST 5
#define SREL_SPACER_X      6

#define SWIFT_MAX_CHILD    4

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

typedef struct StackPairRects {
	XentRect first;
	XentRect second;
} StackPairRects;

typedef int (*StackRelationCheckFn)(
  StackPairRects const *rects, XentContext *ctx, XentNodeId const *nodes, uint32_t count, float eps
);

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

static int stack_relation_second_wider(
  StackPairRects const *rects, XentContext *ctx, XentNodeId const *nodes, uint32_t count, float eps
) {
	( void ) ctx;
	( void ) nodes;
	( void ) count;
	( void ) eps;
	return rects->second.width > rects->first.width ? 0 : 1;
}

static int stack_relation_equal_width(
  StackPairRects const *rects, XentContext *ctx, XentNodeId const *nodes, uint32_t count, float eps
) {
	( void ) ctx;
	( void ) nodes;
	( void ) count;
	return test_float_near(rects->first.width, rects->second.width, eps) ? 0 : 1;
}

static int stack_relation_fixed_vs_flex(
  StackPairRects const *rects, XentContext *ctx, XentNodeId const *nodes, uint32_t count, float eps
) {
	( void ) ctx;
	( void ) nodes;
	( void ) count;
	( void ) eps;
	return rects->first.width >= 85.0f && rects->second.width <= 40.0f && rects->first.width > rects->second.width ? 0
	                                                                                                               : 1;
}

static int stack_relation_first_below(
  StackPairRects const *rects, XentContext *ctx, XentNodeId const *nodes, uint32_t count, float eps
) {
	( void ) ctx;
	( void ) nodes;
	( void ) count;
	( void ) eps;
	return rects->first.y > rects->second.y ? 0 : 1;
}

static int stack_relation_baseline_last(
  StackPairRects const *rects, XentContext *ctx, XentNodeId const *nodes, uint32_t count, float eps
) {
	( void ) ctx;
	( void ) nodes;
	( void ) count;
	return test_float_near(rects->first.y + rects->first.height, rects->second.y + rects->second.height, eps) ? 0 : 1;
}

static int stack_relation_spacer_x(
  StackPairRects const *rects, XentContext *ctx, XentNodeId const *nodes, uint32_t count, float eps
) {
	( void ) eps;
	if (count < 3) return 0;
	XentRect spacer_end = {0};
	TEST_ASSERT(xent_get_layout_rect(ctx, nodes [2], &spacer_end));
	return rects->second.width > 120.0f && spacer_end.x > rects->second.x ? 0 : 1;
}

static int stack_check_relative(XentContext *ctx, XentNodeId const *nodes, uint32_t count, int kind, float eps) {
	static StackRelationCheckFn const checks [] = {
	  NULL,
	  stack_relation_second_wider,
	  stack_relation_equal_width,
	  stack_relation_fixed_vs_flex,
	  stack_relation_first_below,
	  stack_relation_baseline_last,
	  stack_relation_spacer_x,
	};
	if (kind <= SREL_NONE || ( size_t ) kind >= sizeof(checks) / sizeof(checks [0]) || count < 2) return 0;

	StackPairRects rects = {0};
	TEST_ASSERT(xent_get_layout_rect(ctx, nodes [0], &rects.first));
	TEST_ASSERT(xent_get_layout_rect(ctx, nodes [1], &rects.second));
	return checks [kind](&rects, ctx, nodes, count, eps);
}

static int run_stack_case(StackCase const *spec) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root                    = stack_make_root(ctx, spec);
	XentNodeId nodes [SWIFT_MAX_CHILD] = {XENT_NODE_INVALID};
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

	for (size_t i = 0; i < sizeof(cases) / sizeof(cases [0]); ++i) TEST_ASSERT(run_stack_case(&cases [i]) == 0);

	return 0;
}

int main(void) {
	XentTestFn const tests [] = {
	  test_stack_table_cases,
	};

	return test_run_all(tests, sizeof(tests) / sizeof(tests [0]));
}
