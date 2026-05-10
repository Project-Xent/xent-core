#include "../xent_internal.h"

#if XENT_ISPC_ENABLED
  #include "xent_ispc_kernels_ispc.h"
#endif

static float xent_round_to_pixel_grid(XentContext const *ctx, float value) {
	if (!ctx->config.enable_pixel_rounding) return value;
	float scale = ctx->config.point_scale_factor;
	if (!(scale > 0.0f) || !isfinite(scale) || !isfinite(value)) return value;
	return roundf(value * scale) / scale;
}

void xent_quantize_node_layout(XentContext *ctx, XentNodeId node) {
	if (!xent_is_valid_node(ctx, node)) return;
	ctx->nodes.layout.abs_x [node]     = xent_round_to_pixel_grid(ctx, ctx->nodes.layout.abs_x [node]);
	ctx->nodes.layout.abs_y [node]     = xent_round_to_pixel_grid(ctx, ctx->nodes.layout.abs_y [node]);
	ctx->nodes.layout.decided_w [node] = xent_round_to_pixel_grid(ctx, ctx->nodes.layout.decided_w [node]);
	ctx->nodes.layout.decided_h [node] = xent_round_to_pixel_grid(ctx, ctx->nodes.layout.decided_h [node]);
}

#if XENT_ISPC_ENABLED
static void xent_batch_quantize_contiguous(XentContext *ctx, uint32_t count, float scale) {
	xent_ispc_quantize_f32(ctx->nodes.layout.abs_x + 1, count, scale);
	xent_ispc_quantize_f32(ctx->nodes.layout.abs_y + 1, count, scale);
	xent_ispc_quantize_f32(ctx->nodes.layout.decided_w + 1, count, scale);
	xent_ispc_quantize_f32(ctx->nodes.layout.decided_h + 1, count, scale);
}

typedef struct XentQuantizeBuffers {
	float *x;
	float *y;
	float *w;
	float *h;
} XentQuantizeBuffers;

static XentQuantizeBuffers xent_alloc_quantize_buffers(XentContext *ctx, uint32_t count) {
	size_t bytes = sizeof(float) * ( size_t ) count;
	return (XentQuantizeBuffers) {
	  ( float * ) xent_scratch_alloc(ctx, bytes, _Alignof(float)),
	  ( float * ) xent_scratch_alloc(ctx, bytes, _Alignof(float)),
	  ( float * ) xent_scratch_alloc(ctx, bytes, _Alignof(float)),
	  ( float * ) xent_scratch_alloc(ctx, bytes, _Alignof(float)),
	};
}

static bool xent_quantize_buffers_valid(XentQuantizeBuffers buffers) {
	return buffers.x && buffers.y && buffers.w && buffers.h;
}

static void xent_gather_quantize_buffers(XentContext const *ctx, XentQuantizeBuffers buffers, uint32_t count) {
	for (uint32_t i = 0u; i < count; ++i) {
		XentNodeId n  = ctx->work_order [i];
		buffers.x [i] = ctx->nodes.layout.abs_x [n];
		buffers.y [i] = ctx->nodes.layout.abs_y [n];
		buffers.w [i] = ctx->nodes.layout.decided_w [n];
		buffers.h [i] = ctx->nodes.layout.decided_h [n];
	}
}

static void xent_scatter_quantize_buffers(XentContext *ctx, XentQuantizeBuffers buffers, uint32_t count) {
	for (uint32_t i = 0u; i < count; ++i) {
		XentNodeId n                    = ctx->work_order [i];
		ctx->nodes.layout.abs_x [n]     = buffers.x [i];
		ctx->nodes.layout.abs_y [n]     = buffers.y [i];
		ctx->nodes.layout.decided_w [n] = buffers.w [i];
		ctx->nodes.layout.decided_h [n] = buffers.h [i];
	}
}

