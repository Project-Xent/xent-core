#ifndef XENT_DISPLAY_H
#define XENT_DISPLAY_H

#include <assert.h>

#include "xent_types.h"

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

#define XENT_DL_TEXT_WRAP 0x01u
#define XENT_DL_TEXT_ICON 0x02u

typedef struct XentTextStyle {
	float        font_size;
	uint16_t     font_weight;
	uint8_t      h_align;
	uint8_t      v_align;
	uint8_t      flags;
	uint8_t      max_lines;
	XentTextTrim trim;
	char const  *font_family;
} XentTextStyle;

typedef enum XentImageFit
{
	XENT_IMAGE_FIT_FILL = 0,
	XENT_IMAGE_FIT_UNIFORM,
	XENT_IMAGE_FIT_UNIFORM_TO_FILL,
	XENT_IMAGE_FIT_NONE,
} XentImageFit;

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
			uint32_t  font_offset;
			uint32_t  font_len;
			float     font_size;
			uint16_t  font_weight;
			uint8_t   h_align;
			uint8_t   v_align;
			uint8_t   flags;
			uint8_t   max_lines;
			uint8_t   trim;
			XentColor color;
		} text;

		struct {
			XentRect       bounds;
			XentResourceId image;
			XentImageFit   stretch;
			uint32_t       frame;
			XentInsets     nine;
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
/** Copies `utf8`. The caller may free it. */
bool xent_dl_drawtext(
  XentDlBuilder *b, XentRect bounds, char const *utf8, float font_size, uint16_t font_weight, XentColor color
);
/** Copies `utf8`. The caller may free it. */
bool xent_dl_drawtextstyle(
  XentDlBuilder *b, XentRect bounds, char const *utf8, XentTextStyle const *style, XentColor color
);
bool xent_dl_drawimg(XentDlBuilder *b, XentRect bounds, XentResourceId image, XentImageFit stretch);
bool xent_dl_drawframe(
  XentDlBuilder *b, XentRect bounds, XentResourceId image, XentImageFit stretch, uint32_t frame, XentInsets nine
);
bool xent_dl_pushcliprect(XentDlBuilder *b, XentRect rect);
bool xent_dl_pushcliprrect(XentDlBuilder *b, XentRect rect, float radius);
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
bool xent_dl_pushclipellipse(XentDlBuilder *b, XentRect bounds);
bool xent_dl_fillshape(XentDlBuilder *b, XentResourceId shape, XentRect bounds, XentColor color);
bool xent_dl_filltri(XentDlBuilder *b, float x0, float y0, float x1, float y1, float x2, float y2, XentColor color);
bool xent_dl_strokerrectgrad(XentDlBuilder *b, XentRect rect, float radius, float width, XentLinearGrad const *grad);
bool xent_dl_fillrectgrad(XentDlBuilder *b, XentRect rect, XentLinearGrad const *grad);

/** Consumes `b`, including when a clip, transform, or opacity push is still open. */
XENT_NODISCARD XentDl *xent_dl_end(XentDlBuilder *b);
void                   xent_dl_free(XentDl *list);

uint32_t               xent_dl_ncmd(XentDl const *list);
XentDlCmd const       *xent_dl_cmd(XentDl const *list, uint32_t index);
/** Points into `list` until `xent_dl_free`. */
char const            *xent_dl_text(XentDl const *list, uint32_t offset, uint32_t len);

static_assert(sizeof(XentStrokeCap) == 4, "XentStrokeCap crosses the ABI as int32");
static_assert(sizeof(XentDlOp) == 4, "XentDlOp crosses the ABI as int32");
static_assert(sizeof(XentImageFit) == 4, "XentImageFit crosses the ABI as int32");

#endif
