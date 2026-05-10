#include "../xent_internal.h"

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
} FlexLayoutFrame;

typedef struct FlexLinePlacement {
	float main_start_offset;
	float effective_gap;
	float baseline_target;
	bool  has_baseline_items;
} FlexLinePlacement;

typedef struct FlexLayoutChildrenRequest {
	XentContext             *ctx;
	FlexLayoutFrame const   *frame;
	FlexLineData const      *line;
	FlexLinePlacement const *placement;
	FlexChildData           *children;
	float                    cross_cursor;
} FlexLayoutChildrenRequest;

typedef struct FlexLinePrepareContext {
	XentContext const     *ctx;
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
	FlexSpacingRule rule = flex_spacing_rule(request.mode);
	*out_start_offset    = request.remaining * rule.start_fixed
	                     + flex_spacing_share(request.remaining, rule.start_share, request.item_count, rule.start_add);
	*out_effective_gap
	  = request.base_gap + flex_spacing_share(request.remaining, rule.gap_share, request.item_count, rule.gap_add);
}

static float flex_child_shrink_weight(FlexChildData const *child) {
	return child->shrink * (child->base_main > 0.0f ? child->base_main : 1.0f);
}

static float flex_child_cross_outer(FlexChildData const *child) {
	return child->base_cross + child->margin_cross_lead + child->margin_cross_trail;
}

