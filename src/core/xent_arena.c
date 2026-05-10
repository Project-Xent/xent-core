#include "../xent_internal.h"

#define XENT_GROW_FIELD_MAX 128u

typedef struct XentGrowField {
	size_t offset;
	size_t elem_size;
} XentGrowField;

#define XENT_NODE_GROW_FIELD(field) {offsetof(XentNodeStore, field), sizeof((( XentNodeStore * ) 0)->field [0])}

static XentGrowField const XENT_NODE_GROW_FIELDS [] = {
  XENT_NODE_GROW_FIELD(lifetime.alive),

  XENT_NODE_GROW_FIELD(topology.parent),
  XENT_NODE_GROW_FIELD(topology.first_child),
  XENT_NODE_GROW_FIELD(topology.last_child),
  XENT_NODE_GROW_FIELD(topology.next_sibling),
  XENT_NODE_GROW_FIELD(topology.prev_sibling),
  XENT_NODE_GROW_FIELD(topology.child_count),

  XENT_NODE_GROW_FIELD(layout.protocol),
  XENT_NODE_GROW_FIELD(layout.direction),
  XENT_NODE_GROW_FIELD(layout.dirty_flags),
  XENT_NODE_GROW_FIELD(layout.proposed_w),
  XENT_NODE_GROW_FIELD(layout.proposed_h),
  XENT_NODE_GROW_FIELD(layout.decided_w),
  XENT_NODE_GROW_FIELD(layout.decided_h),
  XENT_NODE_GROW_FIELD(layout.abs_x),
  XENT_NODE_GROW_FIELD(layout.abs_y),
  XENT_NODE_GROW_FIELD(layout.style_w),
  XENT_NODE_GROW_FIELD(layout.style_h),
  XENT_NODE_GROW_FIELD(layout.style_w_percent),
  XENT_NODE_GROW_FIELD(layout.style_h_percent),
  XENT_NODE_GROW_FIELD(layout.aspect_ratio),
  XENT_NODE_GROW_FIELD(layout.min_w),
  XENT_NODE_GROW_FIELD(layout.min_h),
  XENT_NODE_GROW_FIELD(layout.max_w),
  XENT_NODE_GROW_FIELD(layout.max_h),
  XENT_NODE_GROW_FIELD(layout.margin_l),
  XENT_NODE_GROW_FIELD(layout.margin_t),
  XENT_NODE_GROW_FIELD(layout.margin_r),
  XENT_NODE_GROW_FIELD(layout.margin_b),
  XENT_NODE_GROW_FIELD(layout.padding_l),
  XENT_NODE_GROW_FIELD(layout.padding_t),
  XENT_NODE_GROW_FIELD(layout.padding_r),
  XENT_NODE_GROW_FIELD(layout.padding_b),
  XENT_NODE_GROW_FIELD(layout.gap),
  XENT_NODE_GROW_FIELD(layout.abs_pos_x),
  XENT_NODE_GROW_FIELD(layout.abs_pos_y),
  XENT_NODE_GROW_FIELD(layout.z_index),

  XENT_NODE_GROW_FIELD(flex.grow),
  XENT_NODE_GROW_FIELD(flex.shrink),
  XENT_NODE_GROW_FIELD(flex.basis),
  XENT_NODE_GROW_FIELD(flex.direction),
  XENT_NODE_GROW_FIELD(flex.wrap),
  XENT_NODE_GROW_FIELD(flex.justify_content),
  XENT_NODE_GROW_FIELD(flex.align_items),
  XENT_NODE_GROW_FIELD(flex.align_self),
  XENT_NODE_GROW_FIELD(flex.align_content),

  XENT_NODE_GROW_FIELD(stack.axis),
  XENT_NODE_GROW_FIELD(stack.align),
  XENT_NODE_GROW_FIELD(stack.priority),
  XENT_NODE_GROW_FIELD(stack.spacer),

  XENT_NODE_GROW_FIELD(text.content),
  XENT_NODE_GROW_FIELD(text.font_size),
  XENT_NODE_GROW_FIELD(text.line_break_policy),
  XENT_NODE_GROW_FIELD(text.intrinsic_valid),
  XENT_NODE_GROW_FIELD(text.intrinsic_constraint_w),
  XENT_NODE_GROW_FIELD(text.intrinsic_font_size),
  XENT_NODE_GROW_FIELD(text.intrinsic_line_break_policy),
  XENT_NODE_GROW_FIELD(text.intrinsic_width_mode),
  XENT_NODE_GROW_FIELD(text.intrinsic_w),
  XENT_NODE_GROW_FIELD(text.intrinsic_h),
  XENT_NODE_GROW_FIELD(text.intrinsic_lines),

  XENT_NODE_GROW_FIELD(semantics.role),
  XENT_NODE_GROW_FIELD(semantics.label),
  XENT_NODE_GROW_FIELD(semantics.flags),
  XENT_NODE_GROW_FIELD(semantics.checked),
  XENT_NODE_GROW_FIELD(semantics.enabled),
  XENT_NODE_GROW_FIELD(semantics.expanded),
  XENT_NODE_GROW_FIELD(semantics.selected),
  XENT_NODE_GROW_FIELD(semantics.value_now),
  XENT_NODE_GROW_FIELD(semantics.value_min),
  XENT_NODE_GROW_FIELD(semantics.value_max),

  XENT_NODE_GROW_FIELD(external.userdata),
  XENT_NODE_GROW_FIELD(external.payload),
  XENT_NODE_GROW_FIELD(external.payload_type),
  XENT_NODE_GROW_FIELD(external.payload_destroy),
  XENT_NODE_GROW_FIELD(external.payload_destroy_userdata),
  XENT_NODE_GROW_FIELD(external.control_type),

  XENT_NODE_GROW_FIELD(focus.focusable),
  XENT_NODE_GROW_FIELD(focus.tab_index),

  XENT_NODE_GROW_FIELD(grid.def),
  XENT_NODE_GROW_FIELD(grid.row),
  XENT_NODE_GROW_FIELD(grid.column),
  XENT_NODE_GROW_FIELD(grid.row_span),
  XENT_NODE_GROW_FIELD(grid.column_span),
};