static bool xent_batch_quantize_sparse_ispc(XentContext *ctx, uint32_t count, float scale) {
	XentQuantizeBuffers buffers = xent_alloc_quantize_buffers(ctx, count);
	if (!xent_quantize_buffers_valid(buffers)) return false;
	xent_gather_quantize_buffers(ctx, buffers, count);
	xent_ispc_quantize_f32(buffers.x, count, scale);
	xent_ispc_quantize_f32(buffers.y, count, scale);
	xent_ispc_quantize_f32(buffers.w, count, scale);
	xent_ispc_quantize_f32(buffers.h, count, scale);
	xent_scatter_quantize_buffers(ctx, buffers, count);
	return true;
}
#endif

static void xent_batch_quantize_scalar(XentContext *ctx, uint32_t count) {
	for (uint32_t i = 0u; i < count; ++i) {
		XentNodeId n = ctx->work_order [i];
		if (xent_is_valid_node(ctx, n)) xent_quantize_node_layout(ctx, n);
	}
}

void xent_batch_quantize_layout(XentContext *ctx) {
	if (!ctx || !ctx->config.enable_pixel_rounding) return;
	float scale = ctx->config.point_scale_factor;
	if (!(scale > 0.0f) || !isfinite(scale)) return;
	uint32_t count = ctx->work_count;
#if XENT_ISPC_ENABLED
	if (count >= 64u && count == ctx->nodes.count) {
		xent_batch_quantize_contiguous(ctx, count, scale);
		return;
	}
	if (count >= 64u && xent_batch_quantize_sparse_ispc(ctx, count, scale)) return;
#endif
	xent_batch_quantize_scalar(ctx, count);
}

static void xent_invalidate_all_layout(XentContext *ctx) {
	if (!ctx) return;
	uint32_t n = ctx->nodes.count;
#if XENT_ISPC_ENABLED
	if (n >= 64u) {
		uint32_t or_value = XENT_DIRTY_LAYOUT | XENT_DIRTY_SUBTREE | XENT_DIRTY_SELF;
		xent_ispc_or_u32_masked(ctx->nodes.layout.dirty_flags + 1, ctx->nodes.lifetime.alive + 1, n, or_value);
		xent_ispc_zero_u8_masked(ctx->nodes.text.intrinsic_valid + 1, ctx->nodes.lifetime.alive + 1, n);
	}
	else {
#endif
		for (uint32_t i = 1u; i <= n; ++i) {
			if (ctx->nodes.lifetime.alive [i]) {
				ctx->nodes.layout.dirty_flags [i]   |= XENT_DIRTY_LAYOUT | XENT_DIRTY_SUBTREE | XENT_DIRTY_SELF;
				ctx->nodes.text.intrinsic_valid [i]  = 0u;
			}
		}
#if XENT_ISPC_ENABLED
	}
#endif
	ctx->last_layout_root        = XENT_NODE_INVALID;
	ctx->last_layout_available_w = NAN;
	ctx->last_layout_available_h = NAN;
}

bool xent_set_point_scale_factor(XentContext *ctx, float point_scale_factor) {
	if (!ctx || !(point_scale_factor > 0.0f) || !isfinite(point_scale_factor)) return false;
	if (ctx->config.point_scale_factor != point_scale_factor) {
		ctx->config.point_scale_factor = point_scale_factor;
		xent_invalidate_all_layout(ctx);
	}
	return true;
}

float xent_get_point_scale_factor(XentContext const *ctx) {
	if (!ctx) return 1.0f;
	return ctx->config.point_scale_factor;
}

bool xent_set_pixel_rounding_enabled(XentContext *ctx, bool enabled) {
	if (!ctx) return false;
	if (ctx->config.enable_pixel_rounding != enabled) {
		ctx->config.enable_pixel_rounding = enabled;
		xent_invalidate_all_layout(ctx);
	}
	return true;
}

bool xent_is_pixel_rounding_enabled(XentContext const *ctx) {
	if (!ctx) return false;
	return ctx->config.enable_pixel_rounding;
}
