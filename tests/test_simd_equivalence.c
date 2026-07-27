#include "test_common.h"

enum
{
	NODE_COUNT = 96,
};

typedef struct Fixture {
	XentCtx   *ctx;
	XentNodeId root;
	XentNodeId nodes [NODE_COUNT];
} Fixture;

static Fixture fixture_create(bool simd, bool rounding) {
	XentCfg config = {
	  .point_scale_factor    = 1.25f,
	  .enable_pixel_rounding = rounding,
	  .enable_simd           = simd,
	};
	return (Fixture) {.ctx = xent_ctx_create(&config)};
}

static int fixture_compare(Fixture const *scalar, Fixture const *simd, float epsilon) {
	for (uint32_t i = 0u; i < NODE_COUNT; ++i) {
		XentRect a = {0};
		XentRect b = {0};
		TEST_ASSERT(xent_layout_rect(scalar->ctx, scalar->nodes [i], &a));
		TEST_ASSERT(xent_layout_rect(simd->ctx, simd->nodes [i], &b));
		TEST_ASSERT(test_float_near(a.x, b.x, epsilon));
		TEST_ASSERT(test_float_near(a.y, b.y, epsilon));
		TEST_ASSERT(test_float_near(a.w, b.w, epsilon));
		TEST_ASSERT(test_float_near(a.h, b.h, epsilon));
	}
	return 0;
}

static void fixture_destroy(Fixture *fixture) {
	xent_ctx_destroy(fixture->ctx);
	fixture->ctx = NULL;
}

static int build_flex(Fixture *fixture, bool growing) {
	fixture->root = xent_node_create(fixture->ctx);
	TEST_ASSERT(xent_setproto(fixture->ctx, fixture->root, XENT_PROTOCOL_FLEX));
	TEST_ASSERT(xent_setflexdir(fixture->ctx, fixture->root, XENT_FLEX_ROW));
	TEST_ASSERT(xent_setsize(fixture->ctx, fixture->root, (XentSize) {growing ? 900.0f : 420.0f, 80.0f}));
	TEST_ASSERT(xent_setgap(fixture->ctx, fixture->root, 0.25f));

	for (uint32_t i = 0u; i < NODE_COUNT; ++i) {
		XentNodeId node    = xent_node_create(fixture->ctx);
		float      width   = growing ? NAN : 7.0f + ( float ) (i % 5u);
		float      grow    = 0.25f + ( float ) (i % 4u) * 0.25f;
		float      shrink  = 0.5f + ( float ) (i % 3u) * 0.25f;
		fixture->nodes [i] = node;
		TEST_ASSERT(xent_setsize(fixture->ctx, node, (XentSize) {width, 10.0f + ( float ) (i % 7u)}));
		TEST_ASSERT(xent_setgrow(fixture->ctx, node, grow));
		TEST_ASSERT(xent_setshrink(fixture->ctx, node, shrink));
		TEST_ASSERT(xent_setm(fixture->ctx, node, (XentInsets) {0.1f, 0.0f, 0.15f, 0.0f}));
		TEST_ASSERT(xent_node_append(fixture->ctx, fixture->root, node));
	}
	return xent_layout(fixture->ctx, fixture->root, growing ? 900.0f : 420.0f, 80.0f) ? 0 : 1;
}

static int test_flex_equivalence(bool growing) {
	Fixture scalar = fixture_create(false, false);
	Fixture simd   = fixture_create(true, false);
	TEST_ASSERT(scalar.ctx != NULL);
	TEST_ASSERT(simd.ctx != NULL);
	TEST_ASSERT(build_flex(&scalar, growing) == 0);
	TEST_ASSERT(build_flex(&simd, growing) == 0);
	TEST_ASSERT(fixture_compare(&scalar, &simd, 0.01f) == 0);
	fixture_destroy(&scalar);
	fixture_destroy(&simd);
	return 0;
}

