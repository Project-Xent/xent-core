#include "../xent_internal.h"

typedef struct XentDecodedGlyph {
	uint32_t codepoint;
	uint32_t cluster;
	uint8_t  break_after;
} XentDecodedGlyph;

typedef struct XentMonoLines {
	uint32_t *starts;
	uint32_t *counts;
	uint32_t  count;
} XentMonoLines;

typedef struct XentMonoShape {
	XentTextBackend const      *backend;
	XentTextShapeRequest const *request;
	XentTextShapeOutput const  *output;
	XentMonoBackendState const *state;
	XentDecodedGlyph           *glyphs;
	uint32_t                    glyph_count;
	XentMonoLines               lines;
	uint32_t                    run_count;
	float                       glyph_width;
	float                       line_height;
	float                       max_line_width;
} XentMonoShape;

static bool     xent_utf8_is_continuation(uint8_t byte) { return (byte & 0xc0u) == 0x80u; }

static uint32_t xent_utf8_sequence_length(uint8_t first) {
	if ((first & 0x80u) == 0u) return 1u;
	if ((first & 0xe0u) == 0xc0u) return 2u;
	if ((first & 0xf0u) == 0xe0u) return 3u;
	if ((first & 0xf8u) == 0xf0u) return 4u;
	return 0u;
}

static bool xent_utf8_has_sequence(uint8_t const *bytes, size_t text_len, size_t cursor, uint32_t length) {
	if (length == 0u || cursor + ( size_t ) length > text_len) return false;
	for (uint32_t i = 1u; i < length; ++i)
		if (!xent_utf8_is_continuation(bytes [cursor + ( size_t ) i])) return false;
	return true;
}

static uint32_t xent_utf8_decode_sequence(uint8_t const *bytes, size_t cursor, uint32_t length) {
	if (length == 1u) return bytes [cursor];
	if (length == 2u) return (( uint32_t ) (bytes [cursor] & 0x1fu) << 6u) | ( uint32_t ) (bytes [cursor + 1u] & 0x3fu);
	if (length == 3u) {
		return (( uint32_t ) (bytes [cursor] & 0x0fu) << 12u)
		     | (( uint32_t ) (bytes [cursor + 1u] & 0x3fu) << 6u)
		     | ( uint32_t ) (bytes [cursor + 2u] & 0x3fu);
	}
	return (( uint32_t ) (bytes [cursor] & 0x07u) << 18u)
	     | (( uint32_t ) (bytes [cursor + 1u] & 0x3fu) << 12u)
	     | (( uint32_t ) (bytes [cursor + 2u] & 0x3fu) << 6u)
	     | ( uint32_t ) (bytes [cursor + 3u] & 0x3fu);
}

static bool xent_utf8_next(char const *text, size_t text_len, size_t *cursor, uint32_t *out_codepoint) {
	if (!text || !cursor || !out_codepoint || *cursor >= text_len) return false;

	uint8_t const *bytes  = ( uint8_t const * ) text;
	uint32_t       length = xent_utf8_sequence_length(bytes [*cursor]);
	if (xent_utf8_has_sequence(bytes, text_len, *cursor, length)) {
		*out_codepoint  = xent_utf8_decode_sequence(bytes, *cursor, length);
		*cursor        += length;
		return true;
	}

	*out_codepoint  = 0xfffdu;
	*cursor        += 1u;
	return true;
}

static bool xent_is_break_opportunity(uint32_t codepoint) {
	static uint32_t const breakpoints [] = {
	  ( uint32_t ) ' ',
	  ( uint32_t ) '\t',
	  ( uint32_t ) '-',
	  ( uint32_t ) ',',
	  ( uint32_t ) '.',
	  ( uint32_t ) ';',
	  ( uint32_t ) ':',
	  ( uint32_t ) '/',
	  0x00b7u,
	  0x200bu,
	  0x3001u,
	  0x3002u,
	  0xff0cu,
	  0xff0eu,
	};
	for (uint32_t i = 0u; i < sizeof(breakpoints) / sizeof(breakpoints [0]); ++i)
		if (breakpoints [i] == codepoint) return true;
	return false;
}

static bool xent_glyph_buffer_grow(XentDecodedGlyph **glyphs, uint32_t *capacity) {
	uint32_t          next_capacity = *capacity * 2u;
	XentDecodedGlyph *next
	  = ( XentDecodedGlyph * ) realloc(*glyphs, sizeof(XentDecodedGlyph) * ( size_t ) next_capacity);
	if (!next) return false;
	*glyphs   = next;
	*capacity = next_capacity;
	return true;
}

