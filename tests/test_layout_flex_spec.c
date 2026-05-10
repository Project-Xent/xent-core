#include "test_common.h"

#define FCHK_X                1u
#define FCHK_Y                2u
#define FCHK_W                4u
#define FCHK_H                8u

#define FREL_NONE             0
#define FREL_FIRST_BELOW      1
#define FREL_BASELINE_LAST    2

#define FLEX_MAX_CHILD        4
#define FLEX_FREEZE_MAX_CHILD 4

typedef struct FlexChildCase {
	XentSize      size;
	char const   *text;
	XentFlexAlign align_self;
	float         basis;
	XentSize      min_size;
	XentSize      max_size;
	XentInsets    margin;
	float         expected_x;
	float         expected_y;
	float         expected_w;
	float         expected_h;
	uint32_t      check_mask;
} FlexChildCase;

typedef struct FlexCase {
	XentSize             root_size;
	XentFlexDirection    flex_dir;
	XentDirection        direction;
	XentFlexJustify      justify;
	XentFlexAlign        align_items;
	int                  wrap;
	XentFlexAlignContent align_content;
	uint32_t             child_count;
	FlexChildCase        children [FLEX_MAX_CHILD];
	int                  relative_assert;
	float                eps;
	float                relayout_first_basis;
	float                relayout_first_w;
} FlexCase;

typedef struct FlexFreezeChildCase {
	float basis;
	float grow;
	float shrink;
	float min_w;
	float max_w;
	float expected_w;
} FlexFreezeChildCase;

typedef struct FlexFreezeCase {
	float               root_w;
	uint32_t            child_count;
	FlexFreezeChildCase children [FLEX_FREEZE_MAX_CHILD];
} FlexFreezeCase;

static int has_finite_size(XentSize s) { return isfinite(s.width) || isfinite(s.height); }

static int has_finite_inset_value(XentInsets i) {
	return isfinite(i.top) || isfinite(i.right) || isfinite(i.bottom) || isfinite(i.left);
}

static XentNodeId flex_make_root(XentContext *ctx, FlexCase const *spec) {
	XentNodeId root = xent_create_node(ctx);
	xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
	xent_set_flex_direction(ctx, root, spec->flex_dir);
	xent_set_size(ctx, root, spec->root_size);
	if (spec->direction != XENT_DIRECTION_INHERIT) xent_set_direction(ctx, root, spec->direction);
	xent_set_flex_justify_content(ctx, root, spec->justify);
	xent_set_flex_align_items(ctx, root, spec->align_items);
	if (spec->wrap) xent_set_flex_wrap(ctx, root, XENT_FLEX_WRAP);
	if (spec->align_content != XENT_FLEX_ALIGN_CONTENT_START)
		xent_set_flex_align_content(ctx, root, spec->align_content);
	return root;
}

static XentNodeId flex_make_child(XentContext *ctx, XentNodeId root, FlexChildCase const *spec) {
	XentNodeId node = xent_create_node(ctx);
	if (has_finite_size(spec->size)) xent_set_size(ctx, node, spec->size);
	if (spec->text) xent_set_text(ctx, node, spec->text);
	if (spec->align_self != XENT_FLEX_ALIGN_AUTO) xent_set_flex_align_self(ctx, node, spec->align_self);
	if (isfinite(spec->basis)) xent_set_flex_basis(ctx, node, spec->basis);
	if (has_finite_size(spec->min_size)) xent_set_min_size(ctx, node, spec->min_size);
	if (has_finite_size(spec->max_size)) xent_set_max_size(ctx, node, spec->max_size);
	if (has_finite_inset_value(spec->margin)) xent_set_margin(ctx, node, spec->margin);
	xent_append_child(ctx, root, node);
	return node;
}

