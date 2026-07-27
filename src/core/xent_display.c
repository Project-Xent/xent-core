#include "xent/xent_display.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct XentDlBuilder {
	XentDlCmd *cmds;
	uint32_t   count;
	uint32_t   capacity;
	char      *strings;
	uint32_t   strings_len;
	uint32_t   strings_cap;
	int32_t    clip_depth;
	int32_t    transform_depth;
	int32_t    opacity_depth;
};

struct XentDl {
	XentDlCmd *cmds;
	uint32_t   count;
	char      *strings;
	uint32_t   strings_len;
};

XentDlBuilder *xent_dl_begin(void) { return ( XentDlBuilder * ) calloc(1, sizeof(XentDlBuilder)); }

void           xent_dl_abort(XentDlBuilder *b) {
	if (!b) return;
	free(b->cmds);
	free(b->strings);
	free(b);
}

static bool display_finite(float v) { return isfinite(( double ) v) != 0; }

static bool display_finite_rect(XentRect r) {
	return display_finite(r.x)
	    && display_finite(r.y)
	    && display_finite(r.w)
	    && display_finite(r.h)
	    && r.w >= 0.0f
	    && r.h >= 0.0f;
}

static bool display_has_area(XentRect r) { return r.w > 0.0f && r.h > 0.0f; }

static bool display_finite_affine(XentAffine2 const *m) {
	return display_finite(m->m00)
	    && display_finite(m->m01)
	    && display_finite(m->m02)
	    && display_finite(m->m10)
	    && display_finite(m->m11)
	    && display_finite(m->m12);
}

static bool display_cap_ok(XentStrokeCap cap) {
	return cap == XENT_STROKE_CAP_FLAT || cap == XENT_STROKE_CAP_ROUND || cap == XENT_STROKE_CAP_SQUARE;
}

static bool display_reserve_cmds(XentDlBuilder *b) {
	if (b->count < b->capacity) return true;
	uint32_t new_cap = b->capacity ? b->capacity * 2u : 32u;
	if (new_cap <= b->capacity) return false;
	XentDlCmd *n = ( XentDlCmd * ) realloc(b->cmds, sizeof(*n) * ( size_t ) new_cap);
	if (!n) return false;
	b->cmds     = n;
	b->capacity = new_cap;
	return true;
}

static bool display_push(XentDlBuilder *b, XentDlCmd cmd) {
	if (!b || !display_reserve_cmds(b)) return false;
	b->cmds [b->count++] = cmd;
	return true;
}

static uint32_t display_next_string_cap(uint32_t cap, uint32_t needed) {
	uint32_t new_cap = cap ? cap * 2u : 64u;
	while (new_cap < needed) {
		if (new_cap > UINT32_MAX / 2u) return 0u;
		new_cap *= 2u;
	}
	return new_cap;
}

static bool display_grow_strings(XentDlBuilder *b, uint32_t end) {
	if (end <= b->strings_cap) return true;
	uint32_t new_cap = display_next_string_cap(b->strings_cap, end);
	if (new_cap == 0u) return false;
	char *n = ( char * ) realloc(b->strings, ( size_t ) new_cap);
	if (!n) return false;
	b->strings     = n;
	b->strings_cap = new_cap;
	return true;
}

static bool display_append_string(XentDlBuilder *b, char const *utf8, uint32_t *out_off, uint32_t *out_len) {
	size_t len = utf8 ? strlen(utf8) : 0u;
	if (len > UINT32_MAX) return false;
	uint32_t need = ( uint32_t ) len;
	/* Keep a trailing NUL so backends may treat the offset as a C string.
	 * text_len excludes the NUL; the byte at offset+len is always '\0'. */
	if (b->strings_len > UINT32_MAX - need - 1u) return false;
	uint32_t end = b->strings_len + need + 1u;
	if (!display_grow_strings(b, end)) return false;
	if (need) memcpy(b->strings + b->strings_len, utf8, ( size_t ) need);
	b->strings [b->strings_len + need] = '\0';
	*out_off                           = b->strings_len;
	*out_len                           = need;
	b->strings_len                     = end;
	return true;
}

bool xent_dl_fillrect(XentDlBuilder *b, XentRect rect, XentColor color) {
	if (!display_finite_rect(rect)) return false;
	if (!display_has_area(rect)) return true;
	XentDlCmd cmd    = {.op = XENT_DL_FILLRECT};
	cmd.u.geom.rect  = rect;
	cmd.u.geom.color = color;
	return display_push(b, cmd);
}

