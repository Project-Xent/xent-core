#include "test_common.h"

#include "xent/xent_display.h"

#include <math.h>
#include <string.h>

static int test_display_list_immutable(void) {
	XentDlBuilder *b = xent_dl_begin();
	TEST_ASSERT(b != NULL);

	XentRect  r    = {0.0f, 0.0f, 100.0f, 40.0f};
	XentColor fill = xent_color_rgba(32, 64, 128, 255);
	TEST_ASSERT(xent_dl_fillrrect(b, r, 4.0f, fill));
	TEST_ASSERT(xent_dl_pushclip_rect(b, r));
	TEST_ASSERT(xent_dl_drawtext(b, r, "hello", 14.0f, 400u, xent_color_rgba(0, 0, 0, 255)));
	TEST_ASSERT(xent_dl_drawimg(b, r, ( XentResourceId ) 7u, XENT_IMAGE_FIT_UNIFORM));
	TEST_ASSERT(xent_dl_popclip(b));

	XentDl *list = xent_dl_end(b);
	TEST_ASSERT(list != NULL);
	TEST_ASSERT(xent_dl_ncmd(list) == 5u);

	XentDlCmd const *c0 = xent_dl_cmd(list, 0u);
	TEST_ASSERT(c0 && c0->op == XENT_DL_FILLRRECT);
	TEST_ASSERT(c0->u.geom.radius == 4.0f);

	XentDlCmd const *c2 = xent_dl_cmd(list, 2u);
	TEST_ASSERT(c2 && c2->op == XENT_DL_DRAWTEXT);
	char const *text = xent_dl_text(list, c2->u.text.text_offset, c2->u.text.text_len);
	TEST_ASSERT(text && c2->u.text.text_len == 5u && memcmp(text, "hello", 5) == 0);
	TEST_ASSERT(text [5] == '\0'); /* C-string safe for strlen-based backends */

	XentDlCmd const *c3 = xent_dl_cmd(list, 3u);
	TEST_ASSERT(c3 && c3->op == XENT_DL_DRAWIMG && c3->u.image.image == ( XentResourceId ) 7u);

	TEST_ASSERT(xent_dl_cmd(list, 99u) == NULL);
	xent_dl_free(list);
	return 0;
}

static int test_display_unbalanced_reject(void) {
	XentDlBuilder *b = xent_dl_begin();
	TEST_ASSERT(b != NULL);
	TEST_ASSERT(xent_dl_pushclip_rect(b, (XentRect) {0, 0, 1, 1}));
	TEST_ASSERT(xent_dl_end(b) == NULL);
	return 0;
}

static int test_display_reject_nonfinite(void) {
	XentDlBuilder *b = xent_dl_begin();
	TEST_ASSERT(b != NULL);
	float nan = nanf("");
	TEST_ASSERT(!xent_dl_fillrect(b, (XentRect) {nan, 0, 1, 1}, xent_color_rgba(0, 0, 0, 255)));
	TEST_ASSERT(!xent_dl_pushtransform(b, (XentAffine2) {1, 0, nan, 0, 1, 0}));
	TEST_ASSERT(!xent_dl_strokearc(
	  b, (XentRect) {0, 0, 10, 10}, 0.0f, nan, xent_color_rgba(0, 0, 0, 255), 2.0f, XENT_STROKE_CAP_FLAT
	));
	XentDl *list = xent_dl_end(b);
	TEST_ASSERT(list != NULL);
	TEST_ASSERT(xent_dl_ncmd(list) == 0u);
	xent_dl_free(list);
	return 0;
}