static int flex_check_rect(XentContext *ctx, XentNodeId node, FlexChildCase const *expect, float eps) {
	XentRect rect = {0};
	TEST_ASSERT(xent_get_layout_rect(ctx, node, &rect));
	if (expect->check_mask & FCHK_X) TEST_ASSERT(test_float_near(rect.x, expect->expected_x, eps));
	if (expect->check_mask & FCHK_Y) TEST_ASSERT(test_float_near(rect.y, expect->expected_y, eps));
	if (expect->check_mask & FCHK_W) TEST_ASSERT(test_float_near(rect.width, expect->expected_w, eps));
	if (expect->check_mask & FCHK_H) TEST_ASSERT(test_float_near(rect.height, expect->expected_h, eps));
	return 0;
}

static int flex_check_relative(XentContext *ctx, XentNodeId a, XentNodeId b, int kind, float eps) {
	if (kind == FREL_NONE) return 0;
	XentRect ra = {0};
	XentRect rb = {0};
	TEST_ASSERT(xent_get_layout_rect(ctx, a, &ra));
	TEST_ASSERT(xent_get_layout_rect(ctx, b, &rb));
	if (kind == FREL_FIRST_BELOW) TEST_ASSERT(ra.y > rb.y);
	if (kind == FREL_BASELINE_LAST) TEST_ASSERT(test_float_near(ra.y + ra.height, rb.y + rb.height, eps));
	return 0;
}

static int run_flex_case(FlexCase const *spec) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root                   = flex_make_root(ctx, spec);
	XentNodeId nodes [FLEX_MAX_CHILD] = {XENT_NODE_INVALID};
	for (uint32_t i = 0; i < spec->child_count; ++i) nodes [i] = flex_make_child(ctx, root, &spec->children [i]);

	TEST_ASSERT(xent_layout(ctx, root, spec->root_size.width, spec->root_size.height));

	for (uint32_t i = 0; i < spec->child_count; ++i)
		TEST_ASSERT(flex_check_rect(ctx, nodes [i], &spec->children [i], spec->eps) == 0);

	if (spec->child_count >= 2)
		TEST_ASSERT(flex_check_relative(ctx, nodes [0], nodes [1], spec->relative_assert, spec->eps) == 0);

	if (isfinite(spec->relayout_first_basis)) {
		xent_set_flex_basis(ctx, nodes [0], spec->relayout_first_basis);
		TEST_ASSERT(xent_layout(ctx, root, spec->root_size.width, spec->root_size.height));
		XentRect rect = {0};
		TEST_ASSERT(xent_get_layout_rect(ctx, nodes [0], &rect));
		TEST_ASSERT(test_float_near(rect.width, spec->relayout_first_w, spec->eps));
	}

	xent_destroy_context(ctx);
	return 0;
}

#define FLEX_NO_RELAYOUT NAN

