#include "../xent_internal.h"

bool xent_is_valid_node(XentContext const *ctx, XentNodeId node) {
	if (!ctx || node == XENT_NODE_INVALID || node >= ctx->nodes.capacity) return false;
	return ctx->nodes.lifetime.alive [node] != 0u;
}

static bool xent_push_free_id(XentContext *ctx, XentNodeId node) {
	if (ctx->free_count == ctx->free_capacity) {
		uint32_t    new_cap = ctx->free_capacity ? ctx->free_capacity * 2u : 64u;
		XentNodeId *new_mem = ( XentNodeId * ) realloc(ctx->free_ids, sizeof(XentNodeId) * ( size_t ) new_cap);
		if (!new_mem) return false;
		ctx->free_ids      = new_mem;
		ctx->free_capacity = new_cap;
	}
	ctx->free_ids [ctx->free_count++] = node;
	return true;
}

static void xent_free_grid_def(XentGridDef *def) {
	if (!def) return;
	free(def->row_modes);
	free(def->row_values);
	free(def->col_modes);
	free(def->col_values);
	free(def);
}

XentNodeId xent_create_node(XentContext *ctx) {
	if (!ctx) return XENT_NODE_INVALID;

	XentNodeId id = XENT_NODE_INVALID;
	if (ctx->free_count > 0u) { id = ctx->free_ids [--ctx->free_count]; }
	else {
		id = ++ctx->nodes.count;
		if (!xent_ensure_node_capacity(ctx, id + 1u)) {
			ctx->nodes.count--;
			return XENT_NODE_INVALID;
		}
	}

	ctx->nodes.lifetime.alive [id]         = 1u;
	ctx->nodes.topology.parent [id]        = XENT_NODE_INVALID;
	ctx->nodes.topology.first_child [id]   = XENT_NODE_INVALID;
	ctx->nodes.topology.last_child [id]    = XENT_NODE_INVALID;
	ctx->nodes.topology.next_sibling [id]  = XENT_NODE_INVALID;
	ctx->nodes.topology.prev_sibling [id]  = XENT_NODE_INVALID;
	ctx->nodes.topology.child_count [id]   = 0u;

	ctx->nodes.layout.protocol [id]        = ( uint8_t ) XENT_PROTOCOL_ABSOLUTE;
	ctx->nodes.layout.direction [id]       = ( uint8_t ) XENT_DIRECTION_INHERIT;
	ctx->nodes.layout.dirty_flags [id]     = XENT_DIRTY_NONE;

	ctx->nodes.layout.style_w [id]         = NAN;
	ctx->nodes.layout.style_h [id]         = NAN;
	ctx->nodes.layout.style_w_percent [id] = NAN;
	ctx->nodes.layout.style_h_percent [id] = NAN;
	ctx->nodes.layout.aspect_ratio [id]    = NAN;
	ctx->nodes.layout.min_w [id]           = 0.0f;
	ctx->nodes.layout.min_h [id]           = 0.0f;
	ctx->nodes.layout.max_w [id]           = INFINITY;
	ctx->nodes.layout.max_h [id]           = INFINITY;

	ctx->nodes.flex.grow [id]              = 0.0f;
	ctx->nodes.flex.shrink [id]            = 1.0f;
	ctx->nodes.flex.basis [id]             = NAN;
	ctx->nodes.flex.direction [id]         = ( uint8_t ) XENT_FLEX_ROW;
	ctx->nodes.flex.wrap [id]              = ( uint8_t ) XENT_FLEX_NO_WRAP;
	ctx->nodes.flex.justify_content [id]   = ( uint8_t ) XENT_FLEX_JUSTIFY_START;
	ctx->nodes.flex.align_items [id]       = ( uint8_t ) XENT_FLEX_ALIGN_START;
	ctx->nodes.flex.align_self [id]        = ( uint8_t ) XENT_FLEX_ALIGN_AUTO;
	ctx->nodes.flex.align_content [id]     = ( uint8_t ) XENT_FLEX_ALIGN_CONTENT_START;

	ctx->nodes.stack.axis [id]             = ( uint8_t ) XENT_AXIS_HORIZONTAL;
	ctx->nodes.stack.align [id]            = ( uint8_t ) XENT_STACK_ALIGN_START;
	ctx->nodes.stack.priority [id]         = 0.0f;
	ctx->nodes.stack.spacer [id]           = 0u;

	ctx->nodes.layout.abs_pos_x [id]       = 0.0f;
	ctx->nodes.layout.abs_pos_y [id]       = 0.0f;
	ctx->nodes.layout.gap [id]             = 0.0f;
	ctx->nodes.layout.z_index [id]         = 0;

	if (ctx->nodes.text.content [id]) {
		free(ctx->nodes.text.content [id]);
		ctx->nodes.text.content [id] = NULL;
	}
	ctx->nodes.text.font_size [id]                   = 14.0f;
	ctx->nodes.text.line_break_policy [id]           = ( uint8_t ) XENT_LINE_BREAK_CHAR_WRAP;
	ctx->nodes.text.intrinsic_valid [id]             = 0u;
	ctx->nodes.text.intrinsic_constraint_w [id]      = NAN;
	ctx->nodes.text.intrinsic_font_size [id]         = 0.0f;
	ctx->nodes.text.intrinsic_line_break_policy [id] = ( uint8_t ) XENT_LINE_BREAK_CHAR_WRAP;
	ctx->nodes.text.intrinsic_width_mode [id]        = ( uint8_t ) XENT_MEASURE_UNDEFINED;
	ctx->nodes.text.intrinsic_w [id]                 = 0.0f;
	ctx->nodes.text.intrinsic_h [id]                 = 0.0f;
	ctx->nodes.text.intrinsic_lines [id]             = 0u;

	ctx->nodes.semantics.role [id]                   = ( uint8_t ) XENT_SEMANTIC_NONE;
	if (ctx->nodes.semantics.label [id]) {
		free(ctx->nodes.semantics.label [id]);
		ctx->nodes.semantics.label [id] = NULL;
	}
	ctx->nodes.semantics.flags [id]                   = 0u;

	ctx->nodes.external.userdata [id]                 = NULL;
	ctx->nodes.external.payload [id]                  = NULL;
	ctx->nodes.external.payload_type [id]             = 0u;
	ctx->nodes.external.payload_destroy [id]          = NULL;
	ctx->nodes.external.payload_destroy_userdata [id] = NULL;
	ctx->nodes.external.control_type [id]             = ( uint8_t ) XENT_CONTROL_CONTAINER;
	ctx->nodes.semantics.checked [id]                 = 0u;
	ctx->nodes.semantics.enabled [id]                 = 1u;
	ctx->nodes.semantics.expanded [id]                = 0u;
	ctx->nodes.semantics.selected [id]                = 0u;
	ctx->nodes.semantics.value_now [id]               = 0.0f;
	ctx->nodes.semantics.value_min [id]               = 0.0f;
	ctx->nodes.semantics.value_max [id]               = 0.0f;

	if (ctx->nodes.grid.def [id]) {
		xent_free_grid_def(ctx->nodes.grid.def [id]);
		ctx->nodes.grid.def [id] = NULL;
	}
	ctx->nodes.grid.row [id]         = 0;
	ctx->nodes.grid.column [id]      = 0;
	ctx->nodes.grid.row_span [id]    = 1;
	ctx->nodes.grid.column_span [id] = 1;
	xent_mark_dirty(ctx, id, XENT_DIRTY_LAYOUT);

	return id;
}