static int build_stack(Fixture *fixture, bool spacious) {
	float width   = spacious ? 1800.0f : 360.0f;
	fixture->root = xent_node_create(fixture->ctx);
	TEST_ASSERT(xent_setproto(fixture->ctx, fixture->root, XENT_PROTOCOL_SWIFTSTACK));
	TEST_ASSERT(xent_stack_setaxis(fixture->ctx, fixture->root, XENT_AXIS_HORIZONTAL));
	TEST_ASSERT(xent_setsize(fixture->ctx, fixture->root, (XentSize) {width, 64.0f}));
	TEST_ASSERT(xent_setgap(fixture->ctx, fixture->root, 0.5f));

	for (uint32_t i = 0u; i < NODE_COUNT; ++i) {
		XentNodeId node    = xent_node_create(fixture->ctx);
		bool       spacer  = spacious && i % 11u == 0u;
		fixture->nodes [i] = node;
		if (spacer) TEST_ASSERT(xent_stack_setspacer(fixture->ctx, node, true));
		else {
			TEST_ASSERT(xent_setsize(fixture->ctx, node, (XentSize) {8.0f + ( float ) (i % 5u), 12.0f}));
			TEST_ASSERT(xent_stack_setprio(fixture->ctx, node, ( float ) (i % 4u)));
		}
		TEST_ASSERT(xent_node_append(fixture->ctx, fixture->root, node));
	}
	return xent_layout(fixture->ctx, fixture->root, width, 64.0f) ? 0 : 1;
}

static int test_stack_equivalence(bool spacious) {
	Fixture scalar = fixture_create(false, false);
	Fixture simd   = fixture_create(true, false);
	TEST_ASSERT(scalar.ctx != NULL);
	TEST_ASSERT(simd.ctx != NULL);
	TEST_ASSERT(build_stack(&scalar, spacious) == 0);
	TEST_ASSERT(build_stack(&simd, spacious) == 0);
	TEST_ASSERT(fixture_compare(&scalar, &simd, 0.01f) == 0);
	fixture_destroy(&scalar);
	fixture_destroy(&simd);
	return 0;
}

static int build_rounding(Fixture *fixture) {
	fixture->root = xent_node_create(fixture->ctx);
	TEST_ASSERT(xent_setproto(fixture->ctx, fixture->root, XENT_PROTOCOL_ABSOLUTE));
	TEST_ASSERT(xent_setsize(fixture->ctx, fixture->root, (XentSize) {800.3f, 600.7f}));
	for (uint32_t i = 0u; i < NODE_COUNT; ++i) {
		XentNodeId node    = xent_node_create(fixture->ctx);
		fixture->nodes [i] = node;
		TEST_ASSERT(xent_setsize(
		  fixture->ctx, node, (XentSize) {3.25f + ( float ) (i % 9u) * 0.11f, 4.75f + ( float ) (i % 7u) * 0.13f}
		));
		TEST_ASSERT(
		  xent_setpos(fixture->ctx, node, (XentPoint) {1.2f + ( float ) i * 2.31f, 2.7f + ( float ) i * 1.17f})
		);
		TEST_ASSERT(xent_node_append(fixture->ctx, fixture->root, node));
	}
	return xent_layout(fixture->ctx, fixture->root, 800.3f, 600.7f) ? 0 : 1;
}

static int test_rounding_equivalence(void) {
	Fixture scalar = fixture_create(false, true);
	Fixture simd   = fixture_create(true, true);
	TEST_ASSERT(scalar.ctx != NULL);
	TEST_ASSERT(simd.ctx != NULL);
	TEST_ASSERT(build_rounding(&scalar) == 0);
	TEST_ASSERT(build_rounding(&simd) == 0);
	TEST_ASSERT(fixture_compare(&scalar, &simd, 0.001f) == 0);
	fixture_destroy(&scalar);
	fixture_destroy(&simd);
	return 0;
}

static int test_flex_grow(void) { return test_flex_equivalence(true); }

static int test_flex_shrink(void) { return test_flex_equivalence(false); }

static int test_stack_expand(void) { return test_stack_equivalence(true); }

static int test_stack_shrink(void) { return test_stack_equivalence(false); }

int        main(void) {
	XentTestFn const tests [] = {
	  test_flex_grow,
	  test_flex_shrink,
	  test_stack_expand,
	  test_stack_shrink,
	  test_rounding_equivalence,
	};
	return test_run_all(tests, sizeof(tests) / sizeof(tests [0]));
}
