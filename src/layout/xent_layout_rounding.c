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

void xent_batch_quantize_layout(XentContext *ctx) {
	if (!ctx || !ctx->config.enable_pixel_rounding) return;
	float scale = ctx->config.point_scale_factor;
	if (!(scale > 0.0f) || !isfinite(scale)) return;
	uint32_t count = ctx->work_count;
#if XENT_ISPC_ENABLED
	if (count >= 64u) {
		if (count == ctx->nodes.count) {
			xent_ispc_quantize_f32(ctx->nodes.layout.abs_x + 1, count, scale);
			xent_ispc_quantize_f32(ctx->nodes.layout.abs_y + 1, count, scale);
			xent_ispc_quantize_f32(ctx->nodes.layout.decided_w + 1, count, scale);
			xent_ispc_quantize_f32(ctx->nodes.layout.decided_h + 1, count, scale);
			return;
		}

		size_t buf_bytes = sizeof(float) * ( size_t ) count;
		float *buf_x     = ( float * ) xent_scratch_alloc(ctx, buf_bytes, _Alignof(float));
		float *buf_y     = ( float * ) xent_scratch_alloc(ctx, buf_bytes, _Alignof(float));
		float *buf_w     = ( float * ) xent_scratch_alloc(ctx, buf_bytes, _Alignof(float));
		float *buf_h     = ( float * ) xent_scratch_alloc(ctx, buf_bytes, _Alignof(float));
		if (buf_x && buf_y && buf_w && buf_h) {
			for (uint32_t i = 0; i < count; ++i) {
				XentNodeId n = ctx->work_order [i];
				buf_x [i]   = ctx->nodes.layout.abs_x [n];
				buf_y [i]   = ctx->nodes.layout.abs_y [n];
				buf_w [i]   = ctx->nodes.layout.decided_w [n];
				buf_h [i]   = ctx->nodes.layout.decided_h [n];
			}
			xent_ispc_quantize_f32(buf_x, count, scale);
			xent_ispc_quantize_f32(buf_y, count, scale);
			xent_ispc_quantize_f32(buf_w, count, scale);
			xent_ispc_quantize_f32(buf_h, count, scale);
			for (uint32_t i = 0; i < count; ++i) {
				XentNodeId n                        = ctx->work_order [i];
				ctx->nodes.layout.abs_x [n]         = buf_x [i];
				ctx->nodes.layout.abs_y [n]         = buf_y [i];
				ctx->nodes.layout.decided_w [n]     = buf_w [i];
				ctx->nodes.layout.decided_h [n]     = buf_h [i];
			}
			return;
		}
	}
#endif
	for (uint32_t i = 0; i < count; ++i) {
		XentNodeId n = ctx->work_order [i];
		if (xent_is_valid_node(ctx, n)) xent_quantize_node_layout(ctx, n);
	}
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