static void xent_write_decoded_glyph(XentDecodedGlyph *glyph, uint32_t codepoint, size_t cluster) {
	glyph->codepoint   = codepoint;
	glyph->cluster     = ( uint32_t ) cluster;
	glyph->break_after = xent_is_break_opportunity(codepoint) ? 1u : 0u;
}

static bool xent_decode_glyphs(char const *text, XentDecodedGlyph **out_glyphs, uint32_t *out_count) {
	*out_glyphs           = NULL;
	*out_count            = 0u;

	size_t const text_len = strlen(text);
	if (text_len == 0u) return true;

	uint32_t          capacity = 64u;
	XentDecodedGlyph *glyphs   = ( XentDecodedGlyph * ) malloc(sizeof(XentDecodedGlyph) * ( size_t ) capacity);
	if (!glyphs) return false;

	size_t   cursor = 0u;
	uint32_t count  = 0u;
	while (cursor < text_len) {
		uint32_t codepoint = 0u;
		size_t   cluster   = cursor;
		if (!xent_utf8_next(text, text_len, &cursor, &codepoint)) break;
		if (count == capacity && !xent_glyph_buffer_grow(&glyphs, &capacity)) {
			free(glyphs);
			return false;
		}
		xent_write_decoded_glyph(&glyphs [count++], codepoint, cluster);
	}

	*out_glyphs = glyphs;
	*out_count  = count;
	return true;
}

static bool xent_line_break_policy_valid(XentLineBreakPolicy policy) {
	return policy == XENT_LINE_BREAK_NO_WRAP
	    || policy == XENT_LINE_BREAK_WORD_WRAP
	    || policy == XENT_LINE_BREAK_CHAR_WRAP;
}

static bool xent_width_mode_valid(XentMeasureMode mode) {
	return mode == XENT_MEASURE_UNDEFINED || mode == XENT_MEASURE_AT_MOST || mode == XENT_MEASURE_EXACTLY;
}

static bool xent_shape_output_valid(XentTextShapeOutput const *output) {
	if (!output || !output->result) return false;
	if (output->glyphs == NULL && output->glyph_capacity != 0u) return false;
	if (output->runs == NULL && output->run_capacity != 0u) return false;
	if (output->lines == NULL && output->line_capacity != 0u) return false;
	return true;
}

static bool xent_shape_request_valid(XentTextShapeRequest const *request) {
	if (!request || !request->text) return false;
	if (!xent_line_break_policy_valid(request->line_break_policy)) return false;
	return xent_width_mode_valid(request->width_mode);
}

static bool xent_shape_init(
  XentMonoShape *shape, XentTextBackend const *backend, XentTextShapeRequest const *request,
  XentTextShapeOutput const *output
) {
	if (!backend || !xent_shape_request_valid(request) || !xent_shape_output_valid(output)) return false;

	*shape         = (XentMonoShape) {0};
	shape->backend = backend;
	shape->request = request;
	shape->output  = output;
	shape->state   = ( XentMonoBackendState const * ) backend->userdata;
	if (!shape->state) return false;

	shape->glyph_width = shape->state->glyph_width;
	shape->line_height = shape->state->line_height;
	return xent_decode_glyphs(request->text, &shape->glyphs, &shape->glyph_count);
}

static void xent_shape_destroy(XentMonoShape *shape) {
	free(shape->glyphs);
	free(shape->lines.starts);
	free(shape->lines.counts);
}

static uint32_t xent_resolve_max_glyphs_per_line(
  float glyph_w, float width_constraint, XentLineBreakPolicy line_break_policy, XentMeasureMode width_mode
) {
	if (width_mode == XENT_MEASURE_UNDEFINED
		|| line_break_policy == XENT_LINE_BREAK_NO_WRAP
		|| !isfinite(width_constraint)
		|| width_constraint <= 0.0f)
	{
		return UINT32_MAX;
	}
	uint32_t fit = ( uint32_t ) (width_constraint / glyph_w);
	return fit == 0u ? 1u : fit;
}

static bool xent_alloc_line_buffers(XentMonoShape *shape) {
	if (shape->glyph_count == 0u) return true;
	shape->lines.starts = ( uint32_t * ) malloc(sizeof(uint32_t) * ( size_t ) shape->glyph_count);
	shape->lines.counts = ( uint32_t * ) malloc(sizeof(uint32_t) * ( size_t ) shape->glyph_count);
	return shape->lines.starts && shape->lines.counts;
}