static void xent_destroy_single_node(XentContext *ctx, XentNodeId node) {
	free(ctx->nodes.text.content [node]);
	ctx->nodes.text.content [node]         = NULL;
	ctx->nodes.text.intrinsic_valid [node] = 0u;
	free(ctx->nodes.semantics.label [node]);
	ctx->nodes.semantics.label [node]   = NULL;
	ctx->nodes.external.userdata [node] = NULL;
	if (ctx->nodes.external.payload_destroy [node] && ctx->nodes.external.payload [node])
		ctx->nodes.external.payload_destroy [node](
		  ctx->nodes.external.payload [node], ctx->nodes.external.payload_destroy_userdata [node]
		);
	ctx->nodes.external.payload [node]                  = NULL;
	ctx->nodes.external.payload_type [node]             = 0u;
	ctx->nodes.external.payload_destroy [node]          = NULL;
	ctx->nodes.external.payload_destroy_userdata [node] = NULL;

	xent_free_grid_def(ctx->nodes.grid.def [node]);
	ctx->nodes.grid.def [node]       = NULL;

	ctx->nodes.lifetime.alive [node] = 0u;
	if (ctx->node_lifecycle) {
		ctx->node_lifecycle(
		  ctx, node, XENT_NODE_EVENT_DESTROY, ctx->nodes.topology.parent [node], XENT_NODE_INVALID,
		  ctx->node_lifecycle_userdata
		);
	}
	ctx->nodes.topology.parent [node]       = XENT_NODE_INVALID;
	ctx->nodes.topology.first_child [node]  = XENT_NODE_INVALID;
	ctx->nodes.topology.last_child [node]   = XENT_NODE_INVALID;
	ctx->nodes.topology.next_sibling [node] = XENT_NODE_INVALID;
	ctx->nodes.topology.prev_sibling [node] = XENT_NODE_INVALID;
	ctx->nodes.topology.child_count [node]  = 0u;
	ctx->nodes.layout.dirty_flags [node]    = XENT_DIRTY_NONE;
	( void ) xent_push_free_id(ctx, node);
}

