#include "test_common.h"

typedef struct ChildSnap {
	XentNodeId first;
	XentNodeId last;
	XentNodeId prev;
	XentNodeId next;
	XentNodeId parent;
	uint32_t   count;
	uint32_t   dirty;
} ChildSnap;

static ChildSnap snap_node(XentCtx const *ctx, XentNodeId node) {
	return (ChildSnap) {
	  .first  = xent_node_first(ctx, node),
	  .last   = xent_node_last(ctx, node),
	  .prev   = xent_node_prev(ctx, node),
	  .next   = xent_node_next(ctx, node),
	  .parent = xent_node_parent(ctx, node),
	  .count  = xent_node_nchild(ctx, node),
	  .dirty  = xent_node_dirty(ctx, node),
	};
}

static int assert_order(XentCtx const *ctx, XentNodeId parent, XentNodeId const *order, uint32_t count) {
	TEST_ASSERT(xent_node_nchild(ctx, parent) == count);
	if (count == 0u) {
		TEST_ASSERT(xent_node_first(ctx, parent) == XENT_NODE_INVALID);
		TEST_ASSERT(xent_node_last(ctx, parent) == XENT_NODE_INVALID);
		return 0;
	}
	TEST_ASSERT(xent_node_first(ctx, parent) == order [0]);
	TEST_ASSERT(xent_node_last(ctx, parent) == order [count - 1u]);
	XentNodeId cursor = order [0];
	for (uint32_t i = 0u; i < count; i++) {
		TEST_ASSERT(cursor == order [i]);
		TEST_ASSERT(xent_node_parent(ctx, cursor) == parent);
		TEST_ASSERT(xent_node_prev(ctx, cursor) == (i == 0u ? XENT_NODE_INVALID : order [i - 1u]));
		TEST_ASSERT(xent_node_next(ctx, cursor) == (i + 1u == count ? XENT_NODE_INVALID : order [i + 1u]));
		cursor = xent_node_next(ctx, cursor);
	}
	TEST_ASSERT(cursor == XENT_NODE_INVALID);
	return 0;
}

static int assert_snap_eq(ChildSnap const *a, ChildSnap const *b) {
	TEST_ASSERT(a->first == b->first);
	TEST_ASSERT(a->last == b->last);
	TEST_ASSERT(a->prev == b->prev);
	TEST_ASSERT(a->next == b->next);
	TEST_ASSERT(a->parent == b->parent);
	TEST_ASSERT(a->count == b->count);
	return 0;
}

