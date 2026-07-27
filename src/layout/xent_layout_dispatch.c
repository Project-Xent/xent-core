#include "../xent_internal.h"

#include "xent_layout_flex.h"

typedef struct XentDirtyPlan {
	XentNodeId        *direct_dirty;
	XentNodeId        *roots;
	XentNodeId        *stack;
	uint32_t           direct_count;
	uint32_t           root_count;
	uint32_t           affected_nodes;
	XentLayoutStrategy strategy;
} XentDirtyPlan;

void xent_layout_dispatch_node(XentLayoutRequest const *request) {
	XentCtx     *ctx      = request->ctx;
	XentNodeId   node     = request->node;
	XentProtocol protocol = ( XentProtocol ) ctx->nodes.layout.protocol [xent_node_index(node)];
	switch (protocol) {
	case XENT_PROTOCOL_FLEX       : xent_layout_node_flex(request); break;
	case XENT_PROTOCOL_SWIFTSTACK : xent_layout_node_swiftstack(request); break;
	case XENT_PROTOCOL_GRID       : xent_layout_node_grid(request); break;
	case XENT_PROTOCOL_ABSOLUTE   :
	default                       : xent_layout_node_absolute(request); break;
	}
}

static bool is_flow_layout_container(XentCtx const *ctx, XentNodeId node) {
	XentProtocol protocol = ( XentProtocol ) ctx->nodes.layout.protocol [xent_node_index(node)];
	return protocol == XENT_PROTOCOL_FLEX || protocol == XENT_PROTOCOL_SWIFTSTACK || protocol == XENT_PROTOCOL_GRID;
}

static bool is_auto_sized_parent(XentCtx const *ctx, XentNodeId node) {
	return (isnan(ctx->nodes.layout.style_w [xent_node_index(node)])
			 && isnan(ctx->nodes.layout.style_w_percent [xent_node_index(node)]))
	    || (isnan(ctx->nodes.layout.style_h [xent_node_index(node)]) && isnan(ctx->nodes.layout.style_h_percent [xent_node_index(node)]));
}

static bool can_promote_dirty_root(XentCtx const *ctx, XentNodeId node) {
	return is_flow_layout_container(ctx, node) && is_auto_sized_parent(ctx, node);
}

static XentNodeId promote_dirty_root(XentCtx const *ctx, XentNodeId dirty_node, XentNodeId root) {
	/* A node's geometry is decided by its PARENT's layout pass (absolute placement,
	 * flex distribution), and run_dirty_plan back-derives each recompute root's
	 * origin as abs_x - abs_pos_x — valid only while that root's own placement
	 * inputs are unchanged. The dirty node itself can therefore never be its own
	 * recompute root: when its abs_pos/size changed, the stale abs_x cancels the
	 * new offset and the node is re-laid in place (frozen layout animations).
	 * Always hop to the parent first; the flow climb below is unchanged. */
	XentNodeId candidate = dirty_node;
	XentNodeId parent    = ctx->nodes.topology.parent [xent_node_index(dirty_node)];
	if (parent != XENT_NODE_INVALID) candidate = parent;

	while (candidate != dirty_node && candidate != root && can_promote_dirty_root(ctx, candidate)) {
		parent = ctx->nodes.topology.parent [xent_node_index(candidate)];
		if (parent == XENT_NODE_INVALID || !is_flow_layout_container(ctx, parent)) break;
		candidate = parent;
	}
	return candidate;
}

static bool is_ancestor_of(XentCtx const *ctx, XentNodeId maybe_ancestor, XentNodeId node) {
	for (XentNodeId cursor = ctx->nodes.topology.parent [xent_node_index(node)]; cursor != XENT_NODE_INVALID;
	  cursor               = ctx->nodes.topology.parent [xent_node_index(cursor)])
		if (cursor == maybe_ancestor) return true;
	return false;
}

static bool is_in_subtree(XentCtx const *ctx, XentNodeId root, XentNodeId node) {
	if (root == node) return true;
	return is_ancestor_of(ctx, root, node);
}