static void xent_add_line(XentMonoLines *lines, uint32_t start, uint32_t count) {
	lines->starts [lines->count]  = start;
	lines->counts [lines->count]  = count;
	lines->count                 += 1u;
}

static void xent_build_lines_no_wrap(XentMonoShape *shape) { xent_add_line(&shape->lines, 0u, shape->glyph_count); }

static void xent_build_lines_char_wrap(XentMonoShape *shape, uint32_t max_glyphs_per_line) {
	for (uint32_t cursor = 0u; cursor < shape->glyph_count;) {
		uint32_t count = shape->glyph_count - cursor;
		if (count > max_glyphs_per_line) count = max_glyphs_per_line;
		xent_add_line(&shape->lines, cursor, count);
		cursor += count;
	}
}

static uint32_t xent_scan_word_line_end(XentMonoShape const *shape, uint32_t line_start, uint32_t max_glyphs_per_line) {
	uint32_t scan       = line_start;
	uint32_t last_break = UINT32_MAX;

	while (scan < shape->glyph_count && (scan - line_start) < max_glyphs_per_line) {
		if (shape->glyphs [scan].break_after != 0u) last_break = scan;
		scan += 1u;
	}

	if (scan < shape->glyph_count && last_break != UINT32_MAX && last_break >= line_start) return last_break + 1u;
	return scan > line_start ? scan : line_start + 1u;
}

static void xent_build_lines_word_wrap(XentMonoShape *shape, uint32_t max_glyphs_per_line) {
	for (uint32_t line_start = 0u; line_start < shape->glyph_count;) {
		uint32_t line_end = xent_scan_word_line_end(shape, line_start, max_glyphs_per_line);
		xent_add_line(&shape->lines, line_start, line_end - line_start);
		line_start = line_end;
	}
}

static bool xent_build_line_plan(XentMonoShape *shape) {
	shape->lines.count = 1u;
	if (shape->glyph_count == 0u) return true;
	if (!xent_alloc_line_buffers(shape)) return false;

	shape->lines.count           = 0u;
	uint32_t max_glyphs_per_line = xent_resolve_max_glyphs_per_line(
	  shape->glyph_width, shape->request->width_constraint, shape->request->line_break_policy,
	  shape->request->width_mode
	);

	if (max_glyphs_per_line == UINT32_MAX) xent_build_lines_no_wrap(shape);
	else if (shape->request->line_break_policy == XENT_LINE_BREAK_WORD_WRAP)
		xent_build_lines_word_wrap(shape, max_glyphs_per_line);
	else xent_build_lines_char_wrap(shape, max_glyphs_per_line);
	return true;
}

static void xent_compute_line_metrics(XentMonoShape *shape) {
	shape->max_line_width = 0.0f;
	for (uint32_t i = 0u; i < shape->lines.count; ++i) {
		float line_width = shape->glyph_width * ( float ) shape->lines.counts [i];
		if (line_width > shape->max_line_width) shape->max_line_width = line_width;
	}
}

static float xent_resolve_result_width(XentMonoShape const *shape) {
	if (shape->request->width_mode == XENT_MEASURE_EXACTLY
		&& isfinite(shape->request->width_constraint)
		&& shape->request->width_constraint >= 0.0f)
	{
		return shape->request->width_constraint;
	}
	return shape->max_line_width;
}

static void xent_write_result_summary(XentMonoShape *shape) {
	XentShapingResult *result  = shape->output->result;
	shape->run_count           = shape->glyph_count > 0u ? 1u : 0u;

	result->glyph_count        = shape->glyph_count;
	result->run_count          = shape->run_count;
	result->line_count         = shape->lines.count;
	result->truncated          = false;
	result->metrics.width      = xent_resolve_result_width(shape);
	result->metrics.height     = shape->line_height * ( float ) shape->lines.count;
	result->metrics.line_count = shape->lines.count;
}

static bool xent_output_has_capacity(XentMonoShape *shape) {
	XentTextShapeOutput const *output = shape->output;
	if ((output->glyphs && output->glyph_capacity < shape->glyph_count)
		|| (output->runs && output->run_capacity < shape->run_count)
		|| (output->lines && output->line_capacity < shape->lines.count))
	{
		output->result->truncated = true;
		return false;
	}
	return true;
}

static void xent_write_empty_line(XentShapedLine *line) {
	line->glyph_start = 0u;
	line->glyph_count = 0u;
	line->width       = 0.0f;
}

