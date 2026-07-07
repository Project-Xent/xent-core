#ifndef XENT_TYPES_H
#define XENT_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* --- Toolchain contract -------------------------------------------------
 * The Xent stack builds as C23 (its own sources pass -std=c23 / /std:clatest).
 * The PUBLIC HEADERS stay C23-optional so toolchain probes and mixed-standard
 * consumers can include them without -std=c23: no hard C-standard #error (one
 * such #error broke xmake's has_cfuncs checks under clang/mingw, which compile
 * probes without -std=c23), and XENT_NODISCARD emits the attribute only where
 * the active standard accepts the [[...]] syntax. On MSVC the C23 features the
 * *sources* use land in cl 19.40 (VS 2022 17.10); guard only that, by compiler
 * version — which never trips a probe. See the MSVC matrix in Xent/CODE_STYLE.md. */
#if defined(_MSC_VER) && _MSC_VER < 1940
#  error "Xent requires MSVC 19.40 (Visual Studio 2022 17.10) or newer."
#endif

/* [[nodiscard]] only where the active standard accepts the [[...]] syntax
 * (C23, or C++17); in a C17/C11 probe it degrades to nothing. */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#  ifdef __has_c_attribute
#    if __has_c_attribute(nodiscard)
#      define XENT_NODISCARD [[nodiscard]]
#    endif
#  endif
#elif defined(__cplusplus) && __cplusplus >= 201703L
#  define XENT_NODISCARD [[nodiscard]]
#endif
#ifndef XENT_NODISCARD
#  define XENT_NODISCARD
#endif

typedef uint32_t XentNodeId;

#define XENT_NODE_INVALID (( XentNodeId ) 0)

struct XentContext;
struct XentPlugin;
struct XentTextBackend;

typedef struct XentContext     XentContext;
typedef struct XentPlugin      XentPlugin;
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
	XENT_FLEX_ALIGN_CONTENT_STRETCH       = 6, /**< grow each line's cross size to fill free space */
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

typedef enum XentSemanticRole
{
	XENT_SEMANTIC_NONE = 0,
	XENT_SEMANTIC_ROOT,
	XENT_SEMANTIC_CONTAINER,
	XENT_SEMANTIC_TEXT,
	XENT_SEMANTIC_BUTTON,
	XENT_SEMANTIC_IMAGE,
	XENT_SEMANTIC_CUSTOM,
} XentSemanticRole;

typedef enum XentDirtyFlags
{
	XENT_DIRTY_NONE    = 0,
	XENT_DIRTY_SELF    = 1u << 0,
	XENT_DIRTY_SUBTREE = 1u << 1,
	XENT_DIRTY_LAYOUT  = 1u << 2,
} XentDirtyFlags;

typedef enum XentLayoutStrategy
{
	XENT_LAYOUT_STRATEGY_NONE          = 0,
	XENT_LAYOUT_STRATEGY_FULL_TREE     = 1,
	XENT_LAYOUT_STRATEGY_DIRTY_SUBTREE = 2,
} XentLayoutStrategy;

typedef enum XentLineBreakPolicy
{
	XENT_LINE_BREAK_NO_WRAP   = 0,
	XENT_LINE_BREAK_WORD_WRAP = 1,
	XENT_LINE_BREAK_CHAR_WRAP = 2,
} XentLineBreakPolicy;

typedef enum XentMeasureMode
{
	XENT_MEASURE_UNDEFINED   = 0,
	XENT_MEASURE_AT_MOST     = 1,
	XENT_MEASURE_EXACTLY     = 2,
	XENT_MEASURE_MIN_CONTENT = 3, /**< break at every opportunity; width = longest unbreakable run */
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
	XENT_CHILD_ORDER_Z_ASC   = 2,
	XENT_CHILD_ORDER_Z_DESC  = 3,
} XentChildOrder;

typedef enum XentNodeLifecycleEvent
{
	XENT_NODE_EVENT_DESTROY  = 1,
	XENT_NODE_EVENT_REPARENT = 2,
} XentNodeLifecycleEvent;

typedef void (*XentNodeLifecycleFn)(
  XentContext *ctx, XentNodeId node, XentNodeLifecycleEvent event, XentNodeId old_parent, XentNodeId new_parent,
  void *userdata
);

typedef struct XentTextMetrics {
	float    width;
	float    height;
	uint32_t line_count;
} XentTextMetrics;

/* Measure and shape requests intentionally remain separate API types even
   though their fields match today. Real shaping backends are expected to grow
   shape-only inputs such as script/language/features without changing the
   measurement call surface. Keep common fields in sync until then. */
typedef struct XentTextMeasureRequest {
	char const         *text;
	float               font_size;
	uint16_t            font_weight; /**< CSS scale 100..900; 0 = default (400). */
	float               width_constraint;
	XentLineBreakPolicy line_break_policy;
	XentMeasureMode     width_mode;
} XentTextMeasureRequest;

typedef struct XentTextShapeRequest {
	char const         *text;
	float               font_size;
	uint16_t            font_weight; /**< CSS scale 100..900; 0 = default (400). */
	float               width_constraint;
	XentLineBreakPolicy line_break_policy;
	XentMeasureMode     width_mode;
} XentTextShapeRequest;

typedef struct XentTextCacheStats {
	uint64_t hits;
	uint64_t misses;
	uint64_t inserts;
	uint64_t evictions;
} XentTextCacheStats;

typedef struct XentShapedGlyph {
	uint32_t codepoint;
	uint32_t cluster;
	uint32_t line_index;
	float    advance;
	float    offset_x;
	float    offset_y;
} XentShapedGlyph;

typedef struct XentShapedRun {
	uint32_t glyph_start;
	uint32_t glyph_count;
	uint32_t line_start;
	uint32_t line_count;
} XentShapedRun;

typedef struct XentShapedLine {
	uint32_t glyph_start;
	uint32_t glyph_count;
	float    width;
} XentShapedLine;

typedef struct XentShapingResult {
	XentTextMetrics metrics;
	uint32_t        glyph_count;
	uint32_t        run_count;
	uint32_t        line_count;
	bool            truncated;
} XentShapingResult;

typedef struct XentTextShapeOutput {
	XentShapedGlyph   *glyphs;
	uint32_t           glyph_capacity;
	XentShapedRun     *runs;
	uint32_t           run_capacity;
	XentShapedLine    *lines;
	uint32_t           line_capacity;
	XentShapingResult *result;
} XentTextShapeOutput;

typedef struct XentProfileStats {
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
} XentProfileStats;

typedef struct XentConfig {
	uint32_t initial_capacity;
	float    mono_glyph_width;
	float    mono_line_height;
	bool     enable_simd;
	float    point_scale_factor;
	bool     enable_pixel_rounding;
} XentConfig;

#endif