static bool append_unique_root(XentNodeId *roots, uint32_t *root_count, XentNodeId root) {
	for (uint32_t i = 0; i < *root_count; ++i)
		if (roots [i] == root) return false;
	roots [(*root_count)++] = root;
	return true;
}

static bool has_nested_root(XentCtx const *ctx, XentNodeId const *roots, uint32_t root_count, uint32_t index) {
	if (roots [index] == XENT_NODE_INVALID) return false;

	for (uint32_t j = 0; j < root_count; ++j) {
		if (index == j || roots [j] == XENT_NODE_INVALID) continue;
		if (is_ancestor_of(ctx, roots [j], roots [index])) return true;
	}

	return false;
}

static void drop_nested_roots(XentCtx const *ctx, XentNodeId *roots, uint32_t root_count) {
	for (uint32_t i = 0; i < root_count; ++i)
		if (has_nested_root(ctx, roots, root_count, i)) roots [i] = XENT_NODE_INVALID;
}

static uint32_t compact_roots(XentNodeId *roots, uint32_t root_count) {
	uint32_t compact = 0u;
	for (uint32_t i = 0; i < root_count; ++i)
		if (roots [i] != XENT_NODE_INVALID) roots [compact++] = roots [i];
	return compact;
}

static uint32_t collect_recompute_roots(
  XentCtx const *ctx, XentNodeId root, XentNodeId const *direct_dirty, uint32_t direct_count, XentNodeId *out_roots
) {
	uint32_t root_count = 0u;
	for (uint32_t i = 0; i < direct_count; ++i) {
		XentNodeId promoted = promote_dirty_root(ctx, direct_dirty [i], root);
		if (promoted == root) return 0u;
		append_unique_root(out_roots, &root_count, promoted);
	}

	drop_nested_roots(ctx, out_roots, root_count);
	return compact_roots(out_roots, root_count);
}

static bool
can_reuse_root_snapshot(XentCtx const *ctx, XentNodeId root, float available_width, float available_height) {
	if (ctx->last_layout_root != root) return false;
	if (!isfinite(ctx->last_layout_available_w) || !isfinite(ctx->last_layout_available_h)) return false;
	if (ctx->last_layout_available_w != available_width || ctx->last_layout_available_h != available_height)
		return false;
	return isfinite(ctx->nodes.layout.decided_w [xent_node_index(root)])
	    && ctx->nodes.layout.decided_w [xent_node_index(root)] >= 0.0f
	    && isfinite(ctx->nodes.layout.decided_h [xent_node_index(root)])
	    && ctx->nodes.layout.decided_h [xent_node_index(root)] >= 0.0f;
}

static uint32_t count_subtree_nodes(XentCtx const *ctx, XentNodeId root, XentNodeId *stack, uint32_t stack_capacity) {
	if (!stack || stack_capacity == 0u || !xent_node_valid(ctx, root)) return 0u;

	uint32_t count       = 0u;
	uint32_t stack_size  = 0u;
	stack [stack_size++] = root;

	while (stack_size > 0u) {
		XentNodeId node   = stack [--stack_size];
		count            += 1u;
		XentNodeId child  = ctx->nodes.topology.first_child [xent_node_index(node)];
		while (child != XENT_NODE_INVALID && stack_size < stack_capacity) {
			stack [stack_size++] = child;
			child                = ctx->nodes.topology.next_sibling [xent_node_index(child)];
		}
		if (child != XENT_NODE_INVALID) return 0u;
	}

	return count;
}

static uint32_t count_dirty_nodes(XentCtx *ctx, XentNodeId root) {
	xent_compact_dirty_nodes(ctx);
	uint32_t dirty_count = 0u;
	for (uint32_t i = 0; i < ctx->dirty_count; ++i) {
		XentNodeId node = ctx->dirty_nodes [i];
		if (xent_node_valid(ctx, node)
			&& xent_dirty_direct(ctx->nodes.layout.dirty_flags [xent_node_index(node)])
			&& is_in_subtree(ctx, root, node))
		{
			dirty_count += 1u;
		}
	}
	return dirty_count;
}

