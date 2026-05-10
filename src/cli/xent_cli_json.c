#include "../xent_internal.h"

static char const *json_short_escape(unsigned char c) {
	switch (c) {
	case '\n' : return "\\n";
	case '\r' : return "\\r";
	case '\t' : return "\\t";
	case '\b' : return "\\b";
	case '\f' : return "\\f";
	default   : return NULL;
	}
}

static void json_escape_char(FILE *out, unsigned char c) {
	char const *short_escape = json_short_escape(c);
	if (short_escape) {
		fputs(short_escape, out);
		return;
	}
	if (c < 0x20u) {
		fprintf(out, "\\u%04x", ( unsigned ) c);
		return;
	}
	if (c == '"' || c == '\\') fputc('\\', out);
	fputc(c, out);
}

static void json_escape(FILE *out, char const *text) {
	if (!text) return;
	while (*text) json_escape_char(out, ( unsigned char ) *text++);
}

static bool dump_json_node_open(XentContext const *ctx, XentNodeId node, FILE *out) {
	if (!xent_is_valid_node(ctx, node)) return false;

	fprintf(
	  out, "{\"id\":%u,\"protocol\":%u,\"rect\":{\"x\":%.3f,\"y\":%.3f,\"w\":%.3f,\"h\":%.3f}", node,
	  ( unsigned ) ctx->nodes.layout.protocol [node], ctx->nodes.layout.abs_x [node], ctx->nodes.layout.abs_y [node],
	  ctx->nodes.layout.decided_w [node], ctx->nodes.layout.decided_h [node]
	);

	char const *label = xent_get_semantic_label(ctx, node);
	char const *text  = xent_get_text(ctx, node);

	if (label) {
		fputs(",\"label\":\"", out);
		json_escape(out, label);
		fputc('"', out);
	}
	if (text) {
		fputs(",\"text\":\"", out);
		json_escape(out, text);
		fputc('"', out);
	}

	fputs(",\"children\":[", out);
	return true;
}

typedef struct JsonDumpFrame {
	XentNodeId node;
	XentNodeId next_child;
	bool       first_child;
} JsonDumpFrame;

static bool json_stack_push(JsonDumpFrame **stack, uint32_t *top, uint32_t *capacity, JsonDumpFrame frame) {
	if (*top == *capacity) {
		uint32_t       new_cap = *capacity ? *capacity * 2u : 64u;
		JsonDumpFrame *new_mem = ( JsonDumpFrame * ) realloc(*stack, sizeof(*new_mem) * ( size_t ) new_cap);
		if (!new_mem) return false;
		*stack    = new_mem;
		*capacity = new_cap;
	}
	(*stack) [(*top)++] = frame;
	return true;
}

bool xent_dump_layout_json(XentContext const *ctx, XentNodeId root, FILE *out) {
	if (!xent_is_valid_node(ctx, root) || !out) return false;

	JsonDumpFrame *stack    = NULL;
	uint32_t       top      = 0u;
	uint32_t       capacity = 0u;
	bool           ok
	  = dump_json_node_open(ctx, root, out)
	 && json_stack_push(&stack, &top, &capacity, (JsonDumpFrame) {root, ctx->nodes.topology.first_child [root], true});

	while (ok && top > 0u) {
		JsonDumpFrame *frame = &stack [top - 1u];
		if (frame->next_child == XENT_NODE_INVALID) {
			fputs("]}", out);
			top -= 1u;
			continue;
		}

		XentNodeId child  = frame->next_child;
		frame->next_child = ctx->nodes.topology.next_sibling [child];
		if (!frame->first_child) fputc(',', out);
		frame->first_child = false;

		ok                 = dump_json_node_open(ctx, child, out)
		                  && json_stack_push(
			&stack, &top, &capacity, (JsonDumpFrame) {child, ctx->nodes.topology.first_child [child], true}
		  );
	}

	free(stack);
	return ok;
}
