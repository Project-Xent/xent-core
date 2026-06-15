#include "../xent_internal.h"

#if XENT_ISPC_ENABLED
  #include "xent_ispc_kernels_ispc.h"
#endif

typedef struct StackChildData {
	XentNodeId node_id;
	float      preferred_main;
	float      preferred_cross;
	float      margin_lead;
	float      margin_trail;
	float      margin_cross_lead;
	float      margin_cross_trail;
	float      priority;
	bool       fixed_main;
	bool       spacer;
} StackChildData;

typedef struct StackLayoutFrame {
	XentNodeId node;
	bool       horizontal;
	bool       baseline_align;
	float      content_x;
	float      content_y;
	float      content_w;
	float      content_h;
	float      available_main;
	float      available_cross;
	float      gap;
} StackLayoutFrame;

typedef struct StackBuffers {
	StackChildData *children;
	uint32_t       *priority_order;
	float          *main_sizes;
	uint8_t        *ispc_mask;
	float          *ispc_weights;
	uint32_t       *ispc_indices;
} StackBuffers;

typedef struct StackCollectStats {
	float    sum_min;
	uint32_t spacer_count;
	float    priority_sum;
	uint32_t count;
} StackCollectStats;

typedef struct StackBaselineInfo {
	float target;
	bool  active;
} StackBaselineInfo;

typedef struct StackChildRect {
	float x;
	float y;
	float w;
	float h;
} StackChildRect;

typedef struct StackChildRectRequest {
	XentContext            *ctx;
	StackLayoutFrame const *frame;
	StackChildData const   *child;
	StackBaselineInfo       baseline;
	float                   child_main;
	float                   cursor;
} StackChildRectRequest;

typedef struct StackPriorityGroup {
	uint32_t start;
	uint32_t end;
	float    total;
	float    flexible_total;
	float    fixed_total;
} StackPriorityGroup;

typedef struct StackReduceRequest {
	StackPriorityGroup group;
	float              reduce_amount;
	float              total;
	bool               fixed;
} StackReduceRequest;

static float xent_swiftstack_clampf(float v, float min_v, float max_v) {
	if (v < min_v) v = min_v;
	if (v > max_v) v = max_v;
	return v;
}

static int xent_compare_swiftstack_order_asc(void const *a, void const *b, void *context) {
	StackChildData const *children = ( StackChildData const * ) context;
	uint32_t              ia       = *( uint32_t const * ) a;
	uint32_t              ib       = *( uint32_t const * ) b;
	float                 pa       = children [ia].priority;
	float                 pb       = children [ib].priority;
	if (pa < pb) return -1;
	if (pa > pb) return 1;
	if (ia < ib) return -1;
	if (ia > ib) return 1;
	return 0;
}

static bool xent_same_priority(float a, float b) {
	float const eps = 0.0001f;
	return fabsf(a - b) <= eps;
}

static void stack_begin_layout(XentLayoutRequest const *request, StackLayoutFrame *frame) {
	XentContext *ctx         = request->ctx;
	XentNodeId   node        = request->node;
	float        available_w = request->available_w;
	float        available_h = request->available_h;
	float        origin_x    = request->origin_x;
	float        origin_y    = request->origin_y;
	float        width       = 0.0f;
	float        height      = 0.0f;
	xent_decide_node_box(ctx, node, available_w, available_h, request->definite_w, request->definite_h, &width, &height);

	ctx->nodes.layout.proposed_w [node] = available_w;
	ctx->nodes.layout.proposed_h [node] = available_h;
	ctx->nodes.layout.decided_w [node]  = width;
	ctx->nodes.layout.decided_h [node]  = height;
	ctx->nodes.layout.abs_x [node]      = origin_x + ctx->nodes.layout.abs_pos_x [node];
	ctx->nodes.layout.abs_y [node]      = origin_y + ctx->nodes.layout.abs_pos_y [node];
	xent_quantize_node_layout(ctx, node);

	width                 = ctx->nodes.layout.decided_w [node];
	height                = ctx->nodes.layout.decided_h [node];
	frame->node           = node;
	frame->horizontal     = ctx->nodes.stack.axis [node] == ( uint8_t ) XENT_AXIS_HORIZONTAL;
	frame->baseline_align = frame->horizontal && ctx->nodes.stack.align [node] == ( uint8_t ) XENT_STACK_ALIGN_BASELINE;
	frame->content_x      = ctx->nodes.layout.abs_x [node] + ctx->nodes.layout.padding_l [node];
	frame->content_y      = ctx->nodes.layout.abs_y [node] + ctx->nodes.layout.padding_t [node];
	frame->content_w      = width - (ctx->nodes.layout.padding_l [node] + ctx->nodes.layout.padding_r [node]);
	frame->content_h      = height - (ctx->nodes.layout.padding_t [node] + ctx->nodes.layout.padding_b [node]);
	if (frame->content_w < 0.0f) frame->content_w = 0.0f;
	if (frame->content_h < 0.0f) frame->content_h = 0.0f;
	frame->available_main  = frame->horizontal ? frame->content_w : frame->content_h;
	frame->available_cross = frame->horizontal ? frame->content_h : frame->content_w;
	frame->gap             = ctx->nodes.layout.gap [node];
	if (frame->gap < 0.0f) frame->gap = 0.0f;
}