static bool alloc_dirty_plan(XentCtx *ctx, XentDirtyPlan *plan, uint32_t total_nodes) {
	size_t   node_bytes  = sizeof(XentNodeId) * ( size_t ) plan->direct_count;
	size_t   stack_bytes = sizeof(XentNodeId) * ( size_t ) total_nodes;
	uint8_t *block = ( uint8_t * ) xent_scratch_alloc(ctx, node_bytes + node_bytes + stack_bytes, _Alignof(XentNodeId));
	if (!block) return false;

	plan->direct_dirty = ( XentNodeId * ) block;
	plan->roots        = ( XentNodeId * ) (block + node_bytes);
	plan->stack        = ( XentNodeId * ) (block + node_bytes + node_bytes);
	return true;
}

static void collect_dirty_nodes(XentCtx const *ctx, XentNodeId root, XentDirtyPlan *plan) {
	uint32_t write = 0u;
	for (uint32_t i = 0; i < ctx->dirty_count; ++i) {
		XentNodeId node = ctx->dirty_nodes [i];
		if (xent_node_valid(ctx, node)
			&& xent_dirty_direct(ctx->nodes.layout.dirty_flags [xent_node_index(node)])
			&& is_in_subtree(ctx, root, node))
		{
			plan->direct_dirty [write++] = node;
		}
	}
	plan->direct_count = write;
}

static bool root_has_valid_layout(XentCtx const *ctx, XentNodeId root) {
	return isfinite(ctx->nodes.layout.decided_w [xent_node_index(root)])
	    && ctx->nodes.layout.decided_w [xent_node_index(root)] >= 0.0f
	    && isfinite(ctx->nodes.layout.decided_h [xent_node_index(root)])
	    && ctx->nodes.layout.decided_h [xent_node_index(root)] >= 0.0f;
}

static bool measure_dirty_plan_cost(XentCtx const *ctx, XentDirtyPlan *plan, uint32_t total_nodes) {
	plan->affected_nodes = 0u;
	for (uint32_t i = 0; i < plan->root_count; ++i) {
		XentNodeId root = plan->roots [i];
		if (!root_has_valid_layout(ctx, root)) return false;

		uint32_t subtree_nodes = count_subtree_nodes(ctx, root, plan->stack, total_nodes);
		if (subtree_nodes == 0u) return false;
		plan->affected_nodes += subtree_nodes;
	}
	return true;
}

static bool build_dirty_plan(
  XentCtx *ctx, XentNodeId root, uint32_t total_nodes, uint32_t direct_dirty_count, XentDirtyPlan *plan
) {
	*plan              = (XentDirtyPlan) {0};
	plan->direct_count = direct_dirty_count;
	plan->strategy     = XENT_LAYOUT_STRATEGY_FULL;

	if (direct_dirty_count == 0u || total_nodes == 0u) return false;
	if (!alloc_dirty_plan(ctx, plan, total_nodes)) return false;

	collect_dirty_nodes(ctx, root, plan);
	plan->root_count = collect_recompute_roots(ctx, root, plan->direct_dirty, plan->direct_count, plan->roots);
	if (plan->root_count == 0u) return false;
	if (!measure_dirty_plan_cost(ctx, plan, total_nodes)) return false;
	if ((plan->affected_nodes + 1u) >= total_nodes) return false;

	plan->strategy = XENT_LAYOUT_STRATEGY_DIRTY;
	return true;
}

static void run_dirty_plan(XentCtx *ctx, XentDirtyPlan const *plan) {
	for (uint32_t i = 0; i < plan->root_count; ++i) {
		XentNodeId root = plan->roots [i];
		float      origin_x
		  = ctx->nodes.layout.abs_x [xent_node_index(root)] - ctx->nodes.layout.abs_pos_x [xent_node_index(root)];
		float origin_y
		  = ctx->nodes.layout.abs_y [xent_node_index(root)] - ctx->nodes.layout.abs_pos_y [xent_node_index(root)];
		/* The root's decided size came from its parent's layout pass, which is
		 * unchanged here. Honor it as definite when that parent is a flow
		 * container (which sizes its children) so a wrap-content root does not
		 * re-derive a different max-content size than the full pass produced. */
		XentNodeId parent      = ctx->nodes.topology.parent [xent_node_index(root)];
		bool       flow_parent = parent != XENT_NODE_INVALID && is_flow_layout_container(ctx, parent);
		xent_layout_dispatch_node(&(XentLayoutRequest) {
		  ctx, root, ctx->nodes.layout.decided_w [xent_node_index(root)],
		  ctx->nodes.layout.decided_h [xent_node_index(root)], origin_x, origin_y, flow_parent, flow_parent});
	}
}

