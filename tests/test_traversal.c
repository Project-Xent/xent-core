#include "test_common.h"

typedef struct TraversalSeen {
	XentNodeId order [8];
	uint32_t   count;
	float      child_scroll_x;
} TraversalSeen;

static bool traversal_effects(XentTraversalVisit const *visit, XentTraversalEffects *effects, void *userdata) {
	( void ) userdata;
	if (visit->depth == 0u) {
		effects->clips_children = true;
		effects->child_scroll_x = 20.0f;
	}
	return true;
}

static XentTraversalAction traversal_enter(XentTraversalVisit const *visit, void *userdata) {
	TraversalSeen *seen         = ( TraversalSeen * ) userdata;
	seen->order [seen->count++] = visit->node;
	if (visit->depth == 1u) seen->child_scroll_x = visit->accumulated_scroll_x;
	return XENT_TRAVERSAL_CONTINUE;
}

int main(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root = xent_create_node(ctx);
	XentNodeId a    = xent_create_node(ctx);
	XentNodeId b    = xent_create_node(ctx);
	TEST_ASSERT(root != XENT_NODE_INVALID && a != XENT_NODE_INVALID && b != XENT_NODE_INVALID);
	TEST_ASSERT(xent_append_child(ctx, root, a));
	TEST_ASSERT(xent_append_child(ctx, root, b));
	TEST_ASSERT(xent_set_size(ctx, root, (XentSize) {100.0f, 100.0f}));
	TEST_ASSERT(xent_set_size(ctx, a, (XentSize) {10.0f, 10.0f}));
	TEST_ASSERT(xent_set_size(ctx, b, (XentSize) {10.0f, 10.0f}));
	TEST_ASSERT(xent_set_absolute_position(ctx, a, (XentPoint) {30.0f, 0.0f}));
	TEST_ASSERT(xent_set_absolute_position(ctx, b, (XentPoint) {40.0f, 0.0f}));
	TEST_ASSERT(xent_layout(ctx, root, 100.0f, 100.0f));

	TraversalSeen        seen    = {0};
	XentTraversalOptions options = {
	  .child_order    = XENT_CHILD_ORDER_REVERSE,
	  .cull_to_clip   = true,
	  .effects        = traversal_effects,
	  .enter          = traversal_enter,
	  .visit_userdata = &seen,
	};
	TEST_ASSERT(xent_traverse_layout(ctx, root, &options));
	TEST_ASSERT(seen.count == 3u);
	TEST_ASSERT(seen.order [0] == root);
	TEST_ASSERT(seen.order [1] == b);
	TEST_ASSERT(seen.order [2] == a);
	TEST_ASSERT(test_float_near(seen.child_scroll_x, 20.0f, 1e-6f));

	xent_destroy_context(ctx);
	return 0;
}