#undef XENT_NODE_GROW_FIELD

static uint32_t xent_node_grow_field_count(void) {
	return ( uint32_t ) (sizeof(XENT_NODE_GROW_FIELDS) / sizeof(XENT_NODE_GROW_FIELDS [0]));
}

static void **xent_grow_field_slot(XentNodeStore *nodes, XentGrowField const *field) {
	return ( void ** ) (( uint8_t * ) nodes + field->offset);
}

static void xent_free_new_arrays(void **new_ptrs, uint32_t count) {
	for (uint32_t i = 0u; i < count; ++i) free(new_ptrs [i]);
}

static bool
xent_alloc_grown_arrays(XentGrowField const *fields, uint32_t field_count, uint32_t new_cap, void **new_ptrs) {
	for (uint32_t i = 0u; i < field_count; ++i) {
		new_ptrs [i] = malloc(fields [i].elem_size * ( size_t ) new_cap);
		if (!new_ptrs [i]) {
			xent_free_new_arrays(new_ptrs, i);
			return false;
		}
	}
	return true;
}

static void xent_commit_grown_array(
  XentNodeStore *nodes, XentGrowField const *field, void *new_ptr, uint32_t old_cap, uint32_t new_cap
) {
	void **slot      = xent_grow_field_slot(nodes, field);
	void  *old_ptr   = *slot;
	size_t old_bytes = field->elem_size * ( size_t ) old_cap;
	size_t new_bytes = field->elem_size * ( size_t ) (new_cap - old_cap);
	if (old_ptr && old_bytes > 0u) memcpy(new_ptr, old_ptr, old_bytes);
	memset(( uint8_t * ) new_ptr + old_bytes, 0, new_bytes);
	free(old_ptr);
	*slot = new_ptr;
}

static bool xent_grow_arrays_two_phase(XentNodeStore *nodes, uint32_t old_cap, uint32_t new_cap) {
	void    *new_ptrs [XENT_GROW_FIELD_MAX] = {0};
	uint32_t field_count                    = xent_node_grow_field_count();
	if (field_count > XENT_GROW_FIELD_MAX) return false;
	if (!xent_alloc_grown_arrays(XENT_NODE_GROW_FIELDS, field_count, new_cap, new_ptrs)) return false;
	for (uint32_t i = 0u; i < field_count; ++i)
		xent_commit_grown_array(nodes, &XENT_NODE_GROW_FIELDS [i], new_ptrs [i], old_cap, new_cap);
	return true;
}

char *xent_strdup(char const *s) {
	if (!s) return NULL;
	size_t len  = strlen(s);
	char  *copy = ( char * ) malloc(len + 1u);
	if (!copy) return NULL;
	memcpy(copy, s, len + 1u);
	return copy;
}