static int test_display_v2_ops_and_affine(void) {
	XentDlBuilder *b = xent_dl_begin();
	TEST_ASSERT(b != NULL);
	XentRect el = {10.0f, 20.0f, 30.0f, 40.0f};
	TEST_ASSERT(xent_dl_fillellipse(b, el, xent_color_rgba(1, 2, 3, 255)));
	TEST_ASSERT(xent_dl_strokeellipse(b, el, xent_color_rgba(4, 5, 6, 255), 2.0f, XENT_STROKE_CAP_ROUND));
	TEST_ASSERT(xent_dl_strokearc(b, el, 90.0f, -45.0f, xent_color_rgba(7, 8, 9, 255), 3.0f, XENT_STROKE_CAP_FLAT));
	TEST_ASSERT(
	  xent_dl_strokeline(b, 0.0f, 0.0f, 10.0f, 0.0f, xent_color_rgba(0, 0, 0, 255), 1.0f, XENT_STROKE_CAP_FLAT)
	);
	TEST_ASSERT(xent_dl_pushclip_ellipse(b, el));
	TEST_ASSERT(xent_dl_popclip(b));
	XentAffine2 m = xent_affine_translate(3.0f, 4.0f);
	TEST_ASSERT(xent_dl_pushtransform(b, m));
	TEST_ASSERT(xent_dl_poptransform(b));
	TEST_ASSERT(
	  xent_dl_fillshape(b, ( XentResourceId ) 42, (XentRect) {0, 0, 80, 32}, xent_color_rgba(255, 255, 255, 255))
	);
	TEST_ASSERT(!xent_dl_fillshape(b, XENT_RESOURCE_INVALID, (XentRect) {0, 0, 80, 32}, xent_color_rgba(0, 0, 0, 255)));

	XentPoint p = xent_affine_map(&m, 1.0f, 2.0f);
	TEST_ASSERT(p.x == 4.0f && p.y == 6.0f);

	XentDl *list = xent_dl_end(b);
	TEST_ASSERT(list != NULL);
	TEST_ASSERT(xent_dl_ncmd(list) == 9u);
	XentDlCmd const *arc = xent_dl_cmd(list, 2u);
	TEST_ASSERT(arc && arc->op == XENT_DL_STROKEARC);
	TEST_ASSERT(arc->u.arc.start_deg == 90.0f && arc->u.arc.sweep_deg == -45.0f);
	XentDlCmd const *xf = xent_dl_cmd(list, 6u);
	TEST_ASSERT(xf && xf->op == XENT_DL_PUSHTRANSFORM);
	TEST_ASSERT(xf->u.transform.m.m02 == 3.0f && xf->u.transform.m.m12 == 4.0f);
	XentDlCmd const *sh = xent_dl_cmd(list, 8u);
	TEST_ASSERT(sh && sh->op == XENT_DL_FILLSHAPE);
	TEST_ASSERT(sh->u.shape.shape == ( XentResourceId ) 42);
	xent_dl_free(list);
	return 0;
}

static int test_shape_resource_preserves_bounds(void) {
	float widths [] = {64.0f, 120.0f, 240.0f};
	for (size_t i = 0; i < sizeof(widths) / sizeof(widths [0]); i++) {
		XentRect       bounds = {16.0f, 8.0f, widths [i], 32.0f};
		XentDlBuilder *db     = xent_dl_begin();
		TEST_ASSERT(db != NULL);
		TEST_ASSERT(xent_dl_fillshape(db, ( XentResourceId ) 42, bounds, xent_color_rgba(1, 1, 1, 255)));
		XentDl *list = xent_dl_end(db);
		TEST_ASSERT(list != NULL);
		XentDlCmd const *cmd = xent_dl_cmd(list, 0u);
		TEST_ASSERT(cmd && cmd->op == XENT_DL_FILLSHAPE);
		TEST_ASSERT(cmd->u.shape.shape == ( XentResourceId ) 42);
		TEST_ASSERT(cmd->u.shape.bounds.w == widths [i]);
		xent_dl_free(list);
	}
	return 0;
}

static int test_display_linear_gradient_ops(void) {
	XentDlBuilder *b = xent_dl_begin();
	TEST_ASSERT(b != NULL);
	XentRect       r        = {0.0f, 0.0f, 40.0f, 32.0f};
	XentColor      c0       = xent_color_rgba(10, 20, 30, 255);
	XentColor      c1       = xent_color_rgba(40, 50, 60, 255);
	XentLinearGrad stroke_g = {0.0f, 32.0f, 0.0f, 29.0f, 0.33f, 1.0f, c0, c1};
	XentLinearGrad fill_g   = {0.0f, 0.0f, 40.0f, 0.0f, 0.0f, 1.0f, c0, c1};
	XentLinearGrad bad_g    = {nanf(""), 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, c0, c1};
	TEST_ASSERT(xent_dl_strokerrect_grad(b, r, 4.0f, 1.0f, &stroke_g));
	TEST_ASSERT(xent_dl_fillrect_grad(b, r, &fill_g));
	TEST_ASSERT(!xent_dl_strokerrect_grad(b, r, 4.0f, 1.0f, &bad_g));
	XentDl *list = xent_dl_end(b);
	TEST_ASSERT(list != NULL);
	TEST_ASSERT(xent_dl_ncmd(list) == 2u);
	XentDlCmd const *s = xent_dl_cmd(list, 0u);
	TEST_ASSERT(s && s->op == XENT_DL_STROKERRECT_GRAD);
	TEST_ASSERT(s->u.linear.radius == 4.0f && s->u.linear.stroke_width == 1.0f);
	TEST_ASSERT(s->u.linear.c0.r == 10 && s->u.linear.c1.b == 60);
	XentDlCmd const *f = xent_dl_cmd(list, 1u);
	TEST_ASSERT(f && f->op == XENT_DL_FILLRECT_GRAD);
	TEST_ASSERT(f->u.linear.x1 == 40.0f);
	xent_dl_free(list);
	return 0;
}