static void xent_write_lines(XentMonoShape const *shape) {
	if (!shape->output->lines) return;
	if (shape->glyph_count == 0u) {
		xent_write_empty_line(&shape->output->lines [0]);
		return;
	}
	for (uint32_t i = 0u; i < shape->lines.count; ++i) {
		shape->output->lines [i].glyph_start = shape->lines.starts [i];
		shape->output->lines [i].glyph_count = shape->lines.counts [i];
		shape->output->lines [i].width       = shape->glyph_width * ( float ) shape->lines.counts [i];
	}
}

static uint32_t xent_find_line_index(XentMonoShape const *shape, uint32_t glyph_index, uint32_t line_index) {
	while (line_index + 1u < shape->lines.count) {
		uint32_t line_end = shape->lines.starts [line_index] + shape->lines.counts [line_index];
		if (glyph_index < line_end) break;
		line_index += 1u;
	}
	return line_index;
}

static void xent_write_glyph(XentMonoShape const *shape, uint32_t glyph_index, uint32_t line_index) {
	uint32_t         col = glyph_index - shape->lines.starts [line_index];
	XentShapedGlyph *out = &shape->output->glyphs [glyph_index];
	out->codepoint       = shape->glyphs [glyph_index].codepoint;
	out->cluster         = shape->glyphs [glyph_index].cluster;
	out->line_index      = line_index;
	out->advance         = shape->glyph_width;
	out->offset_x        = shape->glyph_width * ( float ) col;
	out->offset_y        = shape->line_height * ( float ) line_index;
}

static void xent_write_glyphs(XentMonoShape const *shape) {
	if (!shape->output->glyphs || shape->glyph_count == 0u) return;
	uint32_t line_index = 0u;
	for (uint32_t i = 0u; i < shape->glyph_count; ++i) {
		line_index = xent_find_line_index(shape, i, line_index);
		xent_write_glyph(shape, i, line_index);
	}
}

static void xent_write_runs(XentMonoShape const *shape) {
	if (!shape->output->runs || shape->run_count != 1u) return;
	shape->output->runs [0].glyph_start = 0u;
	shape->output->runs [0].glyph_count = shape->glyph_count;
	shape->output->runs [0].line_start  = 0u;
	shape->output->runs [0].line_count  = shape->lines.count;
}

static void xent_write_shape_output(XentMonoShape const *shape) {
	xent_write_lines(shape);
	xent_write_glyphs(shape);
	xent_write_runs(shape);
}

static bool xent_shape_mono(
  XentTextBackend const *backend, XentTextShapeRequest const *request, XentTextShapeOutput const *output
) {
	XentMonoShape shape = {0};
	if (!xent_shape_init(&shape, backend, request, output)) return false;
	if (!xent_build_line_plan(&shape)) {
		xent_shape_destroy(&shape);
		return false;
	}

	xent_compute_line_metrics(&shape);
	xent_write_result_summary(&shape);
	if (!xent_output_has_capacity(&shape)) {
		xent_shape_destroy(&shape);
		return false;
	}

	xent_write_shape_output(&shape);
	xent_shape_destroy(&shape);
	return true;
}

static bool
xent_measure_mono(XentTextBackend const *backend, XentTextMeasureRequest const *request, XentTextMetrics *out_metrics) {
	XentShapingResult    shaped        = {0};
	XentTextShapeRequest shape_request = {
	  request ? request->text : NULL,
	  request ? request->font_size : 0.0f,
	  request ? request->width_constraint : 0.0f,
	  request ? request->line_break_policy : XENT_LINE_BREAK_NO_WRAP,
	  request ? request->width_mode : XENT_MEASURE_UNDEFINED,
	};
	XentTextShapeOutput output = {0};
	output.result              = &shaped;
	if (!xent_shape_mono(backend, &shape_request, &output)) return false;
	*out_metrics = shaped.metrics;
	return true;
}

bool xent_text_backend_mono_init(XentContext *ctx) {
	if (!ctx) return false;

	ctx->mono_state.glyph_width = ctx->config.mono_glyph_width;
	ctx->mono_state.line_height = ctx->config.mono_line_height;

	ctx->mono_backend.name      = "mono_fallback";
	ctx->mono_backend.measure   = xent_measure_mono;
	ctx->mono_backend.shape     = xent_shape_mono;
	ctx->mono_backend.userdata  = &ctx->mono_state;
	ctx->text_backend           = &ctx->mono_backend;
	return true;
}