typedef struct DestroyFrame {
	XentNodeId node;
	bool       expanded;
} DestroyFrame;

static bool destroy_stack_push(DestroyFrame **stack, uint32_t *top, uint32_t *capacity, DestroyFrame frame) {
	if (*top == *capacity) {
		uint32_t      new_cap = *capacity ? *capacity * 2u : 64u;
		DestroyFrame *new_mem = ( DestroyFrame * ) realloc(*stack, sizeof(*new_mem) * ( size_t ) new_cap);
		if (!new_mem) return false;
		*stack    = new_mem;
		*capacity = new_cap;
	}
	(*stack) [(*top)++] = frame;
	return true;
}

static void xent_destroy_subtree(XentContext *ctx, XentNodeId root) {
	DestroyFrame *stack    = NULL;
	uint32_t      top      = 0u;
	uint32_t      capacity = 0u;
	if (!destroy_stack_push(&stack, &top, &capacity, (DestroyFrame) {root, false})) return;

	while (top > 0u) {
		DestroyFrame frame = stack [--top];
		if (frame.expanded) {
			xent_destroy_single_node(ctx, frame.node);
			continue;
		}

		if (!destroy_stack_push(&stack, &top, &capacity, (DestroyFrame) {frame.node, true})) break;
		for (XentNodeId child = ctx->nodes.topology.last_child [frame.node]; child != XENT_NODE_INVALID;
		  child               = ctx->nodes.topology.prev_sibling [child])
		{
			if (!destroy_stack_push(&stack, &top, &capacity, (DestroyFrame) {child, false})) {
				free(stack);
				return;
			}
		}
	}

	free(stack);
}

bool xent_destroy_node(XentContext *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return false;

	XentNodeId parent = ctx->nodes.topology.parent [node];
	if (parent != XENT_NODE_INVALID) ( void ) xent_remove_child(ctx, parent, node);

	xent_destroy_subtree(ctx, node);
	return true;
}

