#include "../xent_internal.h"

#include "xent_layout_flex.h"
#include "xent_layout_util.h"

#if XENT_ISPC_ENABLED
  #include "xent_ispc_kernels_ispc.h"
#endif

typedef struct FlexChildData {
	XentNodeId id;
	float      base_main;
	float      final_main;
	float      hypothetical_main;
	float      min_main;
	float      max_main;
	float      base_cross;
	float      final_cross;
	float      baseline_from_top;
	float      margin_lead;
	float      margin_trail;
	float      margin_cross_lead;
	float      margin_cross_trail;
	float      grow;
	float      shrink;
	uint8_t    align_self;
	uint8_t    resolved_align;
	bool       frozen;
} FlexChildData;

typedef struct FlexLineData {
	uint32_t start;
	uint32_t count;
	float    base_main;
	float    sum_grow;
	float    sum_shrink_weight;
	float    cross_outer;
} FlexLineData;

typedef struct FlexLayoutFrame {
	XentNodeId node;
	bool       row;
	bool       wrap_enabled;
	bool       rtl_main;
	bool       rtl_cross;
	float      content_x;
	float      content_y;
	float      content_w;
	float      content_h;
	float      available_main;
	float      available_cross;
	float      gap;
	uint8_t    sizing_mode; /* XENT_SIZING_* in force for this measurement */
} FlexLayoutFrame;

typedef struct FlexLinePlacement {
	float main_start_offset;
	float effective_gap;
	float baseline_target;
	bool  has_baseline_items;
} FlexLinePlacement;

typedef struct FlexLayoutChildrenRequest {
	XentCtx                 *ctx;
	FlexLayoutFrame const   *frame;
	FlexLineData const      *line;
	FlexLinePlacement const *placement;
	FlexChildData           *children;
	float                    cross_cursor;
} FlexLayoutChildrenRequest;

typedef struct FlexLinePrepareContext {
	XentCtx               *ctx;
	FlexLayoutFrame const *frame;
	FlexLineData const    *line;
	FlexLinePlacement     *placement;
	FlexChildData         *entry;
} FlexLinePrepareContext;

typedef enum FlexSpacingMode
{
	FLEX_SPACING_START,
	FLEX_SPACING_END,
	FLEX_SPACING_CENTER,
	FLEX_SPACING_BETWEEN,
	FLEX_SPACING_AROUND,
	FLEX_SPACING_EVENLY,
} FlexSpacingMode;

typedef struct FlexSpacingRule {
	float start_fixed;
	float start_share;
	int   start_add;
	float gap_share;
	int   gap_add;
} FlexSpacingRule;

typedef struct FlexSpacingRequest {
	FlexSpacingMode mode;
	uint32_t        item_count;
	float           base_gap;
	float           remaining;
} FlexSpacingRequest;

static float flex_spacing_share(float remaining, float numerator, uint32_t count, int add) {
	if (numerator == 0.0f) return 0.0f;
	int denom = ( int ) count + add;
	return denom > 0 ? remaining * numerator / ( float ) denom : 0.0f;
}

static FlexSpacingRule flex_spacing_rule(FlexSpacingMode mode) {
	static FlexSpacingRule const rules [] = {
	  {0.0f, 0.0f, 0, 0.0f, 0 },
	  {1.0f, 0.0f, 0, 0.0f, 0 },
	  {0.5f, 0.0f, 0, 0.0f, 0 },
	  {0.0f, 0.0f, 0, 1.0f, -1},
	  {0.0f, 0.5f, 0, 1.0f, 0 },
	  {0.0f, 1.0f, 1, 1.0f, 1 },
	};
	if (( uint32_t ) mode >= sizeof(rules) / sizeof(rules [0])) return rules [FLEX_SPACING_START];
	return rules [mode];
}

static void flex_resolve_spacing(FlexSpacingRequest request, float *out_start_offset, float *out_effective_gap) {
	FlexSpacingRule rule   = flex_spacing_rule(request.mode);
	/* center/end (start_fixed) may offset by NEGATIVE free space so overflowing
	 * content is centred / end-aligned past the start edge. The distributed
	 * shares (space-between/around/evenly gaps, and around/evenly leading space)
	 * never go negative — they fall back to packing from the start. */
	float           shared = request.remaining < 0.0f ? 0.0f : request.remaining;
	*out_start_offset      = request.remaining * rule.start_fixed
	                       + flex_spacing_share(shared, rule.start_share, request.item_count, rule.start_add);
	*out_effective_gap
	  = request.base_gap + flex_spacing_share(shared, rule.gap_share, request.item_count, rule.gap_add);
}

static float flex_child_shrink_weight(FlexChildData const *child) {
	return child->shrink * (child->base_main > 0.0f ? child->base_main : 1.0f);
}

static float flex_child_cross_outer(FlexChildData const *child) {
	return child->base_cross + child->margin_cross_lead + child->margin_cross_trail;
}

#if XENT_ISPC_ENABLED
static bool flex_compute_line_stats_ispc(
  XentCtx *ctx, FlexLineData *line, FlexChildData const *base, uint32_t count, bool wrap_enabled, float gap
) {
	if (!ctx->config.enable_simd || count < 32u) return false;

	float *bm = xent_scratch_alloc(ctx, sizeof(float) * count * 8u, _Alignof(float));
	if (!bm) return false;

	float *ml  = bm + count;
	float *mt  = ml + count;
	float *bc  = mt + count;
	float *mcl = bc + count;
	float *mct = mcl + count;
	float *gw  = mct + count;
	float *sh  = gw + count;
	for (uint32_t i = 0; i < count; ++i) {
		bm [i]  = base [i].base_main;
		ml [i]  = base [i].margin_lead;
		mt [i]  = base [i].margin_trail;
		bc [i]  = base [i].base_cross;
		mcl [i] = base [i].margin_cross_lead;
		mct [i] = base [i].margin_cross_trail;
		gw [i]  = base [i].grow;
		sh [i]  = base [i].shrink;
	}
	float sum_outer, sum_grow, sum_sw, max_cross;
	xent_ispc_flex_line_stats(bm, ml, mt, bc, mcl, mct, gw, sh, count, &sum_outer, &sum_grow, &sum_sw, &max_cross);
	line->base_main         = sum_outer + (count > 1u ? gap * ( float ) (count - 1u) : 0.0f);
	line->sum_grow          = sum_grow;
	line->sum_shrink_weight = sum_sw;
	if (wrap_enabled) line->cross_outer = max_cross;
	return true;
}
#endif