bool xent_dl_strokerect(XentDlBuilder *b, XentRect rect, XentColor color, float width) {
	if (!display_finite_rect(rect) || !display_finite(width) || width < 0.0f) return false;
	if (!display_has_area(rect) || width == 0.0f) return true;
	XentDlCmd cmd           = {.op = XENT_DL_STROKERECT};
	cmd.u.geom.rect         = rect;
	cmd.u.geom.color        = color;
	cmd.u.geom.stroke_width = width;
	return display_push(b, cmd);
}

bool xent_dl_fillrrect(XentDlBuilder *b, XentRect rect, float radius, XentColor color) {
	if (!display_finite_rect(rect) || !display_finite(radius) || radius < 0.0f) return false;
	if (!display_has_area(rect)) return true;
	XentDlCmd cmd     = {.op = XENT_DL_FILLRRECT};
	cmd.u.geom.rect   = rect;
	cmd.u.geom.color  = color;
	cmd.u.geom.radius = radius;
	return display_push(b, cmd);
}

bool xent_dl_strokerrect(XentDlBuilder *b, XentRect rect, float radius, XentColor color, float width) {
	if (!display_finite_rect(rect)
		|| !display_finite(radius)
		|| radius < 0.0f
		|| !display_finite(width)
		|| width < 0.0f)
		return false;
	if (!display_has_area(rect) || width == 0.0f) return true;
	XentDlCmd cmd           = {.op = XENT_DL_STROKERRECT};
	cmd.u.geom.rect         = rect;
	cmd.u.geom.color        = color;
	cmd.u.geom.radius       = radius;
	cmd.u.geom.stroke_width = width;
	return display_push(b, cmd);
}

bool xent_dl_drawtext_style(
  XentDlBuilder *b, XentRect bounds, char const *utf8, XentTextStyle const *style, XentColor color
) {
	if (!b || !style || !display_finite_rect(bounds) || !display_finite(style->font_size) || style->font_size <= 0.0f)
		return false;
	if (style->h_align > 2u || style->v_align > 2u) return false;
	uint32_t off = 0u, len = 0u;
	if (!display_append_string(b, utf8, &off, &len)) return false;
	uint32_t font_off = 0u, font_len = 0u;
	if (style->font_family
		&& style->font_family [0]
		&& !display_append_string(b, style->font_family, &font_off, &font_len))
		return false;
	XentDlCmd cmd          = {.op = XENT_DL_DRAWTEXT};
	cmd.u.text.bounds      = bounds;
	cmd.u.text.text_offset = off;
	cmd.u.text.text_len    = len;
	cmd.u.text.font_offset = font_off;
	cmd.u.text.font_len    = font_len;
	cmd.u.text.font_size   = style->font_size;
	cmd.u.text.font_weight = style->font_weight;
	cmd.u.text.h_align     = style->h_align;
	cmd.u.text.v_align     = style->v_align;
	cmd.u.text.flags       = style->flags;
	cmd.u.text.color       = color;
	return display_push(b, cmd);
}

bool xent_dl_drawtext(
  XentDlBuilder *b, XentRect bounds, char const *utf8, float font_size, uint16_t font_weight, XentColor color
) {
	XentTextStyle style = {.font_size = font_size, .font_weight = font_weight};
	return xent_dl_drawtext_style(b, bounds, utf8, &style, color);
}

bool xent_dl_drawimg(XentDlBuilder *b, XentRect bounds, XentResourceId image, XentImageFit stretch) {
	if (image == XENT_RESOURCE_INVALID || !display_finite_rect(bounds)) return false;
	XentDlCmd cmd       = {.op = XENT_DL_DRAWIMG};
	cmd.u.image.bounds  = bounds;
	cmd.u.image.image   = image;
	cmd.u.image.stretch = stretch;
	return display_push(b, cmd);
}

bool xent_dl_pushclip_rect(XentDlBuilder *b, XentRect rect) {
	if (!b || !display_finite_rect(rect)) return false;
	XentDlCmd cmd   = {.op = XENT_DL_PUSHCLIP_RECT};
	cmd.u.geom.rect = rect;
	if (!display_push(b, cmd)) return false;
	b->clip_depth++;
	return true;
}

bool xent_dl_pushclip_rrect(XentDlBuilder *b, XentRect rect, float radius) {
	if (!b || !display_finite_rect(rect) || !display_finite(radius) || radius < 0.0f) return false;
	XentDlCmd cmd     = {.op = XENT_DL_PUSHCLIP_RRECT};
	cmd.u.geom.rect   = rect;
	cmd.u.geom.radius = radius;
	if (!display_push(b, cmd)) return false;
	b->clip_depth++;
	return true;
}

bool xent_dl_popclip(XentDlBuilder *b) {
	if (!b || b->clip_depth <= 0) return false;
	XentDlCmd cmd = {.op = XENT_DL_POPCLIP};
	if (!display_push(b, cmd)) return false;
	b->clip_depth--;
	return true;
}