static bool stack_alloc_buffers(XentContext *ctx, uint32_t child_count, StackBuffers *buffers) {
	size_t   children_bytes = sizeof(StackChildData) * ( size_t ) child_count;
	size_t   order_bytes    = sizeof(uint32_t) * ( size_t ) child_count;
	size_t   main_bytes     = sizeof(float) * ( size_t ) child_count;
	size_t   align_order    = _Alignof(uint32_t);
	size_t   align_main     = _Alignof(float);
	size_t   total_bytes    = children_bytes + order_bytes + main_bytes;
#if XENT_ISPC_ENABLED
	size_t   mask_offset    = xent_align_up_size(total_bytes, 1u);
	size_t   weight_offset  = xent_align_up_size(mask_offset + child_count, align_main);
	size_t   index_offset   = xent_align_up_size(weight_offset + main_bytes, align_order);
	total_bytes             = index_offset + order_bytes;
#endif
	uint8_t *block          = ( uint8_t * ) xent_scratch_alloc(ctx, total_bytes, _Alignof(StackChildData));
	if (!block) return false;

	buffers->children       = ( StackChildData * ) block;
	buffers->priority_order = ( uint32_t * ) (block + children_bytes);
	buffers->main_sizes     = ( float * ) (block + children_bytes + order_bytes);
#if XENT_ISPC_ENABLED
	buffers->ispc_mask      = block + mask_offset;
	buffers->ispc_weights   = ( float * ) (block + weight_offset);
	buffers->ispc_indices   = ( uint32_t * ) (block + index_offset);
#else
	( void ) align_order;
	( void ) align_main;
	buffers->ispc_mask    = NULL;
	buffers->ispc_weights = NULL;
	buffers->ispc_indices = NULL;
#endif
	return true;
}

static void stack_apply_child_margins(XentContext const *ctx, StackLayoutFrame const *frame, StackChildData *data) {
	XentNodeId child   = data->node_id;
	data->margin_lead  = frame->horizontal ? ctx->nodes.layout.margin_l [child] : ctx->nodes.layout.margin_t [child];
	data->margin_trail = frame->horizontal ? ctx->nodes.layout.margin_r [child] : ctx->nodes.layout.margin_b [child];
	data->margin_cross_lead
	  = frame->horizontal ? ctx->nodes.layout.margin_t [child] : ctx->nodes.layout.margin_l [child];
	data->margin_cross_trail
	  = frame->horizontal ? ctx->nodes.layout.margin_b [child] : ctx->nodes.layout.margin_r [child];
}

static bool stack_axis_style(float style, float min_v, float max_v, float *out_value) {
	if (isnan(style)) return false;
	*out_value = xent_swiftstack_clampf(style, min_v, max_v);
	return true;
}

static bool stack_main_style(XentContext const *ctx, StackLayoutFrame const *frame, XentNodeId child, float *out_main) {
	if (frame->horizontal)
		return stack_axis_style(
		  ctx->nodes.layout.style_w [child], ctx->nodes.layout.min_w [child], ctx->nodes.layout.max_w [child], out_main
		);
	return stack_axis_style(
	  ctx->nodes.layout.style_h [child], ctx->nodes.layout.min_h [child], ctx->nodes.layout.max_h [child], out_main
	);
}