static void flex_compute_line_stats_scalar(
  FlexLineData *line, FlexChildData const *base, uint32_t count, bool wrap_enabled, float gap
) {
	for (uint32_t i = 0u; i < count; ++i) {
		FlexChildData const *child = &base [i];
		if (i > 0u) line->base_main += gap;
		line->base_main         += child->base_main + child->margin_lead + child->margin_trail;
		line->sum_grow          += child->grow;
		line->sum_shrink_weight += flex_child_shrink_weight(child);

		if (!wrap_enabled) continue;
		float cross_outer = flex_child_cross_outer(child);
		if (cross_outer > line->cross_outer) line->cross_outer = cross_outer;
	}
}

static void compute_line_stats(
  XentCtx *ctx, FlexLineData *line, FlexChildData const *children, bool wrap_enabled, float available_cross, float gap
) {
	line->base_main            = 0.0f;
	line->sum_grow             = 0.0f;
	line->sum_shrink_weight    = 0.0f;
	line->cross_outer          = wrap_enabled ? 0.0f : available_cross;

	uint32_t             count = line->count;
	FlexChildData const *base  = &children [line->start];
#if XENT_ISPC_ENABLED
	if (flex_compute_line_stats_ispc(ctx, line, base, count, wrap_enabled, gap)) return;
#endif
	flex_compute_line_stats_scalar(line, base, count, wrap_enabled, gap);
}

static void flex_begin_layout(XentLayoutRequest const *request, FlexLayoutFrame *frame) {
	XentCtx   *ctx         = request->ctx;
	XentNodeId node        = request->node;
	float      available_w = request->available_w;
	float      available_h = request->available_h;
	float      origin_x    = request->origin_x;
	float      origin_y    = request->origin_y;
	float      width       = 0.0f;
	float      height      = 0.0f;
	xent_decide_node_box(
	  ctx, node, available_w, available_h, request->definite_w, request->definite_h, &width, &height
	);

	ctx->nodes.layout.proposed_w [xent_node_index(node)] = available_w;
	ctx->nodes.layout.proposed_h [xent_node_index(node)] = available_h;
	ctx->nodes.layout.decided_w [xent_node_index(node)]  = width;
	ctx->nodes.layout.decided_h [xent_node_index(node)]  = height;
	ctx->nodes.layout.abs_x [xent_node_index(node)] = origin_x + ctx->nodes.layout.abs_pos_x [xent_node_index(node)];
	ctx->nodes.layout.abs_y [xent_node_index(node)] = origin_y + ctx->nodes.layout.abs_pos_y [xent_node_index(node)];
	xent_quantize_node_layout(ctx, node);

	width                   = ctx->nodes.layout.decided_w [xent_node_index(node)];
	height                  = ctx->nodes.layout.decided_h [xent_node_index(node)];
	frame->node             = node;
	frame->row              = ctx->nodes.flex.direction [xent_node_index(node)] == ( uint8_t ) XENT_FLEX_ROW;
	frame->wrap_enabled     = ctx->nodes.flex.wrap [xent_node_index(node)] == ( uint8_t ) XENT_FLEX_WRAP;
	XentDirection direction = xent_resolved_dir(ctx, node);
	frame->rtl_main         = frame->row && direction == XENT_DIRECTION_RTL;
	frame->rtl_cross        = !frame->row && direction == XENT_DIRECTION_RTL;
	frame->content_x
	  = ctx->nodes.layout.abs_x [xent_node_index(node)] + ctx->nodes.layout.padding_l [xent_node_index(node)];
	frame->content_y
	  = ctx->nodes.layout.abs_y [xent_node_index(node)] + ctx->nodes.layout.padding_t [xent_node_index(node)];
	frame->content_w
	  = width
	  - (ctx->nodes.layout.padding_l [xent_node_index(node)] + ctx->nodes.layout.padding_r [xent_node_index(node)]);
	frame->content_h
	  = height
	  - (ctx->nodes.layout.padding_t [xent_node_index(node)] + ctx->nodes.layout.padding_b [xent_node_index(node)]);
	if (frame->content_w < 0.0f) frame->content_w = 0.0f;
	if (frame->content_h < 0.0f) frame->content_h = 0.0f;
	frame->available_main  = frame->row ? frame->content_w : frame->content_h;
	frame->available_cross = frame->row ? frame->content_h : frame->content_w;
	frame->gap             = ctx->nodes.layout.gap [xent_node_index(node)];
	if (frame->gap < 0.0f) frame->gap = 0.0f;
	frame->sizing_mode = ctx->sizing_mode;
}

static bool
flex_alloc_buffers(XentCtx *ctx, uint32_t child_count, FlexChildData **out_children, FlexLineData **out_lines) {
	size_t   children_bytes = sizeof(FlexChildData) * ( size_t ) child_count;
	size_t   lines_offset   = xent_align_up_size(children_bytes, _Alignof(FlexLineData));
	size_t   lines_bytes    = sizeof(FlexLineData) * ( size_t ) child_count;
	uint8_t *block          = xent_scratch_alloc(ctx, lines_offset + lines_bytes, _Alignof(FlexChildData));
	if (!block) return false;
	memset(block, 0, lines_offset + lines_bytes);
	*out_children = ( FlexChildData * ) block;
	*out_lines    = ( FlexLineData * ) (block + lines_offset);
	return true;
}

