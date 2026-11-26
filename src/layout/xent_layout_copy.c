#include "../xent_internal.h"

static void copy_u8(uint8_t *d, uint8_t const *s, uint32_t di, uint32_t si) { d [di] = s [si]; }

static void copy_f(float *d, float const *s, uint32_t di, uint32_t si) { d [di] = s [si]; }

static void copy_i32(int32_t *d, int32_t const *s, uint32_t di, uint32_t si) { d [di] = s [si]; }

static void copy_layout_style(XentCtx *ctx, uint32_t di, uint32_t si) {
	XentNodeLayoutStore *L = &ctx->nodes.layout;
	copy_u8(L->protocol, L->protocol, di, si);
	copy_u8(L->direction, L->direction, di, si);
	copy_u8(L->collapsed, L->collapsed, di, si);
	copy_u8(L->wrap_content_w, L->wrap_content_w, di, si);
	copy_u8(L->wrap_content_h, L->wrap_content_h, di, si);
	copy_u8(L->scroller, L->scroller, di, si);
	copy_f(L->style_w, L->style_w, di, si);
	copy_f(L->style_h, L->style_h, di, si);
	copy_f(L->style_w_percent, L->style_w_percent, di, si);
	copy_f(L->style_h_percent, L->style_h_percent, di, si);
	copy_f(L->aspect_ratio, L->aspect_ratio, di, si);
	copy_f(L->min_w, L->min_w, di, si);
	copy_f(L->min_h, L->min_h, di, si);
	copy_f(L->max_w, L->max_w, di, si);
	copy_f(L->max_h, L->max_h, di, si);
	copy_f(L->margin_l, L->margin_l, di, si);
	copy_f(L->margin_t, L->margin_t, di, si);
	copy_f(L->margin_r, L->margin_r, di, si);
	copy_f(L->margin_b, L->margin_b, di, si);
	copy_f(L->padding_l, L->padding_l, di, si);
	copy_f(L->padding_t, L->padding_t, di, si);
	copy_f(L->padding_r, L->padding_r, di, si);
	copy_f(L->padding_b, L->padding_b, di, si);
	copy_f(L->gap, L->gap, di, si);
	copy_f(L->abs_pos_x, L->abs_pos_x, di, si);
	copy_f(L->abs_pos_y, L->abs_pos_y, di, si);
	copy_i32(L->z_index, L->z_index, di, si);
}

static void copy_flex_style(XentCtx *ctx, uint32_t di, uint32_t si) {
	XentNodeFlexStore *F = &ctx->nodes.flex;
	copy_f(F->grow, F->grow, di, si);
	copy_f(F->shrink, F->shrink, di, si);
	copy_f(F->basis, F->basis, di, si);
	copy_u8(F->direction, F->direction, di, si);
	copy_u8(F->wrap, F->wrap, di, si);
	copy_u8(F->justify_content, F->justify_content, di, si);
	copy_u8(F->align_items, F->align_items, di, si);
	copy_u8(F->align_self, F->align_self, di, si);
	copy_u8(F->align_content, F->align_content, di, si);
}

static bool copy_stack_style(XentCtx *ctx, uint32_t di, uint32_t si) {
	XentStackComp const *src = xent_stack_comp_c(&ctx->nodes, si);
	if (!src) {
		xent_pool_release(&ctx->nodes.stack_pool, sizeof(XentStackComp), &ctx->nodes.cold.stack [di]);
		return true;
	}
	XentStackComp *dst = xent_stack_comp_ensure(&ctx->nodes, di);
	if (!dst) return false;
	dst->axis     = src->axis;
	dst->align    = src->align;
	dst->priority = src->priority;
	dst->spacer   = src->spacer;
	return true;
}

static bool copy_grid_style(XentCtx *ctx, uint32_t di, uint32_t si) {
	XentGridComp const *src = xent_grid_comp_c(&ctx->nodes, si);
	if (!src) {
		if (ctx->nodes.cold.grid [di]) {
			XentGridComp *old
			  = ( XentGridComp * ) xent_pool_get(&ctx->nodes.grid_pool, sizeof(*old), ctx->nodes.cold.grid [di]);
			if (old) {
				xent_grid_def_free(old->def);
				old->def = NULL;
			}
			xent_pool_release(&ctx->nodes.grid_pool, sizeof(XentGridComp), &ctx->nodes.cold.grid [di]);
		}
		return true;
	}
	XentGridDef *def = xent_grid_def_copy(src->def);
	if (src->def && !def) return false;
	XentGridComp *dst = xent_grid_comp_ensure(&ctx->nodes, di);
	if (!dst) {
		xent_grid_def_free(def);
		return false;
	}
	xent_grid_def_free(dst->def);
	dst->def         = def;
	dst->row         = src->row;
	dst->column      = src->column;
	dst->row_span    = src->row_span;
	dst->column_span = src->column_span;
	return true;
}

static bool copy_text_measure(XentCtx *ctx, XentNodeId dst, uint32_t di, uint32_t si) {
	XentTextComp const *src = xent_text_comp_c(&ctx->nodes, si);
	if (!src) return xent_settext(ctx, dst, NULL);
	if (!xent_settext(ctx, dst, src->content)) return false;
	XentTextComp *t = xent_text_comp_ensure(&ctx->nodes, di);
	if (!t) return false;
	t->font_size         = src->font_size;
	t->font_weight       = src->font_weight;
	t->line_break_policy = src->line_break_policy;
	t->intrinsic_valid   = 0u;
	return true;
}

bool xent_copystyle(XentCtx *ctx, XentNodeId dst, XentNodeId src) {
	if (!xent_node_valid(ctx, dst) || !xent_node_valid(ctx, src) || dst == src) return false;
	uint32_t di = xent_node_index(dst);
	uint32_t si = xent_node_index(src);

	copy_layout_style(ctx, di, si);
	copy_flex_style(ctx, di, si);
	if (!copy_stack_style(ctx, di, si)) return false;
	if (!copy_grid_style(ctx, di, si)) return false;
	if (!copy_text_measure(ctx, dst, di, si)) return false;

	xent_mark_dirty(ctx, dst, XENT_DIRTY_LAYOUT);
	return true;
}
