#include "../xent_internal.h"

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
	XentContext *ctx      = request->ctx;
	XentNodeId   node     = request->node;
	XentProtocol protocol = ( XentProtocol ) ctx->nodes.layout.protocol [node];
	switch (protocol) {
	case XENT_PROTOCOL_FLEX       : xent_layout_node_flex(request); break;
	case XENT_PROTOCOL_SWIFTSTACK : xent_layout_node_swiftstack(request); break;
	case XENT_PROTOCOL_GRID       : xent_layout_node_grid(request); break;
	case XENT_PROTOCOL_ABSOLUTE   :
	default                       : xent_layout_node_absolute(request); break;
	}
}

static bool xent_is_layout_dirty(uint32_t flags) { return (flags & (XENT_DIRTY_LAYOUT | XENT_DIRTY_SELF)) != 0u; }

static bool xent_is_auto_sized_parent(XentContext const *ctx, XentNodeId node) {
	return (isnan(ctx->nodes.layout.style_w [node]) && isnan(ctx->nodes.layout.style_w_percent [node]))
	    || (isnan(ctx->nodes.layout.style_h [node]) && isnan(ctx->nodes.layout.style_h_percent [node]));
}

static bool xent_can_promote_dirty_root(XentContext const *ctx, XentNodeId node) {
	XentProtocol protocol = ( XentProtocol ) ctx->nodes.layout.protocol [node];
	return (protocol == XENT_PROTOCOL_FLEX || protocol == XENT_PROTOCOL_SWIFTSTACK || protocol == XENT_PROTOCOL_GRID)
	    && xent_is_auto_sized_parent(ctx, node);
}

static XentNodeId xent_promote_dirty_root(XentContext const *ctx, XentNodeId dirty_node, XentNodeId root) {
	XentNodeId candidate = dirty_node;
	for (XentNodeId parent = ctx->nodes.topology.parent [candidate]; parent != XENT_NODE_INVALID && parent != root;
	  parent               = ctx->nodes.topology.parent [candidate])
	{
		if (!xent_can_promote_dirty_root(ctx, parent)) break;
		candidate = parent;
	}
	return candidate;
}

static bool xent_is_ancestor_of(XentContext const *ctx, XentNodeId maybe_ancestor, XentNodeId node) {
	for (XentNodeId cursor = ctx->nodes.topology.parent [node]; cursor != XENT_NODE_INVALID;
	  cursor               = ctx->nodes.topology.parent [cursor])
		if (cursor == maybe_ancestor) return true;
	return false;
}

static bool xent_is_in_subtree(XentContext const *ctx, XentNodeId root, XentNodeId node) {
	if (root == node) return true;
	return xent_is_ancestor_of(ctx, root, node);
}

static bool xent_append_unique_root(XentNodeId *roots, uint32_t *root_count, XentNodeId root) {
	for (uint32_t i = 0; i < *root_count; ++i)
		if (roots [i] == root) return false;
	roots [(*root_count)++] = root;
	return true;
}

static bool xent_has_nested_root(XentContext const *ctx, XentNodeId const *roots, uint32_t root_count, uint32_t index) {
	if (roots [index] == XENT_NODE_INVALID) return false;

	for (uint32_t j = 0; j < root_count; ++j) {
		if (index == j || roots [j] == XENT_NODE_INVALID) continue;
		if (xent_is_ancestor_of(ctx, roots [j], roots [index])) return true;
	}

	return false;
}

static void xent_drop_nested_roots(XentContext const *ctx, XentNodeId *roots, uint32_t root_count) {
	for (uint32_t i = 0; i < root_count; ++i)
		if (xent_has_nested_root(ctx, roots, root_count, i)) roots [i] = XENT_NODE_INVALID;
}

static uint32_t xent_compact_roots(XentNodeId *roots, uint32_t root_count) {
	uint32_t compact = 0u;
	for (uint32_t i = 0; i < root_count; ++i)
		if (roots [i] != XENT_NODE_INVALID) roots [compact++] = roots [i];
	return compact;
}

static uint32_t xent_collect_recompute_roots(
  XentContext const *ctx, XentNodeId root, XentNodeId const *direct_dirty, uint32_t direct_count, XentNodeId *out_roots
) {
	uint32_t root_count = 0u;
	for (uint32_t i = 0; i < direct_count; ++i) {
		XentNodeId promoted = xent_promote_dirty_root(ctx, direct_dirty [i], root);
		if (promoted == root) return 0u;
		xent_append_unique_root(out_roots, &root_count, promoted);
	}

	xent_drop_nested_roots(ctx, out_roots, root_count);
	return xent_compact_roots(out_roots, root_count);
}