static float flex_main_margin_lead(XentCtx const *ctx, FlexLayoutFrame const *frame, XentNodeId child) {
	if (!frame->row) return ctx->nodes.layout.margin_t [xent_node_index(child)];
	return frame->rtl_main ? ctx->nodes.layout.margin_r [xent_node_index(child)]
	                       : ctx->nodes.layout.margin_l [xent_node_index(child)];
}

static float flex_main_margin_trail(XentCtx const *ctx, FlexLayoutFrame const *frame, XentNodeId child) {
	if (!frame->row) return ctx->nodes.layout.margin_b [xent_node_index(child)];
	return frame->rtl_main ? ctx->nodes.layout.margin_l [xent_node_index(child)]
	                       : ctx->nodes.layout.margin_r [xent_node_index(child)];
}

static float flex_cross_margin_lead(XentCtx const *ctx, FlexLayoutFrame const *frame, XentNodeId child) {
	if (frame->row) return ctx->nodes.layout.margin_t [xent_node_index(child)];
	return frame->rtl_cross ? ctx->nodes.layout.margin_r [xent_node_index(child)]
	                        : ctx->nodes.layout.margin_l [xent_node_index(child)];
}

static float flex_cross_margin_trail(XentCtx const *ctx, FlexLayoutFrame const *frame, XentNodeId child) {
	if (frame->row) return ctx->nodes.layout.margin_b [xent_node_index(child)];
	return frame->rtl_cross ? ctx->nodes.layout.margin_l [xent_node_index(child)]
	                        : ctx->nodes.layout.margin_r [xent_node_index(child)];
}

static float flex_base_main(XentCtx *ctx, FlexLayoutFrame const *frame, XentNodeId child) {
	float mc_w = 0.0f, mc_h = 0.0f;
	if (frame->row) xent_compute_intrinsic_size(ctx, child, INFINITY, frame->content_h, &mc_w, &mc_h);
	else xent_compute_intrinsic_size(ctx, child, frame->content_w, INFINITY, &mc_w, &mc_h);

	uint32_t idx       = xent_node_index(child);
	float    base_main = frame->row ? mc_w : mc_h;
	float    main_pct  = frame->row ? ctx->nodes.layout.style_w_percent [idx] : ctx->nodes.layout.style_h_percent [idx];
	float    content_main = frame->row ? frame->content_w : frame->content_h;
	float    basis        = ctx->nodes.flex.basis [idx];
	if (!isnan(main_pct) && isfinite(content_main)) base_main = main_pct * content_main;
	if (!isnan(basis)) base_main = basis;
	return base_main < 0.0f ? 0.0f : base_main;
}

static void
flex_describe_child_size(XentCtx *ctx, FlexLayoutFrame const *frame, XentNodeId child, FlexChildData *data) {
	data->base_main         = flex_base_main(ctx, frame, child);
	data->hypothetical_main = xent_clampf(data->base_main, data->min_main, data->max_main);
	data->base_cross
	  = xent_compute_hypothetical_cross(ctx, child, frame->row, data->hypothetical_main, frame->available_cross);
	if (data->base_cross < 0.0f) data->base_cross = 0.0f;
	data->final_cross = data->base_cross;
}

static FlexChildData flex_describe_child(XentCtx *ctx, FlexLayoutFrame const *frame, XentNodeId child) {
	FlexChildData data      = {0};
	data.id                 = child;
	data.margin_lead        = flex_main_margin_lead(ctx, frame, child);
	data.margin_trail       = flex_main_margin_trail(ctx, frame, child);
	data.margin_cross_lead  = flex_cross_margin_lead(ctx, frame, child);
	data.margin_cross_trail = flex_cross_margin_trail(ctx, frame, child);
	data.grow               = ctx->nodes.flex.grow [xent_node_index(child)];
	data.shrink             = ctx->nodes.flex.shrink [xent_node_index(child)];
	data.align_self         = ctx->nodes.flex.align_self [xent_node_index(child)];
	data.min_main           = frame->row ? ctx->nodes.layout.min_w [xent_node_index(child)]
	                                     : ctx->nodes.layout.min_h [xent_node_index(child)];
	data.max_main           = frame->row ? ctx->nodes.layout.max_w [xent_node_index(child)]
	                                     : ctx->nodes.layout.max_h [xent_node_index(child)];
	flex_describe_child_size(ctx, frame, child, &data);
	return data;
}

static uint32_t
flex_collect_children(XentCtx *ctx, FlexLayoutFrame const *frame, FlexChildData *children, uint32_t child_capacity) {
	uint32_t   count = 0u;
	XentNodeId child = ctx->nodes.topology.first_child [xent_node_index(frame->node)];
	while (child != XENT_NODE_INVALID && count < child_capacity) {
		children [count++] = flex_describe_child(ctx, frame, child);
		child              = ctx->nodes.topology.next_sibling [xent_node_index(child)];
	}
	return count;
}

static uint32_t flex_build_lines(
  FlexLayoutFrame const *frame, FlexChildData const *children, uint32_t child_count, FlexLineData *lines
) {
	/* min-content of a wrap container: each item gets its own line, so the
	 * container's min-content MAIN size is the largest item (not the sum). */
	if (frame->wrap_enabled && frame->sizing_mode == XENT_SIZING_MIN_CONTENT) {
		for (uint32_t i = 0u; i < child_count; ++i) {
			lines [i].start = i;
			lines [i].count = 1u;
		}
		return child_count;
	}

	if (!frame->wrap_enabled || !isfinite(frame->available_main) || frame->available_main <= 0.0f) {
		lines [0].start = 0u;
		lines [0].count = child_count;
		return 1u;
	}

	uint32_t line_count = 0u;
	uint32_t line_start = 0u;
	uint32_t line_items = 0u;
	float    line_main  = 0.0f;
	for (uint32_t i = 0u; i < child_count; ++i) {
		float item_outer_main = children [i].hypothetical_main + children [i].margin_lead + children [i].margin_trail;
		float candidate       = line_items == 0u ? item_outer_main : line_main + frame->gap + item_outer_main;
		if (line_items > 0u && candidate > frame->available_main) {
			lines [line_count].start  = line_start;
			lines [line_count].count  = line_items;
			line_count               += 1u;
			line_start                = i;
			line_items                = 0u;
			line_main                 = 0.0f;
			candidate                 = item_outer_main;
		}
		line_main   = candidate;
		line_items += 1u;
	}

	if (line_items > 0u) {
		lines [line_count].start  = line_start;
		lines [line_count].count  = line_items;
		line_count               += 1u;
	}
	return line_count;
}

