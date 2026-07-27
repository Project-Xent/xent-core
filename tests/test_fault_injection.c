#include "test_common.h"
#include "../src/xent_alloc_internal.h"
#include "../src/xent_internal.h"

#if !defined(XENT_ENABLE_FAULT_INJECTION)
  #error "test_fault_injection requires XENT_ENABLE_FAULT_INJECTION"
#endif

static int assert_only_child(XentCtx const *ctx, XentNodeId parent, XentNodeId child) {
	TEST_ASSERT(xent_node_valid(ctx, parent));
	TEST_ASSERT(xent_node_valid(ctx, child));
	TEST_ASSERT(xent_node_parent(ctx, parent) == XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_parent(ctx, child) == parent);
	TEST_ASSERT(xent_node_first(ctx, parent) == child);
	TEST_ASSERT(xent_node_last(ctx, parent) == child);
	TEST_ASSERT(xent_node_prev(ctx, parent) == XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_next(ctx, parent) == XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_prev(ctx, child) == XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_next(ctx, child) == XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_nchild(ctx, parent) == 1u);
	return 0;
}

static int assert_no_children(XentCtx const *ctx, XentNodeId parent) {
	TEST_ASSERT(xent_node_valid(ctx, parent));
	TEST_ASSERT(xent_node_parent(ctx, parent) == XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_first(ctx, parent) == XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_last(ctx, parent) == XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_prev(ctx, parent) == XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_next(ctx, parent) == XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_nchild(ctx, parent) == 0u);
	return 0;
}

static int test_node_store_growth_failure(void) {
	XentCtx   *ctx    = xent_ctx_create(NULL);
	XentNodeId parent = xent_node_create(ctx);
	XentNodeId child  = xent_node_create(ctx);
	TEST_ASSERT(ctx && parent != XENT_NODE_INVALID && child != XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_append(ctx, parent, child));

	while (ctx->nodes.count < ctx->nodes.capacity - 1u) TEST_ASSERT(xent_node_create(ctx) != XENT_NODE_INVALID);

	uint32_t capacity = ctx->nodes.capacity;
	uint32_t count    = ctx->nodes.count;
	xent_alloc_fail_after(XENT_ALLOC_NODE_GROW, 3u);
	TEST_ASSERT(xent_node_create(ctx) == XENT_NODE_INVALID);
	TEST_ASSERT(ctx->nodes.capacity == capacity);
	TEST_ASSERT(ctx->nodes.count == count);
	if (assert_only_child(ctx, parent, child)) return 1;

	xent_test_alloc_reset();
	TEST_ASSERT(xent_node_create(ctx) != XENT_NODE_INVALID);
	TEST_ASSERT(ctx->nodes.capacity > capacity);
	if (assert_only_child(ctx, parent, child)) return 1;
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_scratch_arena_failure(void) {
	XentCtx   *ctx    = xent_ctx_create(NULL);
	XentNodeId parent = xent_node_create(ctx);
	XentNodeId child  = xent_node_create(ctx);
	TEST_ASSERT(ctx && parent != XENT_NODE_INVALID && child != XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_append(ctx, parent, child));

	xent_alloc_fail_after(XENT_ALLOC_SCRATCH_ARENA, 0u);
	TEST_ASSERT(xent_scratch_alloc(ctx, 64u, _Alignof(void *)) == NULL);
	TEST_ASSERT(ctx->scratch_head == NULL);
	TEST_ASSERT(ctx->scratch_current == NULL);
	if (assert_only_child(ctx, parent, child)) return 1;

	xent_test_alloc_reset();
	TEST_ASSERT(xent_scratch_alloc(ctx, 64u, _Alignof(void *)) != NULL);
	if (assert_only_child(ctx, parent, child)) return 1;
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_mutation_dirty_queue_failure(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx);

	xent_alloc_fail_after(XENT_ALLOC_TOPOLOGY_MUTATION, 0u);
	XentNodeId parent = xent_node_create(ctx);
	TEST_ASSERT(parent != XENT_NODE_INVALID);
	TEST_ASSERT(ctx->dirty_nodes == NULL);
	TEST_ASSERT(ctx->dirty_capacity == 0u);
	if (assert_no_children(ctx, parent)) return 1;

	xent_test_alloc_reset();
	TEST_ASSERT(xent_setw(ctx, parent, 40.0f));
	TEST_ASSERT(ctx->dirty_nodes != NULL);
	TEST_ASSERT(ctx->dirty_count == 1u);
	XentNodeId child = xent_node_create(ctx);
	TEST_ASSERT(child != XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_append(ctx, parent, child));
	if (assert_only_child(ctx, parent, child)) return 1;
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_mutation_free_list_failure(void) {
	XentCtx   *ctx    = xent_ctx_create(NULL);
	XentNodeId parent = xent_node_create(ctx);
	XentNodeId child  = xent_node_create(ctx);
	TEST_ASSERT(ctx && parent != XENT_NODE_INVALID && child != XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_append(ctx, parent, child));

	xent_alloc_fail_after(XENT_ALLOC_TOPOLOGY_MUTATION, 0u);
	TEST_ASSERT(xent_node_destroy(ctx, child));
	TEST_ASSERT(!xent_node_valid(ctx, child));
	TEST_ASSERT(ctx->free_count == 0u);
	TEST_ASSERT(ctx->free_capacity == 0u);
	if (assert_no_children(ctx, parent)) return 1;

	xent_test_alloc_reset();
	XentNodeId replacement = xent_node_create(ctx);
	TEST_ASSERT(replacement != XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_index(replacement) != xent_node_index(child));
	if (assert_no_children(ctx, parent)) return 1;
	xent_ctx_destroy(ctx);
	return 0;
}

int main(void) {
	XentTestFn const tests [] = {
	  test_node_store_growth_failure,
	  test_scratch_arena_failure,
	  test_mutation_dirty_queue_failure,
	  test_mutation_free_list_failure,
	};
	return test_run_all(tests, sizeof(tests) / sizeof(tests [0]));
}
