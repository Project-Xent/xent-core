#ifndef XENT_DISPLAY_H
#define XENT_DISPLAY_H

#include "xent_types.h"


/* Immutable, platform-neutral display list. Builders mutate; finished
 * lists expose only const accessors. Backends replay; Core never holds
 * HWND/D2D/GPU objects. Image/shape slots are opaque cookies resolved by the host.
 *
 * Geometry is node-local. Arc angles: 0 = +X, positive sweep is clockwise in
 * y-down space. Affine maps p' = (x*m00 + y*m01 + m02, x*m10 + y*m11 + m12)
 * with row-major storage (m00 m01 m02 / m10 m11 m12). No open PathBuilder. */

typedef uint64_t XentResourceId;
#define XENT_RESOURCE_INVALID (( XentResourceId ) 0)

typedef struct XentColor {
	uint8_t r, g, b, a;
} XentColor;

static XentColor inline xent_color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	return ( XentColor ) {r, g, b, a};
}

typedef enum XentStrokeCap
{
	XENT_STROKE_CAP_FLAT = 0,
	XENT_STROKE_CAP_ROUND,
	XENT_STROKE_CAP_SQUARE,
} XentStrokeCap;

typedef struct XentAffine2 {
	float m00, m01, m02;
	float m10, m11, m12;
} XentAffine2;

static XentAffine2 inline xent_affine_identity(void) { return ( XentAffine2 ) {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f}; }

static XentAffine2 inline xent_affine_translate(float x, float y) {
	return ( XentAffine2 ) {1.0f, 0.0f, x, 0.0f, 1.0f, y};
}

static XentPoint inline xent_affine_map(XentAffine2 const *m, float x, float y) {
	return ( XentPoint ) {x * m->m00 + y * m->m01 + m->m02, x * m->m10 + y * m->m11 + m->m12};
}

typedef enum XentDlOp
{
	XENT_DL_FILLRECT = 1,
	XENT_DL_STROKERECT,
	XENT_DL_FILLRRECT,
	XENT_DL_STROKERRECT,
	XENT_DL_DRAWTEXT,
	XENT_DL_DRAWIMG,
	XENT_DL_PUSHCLIP_RECT,
	XENT_DL_POPCLIP,
	XENT_DL_PUSHTRANSFORM,
	XENT_DL_POPTRANSFORM,
	XENT_DL_PUSHOPACITY,
	XENT_DL_POPOPACITY,
	XENT_DL_FILLELLIPSE,
	XENT_DL_STROKEELLIPSE,
	XENT_DL_STROKEARC,
	XENT_DL_STROKELINE,
	XENT_DL_PUSHCLIP_ELLIPSE,
	XENT_DL_PUSHCLIP_RRECT,
	XENT_DL_FILLSHAPE,
	XENT_DL_FILLTRI,
	XENT_DL_STROKERRECT_GRAD,
	XENT_DL_FILLRECT_GRAD,
} XentDlOp;

/** @brief DRAW_TEXT flags (platform-neutral). */
#define XENT_DL_TEXT_WRAP 0x01u
#define XENT_DL_TEXT_ICON 0x02u

typedef struct XentTextStyle {
	float       font_size;
	uint16_t    font_weight;
	uint8_t     h_align;     /**< 0=left, 1=center, 2=right */
	uint8_t     v_align;     /**< 0=top, 1=center, 2=bottom */
	uint8_t     flags;       /**< XENT_DL_TEXT_WRAP | XENT_DL_TEXT_ICON */
	char const *font_family; /**< NULL = platform default UI font; copied into list string pool */
} XentTextStyle;

typedef enum XentImageFit
{
	XENT_IMAGE_FIT_FILL = 0,
	XENT_IMAGE_FIT_UNIFORM,
	XENT_IMAGE_FIT_UNIFORM_TO_FILL,
	XENT_IMAGE_FIT_NONE,
} XentImageFit;

/* Two-stop linear gradient. Axis (x0,y0)→(x1,y1) is in the same object-local
 * space as the geometry it fills; s0/s1 are stop positions. Replay applies the
 * current transform stack to both geometry and brush points. */
typedef struct XentLinearGrad {
	float     x0, y0, x1, y1;
	float     s0, s1;
	XentColor c0, c1;
} XentLinearGrad;