static void flex_compute_line_set(
  XentCtx *ctx, FlexLayoutFrame const *frame, FlexChildData const *children, FlexLineData *lines, uint32_t line_count
) {
	for (uint32_t i = 0u; i < line_count; ++i)
		compute_line_stats(ctx, &lines [i], children, frame->wrap_enabled, frame->available_cross, frame->gap);
}

static float flex_total_lines_cross(
  FlexLayoutFrame const *frame, FlexLineData const *lines, uint32_t line_count, float *out_line_gap
) {
	float total    = 0.0f;
	float line_gap = (line_count > 1u && frame->wrap_enabled) ? frame->gap : 0.0f;
	for (uint32_t i = 0u; i < line_count; ++i) total += lines [i].cross_outer;
	if (line_count > 1u) total += line_gap * ( float ) (line_count - 1u);
	*out_line_gap = line_gap;
	return total;
}

static bool flex_has_distribution(float delta, FlexLineData const *line) {
	return (delta > 0.0f && line->sum_grow > 0.0f) || (delta < 0.0f && line->sum_shrink_weight > 0.0f);
}

#if XENT_ISPC_ENABLED
static void flex_fill_distribution_buffers(float *base_buffer, FlexLineData const *line, FlexChildData const *base) {
	float *grow_buffer   = base_buffer + line->count;
	float *shrink_buffer = grow_buffer + line->count;
	for (uint32_t i = 0u; i < line->count; ++i) {
		base_buffer [i]   = base [i].base_main;
		grow_buffer [i]   = base [i].grow;
		shrink_buffer [i] = base [i].shrink;
	}
}

static void flex_run_distribution_kernel(float *base_buffer, FlexLineData const *line, float delta) {
	float *grow_buffer   = base_buffer + line->count;
	float *shrink_buffer = grow_buffer + line->count;
	if (delta > 0.0f)
		xent_ispc_flex_distribute_grow(base_buffer, grow_buffer, base_buffer, line->count, delta, line->sum_grow);
	else
		xent_ispc_flex_distribute_shrink(
		  base_buffer, shrink_buffer, base_buffer, line->count, delta, line->sum_shrink_weight
		);
}

static void flex_copy_distribution_result(FlexChildData *base, float const *base_buffer, uint32_t count) {
	for (uint32_t i = 0u; i < count; ++i) base [i].final_main = base_buffer [i];
}

static bool
flex_distribute_line_main_ispc(XentCtx *ctx, FlexLineData const *line, FlexChildData *children, float delta) {
	if (!ctx->config.enable_simd || line->count < 32u || !flex_has_distribution(delta, line)) return false;
	if (delta > 0.0f && line->sum_grow < 1.0f) return false;
	float *base_buffer = xent_scratch_alloc(ctx, sizeof(float) * line->count * 3u, _Alignof(float));
	if (!base_buffer) return false;
	FlexChildData *base = &children [line->start];
	for (uint32_t i = 0u; i < line->count; ++i)
		if (base [i].min_main > 0.0f || isfinite(base [i].max_main)) return false;
	flex_fill_distribution_buffers(base_buffer, line, base);
	flex_run_distribution_kernel(base_buffer, line, delta);
	flex_copy_distribution_result(base, base_buffer, line->count);
	return true;
}
#endif

typedef struct FlexDistributionState {
	float remaining_delta;
	float remaining_grow;
	float remaining_shrink_wt;
	bool  growing;
} FlexDistributionState;

static FlexDistributionState flex_distribution_begin(FlexLineData const *line, float delta) {
	return (FlexDistributionState) {
	  delta,
	  line->sum_grow,
	  line->sum_shrink_weight,
	  delta > 0.0f,
	};
}

static void flex_reset_line_freeze(FlexLineData const *line, FlexChildData *children) {
	for (uint32_t i = 0u; i < line->count; ++i) children [line->start + i].frozen = false;
}

static float flex_distributed_unfrozen_main(FlexChildData const *entry, FlexDistributionState const *state) {
	float size = entry->base_main;
	if (state->growing && state->remaining_grow > 0.0f) {
		/* CSS Flexbox §9.7: when the sum of flex-grow factors is < 1, only
		 * (sum × free space) is distributed — i.e. each item gets free×grow with
		 * the factor NOT normalized. Clamp the denominator to 1 to express this. */
		float denom = state->remaining_grow < 1.0f ? 1.0f : state->remaining_grow;
		return size + state->remaining_delta * (entry->grow / denom);
	}
	if (!state->growing && state->remaining_shrink_wt > 0.0f) {
		float weight  = flex_child_shrink_weight(entry);
		size         += state->remaining_delta * (weight / state->remaining_shrink_wt);
		return size < 0.0f ? 0.0f : size;
	}
	return size;
}

static void
flex_assign_unfrozen_main(FlexLineData const *line, FlexChildData *children, FlexDistributionState const *state) {
	for (uint32_t i = 0u; i < line->count; ++i) {
		FlexChildData *entry = &children [line->start + i];
		if (!entry->frozen) entry->final_main = flex_distributed_unfrozen_main(entry, state);
	}
}