typedef struct XentLayoutPlanInputs {
	XentNodeId root;
	float      available_width;
	float      available_height;
	uint32_t   total_nodes;
	uint32_t   direct_dirty_count;
} XentLayoutPlanInputs;

static XentLayoutStrategy
select_layout_strategy(XentCtx *ctx, XentLayoutPlanInputs const *inputs, XentDirtyPlan *dirty_plan) {
	bool root_dirty     = xent_dirty_direct(ctx->nodes.layout.dirty_flags [xent_node_index(inputs->root)]);
	bool snapshot_valid = can_reuse_root_snapshot(ctx, inputs->root, inputs->available_width, inputs->available_height);

	if (!root_dirty && snapshot_valid && inputs->direct_dirty_count == 0u) return XENT_LAYOUT_STRATEGY_NONE;
	if (!root_dirty
		&& snapshot_valid
		&& build_dirty_plan(ctx, inputs->root, inputs->total_nodes, inputs->direct_dirty_count, dirty_plan))
		return XENT_LAYOUT_STRATEGY_DIRTY;
	return XENT_LAYOUT_STRATEGY_FULL;
}

static void run_selected_layout(
  XentCtx *ctx, XentLayoutPlanInputs const *inputs, XentLayoutStrategy strategy, XentDirtyPlan const *dirty_plan
) {
	if (strategy == XENT_LAYOUT_STRATEGY_DIRTY) {
		run_dirty_plan(ctx, dirty_plan);
		return;
	}
	if (strategy == XENT_LAYOUT_STRATEGY_FULL)
		xent_layout_dispatch_node(&(XentLayoutRequest) {
		  ctx, inputs->root, inputs->available_width, inputs->available_height, 0.0f, 0.0f});
}

static bool prepare_layout_work_order(
  XentCtx *ctx, XentNodeId root, XentLayoutPlanInputs *inputs, XentLayoutStrategy strategy,
  XentDirtyPlan const *dirty_plan
) {
	if (strategy == XENT_LAYOUT_STRATEGY_FULL) {
		if (!xent_build_preorder(ctx, root)) return false;
		inputs->total_nodes = ctx->work_count;
		return true;
	}

	if (strategy == XENT_LAYOUT_STRATEGY_DIRTY)
		return xent_build_preorder_roots(ctx, dirty_plan->roots, dirty_plan->root_count);

	ctx->work_count = 0u;
	return true;
}

bool xent_layout(XentCtx *ctx, XentNodeId root, float available_width, float available_height) {
	if (!xent_node_valid(ctx, root)) return false;

	xent_scratch_reset(ctx);

	uint32_t             direct_dirty_count = count_dirty_nodes(ctx, root);
	XentLayoutPlanInputs inputs
	  = {root, available_width, available_height, ctx->last_layout_node_count, direct_dirty_count};
	XentDirtyPlan      dirty_plan = {0};
	XentLayoutStrategy strategy   = select_layout_strategy(ctx, &inputs, &dirty_plan);

	if (!prepare_layout_work_order(ctx, root, &inputs, strategy, &dirty_plan)) return false;
	run_selected_layout(ctx, &inputs, strategy, &dirty_plan);

	if (ctx->work_count > 0u) xent_batch_quantize_layout(ctx);
	if (ctx->work_count > 0u) xent_dirty_clear_order(ctx);
	else xent_compact_dirty_nodes(ctx);
	ctx->last_layout_root        = root;
	ctx->last_layout_available_w = available_width;
	ctx->last_layout_available_h = available_height;
	ctx->last_layout_strategy    = ( uint8_t ) strategy;
	if (strategy == XENT_LAYOUT_STRATEGY_FULL) ctx->last_layout_node_count = inputs.total_nodes;
	return true;
}
