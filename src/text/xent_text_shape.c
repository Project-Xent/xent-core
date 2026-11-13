#include "../xent_internal.h"

struct XentShapeSink {
	XentGlyph    *glyphs;
	XentGlyphRun *runs;
	uint32_t      glyph_count;
	uint32_t      glyph_capacity;
	uint32_t      run_count;
	uint32_t      run_capacity;
	bool          failed;
};

static uint32_t grown_capacity(uint32_t capacity, uint32_t needed) {
	uint32_t next = capacity ? capacity * 2u : 32u;
	return next < needed ? needed : next;
}

static bool sink_reserve_glyphs(XentShapeSink *sink, uint32_t count) {
	uint32_t needed = sink->glyph_count + count;
	if (needed < sink->glyph_count) return false;
	if (needed <= sink->glyph_capacity) return true;

	uint32_t   capacity = grown_capacity(sink->glyph_capacity, needed);
	XentGlyph *grown
	  = ( XentGlyph * ) xent_realloc_internal(XENT_ALLOC_TEXT, sink->glyphs, sizeof(*grown) * ( size_t ) capacity);
	if (!grown) return false;

	sink->glyphs         = grown;
	sink->glyph_capacity = capacity;
	return true;
}

static bool sink_reserve_run(XentShapeSink *sink) {
	if (sink->run_count < sink->run_capacity) return true;

	uint32_t      capacity = grown_capacity(sink->run_capacity, sink->run_count + 1u);
	XentGlyphRun *grown
	  = ( XentGlyphRun * ) xent_realloc_internal(XENT_ALLOC_TEXT, sink->runs, sizeof(*grown) * ( size_t ) capacity);
	if (!grown) return false;

	sink->runs         = grown;
	sink->run_capacity = capacity;
	return true;
}

bool xent_shape_emit(XentShapeSink *sink, XentGlyphRunDesc const *desc, XentGlyph const *glyphs, uint32_t count) {
	if (!sink || !desc || (count && !glyphs)) return false;
	if (sink->failed) return false;
	if (!sink_reserve_run(sink) || !sink_reserve_glyphs(sink, count)) {
		sink->failed = true;
		return false;
	}

	sink->runs [sink->run_count]  = (XentGlyphRun) {*desc, sink->glyph_count, count};
	sink->run_count              += 1u;
	if (count) memcpy(sink->glyphs + sink->glyph_count, glyphs, sizeof(*glyphs) * ( size_t ) count);
	sink->glyph_count += count;
	return true;
}

static void sink_release(XentShapeSink *sink) {
	free(sink->glyphs);
	free(sink->runs);
}

static XentTextShape *shape_from_sink(XentShapeSink *sink, XentTextMetrics const *metrics) {
	XentTextShape *shape = ( XentTextShape * ) xent_alloc_internal(XENT_ALLOC_TEXT, sizeof(*shape));
	if (!shape) return NULL;

	*shape = (XentTextShape) {sink->glyphs, sink->runs, sink->glyph_count, sink->run_count, *metrics};
	return shape;
}

XentTextShape *xent_text_shape(XentCtx *ctx, XentTextMeasureReq const *request) {
	if (!xent_text_request_valid(ctx, request) || !ctx->text_backend->shape) return NULL;

	XentShapeSink   sink    = {0};
	XentTextMetrics metrics = {0};
	if (!ctx->text_backend->shape(ctx->text_backend, request, &sink, &metrics) || sink.failed) {
		sink_release(&sink);
		return NULL;
	}

	XentTextShape *shape = shape_from_sink(&sink, &metrics);
	if (!shape) sink_release(&sink);
	return shape;
}

void xent_text_freeshape(XentTextShape *shape) {
	if (!shape) return;
	free(( XentGlyph * ) shape->glyphs);
	free(( XentGlyphRun * ) shape->runs);
	free(shape);
}