bool xent_dl_pushtransform(XentDlBuilder *b, XentAffine2 m) {
	if (!b || !display_finite_affine(&m)) return false;
	XentDlCmd cmd     = {.op = XENT_DL_PUSHTRANSFORM};
	cmd.u.transform.m = m;
	if (!display_push(b, cmd)) return false;
	b->transform_depth++;
	return true;
}

bool xent_dl_poptransform(XentDlBuilder *b) {
	if (!b || b->transform_depth <= 0) return false;
	XentDlCmd cmd = {.op = XENT_DL_POPTRANSFORM};
	if (!display_push(b, cmd)) return false;
	b->transform_depth--;
	return true;
}

bool xent_dl_pushopacity(XentDlBuilder *b, float opacity) {
	if (!b || !display_finite(opacity) || opacity < 0.0f || opacity > 1.0f) return false;
	XentDlCmd cmd         = {.op = XENT_DL_PUSHOPACITY};
	cmd.u.opacity.opacity = opacity;
	if (!display_push(b, cmd)) return false;
	b->opacity_depth++;
	return true;
}

bool xent_dl_popopacity(XentDlBuilder *b) {
	if (!b || b->opacity_depth <= 0) return false;
	XentDlCmd cmd = {.op = XENT_DL_POPOPACITY};
	if (!display_push(b, cmd)) return false;
	b->opacity_depth--;
	return true;
}

bool xent_dl_fillellipse(XentDlBuilder *b, XentRect bounds, XentColor color) {
	if (!display_finite_rect(bounds)) return false;
	XentDlCmd cmd        = {.op = XENT_DL_FILLELLIPSE};
	cmd.u.ellipse.bounds = bounds;
	cmd.u.ellipse.color  = color;
	return display_push(b, cmd);
}

bool xent_dl_strokeellipse(XentDlBuilder *b, XentRect bounds, XentColor color, float width, XentStrokeCap cap) {
	if (!display_finite_rect(bounds) || !display_finite(width) || width < 0.0f || !display_cap_ok(cap)) return false;
	XentDlCmd cmd              = {.op = XENT_DL_STROKEELLIPSE};
	cmd.u.ellipse.bounds       = bounds;
	cmd.u.ellipse.color        = color;
	cmd.u.ellipse.stroke_width = width;
	cmd.u.ellipse.cap          = cap;
	return display_push(b, cmd);
}

bool xent_dl_strokearc(
  XentDlBuilder *b, XentRect bounds, float start_deg, float sweep_deg, XentColor color, float width, XentStrokeCap cap
) {
	if (!display_finite_rect(bounds)
		|| !display_finite(start_deg)
		|| !display_finite(sweep_deg)
		|| !display_finite(width)
		|| width < 0.0f
		|| !display_cap_ok(cap))
		return false;
	XentDlCmd cmd          = {.op = XENT_DL_STROKEARC};
	cmd.u.arc.bounds       = bounds;
	cmd.u.arc.start_deg    = start_deg;
	cmd.u.arc.sweep_deg    = sweep_deg;
	cmd.u.arc.color        = color;
	cmd.u.arc.stroke_width = width;
	cmd.u.arc.cap          = cap;
	return display_push(b, cmd);
}

bool xent_dl_strokeline(
  XentDlBuilder *b, float x0, float y0, float x1, float y1, XentColor color, float width, XentStrokeCap cap
) {
	if (!display_finite(x0)
		|| !display_finite(y0)
		|| !display_finite(x1)
		|| !display_finite(y1)
		|| !display_finite(width)
		|| width < 0.0f
		|| !display_cap_ok(cap))
		return false;
	XentDlCmd cmd           = {.op = XENT_DL_STROKELINE};
	cmd.u.line.x0           = x0;
	cmd.u.line.y0           = y0;
	cmd.u.line.x1           = x1;
	cmd.u.line.y1           = y1;
	cmd.u.line.color        = color;
	cmd.u.line.stroke_width = width;
	cmd.u.line.cap          = cap;
	return display_push(b, cmd);
}

bool xent_dl_pushclip_ellipse(XentDlBuilder *b, XentRect bounds) {
	if (!b || !display_finite_rect(bounds)) return false;
	XentDlCmd cmd        = {.op = XENT_DL_PUSHCLIP_ELLIPSE};
	cmd.u.ellipse.bounds = bounds;
	if (!display_push(b, cmd)) return false;
	b->clip_depth++;
	return true;
}

bool xent_dl_fillshape(XentDlBuilder *b, XentResourceId shape, XentRect bounds, XentColor color) {
	if (shape == XENT_RESOURCE_INVALID || !display_finite_rect(bounds)) return false;
	XentDlCmd cmd      = {.op = XENT_DL_FILLSHAPE};
	cmd.u.shape.bounds = bounds;
	cmd.u.shape.shape  = shape;
	cmd.u.shape.color  = color;
	return display_push(b, cmd);
}