static bool flex_freeze_one_violation(FlexChildData *entry, FlexDistributionState *state) {
	if (entry->frozen) return false;
	float clamped = xent_clampf(entry->final_main, entry->min_main, entry->max_main);
	if (clamped == entry->final_main) return false;

	entry->final_main           = clamped;
	entry->frozen               = true;
	state->remaining_delta     -= clamped - entry->base_main;
	state->remaining_grow      -= entry->grow;
	state->remaining_shrink_wt -= flex_child_shrink_weight(entry);
	return true;
}

static bool flex_freeze_violations(FlexLineData const *line, FlexChildData *children, FlexDistributionState *state) {
	bool any_frozen = false;
	for (uint32_t i = 0u; i < line->count; ++i)
		any_frozen |= flex_freeze_one_violation(&children [line->start + i], state);
	if (state->remaining_grow < 0.0f) state->remaining_grow = 0.0f;
	if (state->remaining_shrink_wt < 0.0f) state->remaining_shrink_wt = 0.0f;
	return any_frozen;
}

static void flex_distribute_line_main(XentCtx *ctx, FlexLineData const *line, FlexChildData *children, float delta) {
#if XENT_ISPC_ENABLED
	if (flex_distribute_line_main_ispc(ctx, line, children, delta)) return;
#else
	( void ) ctx;
#endif
	FlexDistributionState state = flex_distribution_begin(line, delta);
	flex_reset_line_freeze(line, children);

	for (uint32_t iteration = 0u; iteration <= line->count; ++iteration) {
		flex_assign_unfrozen_main(line, children, &state);
		if (iteration == line->count || !flex_freeze_violations(line, children, &state)) break;
	}
}

#if XENT_ISPC_ENABLED
static bool
flex_occupied_main_ispc(XentCtx *ctx, FlexLineData const *line, FlexChildData const *children, float *out_sum) {
	if (!ctx->config.enable_simd || line->count < 32u) return false;

	float *fm = xent_scratch_alloc(ctx, sizeof(float) * line->count * 3u, _Alignof(float));
	if (!fm) return false;

	float               *ml   = fm + line->count;
	float               *mt   = ml + line->count;
	FlexChildData const *base = &children [line->start];
	for (uint32_t i = 0u; i < line->count; ++i) {
		fm [i] = base [i].final_main;
		ml [i] = base [i].margin_lead;
		mt [i] = base [i].margin_trail;
	}

	*out_sum = xent_ispc_sum_outer_main(fm, ml, mt, line->count);
	return true;
}
#endif

static float flex_occupied_main(XentCtx *ctx, FlexLineData const *line, FlexChildData const *children, float gap) {
	float occupied_main = line->count > 1u ? gap * ( float ) (line->count - 1u) : 0.0f;
#if XENT_ISPC_ENABLED
	float ispc_sum = 0.0f;
	if (flex_occupied_main_ispc(ctx, line, children, &ispc_sum)) return occupied_main + ispc_sum;
#else
	( void ) ctx;
#endif
	for (uint32_t i = 0u; i < line->count; ++i) {
		FlexChildData const *entry  = &children [line->start + i];
		occupied_main              += entry->final_main + entry->margin_lead + entry->margin_trail;
	}
	return occupied_main;
}

static void flex_resolve_line_justify(
  XentCtx *ctx, FlexLayoutFrame const *frame, FlexLineData const *line, FlexChildData const *children,
  FlexLinePlacement *placement
) {
	float occupied_main  = flex_occupied_main(ctx, line, children, frame->gap);
	float remaining_main = frame->available_main - occupied_main;
	/* Negative free space (overflow) is kept: flex_resolve_spacing offsets
	 * center/end past the start edge but never produces negative gaps. */
	flex_resolve_spacing(
	  (FlexSpacingRequest) {
		( FlexSpacingMode ) ctx->nodes.flex.justify_content [xent_node_index(frame->node)],
		line->count,
		frame->gap,
		remaining_main,
	  },
	  &placement->main_start_offset, &placement->effective_gap
	);
}

static XentFlexAlign flex_resolved_align(FlexLinePrepareContext const *prepare) {
	if (prepare->entry->align_self != ( uint8_t ) XENT_FLEX_ALIGN_AUTO)
		return ( XentFlexAlign ) prepare->entry->align_self;
	return ( XentFlexAlign ) prepare->ctx->nodes.flex.align_items [xent_node_index(prepare->frame->node)];
}

static bool flex_cross_auto(FlexLinePrepareContext const *prepare) {
	XentNodeId child = prepare->entry->id;
	if (prepare->frame->row)
		return isnan(prepare->ctx->nodes.layout.style_h [xent_node_index(child)])
		    && isnan(prepare->ctx->nodes.layout.style_h_percent [xent_node_index(child)]);
	return isnan(prepare->ctx->nodes.layout.style_w [xent_node_index(child)])
	    && isnan(prepare->ctx->nodes.layout.style_w_percent [xent_node_index(child)]);
}

static float flex_resolved_cross_size(FlexLinePrepareContext const *prepare, XentFlexAlign align) {
	float cross_size = prepare->entry->base_cross;
	if (align == XENT_FLEX_ALIGN_STRETCH && flex_cross_auto(prepare))
		cross_size
		  = prepare->line->cross_outer - prepare->entry->margin_cross_lead - prepare->entry->margin_cross_trail;
	/* Clamp the used cross size to the item's min/max cross (CSS: stretch and
	 * content sizes are both bounded by min-/max-*). This keeps final_cross equal
	 * to the size the item is actually dispatched at, so cross positioning (esp.
	 * RTL and center/end) uses the clamped extent, not the pre-clamp line size. */
	XentNodeId child = prepare->entry->id;
	float      min_c = prepare->frame->row ? prepare->ctx->nodes.layout.min_h [xent_node_index(child)]
	                                       : prepare->ctx->nodes.layout.min_w [xent_node_index(child)];
	float      max_c = prepare->frame->row ? prepare->ctx->nodes.layout.max_h [xent_node_index(child)]
	                                       : prepare->ctx->nodes.layout.max_w [xent_node_index(child)];
	cross_size       = xent_clampf(cross_size, min_c, max_c);
	return cross_size < 0.0f ? 0.0f : cross_size;
}

