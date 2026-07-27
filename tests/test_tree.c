#include "test_common.h"
#include "../src/xent_internal.h"

static uint32_t   lifecycle_destroy_count;
static uint32_t   lifecycle_reparent_count;
static uint32_t   lifecycle_second_destroy_count;
static XentNodeId last_lifecycle_node;

static void       test_lifecycle(XentCtx *ctx, XentNodeEvent const *lifecycle, void *userdata) {
	( void ) ctx;
	( void ) userdata;
	last_lifecycle_node = lifecycle->node;
	if (lifecycle->event == XENT_NODE_EVENT_DESTROY) lifecycle_destroy_count++;
	if (lifecycle->event == XENT_NODE_EVENT_REPARENT) lifecycle_reparent_count++;
}

static void test_second_lifecycle(XentCtx *ctx, XentNodeEvent const *lifecycle, void *userdata) {
	( void ) ctx;
	( void ) userdata;
	if (lifecycle->event == XENT_NODE_EVENT_DESTROY) lifecycle_second_destroy_count++;
}

typedef struct SelfRemovingObserver {
	XentObsId id;
	uint32_t  calls;
	bool      removed;
	bool      mutation_rejected;
} SelfRemovingObserver;

static void test_self_removal(XentCtx *ctx, XentNodeEvent const *lifecycle, void *userdata) {
	( void ) lifecycle;
	SelfRemovingObserver *state = ( SelfRemovingObserver * ) userdata;
	state->calls++;
	state->mutation_rejected = xent_node_create(ctx) == XENT_NODE_INVALID;
	state->removed           = xent_node_delobs(ctx, state->id);
}

static int test_generation_handles(XentCtx *ctx) {
	XentNodeId first = xent_node_create(ctx);
	TEST_ASSERT(first != XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_valid(ctx, first));
	TEST_ASSERT(xent_node_index(first) != 0u);
	TEST_ASSERT(xent_node_generation(first) != 0u);

	uint32_t   slot  = xent_node_index(first);
	XentNodeId stale = first;
	TEST_ASSERT(xent_node_destroy(ctx, first));
	TEST_ASSERT(!xent_node_valid(ctx, stale));
	TEST_ASSERT(!xent_node_destroy(ctx, stale));
	TEST_ASSERT(xent_node_parent(ctx, stale) == XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_nchild(ctx, stale) == 0u);
	TEST_ASSERT(!xent_sem_setenabled(ctx, stale, false));

	XentNodeId reused = xent_node_create(ctx);
	TEST_ASSERT(reused != XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_index(reused) == slot);
	TEST_ASSERT(reused != stale);
	TEST_ASSERT(xent_node_generation(reused) != xent_node_generation(stale));
	TEST_ASSERT(xent_node_valid(ctx, reused));
	TEST_ASSERT(!xent_node_valid(ctx, stale));

	XentNodeId parent = xent_node_create(ctx);
	XentNodeId child  = xent_node_create(ctx);
	TEST_ASSERT(parent != XENT_NODE_INVALID && child != XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_append(ctx, parent, child));

	XentNodeId dead_parent = parent;
	TEST_ASSERT(xent_node_destroy(ctx, parent));
	TEST_ASSERT(!xent_node_append(ctx, dead_parent, child));
	TEST_ASSERT(!xent_node_append(ctx, reused, dead_parent));
	TEST_ASSERT(!xent_node_remove(ctx, dead_parent, child));
	TEST_ASSERT(!xent_node_valid(ctx, child));

	TEST_ASSERT(!xent_node_append(ctx, XENT_NODE_INVALID, reused));
	TEST_ASSERT(!xent_node_append(ctx, reused, XENT_NODE_INVALID));
	TEST_ASSERT(!xent_node_destroy(ctx, XENT_NODE_INVALID));
	TEST_ASSERT(xent_node_first(ctx, XENT_NODE_INVALID) == XENT_NODE_INVALID);

	TEST_ASSERT(xent_node_destroy(ctx, reused));
	return 0;
}

static int test_subtree_stale_handles(XentCtx *ctx) {
	XentNodeId subtree = xent_node_create(ctx);
	XentNodeId branch  = xent_node_create(ctx);
	XentNodeId leaf    = xent_node_create(ctx);
	TEST_ASSERT(subtree != XENT_NODE_INVALID && branch != XENT_NODE_INVALID && leaf != XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_append(ctx, subtree, branch));
	TEST_ASSERT(xent_node_append(ctx, branch, leaf));

	XentNodeId stale_subtree = subtree;
	XentNodeId stale_branch  = branch;
	XentNodeId stale_leaf    = leaf;
	uint32_t   before        = lifecycle_destroy_count;
	TEST_ASSERT(xent_node_destroy(ctx, subtree));
	TEST_ASSERT(lifecycle_destroy_count == before + 3u);
	TEST_ASSERT(!xent_node_valid(ctx, stale_subtree));
	TEST_ASSERT(!xent_node_valid(ctx, stale_branch));
	TEST_ASSERT(!xent_node_valid(ctx, stale_leaf));
	TEST_ASSERT(
	  last_lifecycle_node == stale_subtree || last_lifecycle_node == stale_branch || last_lifecycle_node == stale_leaf
	);
	return 0;
}