static uint32_t xent_next_node_capacity(uint32_t old_cap, uint32_t needed) {
	uint32_t new_cap = old_cap ? old_cap : 64u;
	while (new_cap <= needed) new_cap *= 2u;
	return new_cap;
}

static void xent_init_layout_defaults(XentNodeStore *nodes, uint32_t i) {
	nodes->layout.style_w [i]         = NAN;
	nodes->layout.style_h [i]         = NAN;
	nodes->layout.style_w_percent [i] = NAN;
	nodes->layout.style_h_percent [i] = NAN;
	nodes->layout.aspect_ratio [i]    = NAN;
	nodes->layout.min_w [i]           = 0.0f;
	nodes->layout.min_h [i]           = 0.0f;
	nodes->layout.max_w [i]           = INFINITY;
	nodes->layout.max_h [i]           = INFINITY;
	nodes->layout.protocol [i]        = ( uint8_t ) XENT_PROTOCOL_ABSOLUTE;
	nodes->layout.direction [i]       = ( uint8_t ) XENT_DIRECTION_INHERIT;
}

static void xent_init_flex_stack_defaults(XentNodeStore *nodes, uint32_t i) {
	nodes->flex.basis [i]           = NAN;
	nodes->flex.shrink [i]          = 1.0f;
	nodes->flex.direction [i]       = ( uint8_t ) XENT_FLEX_ROW;
	nodes->flex.wrap [i]            = ( uint8_t ) XENT_FLEX_NO_WRAP;
	nodes->flex.justify_content [i] = ( uint8_t ) XENT_FLEX_JUSTIFY_START;
	nodes->flex.align_items [i]     = ( uint8_t ) XENT_FLEX_ALIGN_START;
	nodes->flex.align_self [i]      = ( uint8_t ) XENT_FLEX_ALIGN_AUTO;
	nodes->flex.align_content [i]   = ( uint8_t ) XENT_FLEX_ALIGN_CONTENT_START;
	nodes->stack.axis [i]           = ( uint8_t ) XENT_AXIS_HORIZONTAL;
	nodes->stack.align [i]          = ( uint8_t ) XENT_STACK_ALIGN_START;
}

static void xent_init_text_defaults(XentNodeStore *nodes, uint32_t i) {
	nodes->text.font_size [i]                   = 14.0f;
	nodes->text.line_break_policy [i]           = ( uint8_t ) XENT_LINE_BREAK_CHAR_WRAP;
	nodes->text.intrinsic_valid [i]             = 0u;
	nodes->text.intrinsic_constraint_w [i]      = NAN;
	nodes->text.intrinsic_font_size [i]         = 0.0f;
	nodes->text.intrinsic_line_break_policy [i] = ( uint8_t ) XENT_LINE_BREAK_CHAR_WRAP;
	nodes->text.intrinsic_width_mode [i]        = ( uint8_t ) XENT_MEASURE_UNDEFINED;
	nodes->text.intrinsic_w [i]                 = 0.0f;
	nodes->text.intrinsic_h [i]                 = 0.0f;
	nodes->text.intrinsic_lines [i]             = 0u;
}

static void xent_init_semantic_grid_defaults(XentNodeStore *nodes, uint32_t i) {
	nodes->semantics.enabled [i] = 1u;
	nodes->grid.def [i]          = NULL;
	nodes->grid.row [i]          = 0;
	nodes->grid.column [i]       = 0;
	nodes->grid.row_span [i]     = 1;
	nodes->grid.column_span [i]  = 1;
}

static void xent_init_node_defaults(XentNodeStore *nodes, uint32_t old_cap, uint32_t new_cap) {
	for (uint32_t i = old_cap; i < new_cap; ++i) {
		xent_init_layout_defaults(nodes, i);
		xent_init_flex_stack_defaults(nodes, i);
		xent_init_text_defaults(nodes, i);
		xent_init_semantic_grid_defaults(nodes, i);
	}
}

bool xent_ensure_node_capacity(XentContext *ctx, uint32_t needed) {
	if (!ctx) return false;

	XentNodeStore *n = &ctx->nodes;
	if (needed <= n->capacity) return true;

	uint32_t old_cap = n->capacity;
	uint32_t new_cap = xent_next_node_capacity(old_cap, needed);
	if (!xent_grow_arrays_two_phase(n, old_cap, new_cap)) return false;
	xent_init_node_defaults(n, old_cap, new_cap);

	n->capacity = new_cap;
	return true;
}
