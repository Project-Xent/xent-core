#include "../xent_internal.h"

typedef struct TraversalFrame {
	XentTraversalVisit visit;
	XentNodeId         current_child;
	bool               entered;
	bool               skip_children;
} TraversalFrame;

typedef struct TraversalStack {
	TraversalFrame *items;
	uint32_t        top;
	uint32_t        capacity;
} TraversalStack;

typedef struct TraversalPush {
	XentContext const          *ctx;
	TraversalStack             *stack;
	XentNodeId                  node;
	XentTraversalVisit const   *parent;
	XentTraversalOptions const *options;
} TraversalPush;

static XentRect xent_screen_rect(XentRect rect, float scroll_x, float scroll_y) {
	rect.x -= scroll_x;
	rect.y -= scroll_y;
	return rect;
}

static bool xent_rect_intersects(XentRect a, XentRect b) {
	if (a.x + a.width <= b.x) return false;
	if (b.x + b.width <= a.x) return false;
	if (a.y + a.height <= b.y) return false;
	return b.y + b.height > a.y;
}

static XentRect xent_rect_intersection(XentRect a, XentRect b) {
	float ax1 = a.x + a.width;
	float ay1 = a.y + a.height;
	float bx1 = b.x + b.width;
	float by1 = b.y + b.height;
	float x0  = a.x > b.x ? a.x : b.x;
	float y0  = a.y > b.y ? a.y : b.y;
	float x1  = ax1 < bx1 ? ax1 : bx1;
	float y1  = ay1 < by1 ? ay1 : by1;
	if (x1 < x0) x1 = x0;
	if (y1 < y0) y1 = y0;
	return (XentRect) {x0, y0, x1 - x0, y1 - y0};
}

static XentRect xent_root_clip(void) {
	float extent = 16777216.0f;
	return (XentRect) {-extent, -extent, extent * 2.0f, extent * 2.0f};
}

static XentNodeId xent_first_child_for_order(XentContext const *ctx, XentNodeId node, XentChildOrder order) {
	if (order == XENT_CHILD_ORDER_Z_ASC || order == XENT_CHILD_ORDER_Z_DESC) {
		XentNodeId best = XENT_NODE_INVALID;
		for (XentNodeId child = xent_get_first_child(ctx, node); child != XENT_NODE_INVALID;
		  child               = xent_get_next_sibling(ctx, child))
		{
			if (best == XENT_NODE_INVALID) {
				best = child;
				continue;
			}
			int32_t z_child = ctx->nodes.layout.z_index [child];
			int32_t z_best  = ctx->nodes.layout.z_index [best];
			if ((order == XENT_CHILD_ORDER_Z_ASC && z_child < z_best)
				|| (order == XENT_CHILD_ORDER_Z_DESC && z_child > z_best))
			{
				best = child;
			}
		}
		return best;
	}
	if (order == XENT_CHILD_ORDER_REVERSE) return xent_get_last_child(ctx, node);
	return xent_get_first_child(ctx, node);
}

static XentNodeId xent_next_child_for_order(XentContext const *ctx, XentNodeId node, XentChildOrder order) {
	if (order == XENT_CHILD_ORDER_Z_ASC || order == XENT_CHILD_ORDER_Z_DESC) {
		XentNodeId parent = xent_get_parent(ctx, node);
		if (parent == XENT_NODE_INVALID) return XENT_NODE_INVALID;

		XentNodeId best       = XENT_NODE_INVALID;
		int32_t    node_z     = ctx->nodes.layout.z_index [node];
		bool       after_node = false;
		for (XentNodeId child = xent_get_first_child(ctx, parent); child != XENT_NODE_INVALID;
		  child               = xent_get_next_sibling(ctx, child))
		{
			if (child == node) {
				after_node = true;
				continue;
			}

			int32_t child_z   = ctx->nodes.layout.z_index [child];
			bool    candidate = false;
			if (order == XENT_CHILD_ORDER_Z_ASC) candidate = child_z > node_z || (child_z == node_z && after_node);
			else candidate = child_z < node_z || (child_z == node_z && after_node);
			if (!candidate) continue;

			if (best == XENT_NODE_INVALID) {
				best = child;
				continue;
			}

			int32_t best_z = ctx->nodes.layout.z_index [best];
			if ((order == XENT_CHILD_ORDER_Z_ASC && child_z < best_z)
				|| (order == XENT_CHILD_ORDER_Z_DESC && child_z > best_z))
			{
				best = child;
			}
		}
		return best;
	}
	if (order == XENT_CHILD_ORDER_REVERSE) return xent_get_prev_sibling(ctx, node);
	return xent_get_next_sibling(ctx, node);
}