bool xent_append_child(XentContext *ctx, XentNodeId parent, XentNodeId child) {
	if (!xent_is_valid_node(ctx, parent) || !xent_is_valid_node(ctx, child) || parent == child) return false;

	XentNodeId cursor = parent;
	while (cursor != XENT_NODE_INVALID) {
		if (cursor == child) return false;
		cursor = ctx->nodes.topology.parent [cursor];
	}

	XentNodeId old_parent = ctx->nodes.topology.parent [child];
	if (old_parent != XENT_NODE_INVALID) ( void ) xent_remove_child(ctx, old_parent, child);

	if (ctx->nodes.topology.first_child [parent] == XENT_NODE_INVALID) {
		ctx->nodes.topology.first_child [parent] = child;
		ctx->nodes.topology.last_child [parent]  = child;
	}
	else {
		XentNodeId tail                          = ctx->nodes.topology.last_child [parent];
		ctx->nodes.topology.next_sibling [tail]  = child;
		ctx->nodes.topology.prev_sibling [child] = tail;
		ctx->nodes.topology.last_child [parent]  = child;
	}

	ctx->nodes.topology.parent [child]        = parent;
	ctx->nodes.topology.next_sibling [child]  = XENT_NODE_INVALID;
	ctx->nodes.topology.child_count [parent] += 1u;
	if (ctx->node_lifecycle)
		ctx->node_lifecycle(ctx, child, XENT_NODE_EVENT_REPARENT, old_parent, parent, ctx->node_lifecycle_userdata);

	xent_mark_dirty(ctx, parent, XENT_DIRTY_SUBTREE | XENT_DIRTY_LAYOUT);
	return true;
}

bool xent_remove_child(XentContext *ctx, XentNodeId parent, XentNodeId child) {
	if (!xent_is_valid_node(ctx, parent) || !xent_is_valid_node(ctx, child)) return false;

	XentNodeId cur = ctx->nodes.topology.first_child [parent];
	while (cur != XENT_NODE_INVALID) {
		if (cur == child) break;
		cur = ctx->nodes.topology.next_sibling [cur];
	}

	if (cur == XENT_NODE_INVALID) return false;

	XentNodeId prev = ctx->nodes.topology.prev_sibling [cur];
	XentNodeId next = ctx->nodes.topology.next_sibling [cur];
	if (prev == XENT_NODE_INVALID) ctx->nodes.topology.first_child [parent] = next;
	else ctx->nodes.topology.next_sibling [prev] = next;
	if (next == XENT_NODE_INVALID) ctx->nodes.topology.last_child [parent] = prev;
	else ctx->nodes.topology.prev_sibling [next] = prev;

	ctx->nodes.topology.next_sibling [cur] = XENT_NODE_INVALID;
	ctx->nodes.topology.prev_sibling [cur] = XENT_NODE_INVALID;
	ctx->nodes.topology.parent [cur]       = XENT_NODE_INVALID;
	if (ctx->nodes.topology.child_count [parent] > 0u) ctx->nodes.topology.child_count [parent] -= 1u;

	xent_mark_dirty(ctx, parent, XENT_DIRTY_SUBTREE | XENT_DIRTY_LAYOUT);
	if (ctx->node_lifecycle) {
		ctx->node_lifecycle(
		  ctx, child, XENT_NODE_EVENT_REPARENT, parent, XENT_NODE_INVALID, ctx->node_lifecycle_userdata
		);
	}
	return true;
}

XentNodeId xent_get_parent(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return XENT_NODE_INVALID;
	return ctx->nodes.topology.parent [node];
}

XentNodeId xent_get_first_child(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return XENT_NODE_INVALID;
	return ctx->nodes.topology.first_child [node];
}

XentNodeId xent_get_last_child(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return XENT_NODE_INVALID;
	return ctx->nodes.topology.last_child [node];
}

XentNodeId xent_get_next_sibling(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return XENT_NODE_INVALID;
	return ctx->nodes.topology.next_sibling [node];
}

XentNodeId xent_get_prev_sibling(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return XENT_NODE_INVALID;
	return ctx->nodes.topology.prev_sibling [node];
}

uint32_t xent_get_child_count(XentContext const *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return 0u;
	return ctx->nodes.topology.child_count [node];
}
