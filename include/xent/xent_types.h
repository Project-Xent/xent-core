#ifndef XENT_TYPES_H
#define XENT_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_MSC_VER) && _MSC_VER < 1940
  #error "Xent requires MSVC 19.40 (Visual Studio 2022 17.10) or newer."
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311l
  #ifdef __has_c_attribute
	#if __has_c_attribute(nodiscard)
	  #define XENT_NODISCARD [[nodiscard]]
	#endif
  #endif
#endif
#ifndef XENT_NODISCARD
  #define XENT_NODISCARD
#endif

typedef uint64_t XentNodeId;
typedef uint64_t XentObsId;

#define XENT_NODE_INVALID          (( XentNodeId ) 0)
#define XENT_NODE_OBSERVER_INVALID (( XentObsId ) 0)

static uint32_t inline xent_node_index(XentNodeId id) { return ( uint32_t ) id; }

static uint32_t inline xent_node_generation(XentNodeId id) { return ( uint32_t ) (id >> 32); }

static XentNodeId inline xent_node_make(uint32_t index, uint32_t generation) {
	if (index == 0u) return XENT_NODE_INVALID;
	return (( XentNodeId ) generation << 32) | ( XentNodeId ) index;
}

struct XentCtx;
struct XentTextBackend;

typedef struct XentCtx         XentCtx;
typedef struct XentTextBackend XentTextBackend;

typedef enum XentProtocol
{
	XENT_PROTOCOL_ABSOLUTE   = 1,
	XENT_PROTOCOL_FLEX       = 2,
	XENT_PROTOCOL_SWIFTSTACK = 3,
	XENT_PROTOCOL_GRID       = 4,
} XentProtocol;

typedef enum XentGridSizeMode
{
	XENT_GRID_AUTO  = 0,
	XENT_GRID_PIXEL = 1,
	XENT_GRID_STAR  = 2,
} XentGridSizeMode;

typedef enum XentFlexDirection
{
	XENT_FLEX_ROW    = 0,
	XENT_FLEX_COLUMN = 1,
} XentFlexDirection;

typedef enum XentFlexJustify
{
	XENT_FLEX_JUSTIFY_START         = 0,
	XENT_FLEX_JUSTIFY_END           = 1,
	XENT_FLEX_JUSTIFY_CENTER        = 2,
	XENT_FLEX_JUSTIFY_SPACE_BETWEEN = 3,
	XENT_FLEX_JUSTIFY_SPACE_AROUND  = 4,
	XENT_FLEX_JUSTIFY_SPACE_EVENLY  = 5,
} XentFlexJustify;

typedef enum XentFlexAlign
{
	XENT_FLEX_ALIGN_AUTO     = 0,
	XENT_FLEX_ALIGN_START    = 1,
	XENT_FLEX_ALIGN_END      = 2,
	XENT_FLEX_ALIGN_CENTER   = 3,
	XENT_FLEX_ALIGN_STRETCH  = 4,
	XENT_FLEX_ALIGN_BASELINE = 5,
} XentFlexAlign;

typedef enum XentFlexWrap
{
	XENT_FLEX_NO_WRAP = 0,
	XENT_FLEX_WRAP    = 1,
} XentFlexWrap;

typedef enum XentFlexAlignContent
{
	XENT_FLEX_ALIGN_CONTENT_START         = 0,
	XENT_FLEX_ALIGN_CONTENT_END           = 1,
	XENT_FLEX_ALIGN_CONTENT_CENTER        = 2,
	XENT_FLEX_ALIGN_CONTENT_SPACE_BETWEEN = 3,
	XENT_FLEX_ALIGN_CONTENT_SPACE_AROUND  = 4,
	XENT_FLEX_ALIGN_CONTENT_SPACE_EVENLY  = 5,
	XENT_FLEX_ALIGN_CONTENT_STRETCH       = 6,
} XentFlexAlignContent;

typedef enum XentAxis
{
	XENT_AXIS_HORIZONTAL = 0,
	XENT_AXIS_VERTICAL   = 1,
} XentAxis;

typedef enum XentStackAlign
{
	XENT_STACK_ALIGN_START    = 0,
	XENT_STACK_ALIGN_BASELINE = 1,
} XentStackAlign;

typedef enum XentDirection
{
	XENT_DIRECTION_INHERIT = 0,
	XENT_DIRECTION_LTR     = 1,
	XENT_DIRECTION_RTL     = 2,
} XentDirection;

typedef enum XentSemRole
{
	XENT_SEM_ROLE_NONE = 0,
	XENT_SEM_ROLE_ROOT,
	XENT_SEM_ROLE_CONTAINER,
	XENT_SEM_ROLE_TEXT,
	XENT_SEM_ROLE_BUTTON,
	XENT_SEM_ROLE_IMAGE,
	XENT_SEM_ROLE_CUSTOM,
} XentSemRole;

typedef enum XentDirtyFlags
{
	XENT_DIRTY_NONE    = 0,
	XENT_DIRTY_SELF    = 1u << 0,
	XENT_DIRTY_SUBTREE = 1u << 1,
	XENT_DIRTY_LAYOUT  = 1u << 2,
} XentDirtyFlags;

typedef enum XentLayoutStrategy
{
	XENT_LAYOUT_STRATEGY_NONE  = 0,
	XENT_LAYOUT_STRATEGY_FULL  = 1,
	XENT_LAYOUT_STRATEGY_DIRTY = 2,
} XentLayoutStrategy;