#if XENT_ISPC_ENABLED
static bool flex_compute_line_stats_ispc(
  FlexLineData *line, FlexChildData const *base, uint32_t count, bool wrap_enabled, float gap
) {
	if (count < 32u) return false;

	float *bm = ( float * ) malloc(sizeof(float) * count * 8u);
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
	free(bm);
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

static void xent_compute_line_stats(
  FlexLineData *line, FlexChildData const *children, bool wrap_enabled, float available_cross, float gap
) {
	line->base_main            = 0.0f;
	line->sum_grow             = 0.0f;
	line->sum_shrink_weight    = 0.0f;
	line->cross_outer          = wrap_enabled ? 0.0f : available_cross;

	uint32_t             count = line->count;
	FlexChildData const *base  = &children [line->start];
#if XENT_ISPC_ENABLED
	if (flex_compute_line_stats_ispc(line, base, count, wrap_enabled, gap)) return;
#endif
	flex_compute_line_stats_scalar(line, base, count, wrap_enabled, gap);
}

static float xent_clampf(float value, float min_v, float max_v) {
	if (value < min_v) return min_v;
	if (value > max_v) return max_v;
	return value;
}

static float xent_estimate_baseline(XentContext const *ctx, XentNodeId node, float cross_size) {
	if (cross_size <= 0.0f) return 0.0f;
	if (ctx->nodes.text.content [node] && ctx->nodes.text.content [node][0] != '\0') return cross_size * 0.8f;
	return cross_size;
}

static void flex_begin_layout(XentLayoutRequest const *request, FlexLayoutFrame *frame) {
	XentContext *ctx         = request->ctx;
	XentNodeId   node        = request->node;
	float        available_w = request->available_w;
	float        available_h = request->available_h;
	float        origin_x    = request->origin_x;
	float        origin_y    = request->origin_y;
	float        width       = 0.0f;
	float        height      = 0.0f;
	xent_compute_intrinsic_size(ctx, node, available_w, available_h, &width, &height);

	ctx->nodes.layout.proposed_w [node] = available_w;
	ctx->nodes.layout.proposed_h [node] = available_h;
	ctx->nodes.layout.decided_w [node]  = width;
	ctx->nodes.layout.decided_h [node]  = height;
	ctx->nodes.layout.abs_x [node]      = origin_x + ctx->nodes.layout.abs_pos_x [node];
	ctx->nodes.layout.abs_y [node]      = origin_y + ctx->nodes.layout.abs_pos_y [node];
	xent_quantize_node_layout(ctx, node);

	width                   = ctx->nodes.layout.decided_w [node];
	height                  = ctx->nodes.layout.decided_h [node];
	frame->node             = node;
	frame->row              = ctx->nodes.flex.direction [node] == ( uint8_t ) XENT_FLEX_ROW;
	frame->wrap_enabled     = ctx->nodes.flex.wrap [node] == ( uint8_t ) XENT_FLEX_WRAP;
	XentDirection direction = xent_get_resolved_direction(ctx, node);
	frame->rtl_main         = frame->row && direction == XENT_DIRECTION_RTL;
	frame->rtl_cross        = !frame->row && direction == XENT_DIRECTION_RTL;
	frame->content_x        = ctx->nodes.layout.abs_x [node] + ctx->nodes.layout.padding_l [node];
	frame->content_y        = ctx->nodes.layout.abs_y [node] + ctx->nodes.layout.padding_t [node];
	frame->content_w        = width - (ctx->nodes.layout.padding_l [node] + ctx->nodes.layout.padding_r [node]);
	frame->content_h        = height - (ctx->nodes.layout.padding_t [node] + ctx->nodes.layout.padding_b [node]);
	if (frame->content_w < 0.0f) frame->content_w = 0.0f;
	if (frame->content_h < 0.0f) frame->content_h = 0.0f;
	frame->available_main  = frame->row ? frame->content_w : frame->content_h;
	frame->available_cross = frame->row ? frame->content_h : frame->content_w;
	frame->gap             = ctx->nodes.layout.gap [node];
	if (frame->gap < 0.0f) frame->gap = 0.0f;
}

static bool flex_alloc_buffers(uint32_t child_count, FlexChildData **out_children, FlexLineData **out_lines) {
	FlexChildData *children = ( FlexChildData * ) calloc(( size_t ) child_count, sizeof(FlexChildData));
	FlexLineData  *lines    = ( FlexLineData * ) calloc(( size_t ) child_count, sizeof(FlexLineData));
	if (!children || !lines) {
		free(children);
		free(lines);
		return false;
	}
	*out_children = children;
	*out_lines    = lines;
	return true;
}

static float flex_main_margin_lead(XentContext const *ctx, FlexLayoutFrame const *frame, XentNodeId child) {
	if (!frame->row) return ctx->nodes.layout.margin_t [child];
	return frame->rtl_main ? ctx->nodes.layout.margin_r [child] : ctx->nodes.layout.margin_l [child];
}

static float flex_main_margin_trail(XentContext const *ctx, FlexLayoutFrame const *frame, XentNodeId child) {
	if (!frame->row) return ctx->nodes.layout.margin_b [child];
	return frame->rtl_main ? ctx->nodes.layout.margin_l [child] : ctx->nodes.layout.margin_r [child];
}

static float flex_cross_margin_lead(XentContext const *ctx, FlexLayoutFrame const *frame, XentNodeId child) {
	if (frame->row) return ctx->nodes.layout.margin_t [child];
	return frame->rtl_cross ? ctx->nodes.layout.margin_r [child] : ctx->nodes.layout.margin_l [child];
}

static float flex_cross_margin_trail(XentContext const *ctx, FlexLayoutFrame const *frame, XentNodeId child) {
	if (frame->row) return ctx->nodes.layout.margin_b [child];
	return frame->rtl_cross ? ctx->nodes.layout.margin_l [child] : ctx->nodes.layout.margin_r [child];
}

static void
flex_describe_child_size(XentContext *ctx, FlexLayoutFrame const *frame, XentNodeId child, FlexChildData *data) {
	float intrinsic_w = 0.0f;
	float intrinsic_h = 0.0f;
	xent_compute_intrinsic_size(ctx, child, frame->content_w, frame->content_h, &intrinsic_w, &intrinsic_h);

	float basis     = ctx->nodes.flex.basis [child];
	float base_main = frame->row ? intrinsic_w : intrinsic_h;
	if (!isnan(basis)) base_main = basis;

	data->base_main         = base_main < 0.0f ? 0.0f : base_main;
	data->hypothetical_main = xent_clampf(data->base_main, data->min_main, data->max_main);
	data->base_cross        = frame->row ? intrinsic_h : intrinsic_w;
	data->final_cross       = data->base_cross;
}

static FlexChildData flex_describe_child(XentContext *ctx, FlexLayoutFrame const *frame, XentNodeId child) {
	FlexChildData data      = {0};
	data.id                 = child;
	data.margin_lead        = flex_main_margin_lead(ctx, frame, child);
	data.margin_trail       = flex_main_margin_trail(ctx, frame, child);
	data.margin_cross_lead  = flex_cross_margin_lead(ctx, frame, child);
	data.margin_cross_trail = flex_cross_margin_trail(ctx, frame, child);
	data.grow               = ctx->nodes.flex.grow [child];
	data.shrink             = ctx->nodes.flex.shrink [child];
	data.align_self         = ctx->nodes.flex.align_self [child];
	data.min_main           = frame->row ? ctx->nodes.layout.min_w [child] : ctx->nodes.layout.min_h [child];
	data.max_main           = frame->row ? ctx->nodes.layout.max_w [child] : ctx->nodes.layout.max_h [child];
	flex_describe_child_size(ctx, frame, child, &data);
	return data;
}

static uint32_t flex_collect_children(
  XentContext *ctx, FlexLayoutFrame const *frame, FlexChildData *children, uint32_t child_capacity
) {
	uint32_t   count = 0u;
	XentNodeId child = ctx->nodes.topology.first_child [frame->node];
	while (child != XENT_NODE_INVALID && count < child_capacity) {
		children [count++] = flex_describe_child(ctx, frame, child);
		child              = ctx->nodes.topology.next_sibling [child];
	}
	return count;
}

static uint32_t flex_build_lines(
  FlexLayoutFrame const *frame, FlexChildData const *children, uint32_t child_count, FlexLineData *lines
) {
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
  FlexLayoutFrame const *frame, FlexChildData const *children, FlexLineData *lines, uint32_t line_count
) {
	for (uint32_t i = 0u; i < line_count; ++i)
		xent_compute_line_stats(&lines [i], children, frame->wrap_enabled, frame->available_cross, frame->gap);
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

static bool flex_distribute_line_main_ispc(FlexLineData const *line, FlexChildData *children, float delta) {
	if (line->count < 32u || !flex_has_distribution(delta, line)) return false;
	float *base_buffer = ( float * ) malloc(sizeof(float) * line->count * 3u);
	if (!base_buffer) return false;
	FlexChildData *base = &children [line->start];
	for (uint32_t i = 0u; i < line->count; ++i) {
		if (base [i].min_main > 0.0f || isfinite(base [i].max_main)) {
			free(base_buffer);
			return false;
		}
	}
	flex_fill_distribution_buffers(base_buffer, line, base);
	flex_run_distribution_kernel(base_buffer, line, delta);
	flex_copy_distribution_result(base, base_buffer, line->count);
	free(base_buffer);
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
	if (state->growing && state->remaining_grow > 0.0f)
		return size + state->remaining_delta * (entry->grow / state->remaining_grow);
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

static void flex_distribute_line_main(FlexLineData const *line, FlexChildData *children, float delta) {
#if XENT_ISPC_ENABLED
	if (flex_distribute_line_main_ispc(line, children, delta)) return;
#endif
	FlexDistributionState state = flex_distribution_begin(line, delta);
	flex_reset_line_freeze(line, children);

	for (uint32_t iteration = 0u; iteration <= line->count; ++iteration) {
		flex_assign_unfrozen_main(line, children, &state);
		if (iteration == line->count || !flex_freeze_violations(line, children, &state)) break;
	}
}

#if XENT_ISPC_ENABLED
static bool flex_occupied_main_ispc(FlexLineData const *line, FlexChildData const *children, float *out_sum) {
	if (line->count < 32u) return false;

	float *fm = ( float * ) malloc(sizeof(float) * line->count * 3u);
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
	free(fm);
	return true;
}
#endif

static float flex_occupied_main(FlexLineData const *line, FlexChildData const *children, float gap) {
	float occupied_main = line->count > 1u ? gap * ( float ) (line->count - 1u) : 0.0f;
#if XENT_ISPC_ENABLED
	float ispc_sum = 0.0f;
	if (flex_occupied_main_ispc(line, children, &ispc_sum)) return occupied_main + ispc_sum;
#endif
	for (uint32_t i = 0u; i < line->count; ++i) {
		FlexChildData const *entry  = &children [line->start + i];
		occupied_main              += entry->final_main + entry->margin_lead + entry->margin_trail;
	}
	return occupied_main;
}

static void flex_resolve_line_justify(
  XentContext const *ctx, FlexLayoutFrame const *frame, FlexLineData const *line, FlexChildData const *children,
  FlexLinePlacement *placement
) {
	float occupied_main  = flex_occupied_main(line, children, frame->gap);
	float remaining_main = frame->available_main - occupied_main;
	if (remaining_main < 0.0f) remaining_main = 0.0f;
	flex_resolve_spacing(
	  (FlexSpacingRequest) {
		( FlexSpacingMode ) ctx->nodes.flex.justify_content [frame->node],
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
	return ( XentFlexAlign ) prepare->ctx->nodes.flex.align_items [prepare->frame->node];
}

static bool flex_cross_auto(FlexLinePrepareContext const *prepare) {
	XentNodeId child = prepare->entry->id;
	if (prepare->frame->row)
		return isnan(prepare->ctx->nodes.layout.style_h [child])
		    && isnan(prepare->ctx->nodes.layout.style_h_percent [child]);
	return isnan(prepare->ctx->nodes.layout.style_w [child])
	    && isnan(prepare->ctx->nodes.layout.style_w_percent [child]);
}

static float flex_resolved_cross_size(FlexLinePrepareContext const *prepare, XentFlexAlign align) {
	float cross_size = prepare->entry->base_cross;
	if (align == XENT_FLEX_ALIGN_STRETCH && flex_cross_auto(prepare))
		cross_size
		  = prepare->line->cross_outer - prepare->entry->margin_cross_lead - prepare->entry->margin_cross_trail;
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
	prepare->entry->baseline_from_top = xent_estimate_baseline(prepare->ctx, prepare->entry->id, cross_size);
	flex_note_baseline(prepare, align);
}

static FlexLinePlacement
flex_prepare_line(XentContext *ctx, FlexLayoutFrame const *frame, FlexLineData const *line, FlexChildData *children) {
	FlexLinePlacement placement = {0};
	flex_distribute_line_main(line, children, frame->available_main - line->base_main);
	flex_resolve_line_justify(ctx, frame, line, children, &placement);

	for (uint32_t i = 0u; i < line->count; ++i) {
		FlexLinePrepareContext prepare = {ctx, frame, line, &placement, &children [line->start + i]};
		flex_prepare_child(&prepare);
	}
	return placement;
}

static float flex_cross_offset(
  FlexLayoutFrame const *frame, FlexLineData const *line, FlexLinePlacement const *placement, FlexChildData const *entry
) {
	XentFlexAlign align = ( XentFlexAlign ) entry->resolved_align;
	float cross_free    = line->cross_outer - entry->final_cross - entry->margin_cross_lead - entry->margin_cross_trail;
	if (cross_free < 0.0f) cross_free = 0.0f;

	if (frame->row && align == XENT_FLEX_ALIGN_BASELINE && placement->has_baseline_items) {
		float desired = placement->baseline_target - entry->baseline_from_top;
		return xent_clampf(desired, entry->margin_cross_lead, entry->margin_cross_lead + cross_free);
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
	XentContext             *ctx          = request->ctx;
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
		  ctx, entry->id, child_w, child_h, child_origin_x, child_origin_y});
		main_cursor += entry->final_main + entry->margin_trail;
		if (i + 1u < line->count) main_cursor += placement->effective_gap;
	}
}

static void flex_layout_lines(
  XentContext *ctx, FlexLayoutFrame const *frame, FlexChildData *children, FlexLineData const *lines,
  uint32_t line_count
) {
	float line_gap          = 0.0f;
	float total_lines_cross = flex_total_lines_cross(frame, lines, line_count, &line_gap);
	float remaining_cross   = frame->available_cross - total_lines_cross;
	if (remaining_cross < 0.0f) remaining_cross = 0.0f;

	float cross_start_offset = 0.0f;
	float effective_line_gap = line_gap;
	flex_resolve_spacing(
	  (FlexSpacingRequest) {
		( FlexSpacingMode ) ctx->nodes.flex.align_content [frame->node],
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

void xent_layout_node_flex(XentLayoutRequest const *request) {
	XentContext    *ctx   = request->ctx;
	XentNodeId      node  = request->node;
	FlexLayoutFrame frame = {0};
	flex_begin_layout(request, &frame);

	uint32_t child_capacity = ctx->nodes.topology.child_count [node];
	if (child_capacity == 0u) return;

	FlexChildData *children = NULL;
	FlexLineData  *lines    = NULL;
	if (!flex_alloc_buffers(child_capacity, &children, &lines)) return;

	uint32_t child_count = flex_collect_children(ctx, &frame, children, child_capacity);
	if (child_count > 0u) {
		uint32_t line_count = flex_build_lines(&frame, children, child_count, lines);
		flex_compute_line_set(&frame, children, lines, line_count);
		flex_layout_lines(ctx, &frame, children, lines, line_count);
	}

	free(children);
	free(lines);
}