static void flex_note_baseline(FlexLinePrepareContext const *prepare, XentFlexAlign align) {
	if (!prepare->frame->row || align != XENT_FLEX_ALIGN_BASELINE) return;
	float baseline_edge = prepare->entry->margin_cross_lead + prepare->entry->baseline_from_top;
	if (!prepare->placement->has_baseline_items || baseline_edge > prepare->placement->baseline_target)
		prepare->placement->baseline_target = baseline_edge;
	prepare->placement->has_baseline_items = true;
}

static void flex_prepare_child(FlexLinePrepareContext const *prepare) {
	XentFlexAlign align               = flex_resolved_align(prepare);
	float         cross_size          = flex_resolved_cross_size(prepare, align);
	prepare->entry->resolved_align    = ( uint8_t ) align;
	prepare->entry->final_cross       = cross_size;
	prepare->entry->baseline_from_top = xent_node_baseline(prepare->ctx, prepare->entry->id, cross_size);
	flex_note_baseline(prepare, align);
}

static void flex_prepare_children(
  XentCtx *ctx, FlexLayoutFrame const *frame, FlexLineData const *line, FlexChildData *children,
  FlexLinePlacement *placement
) {
	for (uint32_t i = 0u; i < line->count; ++i) {
		FlexLinePrepareContext prepare = {ctx, frame, line, placement, &children [line->start + i]};
		flex_prepare_child(&prepare);
	}
}

static float flex_baseline_max_descent(FlexLineData const *line, FlexChildData const *children) {
	float max_descent = 0.0f;
	for (uint32_t i = 0u; i < line->count; ++i) {
		FlexChildData const *entry = &children [line->start + i];
		if (( XentFlexAlign ) entry->resolved_align != XENT_FLEX_ALIGN_BASELINE) continue;
		float descent = entry->margin_cross_trail + entry->final_cross - entry->baseline_from_top;
		if (descent > max_descent) max_descent = descent;
	}
	return max_descent;
}

static void
flex_expand_baseline_line(XentCtx *ctx, FlexLayoutFrame const *frame, FlexLineData *line, FlexChildData *children) {
	if (!frame->row) return;
	FlexLinePlacement placement = {0};
	flex_prepare_children(ctx, frame, line, children, &placement);
	if (!placement.has_baseline_items) return;
	float required = placement.baseline_target + flex_baseline_max_descent(line, children);
	if (required > line->cross_outer) line->cross_outer = required;
}

static float flex_size_line(XentCtx *ctx, FlexLayoutFrame const *frame, FlexLineData *line, FlexChildData *children);

static FlexLinePlacement
flex_prepare_line(XentCtx *ctx, FlexLayoutFrame const *frame, FlexLineData const *line, FlexChildData *children) {
	FlexLinePlacement placement = {0};
	flex_distribute_line_main(ctx, line, children, frame->available_main - line->base_main);
	flex_resolve_line_justify(ctx, frame, line, children, &placement);
	flex_prepare_children(ctx, frame, line, children, &placement);
	return placement;
}

static float flex_cross_offset(
  FlexLayoutFrame const *frame, FlexLineData const *line, FlexLinePlacement const *placement, FlexChildData const *entry
) {
	XentFlexAlign align = ( XentFlexAlign ) entry->resolved_align;
	/* cross_free may be NEGATIVE when the item overflows its line — center/end
	 * then offset it past the cross-start edge (CSS). Baseline clamps within the
	 * non-negative range. */
	float cross_free    = line->cross_outer - entry->final_cross - entry->margin_cross_lead - entry->margin_cross_trail;

	if (frame->row && align == XENT_FLEX_ALIGN_BASELINE && placement->has_baseline_items) {
		float desired = placement->baseline_target - entry->baseline_from_top;
		float top     = cross_free > 0.0f ? cross_free : 0.0f;
		return xent_clampf(desired, entry->margin_cross_lead, entry->margin_cross_lead + top);
	}
	if (align == XENT_FLEX_ALIGN_END) return entry->margin_cross_lead + cross_free;
	if (align == XENT_FLEX_ALIGN_CENTER) return entry->margin_cross_lead + (cross_free * 0.5f);
	return entry->margin_cross_lead;
}

static void flex_child_size(FlexLayoutFrame const *frame, FlexChildData const *entry, float *out_w, float *out_h) {
	*out_w = frame->row ? entry->final_main : entry->final_cross;
	*out_h = frame->row ? entry->final_cross : entry->final_main;
	if (*out_w < 0.0f) *out_w = 0.0f;
	if (*out_h < 0.0f) *out_h = 0.0f;
}

static float flex_child_origin_x(
  FlexLayoutFrame const *frame, float main_cursor, float cross_cursor, float cross_offset, float child_w
) {
	if (frame->row && frame->rtl_main) return frame->content_x + (frame->content_w - main_cursor - child_w);
	if (frame->row) return frame->content_x + main_cursor;
	if (frame->rtl_cross) return frame->content_x + (frame->content_w - cross_cursor - cross_offset - child_w);
	return frame->content_x + cross_cursor + cross_offset;
}

static float
flex_child_origin_y(FlexLayoutFrame const *frame, float main_cursor, float cross_cursor, float cross_offset) {
	return frame->row ? frame->content_y + cross_cursor + cross_offset : frame->content_y + main_cursor;
}