static bool
stack_cross_style(XentContext const *ctx, StackLayoutFrame const *frame, XentNodeId child, float *out_cross) {
	if (frame->horizontal)
		return stack_axis_style(
		  ctx->nodes.layout.style_h [child], ctx->nodes.layout.min_h [child], ctx->nodes.layout.max_h [child], out_cross
		);
	return stack_axis_style(
	  ctx->nodes.layout.style_w [child], ctx->nodes.layout.min_w [child], ctx->nodes.layout.max_w [child], out_cross
	);
}

static float stack_intrinsic_cross(
  XentContext const *ctx, StackLayoutFrame const *frame, XentNodeId child, float intrinsic_w, float intrinsic_h
) {
	if (frame->horizontal)
		return xent_swiftstack_clampf(intrinsic_h, ctx->nodes.layout.min_h [child], ctx->nodes.layout.max_h [child]);
	return xent_swiftstack_clampf(intrinsic_w, ctx->nodes.layout.min_w [child], ctx->nodes.layout.max_w [child]);
}

static void stack_resolve_child_sizes(XentContext *ctx, StackLayoutFrame const *frame, StackChildData *data) {
	float intrinsic_w    = 0.0f;
	float intrinsic_h    = 0.0f;
	bool  fixed_main     = stack_main_style(ctx, frame, data->node_id, &data->preferred_main);
	bool  need_intrinsic = !fixed_main || (frame->baseline_align && isnan(ctx->nodes.layout.style_h [data->node_id]));

	if (need_intrinsic)
		xent_compute_intrinsic_size(ctx, data->node_id, frame->content_w, frame->content_h, &intrinsic_w, &intrinsic_h);
	if (!fixed_main) data->preferred_main = frame->horizontal ? intrinsic_w : intrinsic_h;
	data->fixed_main = fixed_main;
	if (!stack_cross_style(ctx, frame, data->node_id, &data->preferred_cross) && need_intrinsic)
		data->preferred_cross = stack_intrinsic_cross(ctx, frame, data->node_id, intrinsic_w, intrinsic_h);
}

static StackChildData stack_describe_child(XentContext *ctx, StackLayoutFrame const *frame, XentNodeId child) {
	StackChildData data = {0};
	data.node_id        = child;
	data.spacer         = ctx->nodes.stack.spacer [child] != 0u;
	data.priority       = ctx->nodes.stack.priority [child];
	stack_apply_child_margins(ctx, frame, &data);
	if (!data.spacer) stack_resolve_child_sizes(ctx, frame, &data);
	return data;
}

static StackCollectStats stack_collect_children(
  XentContext *ctx, StackLayoutFrame const *frame, StackBuffers const *buffers, uint32_t child_count
) {
	StackCollectStats stats = {0};
	XentNodeId        child = ctx->nodes.topology.first_child [frame->node];
	while (child != XENT_NODE_INVALID && stats.count < child_count) {
		ctx->profile.sibling_scans            += 1u;
		StackChildData data                    = stack_describe_child(ctx, frame, child);
		buffers->children [stats.count]        = data;
		buffers->main_sizes [stats.count]      = data.preferred_main;
		buffers->priority_order [stats.count]  = stats.count;
		stats.sum_min                         += data.preferred_main + data.margin_lead + data.margin_trail;
		if (data.spacer) stats.spacer_count += 1u;
		if (!data.fixed_main && data.priority > 0.0f) stats.priority_sum += data.priority;
		child        = ctx->nodes.topology.next_sibling [child];
		stats.count += 1u;
	}
	if (stats.count > 1u) stats.sum_min += frame->gap * ( float ) (stats.count - 1u);
	return stats;
}

#if XENT_ISPC_ENABLED
static bool
stack_expand_spacers_ispc(XentContext *ctx, StackBuffers const *buffers, StackCollectStats const *stats, float each) {
	if (stats->count < 32u) return false;

	uint8_t *spacer_mask = buffers->ispc_mask;
	if (!spacer_mask) return false;

	for (uint32_t i = 0; i < stats->count; ++i) spacer_mask [i] = buffers->children [i].spacer ? 1u : 0u;
	xent_ispc_masked_add_f32(buffers->main_sizes, spacer_mask, stats->count, each);
	return true;
}
#endif