static bool
xent_can_reuse_root_snapshot(XentContext const *ctx, XentNodeId root, float available_width, float available_height) {
	if (ctx->last_layout_root != root) return false;
	if (!isfinite(ctx->last_layout_available_w) || !isfinite(ctx->last_layout_available_h)) return false;
	if (ctx->last_layout_available_w != available_width || ctx->last_layout_available_h != available_height)
		return false;
	return isfinite(ctx->nodes.layout.decided_w [root])
	    && ctx->nodes.layout.decided_w [root] >= 0.0f
	    && isfinite(ctx->nodes.layout.decided_h [root])
	    && ctx->nodes.layout.decided_h [root] >= 0.0f;
}

static uint32_t
xent_count_subtree_nodes(XentContext const *ctx, XentNodeId root, XentNodeId *stack, uint32_t stack_capacity) {
	if (!stack || stack_capacity == 0u || !xent_is_valid_node(ctx, root)) return 0u;

	uint32_t count       = 0u;
	uint32_t stack_size  = 0u;
	stack [stack_size++] = root;

	while (stack_size > 0u) {
		XentNodeId node   = stack [--stack_size];
		count            += 1u;
		XentNodeId child  = ctx->nodes.topology.first_child [node];
		while (child != XENT_NODE_INVALID && stack_size < stack_capacity) {
			stack [stack_size++] = child;
			child                = ctx->nodes.topology.next_sibling [child];
		}
		if (child != XENT_NODE_INVALID) return 0u;
	}

	return count;
}

static uint32_t xent_count_dirty_nodes(XentContext *ctx, XentNodeId root) {
	xent_compact_dirty_nodes(ctx);
	uint32_t dirty_count = 0u;
	for (uint32_t i = 0; i < ctx->dirty_count; ++i) {
		XentNodeId node = ctx->dirty_nodes [i];
		if (xent_is_valid_node(ctx, node)
			&& xent_is_layout_dirty(ctx->nodes.layout.dirty_flags [node])
			&& xent_is_in_subtree(ctx, root, node))
		{
			dirty_count += 1u;
		}
	}
	return dirty_count;
}

static bool xent_alloc_dirty_plan(XentContext *ctx, XentDirtyPlan *plan, uint32_t total_nodes) {
	size_t node_bytes  = sizeof(XentNodeId) * ( size_t ) plan->direct_count;
	plan->direct_dirty = ( XentNodeId * ) xent_scratch_alloc(ctx, node_bytes, _Alignof(XentNodeId));
	plan->roots        = ( XentNodeId * ) xent_scratch_alloc(ctx, node_bytes, _Alignof(XentNodeId));
	plan->stack
	  = ( XentNodeId * ) xent_scratch_alloc(ctx, sizeof(XentNodeId) * ( size_t ) total_nodes, _Alignof(XentNodeId));
	return plan->direct_dirty && plan->roots && plan->stack;
}

static void xent_collect_dirty_nodes(XentContext const *ctx, XentNodeId root, XentDirtyPlan *plan) {
	uint32_t write = 0u;
	for (uint32_t i = 0; i < ctx->dirty_count; ++i) {
		XentNodeId node = ctx->dirty_nodes [i];
		if (xent_is_valid_node(ctx, node)
			&& xent_is_layout_dirty(ctx->nodes.layout.dirty_flags [node])
			&& xent_is_in_subtree(ctx, root, node))
		{
			plan->direct_dirty [write++] = node;
		}
	}
	plan->direct_count = write;
}

static bool xent_root_has_valid_layout(XentContext const *ctx, XentNodeId root) {
	return isfinite(ctx->nodes.layout.decided_w [root])
	    && ctx->nodes.layout.decided_w [root] >= 0.0f
	    && isfinite(ctx->nodes.layout.decided_h [root])
	    && ctx->nodes.layout.decided_h [root] >= 0.0f;
}

static bool xent_measure_dirty_plan_cost(XentContext const *ctx, XentDirtyPlan *plan, uint32_t total_nodes) {
	plan->affected_nodes = 0u;
	for (uint32_t i = 0; i < plan->root_count; ++i) {
		XentNodeId root = plan->roots [i];
		if (!xent_root_has_valid_layout(ctx, root)) return false;

		uint32_t subtree_nodes = xent_count_subtree_nodes(ctx, root, plan->stack, total_nodes);
		if (subtree_nodes == 0u) return false;
		plan->affected_nodes += subtree_nodes;
	}
	return true;
}