static void flex_layout_line_children(FlexLayoutChildrenRequest const *request) {
	XentCtx                 *ctx          = request->ctx;
	FlexLayoutFrame const   *frame        = request->frame;
	FlexLineData const      *line         = request->line;
	FlexLinePlacement const *placement    = request->placement;
	FlexChildData           *children     = request->children;
	float                    cross_cursor = request->cross_cursor;
	float                    main_cursor  = placement->main_start_offset;
	for (uint32_t i = 0u; i < line->count; ++i) {
		FlexChildData *entry  = &children [line->start + i];
		main_cursor          += entry->margin_lead;

		float cross_offset    = flex_cross_offset(frame, line, placement, entry);
		float child_w         = 0.0f;
		float child_h         = 0.0f;
		flex_child_size(frame, entry, &child_w, &child_h);

		float child_origin_x = flex_child_origin_x(frame, main_cursor, cross_cursor, cross_offset, child_w);
		float child_origin_y = flex_child_origin_y(frame, main_cursor, cross_cursor, cross_offset);
		xent_layout_dispatch_node(&(XentLayoutRequest) {
		  ctx, entry->id, child_w, child_h, child_origin_x, child_origin_y, true, true});
		main_cursor += entry->final_main + entry->margin_trail;
		if (i + 1u < line->count) main_cursor += placement->effective_gap;
	}
}

static void flex_size_lines(
  XentCtx *ctx, FlexLayoutFrame const *frame, FlexChildData *children, FlexLineData *lines, uint32_t line_count
) {
	for (uint32_t li = 0u; li < line_count; ++li) {
		float cross = flex_size_line(ctx, frame, &lines [li], children);
		if (frame->wrap_enabled) lines [li].cross_outer = cross;
	}
}

static void flex_layout_lines(
  XentCtx *ctx, FlexLayoutFrame const *frame, FlexChildData *children, FlexLineData *lines, uint32_t line_count
) {
	flex_size_lines(ctx, frame, children, lines, line_count);
	for (uint32_t li = 0u; li < line_count; ++li) flex_expand_baseline_line(ctx, frame, &lines [li], children);
	float line_gap          = 0.0f;
	float total_lines_cross = flex_total_lines_cross(frame, lines, line_count, &line_gap);
	float remaining_cross   = frame->available_cross - total_lines_cross;
	if (remaining_cross < 0.0f) remaining_cross = 0.0f;

	/* align-content: stretch — grow each flex line's cross size equally to absorb
	 * the free cross space, then resolve spacing with none left (no gaps/offset).
	 * Items with align-stretch then fill the grown line (flex_prepare_line below
	 * reads the updated cross_outer). */
	if (( XentFlexAlignContent ) ctx->nodes.flex.align_content [xent_node_index(frame->node)]
		  == XENT_FLEX_ALIGN_CONTENT_STRETCH
		&& line_count > 0u
		&& remaining_cross > 0.0f)
	{
		float add = remaining_cross / ( float ) line_count;
		for (uint32_t li = 0u; li < line_count; ++li) lines [li].cross_outer += add;
		remaining_cross = 0.0f;
	}

	float cross_start_offset = 0.0f;
	float effective_line_gap = line_gap;
	flex_resolve_spacing(
	  (FlexSpacingRequest) {
		( FlexSpacingMode ) ctx->nodes.flex.align_content [xent_node_index(frame->node)],
		line_count,
		line_gap,
		remaining_cross,
	  },
	  &cross_start_offset, &effective_line_gap
	);

	float cross_cursor = cross_start_offset;
	for (uint32_t li = 0u; li < line_count; ++li) {
		FlexLinePlacement placement = flex_prepare_line(ctx, frame, &lines [li], children);
		flex_layout_line_children(&(FlexLayoutChildrenRequest) {
		  ctx, frame, &lines [li], &placement, children, cross_cursor});
		cross_cursor += lines [li].cross_outer;
		if (li + 1u < line_count) cross_cursor += effective_line_gap;
	}
}

/* Build a position-free frame for measuring a flex container's content size. */
static FlexLayoutFrame
flex_measure_frame(XentCtx *ctx, XentNodeId node, float avail_main, float avail_cross, bool row) {
	FlexLayoutFrame frame = {0};
	frame.node            = node;
	frame.row             = row;
	frame.wrap_enabled    = ctx->nodes.flex.wrap [xent_node_index(node)] == ( uint8_t ) XENT_FLEX_WRAP;
	frame.content_w       = row ? avail_main : avail_cross;
	frame.content_h       = row ? avail_cross : avail_main;
	frame.available_main  = avail_main;
	frame.available_cross = avail_cross;
	frame.gap             = ctx->nodes.layout.gap [xent_node_index(node)];
	if (frame.gap < 0.0f) frame.gap = 0.0f;
	frame.sizing_mode = ctx->sizing_mode;
	return frame;
}

/* Resolve a line's used main sizes, then measure each item's hypothetical cross
 * at that used main (Flexbox §9.4 algo-cross-item) and return the line's cross
 * extent (algo-cross-line: the largest outer hypothetical cross). When the main
 * axis is indefinite (max-content sizing) each item keeps its hypothetical main. */
static float flex_size_line(XentCtx *ctx, FlexLayoutFrame const *frame, FlexLineData *line, FlexChildData *children) {
	if (isfinite(frame->available_main))
		flex_distribute_line_main(ctx, line, children, frame->available_main - line->base_main);
	else
		for (uint32_t i = 0u; i < line->count; ++i)
			children [line->start + i].final_main = children [line->start + i].hypothetical_main;

	float max_cross = 0.0f;
	for (uint32_t i = 0u; i < line->count; ++i) {
		FlexChildData *entry = &children [line->start + i];
		entry->base_cross
		  = xent_compute_hypothetical_cross(ctx, entry->id, frame->row, entry->final_main, frame->available_cross);
		float outer = flex_child_cross_outer(entry);
		if (outer > max_cross) max_cross = outer;
	}
	return max_cross;
}