static int test_flex_table_cases(void) {
	static FlexCase const cases [] = {
	    {
	        .root_size       = {300.0f, 100.0f},
	        .flex_dir        = XENT_FLEX_ROW,
	        .direction       = XENT_DIRECTION_INHERIT,
	        .justify         = XENT_FLEX_JUSTIFY_CENTER,
	        .align_items     = XENT_FLEX_ALIGN_START,
	        .child_count     = 2,
	        .children        = {
	            {.size = {50.0f, 20.0f}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN, .expected_x = 100.0f, .expected_y = 0.0f, .check_mask = FCHK_X | FCHK_Y},
	            {.size = {50.0f, 20.0f}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN, .expected_x = 150.0f, .expected_y = 0.0f, .check_mask = FCHK_X | FCHK_Y},
	        },
	        .eps                  = 0.2f,
	        .relayout_first_basis = FLEX_NO_RELAYOUT,
	    },
	    {
	        .root_size       = {300.0f, 100.0f},
	        .flex_dir        = XENT_FLEX_ROW,
	        .direction       = XENT_DIRECTION_INHERIT,
	        .justify         = XENT_FLEX_JUSTIFY_SPACE_BETWEEN,
	        .align_items     = XENT_FLEX_ALIGN_START,
	        .child_count     = 2,
	        .children        = {
	            {.size = {50.0f, 20.0f}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN, .expected_x = 0.0f, .check_mask = FCHK_X},
	            {.size = {50.0f, 20.0f}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN, .expected_x = 250.0f, .check_mask = FCHK_X},
	        },
	        .eps                  = 0.2f,
	        .relayout_first_basis = FLEX_NO_RELAYOUT,
	    },
	    {
	        .root_size       = {300.0f, 100.0f},
	        .flex_dir        = XENT_FLEX_ROW,
	        .direction       = XENT_DIRECTION_INHERIT,
	        .justify         = XENT_FLEX_JUSTIFY_START,
	        .align_items     = XENT_FLEX_ALIGN_CENTER,
	        .child_count     = 2,
	        .children        = {
	            {.size = {50.0f, 20.0f}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN, .expected_x = 0.0f, .expected_y = 40.0f, .check_mask = FCHK_X | FCHK_Y},
	            {.size = {50.0f, 20.0f}, .align_self = XENT_FLEX_ALIGN_END, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN, .expected_x = 50.0f, .expected_y = 80.0f, .check_mask = FCHK_X | FCHK_Y},
	        },
	        .eps                  = 0.2f,
	        .relayout_first_basis = FLEX_NO_RELAYOUT,
	    },
	    {
	        .root_size       = {300.0f, 100.0f},
	        .flex_dir        = XENT_FLEX_ROW,
	        .direction       = XENT_DIRECTION_RTL,
	        .justify         = XENT_FLEX_JUSTIFY_START,
	        .align_items     = XENT_FLEX_ALIGN_START,
	        .child_count     = 2,
	        .children        = {
	            {.size = {50.0f, 20.0f}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN, .expected_x = 250.0f, .check_mask = FCHK_X},
	            {.size = {50.0f, 20.0f}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN, .expected_x = 200.0f, .check_mask = FCHK_X},
	        },
	        .eps                  = 0.2f,
	        .relayout_first_basis = FLEX_NO_RELAYOUT,
	    },
	    {
	        .root_size       = {120.0f, 100.0f},
	        .flex_dir        = XENT_FLEX_ROW,
	        .direction       = XENT_DIRECTION_INHERIT,
	        .justify         = XENT_FLEX_JUSTIFY_START,
	        .align_items     = XENT_FLEX_ALIGN_START,
	        .wrap            = 1,
	        .child_count     = 3,
	        .children        = {
	            {.size = {50.0f, 20.0f}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN, .expected_y = 0.0f, .check_mask = FCHK_Y},
	            {.size = {50.0f, 20.0f}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN, .expected_y = 0.0f, .check_mask = FCHK_Y},
	            {.size = {50.0f, 20.0f}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN, .expected_x = 0.0f, .expected_y = 20.0f, .check_mask = FCHK_X | FCHK_Y},
	        },
	        .eps                  = 0.2f,
	        .relayout_first_basis = FLEX_NO_RELAYOUT,
	    },
	    {
	        .root_size       = {120.0f, 100.0f},
	        .flex_dir        = XENT_FLEX_ROW,
	        .direction       = XENT_DIRECTION_INHERIT,
	        .justify         = XENT_FLEX_JUSTIFY_START,
	        .align_items     = XENT_FLEX_ALIGN_START,
	        .wrap            = 1,
	        .child_count     = 2,
	        .children        = {
	            {.size = {50.0f, 20.0f}, .min_size = {100.0f, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN, .expected_y = 0.0f, .expected_w = 100.0f, .check_mask = FCHK_Y | FCHK_W},
	            {.size = {50.0f, 20.0f}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN, .expected_x = 0.0f, .expected_y = 20.0f, .check_mask = FCHK_X | FCHK_Y},
	        },
	        .eps                  = 0.2f,
	        .relayout_first_basis = FLEX_NO_RELAYOUT,
	    },
	    {
	        .root_size       = {120.0f, 100.0f},
	        .flex_dir        = XENT_FLEX_ROW,
	        .direction       = XENT_DIRECTION_INHERIT,
	        .justify         = XENT_FLEX_JUSTIFY_START,
	        .align_items     = XENT_FLEX_ALIGN_START,
	        .wrap            = 1,
	        .align_content   = XENT_FLEX_ALIGN_CONTENT_SPACE_BETWEEN,
	        .child_count     = 4,
	        .children        = {
	            {.size = {50.0f, 20.0f}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN, .expected_y = 0.0f, .check_mask = FCHK_Y},
	            {.size = {50.0f, 20.0f}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN},
	            {.size = {50.0f, 20.0f}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN, .expected_y = 80.0f, .check_mask = FCHK_Y},
	            {.size = {50.0f, 20.0f}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN},
	        },
	        .eps                  = 0.5f,
	        .relayout_first_basis = FLEX_NO_RELAYOUT,
	    },
	    {
	        .root_size       = {120.0f, 100.0f},
	        .flex_dir        = XENT_FLEX_COLUMN,
	        .direction       = XENT_DIRECTION_RTL,
	        .justify         = XENT_FLEX_JUSTIFY_START,
	        .align_items     = XENT_FLEX_ALIGN_START,
	        .child_count     = 1,
	        .children        = {
	            {.size = {30.0f, 20.0f}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN, .expected_x = 90.0f, .check_mask = FCHK_X},
	        },
	        .eps                  = 0.2f,
	        .relayout_first_basis = FLEX_NO_RELAYOUT,
	    },
	    {
	        .root_size       = {400.0f, 120.0f},
	        .flex_dir        = XENT_FLEX_ROW,
	        .direction       = XENT_DIRECTION_INHERIT,
	        .justify         = XENT_FLEX_JUSTIFY_START,
	        .align_items     = XENT_FLEX_ALIGN_BASELINE,
	        .child_count     = 2,
	        .children        = {
	            {.size = {80.0f, 20.0f}, .text = "small", .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN},
	            {.size = {80.0f, 40.0f}, .text = "large", .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN},
	        },
	        .relative_assert      = FREL_FIRST_BELOW,
	        .eps                  = 0.2f,
	        .relayout_first_basis = FLEX_NO_RELAYOUT,
	    },
	    {
	        .root_size       = {300.0f, 120.0f},
	        .flex_dir        = XENT_FLEX_ROW,
	        .direction       = XENT_DIRECTION_INHERIT,
	        .justify         = XENT_FLEX_JUSTIFY_START,
	        .align_items     = XENT_FLEX_ALIGN_BASELINE,
	        .child_count     = 2,
	        .children        = {
	            {.size = {50.0f, 60.0f}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN},
	            {.size = {50.0f, 20.0f}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {NAN, NAN, NAN, NAN}, .basis = NAN},
	        },
	        .relative_assert      = FREL_BASELINE_LAST,
	        .eps                  = 0.2f,
	        .relayout_first_basis = FLEX_NO_RELAYOUT,
	    },
	    {
	        .root_size       = {200.0f, 60.0f},
	        .flex_dir        = XENT_FLEX_ROW,
	        .direction       = XENT_DIRECTION_INHERIT,
	        .justify         = XENT_FLEX_JUSTIFY_START,
	        .align_items     = XENT_FLEX_ALIGN_START,
	        .child_count     = 1,
	        .children        = {
	            {.size = {NAN, 20.0f}, .basis = 10.0f, .min_size = {30.0f, 0.0f}, .max_size = {40.0f, 100.0f}, .margin = {NAN, NAN, NAN, NAN}, .expected_w = 30.0f, .check_mask = FCHK_W},
	        },
	        .eps                  = 0.2f,
	        .relayout_first_basis = 90.0f,
	        .relayout_first_w     = 40.0f,
	    },
	    {
	        .root_size       = {200.0f, 100.0f},
	        .flex_dir        = XENT_FLEX_ROW,
	        .direction       = XENT_DIRECTION_INHERIT,
	        .justify         = XENT_FLEX_JUSTIFY_START,
	        .align_items     = XENT_FLEX_ALIGN_STRETCH,
	        .child_count     = 1,
	        .children        = {
	            {.size = {50.0f, NAN}, .min_size = {NAN, NAN}, .max_size = {NAN, NAN}, .margin = {0.0f, 10.0f, 0.0f, 15.0f}, .basis = NAN, .expected_y = 10.0f, .expected_h = 75.0f, .check_mask = FCHK_Y | FCHK_H},
	        },
	        .eps                  = 0.2f,
	        .relayout_first_basis = FLEX_NO_RELAYOUT,
	    },
	};

	for (size_t i = 0; i < sizeof(cases) / sizeof(cases [0]); ++i) TEST_ASSERT(run_flex_case(&cases [i]) == 0);

	return 0;
}

static XentNodeId flex_make_freeze_child(XentContext *ctx, XentNodeId root, FlexFreezeChildCase const *spec) {
	XentNodeId node = xent_create_node(ctx);
	xent_set_size(ctx, node, (XentSize) {NAN, 20.0f});
	xent_set_flex_basis(ctx, node, spec->basis);
	xent_set_flex_grow(ctx, node, spec->grow);
	xent_set_flex_shrink(ctx, node, spec->shrink);
	if (isfinite(spec->min_w)) xent_set_min_size(ctx, node, (XentSize) {spec->min_w, NAN});
	if (isfinite(spec->max_w)) xent_set_max_size(ctx, node, (XentSize) {spec->max_w, NAN});
	xent_append_child(ctx, root, node);
	return node;
}

static int run_flex_freeze_case(FlexFreezeCase const *spec) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root = xent_create_node(ctx);
	xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
	xent_set_size(ctx, root, (XentSize) {spec->root_w, 40.0f});
	xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
	xent_set_flex_justify_content(ctx, root, XENT_FLEX_JUSTIFY_START);
	xent_set_flex_align_items(ctx, root, XENT_FLEX_ALIGN_START);

	XentNodeId nodes [FLEX_FREEZE_MAX_CHILD] = {XENT_NODE_INVALID};
	for (uint32_t i = 0u; i < spec->child_count; ++i)
		nodes [i] = flex_make_freeze_child(ctx, root, &spec->children [i]);

	TEST_ASSERT(xent_layout(ctx, root, spec->root_w, 40.0f));
	for (uint32_t i = 0u; i < spec->child_count; ++i) {
		XentRect rect = {0};
		TEST_ASSERT(xent_get_layout_rect(ctx, nodes [i], &rect));
		TEST_ASSERT(test_float_near(rect.width, spec->children [i].expected_w, 0.5f));
	}

	xent_destroy_context(ctx);
	return 0;
}