static void
stack_expand_spacers(XentContext *ctx, StackBuffers const *buffers, StackCollectStats const *stats, float each) {
#if XENT_ISPC_ENABLED
	if (stack_expand_spacers_ispc(ctx, buffers, stats, each)) return;
#else
	( void ) ctx;
#endif
	for (uint32_t i = 0; i < stats->count; ++i)
		if (buffers->children [i].spacer) buffers->main_sizes [i] += each;
}

#if XENT_ISPC_ENABLED
static bool stack_expand_priorities_ispc(
  XentContext *ctx, StackBuffers const *buffers, StackCollectStats const *stats, float remainder
) {
	if (stats->count < 32u) return false;

	uint8_t *prio_mask    = buffers->ispc_mask;
	float   *prio_weights = buffers->ispc_weights;
	if (!prio_mask || !prio_weights) return false;

	for (uint32_t i = 0; i < stats->count; ++i) {
		bool eligible    = !buffers->children [i].fixed_main && buffers->children [i].priority > 0.0f;
		prio_mask [i]    = eligible ? 1u : 0u;
		prio_weights [i] = buffers->children [i].priority;
	}

	xent_ispc_masked_fma_f32(
	  buffers->main_sizes, prio_mask, prio_weights, stats->count, remainder, stats->priority_sum
	);
	return true;
}
#endif

static void stack_expand_priorities(
  XentContext *ctx, StackBuffers const *buffers, StackCollectStats const *stats, float remainder
) {
#if XENT_ISPC_ENABLED
	if (stack_expand_priorities_ispc(ctx, buffers, stats, remainder)) return;
#else
	( void ) ctx;
#endif
	for (uint32_t i = 0; i < stats->count; ++i)
		if (!buffers->children [i].fixed_main && buffers->children [i].priority > 0.0f)
			buffers->main_sizes [i] += remainder * (buffers->children [i].priority / stats->priority_sum);
}

#if XENT_ISPC_ENABLED
static uint32_t
stack_collect_reduce_indices(StackBuffers const *buffers, StackPriorityGroup group, bool fixed, uint32_t *indices) {
	uint32_t count = 0u;
	for (uint32_t i = group.start; i < group.end; ++i) {
		uint32_t child_index = buffers->priority_order [i];
		if (buffers->children [child_index].fixed_main == fixed) indices [count++] = child_index;
	}
	return count;
}

static bool stack_reduce_members_ispc(XentContext *ctx, StackBuffers const *buffers, StackReduceRequest request) {
	uint32_t group_size = request.group.end - request.group.start;
	if (group_size < 16u) return false;
	uint32_t *indices = buffers->ispc_indices;
	if (!indices) return false;
	uint32_t count = stack_collect_reduce_indices(buffers, request.group, request.fixed, indices);
	if (count > 0u)
		xent_ispc_proportional_reduce_gather(buffers->main_sizes, indices, count, request.reduce_amount, request.total);
	return true;
}
#else
static bool stack_reduce_members_ispc(XentContext *ctx, StackBuffers const *buffers, StackReduceRequest request) {
	( void ) ctx;
	( void ) buffers;
	( void ) request;
	return false;
}
#endif

static void stack_reduce_members_scalar(
  StackBuffers const *buffers, StackPriorityGroup group, bool fixed, float reduce_amount, float total
) {
	for (uint32_t i = group.start; i < group.end; ++i) {
		uint32_t child_index = buffers->priority_order [i];
		if (buffers->children [child_index].fixed_main != fixed) continue;
		float current = buffers->main_sizes [child_index];
		float share   = reduce_amount * (current / total);
		if (share > current) share = current;
		buffers->main_sizes [child_index] = current - share;
	}
}

static void stack_reduce_members(XentContext *ctx, StackBuffers const *buffers, StackReduceRequest request) {
	if (request.reduce_amount <= 0.0f || request.total <= 0.0f) return;
	if (stack_reduce_members_ispc(ctx, buffers, request)) return;
	stack_reduce_members_scalar(buffers, request.group, request.fixed, request.reduce_amount, request.total);
}

static void stack_sort_by_priority(XentContext *ctx, StackBuffers const *buffers, uint32_t count) {
	double sort_start_ms     = xent_now_ms();
	ctx->profile.sort_calls += 1u;
	xent_sort_r(
	  buffers->priority_order, ( size_t ) count, sizeof(uint32_t), xent_compare_swiftstack_order_asc,
	  ( void * ) buffers->children
	);
	ctx->profile.swiftstack_sort_ms += (xent_now_ms() - sort_start_ms);
}

