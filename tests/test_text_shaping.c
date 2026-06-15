#include "test_common.h"

int main(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	char const          *mixed          = "ab\xE4\xBD\xA0" "d";

	XentShapingResult    summary        = {0};
	XentTextShapeRequest request        = {mixed, 14.0f, 0u, 16.0f, XENT_LINE_BREAK_CHAR_WRAP, XENT_MEASURE_AT_MOST};
	XentTextShapeOutput  summary_output = {0};
	summary_output.result               = &summary;
	TEST_ASSERT(xent_shape_text(ctx, &request, &summary_output));
	TEST_ASSERT(summary.glyph_count == 4u);
	TEST_ASSERT(summary.run_count == 1u);
	TEST_ASSERT(summary.line_count == 2u);
	TEST_ASSERT(summary.metrics.line_count == 2u);

	XentTextCacheStats shape_stats = xent_get_shape_cache_stats(ctx);
	TEST_ASSERT(shape_stats.misses >= 1u);
	TEST_ASSERT(shape_stats.inserts >= 1u);

	XentShapingResult   summary_cached = {0};
	XentTextShapeOutput cached_output  = {0};
	cached_output.result               = &summary_cached;
	TEST_ASSERT(xent_shape_text(ctx, &request, &cached_output));
	TEST_ASSERT(summary_cached.glyph_count == summary.glyph_count);
	TEST_ASSERT(summary_cached.line_count == summary.line_count);

	shape_stats = xent_get_shape_cache_stats(ctx);
	TEST_ASSERT(shape_stats.hits >= 1u);

	XentShapedGlyph     glyphs [8]      = {0};
	XentShapedRun       runs [2]        = {0};
	XentShapedLine      lines [4]       = {0};
	XentShapingResult   detailed        = {0};
	XentTextShapeOutput detailed_output = {glyphs, 8u, runs, 2u, lines, 4u, &detailed};
	TEST_ASSERT(xent_shape_text(ctx, &request, &detailed_output));
	TEST_ASSERT(detailed.glyph_count == 4u);
	TEST_ASSERT(detailed.run_count == 1u);
	TEST_ASSERT(detailed.line_count == 2u);
	TEST_ASSERT(glyphs [0].line_index == 0u);
	TEST_ASSERT(glyphs [1].line_index == 0u);
	TEST_ASSERT(glyphs [2].line_index == 1u);
	TEST_ASSERT(glyphs [3].line_index == 1u);
	TEST_ASSERT(lines [0].glyph_count == 2u);
	TEST_ASSERT(lines [1].glyph_count == 2u);
	TEST_ASSERT(runs [0].glyph_count == 4u);

	XentShapingResult   truncated        = {0};
	XentTextShapeOutput truncated_output = {glyphs, 2u, runs, 1u, lines, 2u, &truncated};
	TEST_ASSERT(!xent_shape_text(ctx, &request, &truncated_output));
	TEST_ASSERT(truncated.truncated);

	xent_destroy_context(ctx);
	return 0;
}
