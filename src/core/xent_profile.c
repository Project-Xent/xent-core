#include <time.h>

#include "../xent_internal.h"

double xent_now_ms(void) {
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	return ( double ) ts.tv_sec * 1000.0 + ( double ) ts.tv_nsec / 1000000.0;
}

void xent_profile_reset(XentCtx *ctx) {
	if (!ctx) return;
	memset(&ctx->profile, 0, sizeof(ctx->profile));
}

XentProfStats xent_profile_get(XentCtx const *ctx) {
	XentProfStats zero = {0};
	if (!ctx) return zero;
	return ctx->profile;
}

void xent_profile_dump(XentCtx const *ctx, FILE *out) {
	if (!ctx) return;
	FILE *stream = out ? out : stdout;
	fprintf(
	  stream,
	  "profile swiftstack_total_ms=%.3f swiftstack_collect_ms=%.3f swiftstack_sort_ms=%.3f "
	  "swiftstack_text_ms=%.3f flex_total_ms=%.3f flex_collect_ms=%.3f flex_line_ms=%.3f "
	  "flex_text_ms=%.3f grid_total_ms=%.3f grid_track_ms=%.3f grid_children_ms=%.3f "
	  "grid_text_ms=%.3f temp_allocs=%llu sorts=%llu sibling_scans=%llu text_calls=%llu "
	  "swiftstack_calls=%llu flex_calls=%llu grid_calls=%llu text_baseline_fallbacks=%llu\n",
	  ctx->profile.swiftstack_total_ms, ctx->profile.swiftstack_collect_ms, ctx->profile.swiftstack_sort_ms,
	  ctx->profile.swiftstack_text_ms, ctx->profile.flex_total_ms, ctx->profile.flex_collect_ms,
	  ctx->profile.flex_line_ms, ctx->profile.flex_text_ms, ctx->profile.grid_total_ms, ctx->profile.grid_track_ms,
	  ctx->profile.grid_children_ms, ctx->profile.grid_text_ms, ( unsigned long long ) ctx->profile.temp_allocations,
	  ( unsigned long long ) ctx->profile.sort_calls, ( unsigned long long ) ctx->profile.sibling_scans,
	  ( unsigned long long ) ctx->profile.text_measure_calls,
	  ( unsigned long long ) ctx->profile.swiftstack_layout_calls,
	  ( unsigned long long ) ctx->profile.flex_layout_calls, ( unsigned long long ) ctx->profile.grid_layout_calls,
	  ( unsigned long long ) ctx->profile.text_baseline_fallbacks
	);
}
