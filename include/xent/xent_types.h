#ifndef XENT_TYPES_H
#define XENT_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef uint32_t XentNodeId;

#define XENT_NODE_INVALID ((XentNodeId)0)

typedef enum XentProtocol {
    XENT_PROTOCOL_ABSOLUTE = 1,
    XENT_PROTOCOL_FLEX = 2,
    XENT_PROTOCOL_SWIFTSTACK = 3,
    XENT_PROTOCOL_GRID = 4,
} XentProtocol;

typedef enum XentGridSizeMode {
    XENT_GRID_AUTO = 0,
    XENT_GRID_PIXEL = 1,
    XENT_GRID_STAR = 2,
} XentGridSizeMode;

typedef enum XentFlexDirection {
    XENT_FLEX_ROW = 0,
    XENT_FLEX_COLUMN = 1,
} XentFlexDirection;

typedef enum XentFlexJustify {
    XENT_FLEX_JUSTIFY_START = 0,
    XENT_FLEX_JUSTIFY_END = 1,
    XENT_FLEX_JUSTIFY_CENTER = 2,
    XENT_FLEX_JUSTIFY_SPACE_BETWEEN = 3,
    XENT_FLEX_JUSTIFY_SPACE_AROUND = 4,
    XENT_FLEX_JUSTIFY_SPACE_EVENLY = 5,
} XentFlexJustify;

typedef enum XentFlexAlign {
    XENT_FLEX_ALIGN_AUTO = 0,
    XENT_FLEX_ALIGN_START = 1,
    XENT_FLEX_ALIGN_END = 2,
    XENT_FLEX_ALIGN_CENTER = 3,
    XENT_FLEX_ALIGN_STRETCH = 4,
    XENT_FLEX_ALIGN_BASELINE = 5,
} XentFlexAlign;

typedef enum XentFlexWrap {
    XENT_FLEX_NO_WRAP = 0,
    XENT_FLEX_WRAP = 1,
} XentFlexWrap;

typedef enum XentFlexAlignContent {
    XENT_FLEX_ALIGN_CONTENT_START = 0,
    XENT_FLEX_ALIGN_CONTENT_END = 1,
    XENT_FLEX_ALIGN_CONTENT_CENTER = 2,
    XENT_FLEX_ALIGN_CONTENT_SPACE_BETWEEN = 3,
    XENT_FLEX_ALIGN_CONTENT_SPACE_AROUND = 4,
    XENT_FLEX_ALIGN_CONTENT_SPACE_EVENLY = 5,
} XentFlexAlignContent;

typedef enum XentAxis {
    XENT_AXIS_HORIZONTAL = 0,
    XENT_AXIS_VERTICAL = 1,
} XentAxis;

typedef enum XentStackAlign {
    XENT_STACK_ALIGN_START = 0,
    XENT_STACK_ALIGN_BASELINE = 1,
} XentStackAlign;

typedef enum XentDirection {
    XENT_DIRECTION_INHERIT = 0,
    XENT_DIRECTION_LTR = 1,
    XENT_DIRECTION_RTL = 2,
} XentDirection;

typedef enum XentSemanticRole {
    XENT_SEMANTIC_NONE = 0,
    XENT_SEMANTIC_ROOT,
    XENT_SEMANTIC_CONTAINER,
    XENT_SEMANTIC_TEXT,
    XENT_SEMANTIC_BUTTON,
    XENT_SEMANTIC_IMAGE,
    XENT_SEMANTIC_CUSTOM,
} XentSemanticRole;

typedef enum XentControlType {
    XENT_CONTROL_CONTAINER = 0,
    XENT_CONTROL_TEXT,
    XENT_CONTROL_BUTTON,
    XENT_CONTROL_TOGGLE_BUTTON,
    XENT_CONTROL_CHECKBOX,
    XENT_CONTROL_RADIO,
    XENT_CONTROL_SWITCH,
    XENT_CONTROL_SLIDER,
    XENT_CONTROL_TEXT_INPUT,
    XENT_CONTROL_SCROLL,
    XENT_CONTROL_IMAGE,
    XENT_CONTROL_PROGRESS,
    XENT_CONTROL_LIST,
    XENT_CONTROL_TAB,
    XENT_CONTROL_CARD,
    XENT_CONTROL_DIVIDER,
    XENT_CONTROL_CANVAS,
    XENT_CONTROL_PASSWORD_BOX,
    XENT_CONTROL_NUMBER_BOX,
    XENT_CONTROL_HYPERLINK,
    XENT_CONTROL_REPEAT_BUTTON,
    XENT_CONTROL_PROGRESS_RING,
    XENT_CONTROL_INFO_BADGE,
    XENT_CONTROL_CUSTOM,
} XentControlType;

typedef enum XentDirtyFlags {
    XENT_DIRTY_NONE = 0,
    XENT_DIRTY_SELF = 1u << 0,
    XENT_DIRTY_SUBTREE = 1u << 1,
    XENT_DIRTY_LAYOUT = 1u << 2,
} XentDirtyFlags;

typedef enum XentLayoutStrategy {
    XENT_LAYOUT_STRATEGY_NONE = 0,
    XENT_LAYOUT_STRATEGY_FULL_TREE = 1,
    XENT_LAYOUT_STRATEGY_DIRTY_SUBTREE = 2,
} XentLayoutStrategy;

typedef enum XentLineBreakPolicy {
    XENT_LINE_BREAK_NO_WRAP = 0,
    XENT_LINE_BREAK_WORD_WRAP = 1,
    XENT_LINE_BREAK_CHAR_WRAP = 2,
} XentLineBreakPolicy;

typedef enum XentMeasureMode {
    XENT_MEASURE_UNDEFINED = 0,
    XENT_MEASURE_AT_MOST = 1,
    XENT_MEASURE_EXACTLY = 2,
} XentMeasureMode;

typedef struct XentRect {
    float x;
    float y;
    float width;
    float height;
} XentRect;

typedef struct XentTextMetrics {
    float width;
    float height;
    uint32_t line_count;
} XentTextMetrics;

typedef struct XentTextCacheStats {
    uint64_t hits;
    uint64_t misses;
    uint64_t inserts;
} XentTextCacheStats;

typedef struct XentShapedGlyph {
    uint32_t codepoint;
    uint32_t cluster;
    uint32_t line_index;
    float advance;
    float offset_x;
    float offset_y;
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
    float width;
} XentShapedLine;

typedef struct XentShapingResult {
    XentTextMetrics metrics;
    uint32_t glyph_count;
    uint32_t run_count;
    uint32_t line_count;
    bool truncated;
} XentShapingResult;

typedef struct XentProfileStats {
    double swiftstack_total_ms;
    double swiftstack_collect_ms;
    double swiftstack_sort_ms;
    double swiftstack_text_ms;
    uint64_t temp_allocations;
    uint64_t sort_calls;
    uint64_t sibling_scans;
    uint64_t text_measure_calls;
} XentProfileStats;

typedef struct XentConfig {
    uint32_t initial_capacity;
    float mono_glyph_width;
    float mono_line_height;
    bool enable_simd;
    float point_scale_factor;
    bool enable_pixel_rounding;
} XentConfig;

struct XentContext;
struct XentPlugin;
struct XentTextBackend;

typedef struct XentContext XentContext;
typedef struct XentPlugin XentPlugin;
typedef struct XentTextBackend XentTextBackend;

#endif