static void flex_sum_intrinsic_lines(
  XentCtx *ctx, FlexLayoutFrame *frame, FlexChildData *children, FlexLineData *lines, uint32_t line_count,
  float *out_main, float *out_cross
) {
	float total_cross = 0.0f;
	float max_main    = 0.0f;
	for (uint32_t i = 0u; i < line_count; ++i) {
		float line_cross      = flex_size_line(ctx, frame, &lines [i], children);
		float line_main       = flex_occupied_main(ctx, &lines [i], children, frame->gap);
		lines [i].cross_outer = line_cross;
		if (line_main > max_main) max_main = line_main;
		total_cross += line_cross;
	}
	if (line_count > 1u && frame->wrap_enabled) total_cross += frame->gap * ( float ) (line_count - 1u);
	*out_main  = max_main;
	*out_cross = total_cross;
}

void xent_flex_intrinsic_content(
  XentCtx *ctx, XentNodeId node, float avail_main, float avail_cross, bool row, float *out_main, float *out_cross
) {
	*out_main    = 0.0f;
	*out_cross   = 0.0f;
	uint32_t cap = ctx->nodes.topology.child_count [xent_node_index(node)];
	if (cap == 0u) return;

	FlexChildData *children = NULL;
	FlexLineData  *lines    = NULL;
	if (!flex_alloc_buffers(ctx, cap, &children, &lines)) return;

	FlexLayoutFrame frame = flex_measure_frame(ctx, node, avail_main, avail_cross, row);
	uint32_t        count = flex_collect_children(ctx, &frame, children, cap);
	if (count > 0u) {
		uint32_t line_count = flex_build_lines(&frame, children, count, lines);
		flex_compute_line_set(ctx, &frame, children, lines, line_count);
		flex_sum_intrinsic_lines(ctx, &frame, children, lines, line_count, out_main, out_cross);
	}
}

static float flex_avail_main(bool hug, float dim, float avail, float pad) {
	if (!isnan(dim)) return dim - pad;
	return hug ? INFINITY : avail - pad;
}

static float flex_avail_cross(float dim, float avail, float pad) {
	if (!isnan(dim)) return dim - pad;
	return isfinite(avail) ? avail - pad : INFINITY;
}

static bool flex_axis_wrap(float dim, uint8_t wrap_flag, float available) {
	return isnan(dim) && (wrap_flag != 0u || !isfinite(available));
}

typedef struct FlexWrapAxes {
	bool  row;
	bool  wrap_w;
	bool  wrap_h;
	float pad_w;
	float pad_h;
	float avail_main;
	float avail_cross;
} FlexWrapAxes;

static FlexWrapAxes
flex_wrap_axes(XentCtx *ctx, uint32_t idx, float available_w, float available_h, float width, float height) {
	FlexWrapAxes a = {0};
	a.wrap_w       = flex_axis_wrap(width, ctx->nodes.layout.wrap_content_w [idx], available_w);
	a.wrap_h       = flex_axis_wrap(height, ctx->nodes.layout.wrap_content_h [idx], available_h);
	a.row          = ctx->nodes.flex.direction [idx] == ( uint8_t ) XENT_FLEX_ROW;
	a.pad_w        = ctx->nodes.layout.padding_l [idx] + ctx->nodes.layout.padding_r [idx];
	a.pad_h        = ctx->nodes.layout.padding_t [idx] + ctx->nodes.layout.padding_b [idx];
	a.avail_main   = flex_avail_main(
	  a.row ? a.wrap_w : a.wrap_h, a.row ? width : height, a.row ? available_w : available_h, a.row ? a.pad_w : a.pad_h
	);
	a.avail_cross
	  = flex_avail_cross(a.row ? height : width, a.row ? available_h : available_w, a.row ? a.pad_h : a.pad_w);
	return a;
}

static void
flex_apply_wrap_extents(FlexWrapAxes const *a, float main_extent, float cross_extent, float *width, float *height) {
	if (a->wrap_w) *width = (a->row ? main_extent : cross_extent) + a->pad_w;
	if (a->wrap_h) *height = (a->row ? cross_extent : main_extent) + a->pad_h;
}

void xent_flex_wrap_content(
  XentCtx *ctx, XentNodeId node, float available_w, float available_h, float *width, float *height
) {
	uint32_t     idx = xent_node_index(node);
	FlexWrapAxes a   = flex_wrap_axes(ctx, idx, available_w, available_h, *width, *height);
	if ((!a.wrap_w && !a.wrap_h) || ctx->nodes.layout.protocol [idx] != ( uint8_t ) XENT_PROTOCOL_FLEX) return;

	float main_extent = 0.0f, cross_extent = 0.0f;
	xent_flex_intrinsic_content(ctx, node, a.avail_main, a.avail_cross, a.row, &main_extent, &cross_extent);
	flex_apply_wrap_extents(&a, main_extent, cross_extent, width, height);
}

void xent_layout_node_flex(XentLayoutRequest const *request) {
	XentCtx   *ctx                  = request->ctx;
	XentNodeId node                 = request->node;
	double     start                = xent_now_ms();
	ctx->profile.flex_layout_calls += 1u;
	ctx->flex_scope_depth          += 1u;
	FlexLayoutFrame frame           = {0};
	flex_begin_layout(request, &frame);

	uint32_t child_capacity = ctx->nodes.topology.child_count [xent_node_index(node)];
	if (child_capacity == 0u) goto finish;

	FlexChildData *children = NULL;
	FlexLineData  *lines    = NULL;
	if (!flex_alloc_buffers(ctx, child_capacity, &children, &lines)) goto finish;

	double   collect_start        = xent_now_ms();
	uint32_t child_count          = flex_collect_children(ctx, &frame, children, child_capacity);
	ctx->profile.flex_collect_ms += (xent_now_ms() - collect_start);
	if (child_count > 0u) {
		double   line_start = xent_now_ms();
		uint32_t line_count = flex_build_lines(&frame, children, child_count, lines);
		flex_compute_line_set(ctx, &frame, children, lines, line_count);
		flex_layout_lines(ctx, &frame, children, lines, line_count);
		ctx->profile.flex_line_ms += (xent_now_ms() - line_start);
	}

finish:
	ctx->flex_scope_depth      -= 1u;
	ctx->profile.flex_total_ms += (xent_now_ms() - start);
}