static int test_capacity_boundary(XentCtx *ctx) {
	TEST_ASSERT(xent_node_reserve(ctx, 4u));
	XentNodeId nodes [8];
	for (uint32_t i = 0u; i < 8u; i++) {
		nodes [i] = xent_node_create(ctx);
		TEST_ASSERT(nodes [i] != XENT_NODE_INVALID);
		TEST_ASSERT(xent_node_valid(ctx, nodes [i]));
	}
	for (uint32_t i = 0u; i < 8u; i++) TEST_ASSERT(xent_node_destroy(ctx, nodes [i]));
	for (uint32_t i = 0u; i < 8u; i++) TEST_ASSERT(!xent_node_valid(ctx, nodes [i]));

	/* Near-max reserve must fail without wrapping capacity growth into a hang. */
	TEST_ASSERT(!xent_node_reserve(ctx, UINT32_MAX));
	TEST_ASSERT(!xent_node_reserve(ctx, UINT32_MAX - 1u));
	TEST_ASSERT(!xent_node_reserve(ctx, UINT32_MAX - 2u));
	return 0;
}

static int test_generation_wrap_retires_slot(XentCtx *ctx) {
	XentNodeId node = xent_node_create(ctx);
	TEST_ASSERT(node != XENT_NODE_INVALID);
	uint32_t slot = xent_node_index(node);
	TEST_ASSERT(slot != 0u && slot < ctx->nodes.capacity);

	/* Simulate final generation, then destroy: slot must not return to the free list. */
	ctx->nodes.lifetime.generation [slot] = UINT32_MAX;
	XentNodeId final_handle               = xent_make_node_id(slot, UINT32_MAX);
	TEST_ASSERT(xent_node_valid(ctx, final_handle));
	TEST_ASSERT(xent_node_destroy(ctx, final_handle));
	TEST_ASSERT(!xent_node_valid(ctx, final_handle));
	TEST_ASSERT(ctx->nodes.lifetime.generation [slot] == UINT32_MAX);
	TEST_ASSERT(ctx->nodes.lifetime.alive [slot] == 0u);

	for (uint32_t i = 0u; i < ctx->free_count; i++) TEST_ASSERT(ctx->free_indices [i] != slot);

	XentNodeId next = xent_node_create(ctx);
	TEST_ASSERT(next != XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_index(next) != slot);
	TEST_ASSERT(xent_node_destroy(ctx, next));

	/* Ancient gen-1 handle for the retired slot must stay dead. */
	XentNodeId ancient = xent_make_node_id(slot, 1u);
	TEST_ASSERT(!xent_node_valid(ctx, ancient));
	return 0;
}