static bool xent_build_dirty_plan(
  XentContext *ctx, XentNodeId root, uint32_t total_nodes, uint32_t direct_dirty_count, XentDirtyPlan *plan
) {
	*plan              = (XentDirtyPlan) {0};
	plan->direct_count = direct_dirty_count;
	plan->strategy     = XENT_LAYOUT_STRATEGY_FULL_TREE;

	if (direct_dirty_count == 0u || total_nodes == 0u) return false;
	if (!xent_alloc_dirty_plan(ctx, plan, total_nodes)) return false;

	xent_collect_dirty_nodes(ctx, root, plan);
	plan->root_count = xent_collect_recompute_roots(ctx, root, plan->direct_dirty, plan->direct_count, plan->roots);
	if (plan->root_count == 0u) return false;
	if (!xent_measure_dirty_plan_cost(ctx, plan, total_nodes)) return false;
	if ((plan->affected_nodes + 1u) >= total_nodes) return false;

	plan->strategy = XENT_LAYOUT_STRATEGY_DIRTY_SUBTREE;
	return true;
}

static void xent_run_dirty_plan(XentContext *ctx, XentDirtyPlan const *plan) {
	for (uint32_t i = 0; i < plan->root_count; ++i) {
		XentNodeId root     = plan->roots [i];
		float      origin_x = ctx->nodes.layout.abs_x [root] - ctx->nodes.layout.abs_pos_x [root];
		float      origin_y = ctx->nodes.layout.abs_y [root] - ctx->nodes.layout.abs_pos_y [root];
		xent_layout_dispatch_node(&(XentLayoutRequest) {
		  ctx, root, ctx->nodes.layout.decided_w [root], ctx->nodes.layout.decided_h [root], origin_x, origin_y});
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
xent_select_layout_strategy(XentContext *ctx, XentLayoutPlanInputs const *inputs, XentDirtyPlan *dirty_plan) {
	bool root_dirty = xent_is_layout_dirty(ctx->nodes.layout.dirty_flags [inputs->root]);
	bool snapshot_valid
	  = xent_can_reuse_root_snapshot(ctx, inputs->root, inputs->available_width, inputs->available_height);

	if (!root_dirty && snapshot_valid && inputs->direct_dirty_count == 0u) return XENT_LAYOUT_STRATEGY_NONE;
	if (!root_dirty
		&& snapshot_valid
		&& xent_build_dirty_plan(ctx, inputs->root, inputs->total_nodes, inputs->direct_dirty_count, dirty_plan))
		return XENT_LAYOUT_STRATEGY_DIRTY_SUBTREE;
	return XENT_LAYOUT_STRATEGY_FULL_TREE;
}

static void xent_run_selected_layout(
  XentContext *ctx, XentLayoutPlanInputs const *inputs, XentLayoutStrategy strategy, XentDirtyPlan const *dirty_plan
) {
	if (strategy == XENT_LAYOUT_STRATEGY_DIRTY_SUBTREE) {
		xent_run_dirty_plan(ctx, dirty_plan);
		return;
	}
	if (strategy == XENT_LAYOUT_STRATEGY_FULL_TREE)
		xent_layout_dispatch_node(&(XentLayoutRequest) {
		  ctx, inputs->root, inputs->available_width, inputs->available_height, 0.0f, 0.0f});
}

bool xent_layout(XentContext *ctx, XentNodeId root, float available_width, float available_height) {
	if (!xent_is_valid_node(ctx, root)) return false;

	xent_scratch_reset(ctx);

	uint32_t             direct_dirty_count = xent_count_dirty_nodes(ctx, root);
	XentLayoutPlanInputs inputs
	  = {root, available_width, available_height, ctx->last_layout_node_count, direct_dirty_count};
	XentDirtyPlan      dirty_plan = {0};
	XentLayoutStrategy strategy   = xent_select_layout_strategy(ctx, &inputs, &dirty_plan);

	if (strategy == XENT_LAYOUT_STRATEGY_FULL_TREE) {
		if (!xent_build_preorder(ctx, root)) return false;
		inputs.total_nodes = ctx->work_count;
	}
	else if (strategy == XENT_LAYOUT_STRATEGY_DIRTY_SUBTREE) {
		if (!xent_build_preorder_roots(ctx, dirty_plan.roots, dirty_plan.root_count)) return false;
	}
	else { ctx->work_count = 0u; }

	xent_run_selected_layout(ctx, &inputs, strategy, &dirty_plan);

	if (ctx->work_count > 0u) xent_batch_quantize_layout(ctx);
	if (ctx->work_count > 0u) xent_clear_dirty_in_work_order(ctx);
	else xent_compact_dirty_nodes(ctx);
	ctx->last_layout_root        = root;
	ctx->last_layout_available_w = available_width;
	ctx->last_layout_available_h = available_height;
	ctx->last_layout_strategy    = ( uint8_t ) strategy;
	if (strategy == XENT_LAYOUT_STRATEGY_FULL_TREE) ctx->last_layout_node_count = inputs.total_nodes;
	return true;
}
