#include "../xent_internal.h"

static bool xent_realloc_and_zero(void **ptr, size_t elem_size, uint32_t old_cap, uint32_t new_cap) {
	void *new_mem = realloc(*ptr, elem_size * ( size_t ) new_cap);
	if (!new_mem) return false;
	*ptr = new_mem;
	if (new_cap > old_cap)
		memset(( uint8_t * ) *ptr + elem_size * ( size_t ) old_cap, 0, elem_size * ( size_t ) (new_cap - old_cap));
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

bool xent_ensure_node_capacity(XentContext *ctx, uint32_t needed) {
	if (!ctx) return false;

	XentNodeStore *n = &ctx->nodes;
	if (needed <= n->capacity) return true;

	uint32_t old_cap = n->capacity;
	uint32_t new_cap = old_cap ? old_cap : 64u;
	while (new_cap <= needed) new_cap *= 2u;

#define GROW_ARRAY(field)                                                                                     \
	if (!xent_realloc_and_zero(( void ** ) &n->field, sizeof(*n->field), old_cap, new_cap)) { return false; }

	GROW_ARRAY(lifetime.alive);

	GROW_ARRAY(topology.parent);
	GROW_ARRAY(topology.first_child);
	GROW_ARRAY(topology.last_child);
	GROW_ARRAY(topology.next_sibling);
	GROW_ARRAY(topology.prev_sibling);
	GROW_ARRAY(topology.child_count);

	GROW_ARRAY(layout.protocol);
	GROW_ARRAY(layout.direction);
	GROW_ARRAY(layout.dirty_flags);
	GROW_ARRAY(layout.proposed_w);
	GROW_ARRAY(layout.proposed_h);
	GROW_ARRAY(layout.decided_w);
	GROW_ARRAY(layout.decided_h);
	GROW_ARRAY(layout.abs_x);
	GROW_ARRAY(layout.abs_y);
	GROW_ARRAY(layout.style_w);
	GROW_ARRAY(layout.style_h);
	GROW_ARRAY(layout.min_w);
	GROW_ARRAY(layout.min_h);
	GROW_ARRAY(layout.max_w);
	GROW_ARRAY(layout.max_h);
	GROW_ARRAY(layout.margin_l);
	GROW_ARRAY(layout.margin_t);
	GROW_ARRAY(layout.margin_r);
	GROW_ARRAY(layout.margin_b);
	GROW_ARRAY(layout.padding_l);
	GROW_ARRAY(layout.padding_t);
	GROW_ARRAY(layout.padding_r);
	GROW_ARRAY(layout.padding_b);
	GROW_ARRAY(layout.gap);
	GROW_ARRAY(layout.abs_pos_x);
	GROW_ARRAY(layout.abs_pos_y);

	GROW_ARRAY(flex.grow);
	GROW_ARRAY(flex.shrink);
	GROW_ARRAY(flex.basis);
	GROW_ARRAY(flex.direction);
	GROW_ARRAY(flex.wrap);
	GROW_ARRAY(flex.justify_content);
	GROW_ARRAY(flex.align_items);
	GROW_ARRAY(flex.align_self);
	GROW_ARRAY(flex.align_content);

	GROW_ARRAY(stack.axis);
	GROW_ARRAY(stack.align);
	GROW_ARRAY(stack.priority);
	GROW_ARRAY(stack.spacer);

	GROW_ARRAY(text.content);
	GROW_ARRAY(text.font_size);
	GROW_ARRAY(text.line_break_policy);
	GROW_ARRAY(text.intrinsic_valid);
	GROW_ARRAY(text.intrinsic_constraint_w);
	GROW_ARRAY(text.intrinsic_font_size);
	GROW_ARRAY(text.intrinsic_line_break_policy);
	GROW_ARRAY(text.intrinsic_width_mode);
	GROW_ARRAY(text.intrinsic_w);
	GROW_ARRAY(text.intrinsic_h);
	GROW_ARRAY(text.intrinsic_lines);

	GROW_ARRAY(semantics.role);
	GROW_ARRAY(semantics.label);
	GROW_ARRAY(semantics.flags);
	GROW_ARRAY(semantics.checked);
	GROW_ARRAY(semantics.enabled);
	GROW_ARRAY(semantics.expanded);
	GROW_ARRAY(semantics.selected);
	GROW_ARRAY(semantics.value_now);
	GROW_ARRAY(semantics.value_min);
	GROW_ARRAY(semantics.value_max);

	GROW_ARRAY(external.userdata);
	GROW_ARRAY(external.payload);
	GROW_ARRAY(external.payload_type);
	GROW_ARRAY(external.payload_destroy);
	GROW_ARRAY(external.payload_destroy_userdata);
	GROW_ARRAY(external.control_type);

	GROW_ARRAY(focus.focusable);
	GROW_ARRAY(focus.tab_index);

	GROW_ARRAY(grid.def);
	GROW_ARRAY(grid.row);
	GROW_ARRAY(grid.column);
	GROW_ARRAY(grid.row_span);
	GROW_ARRAY(grid.column_span);

#undef GROW_ARRAY

	for (uint32_t i = old_cap; i < new_cap; ++i) {
		n->layout.style_w [i]                   = NAN;
		n->layout.style_h [i]                   = NAN;
		n->layout.min_w [i]                     = 0.0f;
		n->layout.min_h [i]                     = 0.0f;
		n->layout.max_w [i]                     = INFINITY;
		n->layout.max_h [i]                     = INFINITY;
		n->flex.basis [i]                       = NAN;
		n->flex.shrink [i]                      = 1.0f;
		n->flex.direction [i]                   = ( uint8_t ) XENT_FLEX_ROW;
		n->flex.wrap [i]                        = ( uint8_t ) XENT_FLEX_NO_WRAP;
		n->flex.justify_content [i]             = ( uint8_t ) XENT_FLEX_JUSTIFY_START;
		n->flex.align_items [i]                 = ( uint8_t ) XENT_FLEX_ALIGN_START;
		n->flex.align_self [i]                  = ( uint8_t ) XENT_FLEX_ALIGN_AUTO;
		n->flex.align_content [i]               = ( uint8_t ) XENT_FLEX_ALIGN_CONTENT_START;
		n->stack.axis [i]                       = ( uint8_t ) XENT_AXIS_HORIZONTAL;
		n->stack.align [i]                      = ( uint8_t ) XENT_STACK_ALIGN_START;
		n->text.font_size [i]                   = 14.0f;
		n->text.line_break_policy [i]           = ( uint8_t ) XENT_LINE_BREAK_CHAR_WRAP;
		n->text.intrinsic_valid [i]             = 0u;
		n->text.intrinsic_constraint_w [i]      = NAN;
		n->text.intrinsic_font_size [i]         = 0.0f;
		n->text.intrinsic_line_break_policy [i] = ( uint8_t ) XENT_LINE_BREAK_CHAR_WRAP;
		n->text.intrinsic_width_mode [i]        = ( uint8_t ) XENT_MEASURE_UNDEFINED;
		n->text.intrinsic_w [i]                 = 0.0f;
		n->text.intrinsic_h [i]                 = 0.0f;
		n->text.intrinsic_lines [i]             = 0u;
		n->layout.protocol [i]                  = ( uint8_t ) XENT_PROTOCOL_ABSOLUTE;
		n->layout.direction [i]                 = ( uint8_t ) XENT_DIRECTION_INHERIT;
		n->semantics.enabled [i]                = 1u;
		n->grid.def [i]                         = NULL;
		n->grid.row [i]                         = 0;
		n->grid.column [i]                      = 0;
		n->grid.row_span [i]                    = 1;
		n->grid.column_span [i]                 = 1;
	}

	n->capacity = new_cap;
	return true;
}