typedef enum XentLineBreakPolicy
{
	XENT_LINEBREAK_NOWRAP   = 0,
	XENT_LINEBREAK_WORDWRAP = 1,
	XENT_LINEBREAK_CHARWRAP = 2,
} XentLineBreakPolicy;

typedef enum XentMeasureMode
{
	XENT_MEASURE_UNDEFINED   = 0,
	XENT_MEASURE_AT_MOST     = 1,
	XENT_MEASURE_EXACTLY     = 2,
	XENT_MEASURE_MIN_CONTENT = 3,
} XentMeasureMode;

typedef struct XentRect {
	float x;
	float y;
	float w;
	float h;
} XentRect;

typedef struct XentSize {
	float w;
	float h;
} XentSize;

typedef struct XentPoint {
	float x;
	float y;
} XentPoint;

typedef struct XentInsets {
	float left;
	float top;
	float right;
	float bottom;
} XentInsets;

typedef struct XentResolvedInsets {
	float main_start;
	float main_end;
	float cross_start;
	float cross_end;
} XentResolvedInsets;

typedef enum XentChildOrder
{
	XENT_CHILD_ORDER_FORWARD = 0,
	XENT_CHILD_ORDER_REVERSE = 1,
	XENT_CHILD_ORDER_ZASC    = 2,
	XENT_CHILD_ORDER_ZDESC   = 3,
} XentChildOrder;

typedef enum XentNodeEventKind
{
	XENT_NODE_EVENT_DESTROY  = 1,
	XENT_NODE_EVENT_REPARENT = 2,
} XentNodeEventKind;

typedef struct XentNodeEvent {
	XentNodeId        node;
	XentNodeId        old_parent;
	XentNodeId        new_parent;
	XentNodeEventKind event;
} XentNodeEvent;

typedef void (*XentNodeObsFn)(XentCtx *ctx, XentNodeEvent const *lifecycle, void *userdata);

typedef struct XentNodeObs {
	XentNodeObsFn notify;
	void         *userdata;
} XentNodeObs;

typedef enum XentFontSlant
{
	XENT_FONT_SLANT_NORMAL  = 0,
	XENT_FONT_SLANT_ITALIC  = 1,
	XENT_FONT_SLANT_OBLIQUE = 2,
} XentFontSlant;

typedef enum XentTextTrim
{
	XENT_TEXT_TRIM_NONE          = 0,
	XENT_TEXT_TRIM_CHAR          = 1,
	XENT_TEXT_TRIM_CHAR_ELLIPSIS = 2,
	XENT_TEXT_TRIM_WORD          = 3,
	XENT_TEXT_TRIM_WORD_ELLIPSIS = 4,
} XentTextTrim;

typedef struct XentTextRun {
	uint32_t      offset;
	uint32_t      length;
	char const   *font_family;
	float         font_size;
	uint16_t      font_weight;
	uint8_t       font_stretch;
	XentFontSlant slant;
} XentTextRun;

typedef struct XentTextMetrics {
	float    width;
	float    height;
	uint32_t line_count;
	float    first_baseline;
} XentTextMetrics;

typedef struct XentTextMeasureReq {
	char const         *text;
	XentTextRun const  *runs;
	uint32_t            run_count;
	char const         *font_family;
	float               font_size;
	uint16_t            font_weight;
	float               width_constraint;
	uint32_t            max_lines;
	XentTextTrim        trim;
	XentLineBreakPolicy line_break_policy;
	XentMeasureMode     width_mode;
} XentTextMeasureReq;

typedef uint64_t XentFontId;
#define XENT_FONT_INVALID (( XentFontId ) 0)

typedef struct XentGlyph {
	uint32_t id;
	uint32_t cluster;
	float    advance;
	float    offset_x;
	float    offset_y;
} XentGlyph;

typedef struct XentGlyphRunDesc {
	XentFontId font;
	float      font_size;
	float      origin_x;
	float      origin_y;
	uint8_t    bidi_level;
} XentGlyphRunDesc;

typedef struct XentGlyphRun {
	XentGlyphRunDesc desc;
	uint32_t         glyph_offset;
	uint32_t         glyph_count;
} XentGlyphRun;

typedef struct XentTextShape {
	XentGlyph const    *glyphs;
	XentGlyphRun const *runs;
	uint32_t            glyph_count;
	uint32_t            run_count;
	XentTextMetrics     metrics;
} XentTextShape;

typedef struct XentTextCacheStats {
	uint64_t hits;
	uint64_t misses;
	uint64_t inserts;
	uint64_t evictions;
} XentTextCacheStats;

typedef struct XentProfStats {
	double   swiftstack_total_ms;
	double   swiftstack_collect_ms;
	double   swiftstack_sort_ms;
	double   swiftstack_text_ms;
	uint64_t temp_allocations;
	uint64_t sort_calls;
	uint64_t sibling_scans;
	uint64_t text_measure_calls;
	double   flex_total_ms;
	double   flex_collect_ms;
	double   flex_line_ms;
	double   flex_text_ms;
	double   grid_total_ms;
	double   grid_track_ms;
	double   grid_children_ms;
	double   grid_text_ms;
	uint64_t swiftstack_layout_calls;
	uint64_t flex_layout_calls;
	uint64_t grid_layout_calls;
	uint64_t text_baseline_fallbacks;
} XentProfStats;

typedef struct XentCfg {
	uint32_t initial_capacity;
	float    mono_glyph_width;
	float    mono_line_height;
	float    point_scale_factor;
	bool     enable_pixel_rounding;
	bool     enable_simd;
} XentCfg;

#endif