static bool xent_ensure_traversal_capacity(TraversalStack *stack) {
	if (stack->top < stack->capacity) return true;
	uint32_t        new_capacity = stack->capacity ? stack->capacity * 2u : 64u;
	TraversalFrame *new_items
	  = ( TraversalFrame * ) realloc(stack->items, sizeof(TraversalFrame) * ( size_t ) new_capacity);
	if (!new_items) return false;
	stack->items    = new_items;
	stack->capacity = new_capacity;
	return true;
}

static void
xent_init_visit_parent(XentTraversalVisit *visit, XentNodeId node, XentRect layout, XentTraversalVisit const *parent) {
	visit->node           = node;
	visit->parent         = XENT_NODE_INVALID;
	visit->layout_rect    = layout;
	visit->screen_rect    = layout;
	visit->effective_clip = xent_root_clip();

	if (!parent) return;

	visit->parent               = parent->node;
	visit->accumulated_scroll_x = parent->accumulated_scroll_x + parent->effects.child_scroll_x;
	visit->accumulated_scroll_y = parent->accumulated_scroll_y + parent->effects.child_scroll_y;
	visit->screen_rect          = xent_screen_rect(layout, visit->accumulated_scroll_x, visit->accumulated_scroll_y);
	visit->effective_clip       = parent->effective_clip;
	visit->depth                = parent->depth + 1u;
}

static void xent_apply_traversal_effects(XentTraversalVisit *visit, XentTraversalOptions const *options) {
	if (options->effects) ( void ) options->effects(visit, &visit->effects, options->effects_userdata);
	if (visit->effects.clips_children)
		visit->effective_clip = xent_rect_intersection(visit->effective_clip, visit->screen_rect);
}

static bool xent_push_traversal_frame(TraversalPush const *push) {
	if (!xent_ensure_traversal_capacity(push->stack)) return false;

	XentRect layout = {0};
	if (!xent_get_layout_rect(push->ctx, push->node, &layout)) return false;

	TraversalFrame *frame = &push->stack->items [push->stack->top++];
	memset(frame, 0, sizeof(*frame));
	xent_init_visit_parent(&frame->visit, push->node, layout, push->parent);
	xent_apply_traversal_effects(&frame->visit, push->options);
	frame->current_child = xent_first_child_for_order(push->ctx, push->node, push->options->child_order);
	return true;
}

static void xent_enter_traversal_frame(TraversalFrame *frame, XentTraversalOptions const *options, bool *stopped) {
	if (frame->entered) return;

	frame->entered = true;
	if (options->cull_to_clip && !xent_rect_intersects(frame->visit.screen_rect, frame->visit.effective_clip))
		frame->skip_children = true;
	if (!options->enter || frame->skip_children) return;

	XentTraversalAction action = options->enter(&frame->visit, options->visit_userdata);
	if (action == XENT_TRAVERSAL_STOP) {
		*stopped = true;
		return;
	}
	frame->skip_children = action == XENT_TRAVERSAL_SKIP_CHILDREN;
}

static bool xent_push_current_child(
  XentContext const *ctx, TraversalFrame *frame, XentTraversalOptions const *options, TraversalPush *push, bool *pushed
) {
	*pushed = false;
	if (frame->skip_children || frame->current_child == XENT_NODE_INVALID) return true;

	XentNodeId child     = frame->current_child;
	frame->current_child = xent_next_child_for_order(ctx, child, options->child_order);
	push->node           = child;
	push->parent         = &frame->visit;
	*pushed              = true;
	return xent_push_traversal_frame(push);
}

static void xent_leave_traversal_frame(TraversalFrame *frame, XentTraversalOptions const *options, bool *stopped) {
	if (!options->leave) return;

	XentTraversalAction action = options->leave(&frame->visit, options->visit_userdata);
	if (action == XENT_TRAVERSAL_STOP) *stopped = true;
}

bool xent_traverse_layout(XentContext const *ctx, XentNodeId root, XentTraversalOptions const *options) {
	if (!xent_is_valid_node(ctx, root) || !options || (!options->enter && !options->leave)) return false;

	TraversalStack stack   = {0};
	bool           stopped = false;
	TraversalPush  push    = {ctx, &stack, root, NULL, options};
	bool           ok      = xent_push_traversal_frame(&push);

	while (ok && !stopped && stack.top > 0u) {
		TraversalFrame *frame  = &stack.items [stack.top - 1u];
		bool            pushed = false;
		xent_enter_traversal_frame(frame, options, &stopped);
		if (stopped) break;
		ok = xent_push_current_child(ctx, frame, options, &push, &pushed);
		if (pushed) continue;
		xent_leave_traversal_frame(frame, options, &stopped);
		stack.top--;
	}

	free(stack.items);
	return ok;
}