typedef struct XentDlCmd {
	uint16_t op;
	uint16_t reserved;

	union
	{
		struct {
			XentRect  rect;
			XentColor color;
			float     stroke_width;
			float     radius;
		} geom;

		struct {
			XentRect  bounds;
			uint32_t  text_offset;
			uint32_t  text_len;
			uint32_t  font_offset; /**< 0 + font_len 0 => default UI font */
			uint32_t  font_len;
			float     font_size;
			uint16_t  font_weight;
			uint8_t   h_align;
			uint8_t   v_align;
			uint8_t   flags;
			uint8_t   reserved;
			XentColor color;
		} text;

		struct {
			XentRect       bounds;
			XentResourceId image;
			XentImageFit   stretch;
		} image;

		struct {
			XentAffine2 m;
		} transform;

		struct {
			float opacity;
		} opacity;

		struct {
			XentRect      bounds;
			XentColor     color;
			float         stroke_width;
			XentStrokeCap cap;
		} ellipse;

		struct {
			XentRect      bounds;
			float         start_deg;
			float         sweep_deg;
			float         stroke_width;
			XentStrokeCap cap;
			XentColor     color;
		} arc;

		struct {
			float         x0, y0, x1, y1;
			float         stroke_width;
			XentStrokeCap cap;
			XentColor     color;
		} line;

		struct {
			XentRect       bounds;
			XentResourceId shape;
			XentColor      color;
		} shape;

		struct {
			float     x0, y0, x1, y1, x2, y2;
			XentColor color;
		} tri;

		/* Two-stop linear gradient; see XentLinearGrad. */
		struct {
			XentRect  rect;
			float     radius;
			float     stroke_width;
			float     x0, y0, x1, y1;
			float     s0, s1;
			XentColor c0, c1;
		} linear;
	} u;
} XentDlCmd;

typedef struct XentDlBuilder  XentDlBuilder;
typedef struct XentDl         XentDl;

XENT_NODISCARD XentDlBuilder *xent_dl_begin(void);
void                          xent_dl_abort(XentDlBuilder *b);

bool                          xent_dl_fillrect(XentDlBuilder *b, XentRect rect, XentColor color);
bool                          xent_dl_strokerect(XentDlBuilder *b, XentRect rect, XentColor color, float width);
bool                          xent_dl_fillrrect(XentDlBuilder *b, XentRect rect, float radius, XentColor color);
bool xent_dl_strokerrect(XentDlBuilder *b, XentRect rect, float radius, XentColor color, float width);
bool xent_dl_drawtext(
  XentDlBuilder *b, XentRect bounds, char const *utf8, float font_size, uint16_t font_weight, XentColor color
);
bool xent_dl_drawtext_style(
  XentDlBuilder *b, XentRect bounds, char const *utf8, XentTextStyle const *style, XentColor color
);
bool xent_dl_drawimg(XentDlBuilder *b, XentRect bounds, XentResourceId image, XentImageFit stretch);
bool xent_dl_pushclip_rect(XentDlBuilder *b, XentRect rect);
bool xent_dl_pushclip_rrect(XentDlBuilder *b, XentRect rect, float radius);
bool xent_dl_popclip(XentDlBuilder *b);
bool xent_dl_pushtransform(XentDlBuilder *b, XentAffine2 m);
bool xent_dl_poptransform(XentDlBuilder *b);
bool xent_dl_pushopacity(XentDlBuilder *b, float opacity);
bool xent_dl_popopacity(XentDlBuilder *b);

bool xent_dl_fillellipse(XentDlBuilder *b, XentRect bounds, XentColor color);
bool xent_dl_strokeellipse(XentDlBuilder *b, XentRect bounds, XentColor color, float width, XentStrokeCap cap);
bool xent_dl_strokearc(
  XentDlBuilder *b, XentRect bounds, float start_deg, float sweep_deg, XentColor color, float width, XentStrokeCap cap
);
bool xent_dl_strokeline(
  XentDlBuilder *b, float x0, float y0, float x1, float y1, XentColor color, float width, XentStrokeCap cap
);
bool xent_dl_pushclip_ellipse(XentDlBuilder *b, XentRect bounds);
bool xent_dl_fillshape(XentDlBuilder *b, XentResourceId shape, XentRect bounds, XentColor color);
bool xent_dl_filltri(XentDlBuilder *b, float x0, float y0, float x1, float y1, float x2, float y2, XentColor color);
bool xent_dl_strokerrect_grad(XentDlBuilder *b, XentRect rect, float radius, float width, XentLinearGrad const *grad);
bool xent_dl_fillrect_grad(XentDlBuilder *b, XentRect rect, XentLinearGrad const *grad);

/* Consumes the builder. Returned list is immutable until destroyed. */
XENT_NODISCARD XentDl *xent_dl_end(XentDlBuilder *b);
void                   xent_dl_free(XentDl *list);

uint32_t               xent_dl_ncmd(XentDl const *list);
XentDlCmd const       *xent_dl_cmd(XentDl const *list, uint32_t index);
char const            *xent_dl_text(XentDl const *list, uint32_t offset, uint32_t len);

#endif