static int test_flex_freeze_cases(void) {
	static FlexFreezeCase const cases [] = {
	    {
	        .root_w      = 400.0f,
	        .child_count = 1u,
	        .children    = {{100.0f, 1.0f, 1.0f, NAN, 250.0f, 250.0f}},
	    },
	    {
	        .root_w      = 400.0f,
	        .child_count = 2u,
	        .children    = {
	           {100.0f, 1.0f, 1.0f, NAN, 150.0f, 150.0f},
	           {100.0f, 1.0f, 1.0f, NAN, NAN, 250.0f},
	        },
	    },
	    {
	        .root_w      = 100.0f,
	        .child_count = 1u,
	        .children    = {{200.0f, 0.0f, 1.0f, 150.0f, NAN, 150.0f}},
	    },
	    {
	        .root_w      = 200.0f,
	        .child_count = 2u,
	        .children    = {
	           {200.0f, 0.0f, 1.0f, 180.0f, NAN, 180.0f},
	           {200.0f, 0.0f, 1.0f, NAN, NAN, 20.0f},
	        },
	    },
	    {
	        .root_w      = 600.0f,
	        .child_count = 3u,
	        .children    = {
	           {100.0f, 1.0f, 1.0f, NAN, 120.0f, 120.0f},
	           {100.0f, 1.0f, 1.0f, NAN, 180.0f, 180.0f},
	           {100.0f, 1.0f, 1.0f, NAN, NAN, 300.0f},
	        },
	    },
	};

	for (size_t i = 0u; i < sizeof(cases) / sizeof(cases [0]); ++i) TEST_ASSERT(run_flex_freeze_case(&cases [i]) == 0);
	return 0;
}

int main(void) {
	XentTestFn const tests [] = {
	  test_flex_table_cases,
	  test_flex_freeze_cases,
	};

	return test_run_all(tests, sizeof(tests) / sizeof(tests [0]));
}