int main(void) {
	XentCfg bad_cap = {.initial_capacity = UINT32_MAX};
	TEST_ASSERT(xent_ctx_create(&bad_cap) == NULL);
	bad_cap.initial_capacity = UINT32_MAX - 1u;
	TEST_ASSERT(xent_ctx_create(&bad_cap) == NULL);

	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);
	TEST_ASSERT(!xent_node_reserve(NULL, 1u));
	TEST_ASSERT(xent_node_reserve(ctx, 8u));
	XentObsId observer = xent_node_addobs(ctx, &(XentNodeObs) {.notify = test_lifecycle});
	XentObsId second   = xent_node_addobs(ctx, &(XentNodeObs) {.notify = test_second_lifecycle});
	TEST_ASSERT(observer != XENT_NODE_OBSERVER_INVALID);
	TEST_ASSERT(second != XENT_NODE_OBSERVER_INVALID);
	TEST_ASSERT(xent_node_addobs(ctx, NULL) == XENT_NODE_OBSERVER_INVALID);

	SelfRemovingObserver self_removing = {0};
	self_removing.id = xent_node_addobs(ctx, &(XentNodeObs) {.notify = test_self_removal, .userdata = &self_removing});
	TEST_ASSERT(self_removing.id != XENT_NODE_OBSERVER_INVALID);

	XentNodeId root = xent_node_create(ctx);
	XentNodeId a    = xent_node_create(ctx);
	XentNodeId b    = xent_node_create(ctx);
	XentNodeId c    = xent_node_create(ctx);
	TEST_ASSERT(
	  root != XENT_NODE_INVALID && a != XENT_NODE_INVALID && b != XENT_NODE_INVALID && c != XENT_NODE_INVALID
	);

	TEST_ASSERT(xent_node_append(ctx, root, a));
	TEST_ASSERT(self_removing.calls == 1u);
	TEST_ASSERT(self_removing.removed);
	TEST_ASSERT(self_removing.mutation_rejected);
	TEST_ASSERT(xent_node_append(ctx, root, b));
	TEST_ASSERT(xent_node_nchild(ctx, root) == 2u);
	TEST_ASSERT(xent_node_first(ctx, root) == a);
	TEST_ASSERT(xent_node_last(ctx, root) == b);
	TEST_ASSERT(xent_node_next(ctx, a) == b);
	TEST_ASSERT(xent_node_prev(ctx, b) == a);
	TEST_ASSERT(xent_node_parent(ctx, a) == root);
	TEST_ASSERT(xent_node_parent(ctx, b) == root);

	TEST_ASSERT(xent_node_append(ctx, a, c));
	TEST_ASSERT(xent_node_parent(ctx, c) == a);

	uint32_t reparent_before = lifecycle_reparent_count;
	TEST_ASSERT(xent_node_append(ctx, root, c));
	TEST_ASSERT(lifecycle_reparent_count == reparent_before + 1u);
	TEST_ASSERT(xent_node_parent(ctx, c) == root);
	TEST_ASSERT(xent_node_nchild(ctx, a) == 0u);
	TEST_ASSERT(xent_node_nchild(ctx, root) == 3u);

	TEST_ASSERT(xent_node_remove(ctx, root, b));
	TEST_ASSERT(xent_node_parent(ctx, b) == XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_prev(ctx, b) == XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_next(ctx, a) == c);
	TEST_ASSERT(xent_node_prev(ctx, c) == a);
	TEST_ASSERT(xent_node_last(ctx, root) == c);
	TEST_ASSERT(xent_node_nchild(ctx, root) == 2u);

	TEST_ASSERT(xent_sem_checked(ctx, a) == 0u);
	TEST_ASSERT(xent_sem_setchecked(ctx, a, 1));
	TEST_ASSERT(xent_sem_checked(ctx, a) == 1u);
	TEST_ASSERT(xent_sem_setchecked(ctx, a, 2));
	TEST_ASSERT(xent_sem_checked(ctx, a) == 2u);

	TEST_ASSERT(xent_sem_enabled(ctx, a) == true);
	TEST_ASSERT(xent_sem_setenabled(ctx, a, false));
	TEST_ASSERT(xent_sem_enabled(ctx, a) == false);
	TEST_ASSERT(xent_sem_setenabled(ctx, a, true));
	TEST_ASSERT(xent_sem_enabled(ctx, a) == true);

	TEST_ASSERT(xent_sem_expanded(ctx, a) == false);
	TEST_ASSERT(xent_sem_setexpanded(ctx, a, true));
	TEST_ASSERT(xent_sem_expanded(ctx, a) == true);

	TEST_ASSERT(xent_sem_selected(ctx, a) == false);
	TEST_ASSERT(xent_sem_setselected(ctx, a, true));
	TEST_ASSERT(xent_sem_selected(ctx, a) == true);

	float val, vmin, vmax;
	TEST_ASSERT(xent_sem_value(ctx, a, &val, &vmin, &vmax));
	TEST_ASSERT(val == 0.0f && vmin == 0.0f && vmax == 0.0f);
	TEST_ASSERT(xent_sem_setvalue(ctx, a, 0.5f, 0.0f, 1.0f));
	TEST_ASSERT(xent_sem_value(ctx, a, &val, &vmin, &vmax));
	TEST_ASSERT(test_float_near(val, 0.5f, 1e-6f));
	TEST_ASSERT(test_float_near(vmin, 0.0f, 1e-6f));
	TEST_ASSERT(test_float_near(vmax, 1.0f, 1e-6f));
	TEST_ASSERT(!xent_sem_value(ctx, XENT_NODE_INVALID, &val, &vmin, &vmax));
	TEST_ASSERT(xent_sem_value(ctx, a, NULL, NULL, NULL));

	XentNodeId temporary = xent_node_create(ctx);
	TEST_ASSERT(temporary != XENT_NODE_INVALID);
	last_lifecycle_node = XENT_NODE_INVALID;
	TEST_ASSERT(xent_node_destroy(ctx, temporary));
	TEST_ASSERT(lifecycle_destroy_count == 1u);
	TEST_ASSERT(lifecycle_second_destroy_count == 1u);
	TEST_ASSERT(last_lifecycle_node == temporary);
	TEST_ASSERT(!xent_node_valid(ctx, temporary));
	TEST_ASSERT(xent_node_delobs(ctx, second));
	TEST_ASSERT(!xent_node_delobs(ctx, second));

	TEST_ASSERT(xent_sem_setchecked(ctx, c, 1));
	TEST_ASSERT(xent_node_destroy(ctx, c));
	TEST_ASSERT(lifecycle_destroy_count == 2u);
	TEST_ASSERT(lifecycle_second_destroy_count == 1u);
	XentNodeId d = xent_node_create(ctx);
	TEST_ASSERT(d != XENT_NODE_INVALID);
	TEST_ASSERT(xent_sem_checked(ctx, d) == 0u);
	TEST_ASSERT(xent_sem_enabled(ctx, d) == true);

	if (test_generation_handles(ctx)) return 1;
	if (test_subtree_stale_handles(ctx)) return 1;
	if (test_capacity_boundary(ctx)) return 1;
	if (test_generation_wrap_retires_slot(ctx)) return 1;

	TEST_ASSERT(xent_node_delobs(ctx, observer));
	xent_ctx_destroy(ctx);
	return 0;
}
