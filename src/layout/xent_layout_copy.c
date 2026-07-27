#include "../xent_internal.h"

static void copy_u8(uint8_t *d, uint8_t const *s, uint32_t di, uint32_t si) { d [di] = s [si]; }

static void copy_f(float *d, float const *s, uint32_t di, uint32_t si) { d [di] = s [si]; }

static void copy_i32(int32_t *d, int32_t const *s, uint32_t di, uint32_t si) { d [di] = s [si]; }

static void copy_u16(uint16_t *d, uint16_t const *s, uint32_t di, uint32_t si) { d [di] = s [si]; }

static void copy_layout_style(XentCtx *ctx, uint32_t di, uint32_t si) {
	XentNodeLayoutStore *L = &ctx->nodes.layout;
	copy_u8(L->protocol, L->protocol, di, si);
	copy_u8(L->direction, L->direction, di, si);
	copy_u8(L->wrap_content_w, L->wrap_content_w, di, si);
	copy_u8(L->wrap_content_h, L->wrap_content_h, di, si);
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

static void copy_stack_style(XentCtx *ctx, uint32_t di, uint32_t si) {
	XentNodeStackStore *S = &ctx->nodes.stack;
	copy_u8(S->axis, S->axis, di, si);
	copy_u8(S->align, S->align, di, si);
	copy_f(S->priority, S->priority, di, si);
	copy_u8(S->spacer, S->spacer, di, si);
}

static void copy_grid_placement(XentCtx *ctx, uint32_t di, uint32_t si) {
	XentNodeGridStore *G = &ctx->nodes.grid;
	copy_u16(G->row, G->row, di, si);
	copy_u16(G->column, G->column, di, si);
	copy_u16(G->row_span, G->row_span, di, si);
	copy_u16(G->column_span, G->column_span, di, si);
}

static bool copy_text_measure(XentCtx *ctx, XentNodeId dst, uint32_t di, uint32_t si) {
	XentNodeTextStore *T = &ctx->nodes.text;
	if (!xent_settext(ctx, dst, T->content [si])) return false;
	copy_f(T->font_size, T->font_size, di, si);
	copy_u16(T->font_weight, T->font_weight, di, si);
	copy_u8(T->line_break_policy, T->line_break_policy, di, si);
	T->intrinsic_valid [di] = 0u;
	return true;
}

bool xent_copystyle(XentCtx *ctx, XentNodeId dst, XentNodeId src) {
	if (!xent_node_valid(ctx, dst) || !xent_node_valid(ctx, src) || dst == src) return false;
	uint32_t di = xent_node_index(dst);
	uint32_t si = xent_node_index(src);

	copy_layout_style(ctx, di, si);
	copy_flex_style(ctx, di, si);
	copy_stack_style(ctx, di, si);
	copy_grid_placement(ctx, di, si);
	xent_grid_def_free(ctx->nodes.grid.def [di]);
	ctx->nodes.grid.def [di] = xent_grid_def_copy(ctx->nodes.grid.def [si]);
	if (ctx->nodes.grid.def [si] && !ctx->nodes.grid.def [di]) return false;
	if (!copy_text_measure(ctx, dst, di, si)) return false;

	xent_mark_dirty(ctx, dst, XENT_DIRTY_LAYOUT);
	return true;
}