static int test_display_triangle_and_text_style(void) {
	XentDlBuilder *b = xent_dl_begin();
	TEST_ASSERT(b != NULL);
	TEST_ASSERT(xent_dl_filltri(b, 0, 0, 10, 0, 5, 8, xent_color_rgba(1, 2, 3, 255)));
	XentTextStyle style
	  = {.font_size = 12.0f, .font_weight = 600u, .h_align = 1u, .v_align = 1u, .flags = XENT_DL_TEXT_WRAP};
	TEST_ASSERT(xent_dl_drawtext_style(b, (XentRect) {0, 0, 40, 20}, "hi", &style, xent_color_rgba(0, 0, 0, 255)));
	XentDl *list = xent_dl_end(b);
	TEST_ASSERT(list != NULL);
	TEST_ASSERT(xent_dl_ncmd(list) == 2u);
	XentDlCmd const *tri = xent_dl_cmd(list, 0u);
	TEST_ASSERT(tri && tri->op == XENT_DL_FILLTRI);
	TEST_ASSERT(tri->u.tri.x2 == 5.0f && tri->u.tri.y2 == 8.0f);
	XentDlCmd const *tx = xent_dl_cmd(list, 1u);
	TEST_ASSERT(tx && tx->op == XENT_DL_DRAWTEXT);
	TEST_ASSERT(tx->u.text.h_align == 1u && tx->u.text.v_align == 1u);
	TEST_ASSERT((tx->u.text.flags & XENT_DL_TEXT_WRAP) != 0u);
	TEST_ASSERT(tx->u.text.font_len == 0u);
	xent_dl_free(list);

	b = xent_dl_begin();
	TEST_ASSERT(b != NULL);
	XentTextStyle styled = {.font_size = 12.0f, .font_weight = 400u, .font_family = "Consolas"};
	TEST_ASSERT(xent_dl_drawtext_style(b, (XentRect) {0, 0, 40, 20}, "ab", &styled, xent_color_rgba(0, 0, 0, 255)));
	list = xent_dl_end(b);
	TEST_ASSERT(list != NULL);
	tx = xent_dl_cmd(list, 0u);
	TEST_ASSERT(tx && tx->u.text.font_len == 8u);
	TEST_ASSERT(strcmp(xent_dl_text(list, tx->u.text.font_offset, tx->u.text.font_len), "Consolas") == 0);
	xent_dl_free(list);

	b = xent_dl_begin();
	TEST_ASSERT(b != NULL);
	TEST_ASSERT(xent_dl_pushclip_rrect(b, (XentRect) {0, 0, 40, 20}, 4.0f));
	TEST_ASSERT(xent_dl_fillrect(b, (XentRect) {0, 0, 40, 20}, xent_color_rgba(1, 2, 3, 255)));
	TEST_ASSERT(xent_dl_popclip(b));
	/* Zero-area fill is a no-op success (P-44). */
	TEST_ASSERT(xent_dl_fillrect(b, (XentRect) {0, 0, 0, 10}, xent_color_rgba(1, 2, 3, 255)));
	list = xent_dl_end(b);
	TEST_ASSERT(list != NULL);
	TEST_ASSERT(xent_dl_ncmd(list) == 3u);
	TEST_ASSERT(xent_dl_cmd(list, 0u)->op == XENT_DL_PUSHCLIP_RRECT);
	xent_dl_free(list);
	return 0;
}

int main(void) {
	XentTestFn const tests [] = {
	  test_display_list_immutable,
	  test_display_unbalanced_reject,
	  test_display_reject_nonfinite,
	  test_display_v2_ops_and_affine,
	  test_shape_resource_preserves_bounds,
	  test_display_triangle_and_text_style,
	  test_display_linear_gradient_ops,
	};
	return test_run_all(tests, sizeof(tests) / sizeof(tests [0]));
}