static StackPriorityGroup
stack_next_priority_group(StackBuffers const *buffers, StackCollectStats const *stats, uint32_t start) {
	StackPriorityGroup group    = {start, start, 0.0f, 0.0f, 0.0f};
	float              priority = buffers->children [buffers->priority_order [start]].priority;
	while (group.end < stats->count
		   && xent_same_priority(buffers->children [buffers->priority_order [group.end]].priority, priority))
	{
		uint32_t child_index  = buffers->priority_order [group.end];
		float    size         = buffers->main_sizes [child_index];
		group.total          += size;
		if (buffers->children [child_index].fixed_main) group.fixed_total += size;
		else group.flexible_total += size;
		group.end += 1u;
	}
	return group;
}

static void stack_reduce_priority_group(
  XentContext *ctx, StackBuffers const *buffers, StackPriorityGroup group, float group_reduce
) {
	float reduce_flexible = group_reduce < group.flexible_total ? group_reduce : group.flexible_total;
	stack_reduce_members(ctx, buffers, (StackReduceRequest) {group, reduce_flexible, group.flexible_total, false});
	stack_reduce_members(
	  ctx, buffers, (StackReduceRequest) {group, group_reduce - reduce_flexible, group.fixed_total, true}
	);
}

static void
stack_shrink_sizes(XentContext *ctx, StackBuffers const *buffers, StackCollectStats const *stats, float deficit) {
	float reducible_total = xent_simd_sum_f32(buffers->main_sizes, stats->count);
	if (deficit >= reducible_total) {
		xent_simd_fill_f32(buffers->main_sizes, stats->count, 0.0f);
		return;
	}

	stack_sort_by_priority(ctx, buffers, stats->count);
	uint32_t rank = 0u;
	while (rank < stats->count && deficit > 0.0f) {
		StackPriorityGroup group = stack_next_priority_group(buffers, stats, rank);
		rank                     = group.end;
		if (group.total <= 0.0f) continue;

		float group_reduce = deficit < group.total ? deficit : group.total;
		stack_reduce_priority_group(ctx, buffers, group, group_reduce);
		deficit -= group_reduce;
	}
}

static void stack_distribute_main_sizes(
  XentContext *ctx, StackLayoutFrame const *frame, StackBuffers const *buffers, StackCollectStats const *stats
) {
	float remainder = frame->available_main - stats->sum_min;
	if (remainder > 0.0f && stats->spacer_count > 0u) {
		stack_expand_spacers(ctx, buffers, stats, remainder / ( float ) stats->spacer_count);
		return;
	}
	if (remainder > 0.0f && stats->priority_sum > 0.0f) stack_expand_priorities(ctx, buffers, stats, remainder);
	else if (remainder < 0.0f) stack_shrink_sizes(ctx, buffers, stats, -remainder);
}

static float stack_fallback_cross(StackLayoutFrame const *frame, StackChildData const *child) {
	return fmaxf(0.0f, frame->available_cross - child->margin_cross_lead - child->margin_cross_trail);
}

static float stack_baseline_target(
  XentContext *ctx, StackLayoutFrame const *frame, StackBuffers const *buffers, uint32_t count, bool *out_has_baseline
) {
	float target     = 0.0f;
	bool  has_target = false;
	if (!frame->baseline_align) {
		*out_has_baseline = false;
		return 0.0f;
	}

	for (uint32_t i = 0; i < count; ++i) {
		StackChildData const *child = &buffers->children [i];
		if (child->spacer) continue;
		float cross_size = child->preferred_cross > 0.0f ? child->preferred_cross : stack_fallback_cross(frame, child);
		float baseline   = xent_estimate_text_baseline(ctx, child->node_id, cross_size);
		float candidate  = child->margin_cross_lead + baseline;
		if (!has_target || candidate > target) target = candidate;
		has_target = true;
	}

	*out_has_baseline = has_target;
	return target;
}

static float stack_child_cross_size(StackLayoutFrame const *frame, StackChildData const *child) {
	return child->preferred_cross > 0.0f ? child->preferred_cross : stack_fallback_cross(frame, child);
}

