#include "../xent_internal.h"

#if XENT_ISPC_ENABLED
  #include "xent_ispc_kernels_ispc.h"
#endif

void xent_quantize_node_layout(XentCtx *ctx, XentNodeId node) {
	if (!xent_node_valid(ctx, node)) return;
	if (!ctx->config.enable_pixel_rounding) return;
	float scale = ctx->config.point_scale_factor;
	if (!(scale > 0.0f) || !isfinite(scale)) return;

	/* Round to the pixel grid by EDGE, not by size: snap the start edge and the
	 * far edge (position + size) to the grid independently, then derive the size
	 * as their difference. This is what lets adjacent items tile to integer
	 * pixels without cumulative gaps/overlaps (CSS pixel snapping). */
	float x                                         = ctx->nodes.layout.abs_x [xent_node_index(node)];
	float y                                         = ctx->nodes.layout.abs_y [xent_node_index(node)];
	float w                                         = ctx->nodes.layout.decided_w [xent_node_index(node)];
	float h                                         = ctx->nodes.layout.decided_h [xent_node_index(node)];
	float rx                                        = roundf(x * scale) / scale;
	float ry                                        = roundf(y * scale) / scale;
	ctx->nodes.layout.abs_x [xent_node_index(node)] = rx;
	ctx->nodes.layout.abs_y [xent_node_index(node)] = ry;
	if (isfinite(w)) ctx->nodes.layout.decided_w [xent_node_index(node)] = roundf((x + w) * scale) / scale - rx;
	if (isfinite(h)) ctx->nodes.layout.decided_h [xent_node_index(node)] = roundf((y + h) * scale) / scale - ry;
}

static void batch_quantize_scalar(XentCtx *ctx, uint32_t count) {
	for (uint32_t i = 0u; i < count; ++i) {
		XentNodeId n = ctx->work_order [i];
		if (xent_node_valid(ctx, n)) xent_quantize_node_layout(ctx, n);
	}
}

#if XENT_ISPC_ENABLED
typedef struct QuantizeBuffers {
	float *x;
	float *y;
	float *w;
	float *h;
} QuantizeBuffers;

static QuantizeBuffers alloc_quantize_buffers(XentCtx *ctx, uint32_t count) {
	size_t bytes = sizeof(float) * ( size_t ) count;
	float *block = xent_scratch_alloc(ctx, bytes * 4u, _Alignof(float));
	if (!block) return (QuantizeBuffers) {0};
	return (QuantizeBuffers) {block, block + count, block + count * 2u, block + count * 3u};
}

static void gather_quantize_buffers(XentCtx const *ctx, QuantizeBuffers buffers, uint32_t count) {
	for (uint32_t i = 0u; i < count; ++i) {
		uint32_t index = xent_node_index(ctx->work_order [i]);
		buffers.x [i]  = ctx->nodes.layout.abs_x [index];
		buffers.y [i]  = ctx->nodes.layout.abs_y [index];
		buffers.w [i]  = ctx->nodes.layout.decided_w [index];
		buffers.h [i]  = ctx->nodes.layout.decided_h [index];
	}
}

static void scatter_quantize_buffers(XentCtx *ctx, QuantizeBuffers buffers, uint32_t count) {
	for (uint32_t i = 0u; i < count; ++i) {
		uint32_t index                      = xent_node_index(ctx->work_order [i]);
		ctx->nodes.layout.abs_x [index]     = buffers.x [i];
		ctx->nodes.layout.abs_y [index]     = buffers.y [i];
		ctx->nodes.layout.decided_w [index] = buffers.w [i];
		ctx->nodes.layout.decided_h [index] = buffers.h [i];
	}
}

static bool batch_quantize_ispc(XentCtx *ctx, uint32_t count, float scale) {
	if (!ctx->config.enable_simd || count < 64u) return false;
	QuantizeBuffers buffers = alloc_quantize_buffers(ctx, count);
	if (!buffers.x) return false;
	gather_quantize_buffers(ctx, buffers, count);
	xent_ispc_quantize_rects(buffers.x, buffers.y, buffers.w, buffers.h, count, scale);
	scatter_quantize_buffers(ctx, buffers, count);
	return true;
}
#endif

void xent_batch_quantize_layout(XentCtx *ctx) {
	if (!ctx || !ctx->config.enable_pixel_rounding) return;
	float scale = ctx->config.point_scale_factor;
	if (!(scale > 0.0f) || !isfinite(scale)) return;
	uint32_t count = ctx->work_count;
#if XENT_ISPC_ENABLED
	if (batch_quantize_ispc(ctx, count, scale)) return;
#endif
	batch_quantize_scalar(ctx, count);
}

static void invalidate_all_layout(XentCtx *ctx) {
	if (!ctx) return;
	uint32_t n = ctx->nodes.count;
#if XENT_ISPC_ENABLED
	if (ctx->config.enable_simd && n >= 64u) {
		uint32_t flags = XENT_DIRTY_LAYOUT | XENT_DIRTY_SUBTREE | XENT_DIRTY_SELF;
		xent_ispc_or_u32_masked(ctx->nodes.layout.dirty_flags + 1, ctx->nodes.lifetime.alive + 1, n, flags);
		xent_ispc_zero_u8_masked(ctx->nodes.text.intrinsic_valid + 1, ctx->nodes.lifetime.alive + 1, n);
	}
	else
#endif
		for (uint32_t i = 1u; i <= n; ++i) {
			if (ctx->nodes.lifetime.alive [i]) {
				ctx->nodes.layout.dirty_flags [i]   |= XENT_DIRTY_LAYOUT | XENT_DIRTY_SUBTREE | XENT_DIRTY_SELF;
				ctx->nodes.text.intrinsic_valid [i]  = 0u;
			}
		}
	ctx->last_layout_root        = XENT_NODE_INVALID;
	ctx->last_layout_available_w = NAN;
	ctx->last_layout_available_h = NAN;
}

bool xent_setscale(XentCtx *ctx, float point_scale_factor) {
	if (!ctx || !(point_scale_factor > 0.0f) || !isfinite(point_scale_factor)) return false;
	if (ctx->config.point_scale_factor != point_scale_factor) {
		ctx->config.point_scale_factor = point_scale_factor;
		invalidate_all_layout(ctx);
	}
	return true;
}

float xent_scale(XentCtx const *ctx) {
	if (!ctx) return 1.0f;
	return ctx->config.point_scale_factor;
}

bool xent_setrounding(XentCtx *ctx, bool enabled) {
	if (!ctx) return false;
	if (ctx->config.enable_pixel_rounding != enabled) {
		ctx->config.enable_pixel_rounding = enabled;
		invalidate_all_layout(ctx);
	}
	return true;
}

bool xent_rounding(XentCtx const *ctx) {
	if (!ctx) return false;
	return ctx->config.enable_pixel_rounding;
}