static int test_insert_positions(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx);
	XentNodeId parent = xent_node_create(ctx);
	XentNodeId a      = xent_node_create(ctx);
	XentNodeId b      = xent_node_create(ctx);
	XentNodeId c      = xent_node_create(ctx);
	TEST_ASSERT(parent && a && b && c);

	TEST_ASSERT(xent_node_insert(ctx, parent, b, XENT_NODE_INVALID));
	TEST_ASSERT(xent_node_insert(ctx, parent, a, b));
	TEST_ASSERT(xent_node_insert(ctx, parent, c, XENT_NODE_INVALID));
	XentNodeId order [] = {a, b, c};
	if (assert_order(ctx, parent, order, 3u)) return 1;
	TEST_ASSERT(xent_node_dirty(ctx, parent) & XENT_DIRTY_LAYOUT);

	TEST_ASSERT(!xent_node_insert(ctx, parent, a, c));
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_move_and_noop(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx);
	XentNodeId parent = xent_node_create(ctx);
	XentNodeId a      = xent_node_create(ctx);
	XentNodeId b      = xent_node_create(ctx);
	XentNodeId c      = xent_node_create(ctx);
	TEST_ASSERT(xent_node_append(ctx, parent, a));
	TEST_ASSERT(xent_node_append(ctx, parent, b));
	TEST_ASSERT(xent_node_append(ctx, parent, c));

	TEST_ASSERT(xent_node_move(ctx, parent, c, b));
	XentNodeId mid [] = {a, c, b};
	if (assert_order(ctx, parent, mid, 3u)) return 1;

	TEST_ASSERT(xent_node_move(ctx, parent, a, XENT_NODE_INVALID));
	XentNodeId tail [] = {c, b, a};
	if (assert_order(ctx, parent, tail, 3u)) return 1;

	TEST_ASSERT(xent_node_move(ctx, parent, b, a));
	TEST_ASSERT(xent_node_move(ctx, parent, b, a));
	TEST_ASSERT(xent_node_move(ctx, parent, b, b));
	XentNodeId stable [] = {c, b, a};
	if (assert_order(ctx, parent, stable, 3u)) return 1;

	XentNodeId orphan = xent_node_create(ctx);
	TEST_ASSERT(!xent_node_move(ctx, parent, orphan, a));
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_reparent(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx);
	XentNodeId left  = xent_node_create(ctx);
	XentNodeId right = xent_node_create(ctx);
	XentNodeId a     = xent_node_create(ctx);
	XentNodeId b     = xent_node_create(ctx);
	XentNodeId c     = xent_node_create(ctx);
	TEST_ASSERT(xent_node_append(ctx, left, a));
	TEST_ASSERT(xent_node_append(ctx, left, b));
	TEST_ASSERT(xent_node_append(ctx, right, c));

	TEST_ASSERT(xent_node_reparent(ctx, right, b, c));
	XentNodeId left_order []  = {a};
	XentNodeId right_order [] = {b, c};
	if (assert_order(ctx, left, left_order, 1u)) return 1;
	if (assert_order(ctx, right, right_order, 2u)) return 1;
	TEST_ASSERT(xent_node_dirty(ctx, left) & XENT_DIRTY_LAYOUT);
	TEST_ASSERT(xent_node_dirty(ctx, right) & XENT_DIRTY_LAYOUT);

	TEST_ASSERT(!xent_node_reparent(ctx, right, b, XENT_NODE_INVALID));
	XentNodeId detached = xent_node_create(ctx);
	TEST_ASSERT(xent_node_reparent(ctx, left, detached, a));
	XentNodeId left2 [] = {detached, a};
	if (assert_order(ctx, left, left2, 2u)) return 1;
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_rejects_preserve_topology(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx);
	XentNodeId root  = xent_node_create(ctx);
	XentNodeId child = xent_node_create(ctx);
	XentNodeId grand = xent_node_create(ctx);
	XentNodeId other = xent_node_create(ctx);
	TEST_ASSERT(xent_node_append(ctx, root, child));
	TEST_ASSERT(xent_node_append(ctx, child, grand));
	TEST_ASSERT(xent_node_append(ctx, root, other));

	ChildSnap root_before  = snap_node(ctx, root);
	ChildSnap child_before = snap_node(ctx, child);
	ChildSnap grand_before = snap_node(ctx, grand);

	TEST_ASSERT(!xent_node_insert(ctx, grand, root, XENT_NODE_INVALID));
	TEST_ASSERT(!xent_node_reparent(ctx, grand, root, XENT_NODE_INVALID));
	TEST_ASSERT(!xent_node_move(ctx, root, grand, other));
	TEST_ASSERT(!xent_node_insert(ctx, root, other, child));
	TEST_ASSERT(!xent_node_reparent(ctx, root, other, child));

	XentNodeId dead = xent_node_create(ctx);
	TEST_ASSERT(xent_node_destroy(ctx, dead));
	XentNodeId fresh = xent_node_create(ctx);
	TEST_ASSERT(!xent_node_insert(ctx, root, fresh, dead));
	TEST_ASSERT(xent_node_parent(ctx, fresh) == XENT_NODE_INVALID);

	ChildSnap root_after  = snap_node(ctx, root);
	ChildSnap child_after = snap_node(ctx, child);
	ChildSnap grand_after = snap_node(ctx, grand);
	if (assert_snap_eq(&root_before, &root_after)) return 1;
	if (assert_snap_eq(&child_before, &child_after)) return 1;
	if (assert_snap_eq(&grand_before, &grand_after)) return 1;
	XentNodeId order [] = {child, other};
	if (assert_order(ctx, root, order, 2u)) return 1;
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_cycle_and_stale(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx);
	XentNodeId parent = xent_node_create(ctx);
	XentNodeId child  = xent_node_create(ctx);
	TEST_ASSERT(xent_node_append(ctx, parent, child));
	XentNodeId stale = child;
	TEST_ASSERT(xent_node_destroy(ctx, child));
	XentNodeId neu = xent_node_create(ctx);
	TEST_ASSERT(!xent_node_insert(ctx, parent, neu, stale));
	TEST_ASSERT(!xent_node_move(ctx, parent, stale, XENT_NODE_INVALID));
	TEST_ASSERT(!xent_node_reparent(ctx, parent, stale, XENT_NODE_INVALID));
	TEST_ASSERT(xent_node_nchild(ctx, parent) == 0u);
	xent_ctx_destroy(ctx);
	return 0;
}

int main(void) {
	XentTestFn const tests [] = {
	  test_insert_positions,
	  test_move_and_noop,
	  test_reparent,
	  test_rejects_preserve_topology,
	  test_cycle_and_stale,
	};
	if (test_run_all(tests, sizeof(tests) / sizeof(tests [0]))) return 1;
	printf("PASS: ordered topology\n");
	return 0;
}