static float stack_baseline_cross_offset(
  XentContext *ctx, StackLayoutFrame const *frame, StackChildData const *child, float child_h, float baseline_target
) {
	float cross_free = frame->available_cross - child_h - child->margin_cross_lead - child->margin_cross_trail;
	if (cross_free < 0.0f) cross_free = 0.0f;
	float baseline   = xent_estimate_text_baseline(ctx, child->node_id, child_h);
	float desired    = baseline_target - baseline;
	float min_offset = child->margin_cross_lead;
	float max_offset = child->margin_cross_lead + cross_free;
	return xent_swiftstack_clampf(desired, min_offset, max_offset);
}

static StackChildRect stack_horizontal_child_rect(StackChildRectRequest const *request) {
	StackLayoutFrame const *frame    = request->frame;
	StackChildData const   *child    = request->child;
	StackBaselineInfo       baseline = request->baseline;
	StackChildRect          rect     = {
	  request->cursor,
	  frame->content_y + child->margin_cross_lead,
	  request->child_main,
	  stack_fallback_cross(frame, child),
	};

	if (frame->baseline_align && baseline.active && !child->spacer) {
		rect.h = stack_child_cross_size(frame, child);
		rect.y = frame->content_y + stack_baseline_cross_offset(request->ctx, frame, child, rect.h, baseline.target);
	}
	return rect;
}

static StackChildRect stack_vertical_child_rect(StackChildRectRequest const *request) {
	StackLayoutFrame const *frame = request->frame;
	StackChildData const   *child = request->child;
	return (StackChildRect) {
	  frame->content_x + child->margin_cross_lead,
	  request->cursor,
	  stack_fallback_cross(frame, child),
	  request->child_main,
	};
}

static StackChildRect stack_child_rect(StackChildRectRequest const *request) {
	StackChildRect rect
	  = request->frame->horizontal ? stack_horizontal_child_rect(request) : stack_vertical_child_rect(request);

	if (rect.w < 0.0f) rect.w = 0.0f;
	if (rect.h < 0.0f) rect.h = 0.0f;
	return rect;
}

static void
stack_layout_children(XentContext *ctx, StackLayoutFrame const *frame, StackBuffers const *buffers, uint32_t count) {
	StackBaselineInfo baseline = {0};
	baseline.target            = stack_baseline_target(ctx, frame, buffers, count, &baseline.active);
	float cursor               = frame->horizontal ? frame->content_x : frame->content_y;

	for (uint32_t i = 0; i < count; ++i) {
		StackChildData const *child  = &buffers->children [i];
		cursor                      += child->margin_lead;
		float          child_main    = buffers->main_sizes [i];
		StackChildRect rect
		  = stack_child_rect(&(StackChildRectRequest) {ctx, frame, child, baseline, child_main, cursor});
		xent_layout_dispatch_node(&(XentLayoutRequest) {ctx, child->node_id, rect.w, rect.h, rect.x, rect.y, true, true});

		cursor += child_main + child->margin_trail;
		if (i + 1u < count) cursor += frame->gap;
	}
}

void xent_layout_node_swiftstack(XentLayoutRequest const *request) {
	XentContext *ctx                      = request->ctx;
	XentNodeId   node                     = request->node;
	double       swiftstack_start_ms      = xent_now_ms();
	ctx->profile.swiftstack_layout_calls += 1u;
	ctx->swiftstack_scope_depth          += 1u;

	StackLayoutFrame frame                = {0};
	stack_begin_layout(request, &frame);

	uint32_t child_count = ctx->nodes.topology.child_count [node];
	if (child_count > 0u) {
		StackBuffers buffers = {0};
		if (stack_alloc_buffers(ctx, child_count, &buffers)) {
			double            collect_start_ms  = xent_now_ms();
			StackCollectStats stats             = stack_collect_children(ctx, &frame, &buffers, child_count);
			ctx->profile.swiftstack_collect_ms += (xent_now_ms() - collect_start_ms);
			stack_distribute_main_sizes(ctx, &frame, &buffers, &stats);
			stack_layout_children(ctx, &frame, &buffers, stats.count);
		}
	}

	ctx->swiftstack_scope_depth      -= 1u;
	ctx->profile.swiftstack_total_ms += (xent_now_ms() - swiftstack_start_ms);
}
