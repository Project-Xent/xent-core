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
	  "profile swiftstack_total_ms=%.3f swiftstack_collect_ms=%.3f swiftstack_sort_ms=%.3f "
	  "swiftstack_text_ms=%.3f flex_total_ms=%.3f flex_collect_ms=%.3f flex_line_ms=%.3f "
	  "flex_text_ms=%.3f grid_total_ms=%.3f grid_track_ms=%.3f grid_children_ms=%.3f "
	  "grid_text_ms=%.3f temp_allocs=%llu sorts=%llu sibling_scans=%llu text_calls=%llu "
	  "swiftstack_calls=%llu flex_calls=%llu grid_calls=%llu text_baseline_fallbacks=%llu\n",
	  ctx->profile.swiftstack_total_ms,
	  ctx->profile.swiftstack_collect_ms,
	  ctx->profile.swiftstack_sort_ms,
	  ctx->profile.swiftstack_text_ms,
	  ctx->profile.flex_total_ms,
	  ctx->profile.flex_collect_ms,
	  ctx->profile.flex_line_ms,
	  ctx->profile.flex_text_ms,
	  ctx->profile.grid_total_ms,
	  ctx->profile.grid_track_ms,
	  ctx->profile.grid_children_ms,
	  ctx->profile.grid_text_ms,
	  ( unsigned long long ) ctx->profile.temp_allocations,
	  ( unsigned long long ) ctx->profile.sort_calls,
	  ( unsigned long long ) ctx->profile.sibling_scans,
	  ( unsigned long long ) ctx->profile.text_measure_calls,
	  ( unsigned long long ) ctx->profile.swiftstack_layout_calls,
	  ( unsigned long long ) ctx->profile.flex_layout_calls,
	  ( unsigned long long ) ctx->profile.grid_layout_calls,
	  ( unsigned long long ) ctx->profile.text_baseline_fallbacks
	);
}

void xent_scratch_reset(XentContext *ctx) {
	if (!ctx) return;
	for (XentScratchChunk *chunk = ctx->scratch_head; chunk; chunk = chunk->next) chunk->used = 0u;
	ctx->scratch_current = ctx->scratch_head;
}

/* Bump offset within @p chunk that satisfies @p alignment, computed from the
 * chunk's absolute base so any header padding before `data` is accounted for.
 * Returns SIZE_MAX when the aligned start already exceeds the chunk capacity. */
static size_t xent_chunk_aligned_offset(XentScratchChunk const *chunk, size_t alignment) {
	uintptr_t base    = ( uintptr_t ) chunk->data;
	uintptr_t cursor  = base + chunk->used;
	uintptr_t aligned = (cursor + (alignment - 1u)) & ~( uintptr_t ) (alignment - 1u);
	size_t    offset  = ( size_t ) (aligned - base);
	return offset > chunk->cap ? SIZE_MAX : offset;
}

static void *xent_chunk_take(XentScratchChunk *chunk, size_t offset, size_t bytes) {
	chunk->used = offset + bytes;
	return chunk->data + offset;
}

/* Worst-case backing a fresh chunk must hold for one request: the payload plus
 * the maximum alignment padding. Returns 0 on size_t overflow. */
static size_t xent_scratch_request_size(size_t bytes, size_t alignment) {
	if (bytes > SIZE_MAX - (alignment - 1u)) return 0u;
	return bytes + (alignment - 1u);
}

static void xent_scratch_append(XentContext *ctx, XentScratchChunk *chunk) {
	if (!ctx->scratch_head) {
		ctx->scratch_head = chunk;
		return;
	}
	XentScratchChunk *tail = ctx->scratch_head;
	while (tail->next) tail = tail->next;
	tail->next = chunk;
}

void *xent_scratch_alloc(XentContext *ctx, size_t bytes, size_t alignment) {
	if (!ctx || bytes == 0u) return NULL;

	if (alignment == 0u) alignment = sizeof(void *);

	for (XentScratchChunk *chunk = ctx->scratch_current; chunk; chunk = chunk->next) {
		size_t offset = xent_chunk_aligned_offset(chunk, alignment);
		if (offset == SIZE_MAX || bytes > chunk->cap - offset) continue;
		ctx->scratch_current           = chunk;
		ctx->profile.temp_allocations += 1u;
		return xent_chunk_take(chunk, offset, bytes);
	}

	size_t need = xent_scratch_request_size(bytes, alignment);
	if (need == 0u) return NULL;
	size_t default_cap = ctx->scratch_chunk_size ? ctx->scratch_chunk_size : XENT_SCRATCH_CHUNK_SIZE;
	size_t cap         = default_cap > need ? default_cap : need;

	XentScratchChunk *chunk = ( XentScratchChunk * ) malloc(sizeof(*chunk) + cap);
	if (!chunk) return NULL;
	chunk->next = NULL;
	chunk->cap  = cap;
	chunk->used = 0u;

	xent_scratch_append(ctx, chunk);
	ctx->scratch_current           = chunk;
	ctx->profile.temp_allocations += 1u;

	size_t offset = xent_chunk_aligned_offset(chunk, alignment);
	return xent_chunk_take(chunk, offset, bytes);
}

void xent_free_scratch(XentContext *ctx) {
	if (!ctx) return;
	XentScratchChunk *chunk = ctx->scratch_head;
	while (chunk) {
		XentScratchChunk *next = chunk->next;
		free(chunk);
		chunk = next;
	}
	ctx->scratch_head    = NULL;
	ctx->scratch_current = NULL;
}
