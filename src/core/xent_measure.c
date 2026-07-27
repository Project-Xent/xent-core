#include "../xent_internal.h"

#include "xent/xent_measure.h"

#include <math.h>
#include <string.h>

typedef struct XentExternalMeasureSlot {
	XentNodeId            owner;
	XentExternalMeasureFn fn;
	void                 *userdata;
} XentExternalMeasureSlot;

static bool measure_reserve(XentCtx *ctx, uint32_t idx) {
	if (idx < ctx->external_measure_cap) return true;
	uint32_t new_cap = ctx->external_measure_cap ? ctx->external_measure_cap * 2u : 32u;
	while (new_cap <= idx) {
		if (new_cap > UINT32_MAX / 2u) return false;
		new_cap *= 2u;
	}
	XentExternalMeasureSlot *n
	  = ( XentExternalMeasureSlot * ) realloc(ctx->external_measures, sizeof(*n) * ( size_t ) new_cap);
	if (!n) return false;
	memset(n + ctx->external_measure_cap, 0, sizeof(*n) * ( size_t ) (new_cap - ctx->external_measure_cap));
	ctx->external_measures    = n;
	ctx->external_measure_cap = new_cap;
	return true;
}

static XentExternalMeasureSlot *measure_owned(XentCtx const *ctx, XentNodeId node) {
	uint32_t idx = xent_node_index(node);
	if (!ctx || idx >= ctx->external_measure_cap) return NULL;
	XentExternalMeasureSlot *slot = &ctx->external_measures [idx];
	return (slot->fn && slot->owner == node) ? slot : NULL;
}

bool xent_node_setmeasure(XentCtx *ctx, XentNodeId node, XentExternalMeasureFn fn, void *userdata) {
	if (!ctx || !fn || !xent_node_valid(ctx, node)) return false;
	uint32_t idx = xent_node_index(node);
	if (!measure_reserve(ctx, idx)) return false;
	XentExternalMeasureSlot *slot = &ctx->external_measures [idx];
	slot->owner                   = node;
	slot->fn                      = fn;
	slot->userdata                = userdata;
	xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT | XENT_DIRTY_SELF);
	return true;
}

bool xent_node_clearmeasure(XentCtx *ctx, XentNodeId node) {
	if (!ctx || node == XENT_NODE_INVALID) return false;
	XentExternalMeasureSlot *slot = measure_owned(ctx, node);
	if (!slot) return false;
	slot->owner    = XENT_NODE_INVALID;
	slot->fn       = NULL;
	slot->userdata = NULL;
	if (xent_node_valid(ctx, node)) xent_mark_dirty(ctx, node, XENT_DIRTY_LAYOUT | XENT_DIRTY_SELF);
	return true;
}

bool xent_node_hasmeasure(XentCtx const *ctx, XentNodeId node) { return measure_owned(ctx, node) != NULL; }

void xent_extmeasure_on_destroy(XentCtx *ctx, XentNodeId node) {
	XentExternalMeasureSlot *slot = measure_owned(ctx, node);
	if (!slot) return;
	slot->owner    = XENT_NODE_INVALID;
	slot->fn       = NULL;
	slot->userdata = NULL;
}

void xent_extmeasure_clear(XentCtx *ctx) {
	if (!ctx) return;
	free(ctx->external_measures);
	ctx->external_measures    = NULL;
	ctx->external_measure_cap = 0u;
}

static XentMeasureMode measure_axis_mode(float size, float available) {
	if (!isnan(size)) return XENT_MEASURE_EXACTLY;
	if (isfinite(available) && available >= 0.0f) return XENT_MEASURE_AT_MOST;
	return XENT_MEASURE_UNDEFINED;
}

static bool measure_size_ok(XentSize size) {
	return isfinite(size.w) && size.w >= 0.0f && isfinite(size.h) && size.h >= 0.0f;
}

static void apply_measured_axes(float *width, float *height, XentSize size) {
	if (isnan(*width)) *width = size.w;
	if (isnan(*height)) *height = size.h;
}

bool xent_resolve_external_measure(
  XentCtx *ctx, XentNodeId node, float available_w, float available_h, float *width, float *height
) {
	if (!width || !height) return false;
	XentExternalMeasureSlot *slot = measure_owned(ctx, node);
	if (!slot || (!isnan(*width) && !isnan(*height))) return false;

	XentMeasureMode   width_mode  = measure_axis_mode(*width, available_w);
	XentMeasureMode   height_mode = measure_axis_mode(*height, available_h);
	XentExtMeasureReq req         = {
	  width_mode == XENT_MEASURE_EXACTLY ? *width : available_w,
	  height_mode == XENT_MEASURE_EXACTLY ? *height : available_h,
	  width_mode,
	  height_mode,
	};

	XentSize size = {NAN, NAN};
	if (!slot->fn(slot->userdata, &req, &size) || !measure_size_ok(size)) return false;
	apply_measured_axes(width, height, size);
	return true;
}
