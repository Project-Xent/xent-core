#include <time.h>

#include "../xent_internal.h"

double xent_now_ms(void) {
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	return ( double ) ts.tv_sec * 1000.0 + ( double ) ts.tv_nsec / 1000000.0;
}

void xent_profile_reset(XentContext *ctx) {
	if (!ctx) return;
	memset(&ctx->profile, 0, sizeof(ctx->profile));
}

XentProfileStats xent_profile_get(XentContext const *ctx) {
	XentProfileStats zero = {0};
	if (!ctx) return zero;
	return ctx->profile;
}

void xent_profile_dump(XentContext const *ctx, FILE *out) {
	if (!ctx) return;
	FILE *stream = out ? out : stdout;
	fprintf(stream,
            "swiftstack_profile total_ms=%.3f collect_ms=%.3f sort_ms=%.3f text_ms=%.3f "
            "temp_allocs=%llu sorts=%llu sibling_scans=%llu text_calls=%llu\n",
            ctx->profile.swiftstack_total_ms,
            ctx->profile.swiftstack_collect_ms,
            ctx->profile.swiftstack_sort_ms,
            ctx->profile.swiftstack_text_ms,
            (unsigned long long)ctx->profile.temp_allocations,
            (unsigned long long)ctx->profile.sort_calls,
            (unsigned long long)ctx->profile.sibling_scans,
            (unsigned long long)ctx->profile.text_measure_calls);
}

void xent_scratch_reset(XentContext *ctx) {
	if (!ctx) return;
	ctx->scratch_size = 0u;
}

void *xent_scratch_alloc(XentContext *ctx, size_t bytes, size_t alignment) {
	if (!ctx || bytes == 0u) return NULL;

	if (alignment == 0u) alignment = sizeof(void *);

	size_t aligned_offset = ctx->scratch_size;
	size_t mask           = alignment - 1u;
	aligned_offset        = (aligned_offset + mask) & ~mask;
	size_t needed         = aligned_offset + bytes;

	if (needed > ctx->scratch_capacity) {
		size_t new_cap = ctx->scratch_capacity ? ctx->scratch_capacity * 2u : 4096u;
		while (new_cap < needed) new_cap *= 2u;

		uint8_t *new_mem = ( uint8_t * ) realloc(ctx->scratch, new_cap);
		if (!new_mem) return NULL;
		ctx->scratch          = new_mem;
		ctx->scratch_capacity = new_cap;
	}

	void *result                   = ctx->scratch + aligned_offset;
	ctx->scratch_size              = needed;
	ctx->profile.temp_allocations += 1u;
	return result;
}