bool xent_dl_filltri(XentDlBuilder *b, float x0, float y0, float x1, float y1, float x2, float y2, XentColor color) {
	if (!display_finite(x0)
		|| !display_finite(y0)
		|| !display_finite(x1)
		|| !display_finite(y1)
		|| !display_finite(x2)
		|| !display_finite(y2))
		return false;
	XentDlCmd cmd   = {.op = XENT_DL_FILLTRI};
	cmd.u.tri.x0    = x0;
	cmd.u.tri.y0    = y0;
	cmd.u.tri.x1    = x1;
	cmd.u.tri.y1    = y1;
	cmd.u.tri.x2    = x2;
	cmd.u.tri.y2    = y2;
	cmd.u.tri.color = color;
	return display_push(b, cmd);
}

static bool display_linear_axis_ok(float x0, float y0, float x1, float y1) {
	return display_finite(x0) && display_finite(y0) && display_finite(x1) && display_finite(y1);
}

bool xent_dl_strokerrect_grad(XentDlBuilder *b, XentRect rect, float radius, float width, XentLinearGrad const *grad) {
	if (!grad
		|| !display_finite_rect(rect)
		|| !display_finite(radius)
		|| radius < 0.0f
		|| !display_finite(width)
		|| width < 0.0f
		|| !display_linear_axis_ok(grad->x0, grad->y0, grad->x1, grad->y1)
		|| !display_finite(grad->s0)
		|| !display_finite(grad->s1))
		return false;
	XentDlCmd cmd             = {.op = XENT_DL_STROKERRECT_GRAD};
	cmd.u.linear.rect         = rect;
	cmd.u.linear.radius       = radius;
	cmd.u.linear.stroke_width = width;
	cmd.u.linear.x0           = grad->x0;
	cmd.u.linear.y0           = grad->y0;
	cmd.u.linear.x1           = grad->x1;
	cmd.u.linear.y1           = grad->y1;
	cmd.u.linear.s0           = grad->s0;
	cmd.u.linear.s1           = grad->s1;
	cmd.u.linear.c0           = grad->c0;
	cmd.u.linear.c1           = grad->c1;
	return display_push(b, cmd);
}

bool xent_dl_fillrect_grad(XentDlBuilder *b, XentRect rect, XentLinearGrad const *grad) {
	if (!grad
		|| !display_finite_rect(rect)
		|| !display_linear_axis_ok(grad->x0, grad->y0, grad->x1, grad->y1)
		|| !display_finite(grad->s0)
		|| !display_finite(grad->s1))
		return false;
	XentDlCmd cmd             = {.op = XENT_DL_FILLRECT_GRAD};
	cmd.u.linear.rect         = rect;
	cmd.u.linear.radius       = 0.0f;
	cmd.u.linear.stroke_width = 0.0f;
	cmd.u.linear.x0           = grad->x0;
	cmd.u.linear.y0           = grad->y0;
	cmd.u.linear.x1           = grad->x1;
	cmd.u.linear.y1           = grad->y1;
	cmd.u.linear.s0           = grad->s0;
	cmd.u.linear.s1           = grad->s1;
	cmd.u.linear.c0           = grad->c0;
	cmd.u.linear.c1           = grad->c1;
	return display_push(b, cmd);
}

XentDl *xent_dl_end(XentDlBuilder *b) {
	if (!b) return NULL;
	if (b->clip_depth || b->transform_depth || b->opacity_depth) {
		xent_dl_abort(b);
		return NULL;
	}
	XentDl *list = ( XentDl * ) calloc(1, sizeof(*list));
	if (!list) {
		xent_dl_abort(b);
		return NULL;
	}
	list->cmds        = b->cmds;
	list->count       = b->count;
	list->strings     = b->strings;
	list->strings_len = b->strings_len;
	b->cmds           = NULL;
	b->strings        = NULL;
	free(b);
	return list;
}

void xent_dl_free(XentDl *list) {
	if (!list) return;
	free(list->cmds);
	free(list->strings);
	free(list);
}

uint32_t         xent_dl_ncmd(XentDl const *list) { return list ? list->count : 0u; }

XentDlCmd const *xent_dl_cmd(XentDl const *list, uint32_t index) {
	if (!list || index >= list->count) return NULL;
	return &list->cmds [index];
}

char const *xent_dl_text(XentDl const *list, uint32_t offset, uint32_t len) {
	if (!list || !list->strings) return NULL;
	if (offset > list->strings_len || len > list->strings_len - offset) return NULL;
	return list->strings + offset;
}
