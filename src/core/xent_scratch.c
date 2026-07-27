#include "xent_scratch.h"

#include "../xent_alloc_internal.h"
#include "../xent_internal.h"

/** One node of the non-moving scratch arena. `data` is a flexible array of
 * `cap` bytes; `used` is the bump cursor. Chunks are linked in allocation order,
 * never reallocated, and retained across resets so live interior pointers handed
 * to callers stay valid for the lifetime of the context. */
struct XentScratchChunk {
	struct XentScratchChunk *next;
	size_t                   cap;
	size_t                   used;
	uint8_t                  data [];
};

void xent_scratch_reset(XentCtx *ctx) {
	if (!ctx) return;
	for (XentScratchChunk *chunk = ctx->scratch_head; chunk; chunk = chunk->next) chunk->used = 0u;
	ctx->scratch_current = ctx->scratch_head;
}

/* Bump offset within @p chunk that satisfies @p alignment, computed from the
 * chunk's absolute base so any header padding before `data` is accounted for.
 * Returns SIZE_MAX when the aligned start already exceeds the chunk capacity. */
static size_t chunk_aligned_offset(XentScratchChunk const *chunk, size_t alignment) {
	uintptr_t base    = ( uintptr_t ) chunk->data;
	uintptr_t cursor  = base + chunk->used;
	uintptr_t aligned = (cursor + (alignment - 1u)) & ~( uintptr_t ) (alignment - 1u);
	size_t    offset  = ( size_t ) (aligned - base);
	return offset > chunk->cap ? SIZE_MAX : offset;
}

static void *chunk_take(XentScratchChunk *chunk, size_t offset, size_t bytes) {
	chunk->used = offset + bytes;
	return chunk->data + offset;
}

/* Worst-case backing a fresh chunk must hold for one request: the payload plus
 * the maximum alignment padding. Returns 0 on size_t overflow. */
static size_t scratch_request_size(size_t bytes, size_t alignment) {
	if (bytes > SIZE_MAX - (alignment - 1u)) return 0u;
	return bytes + (alignment - 1u);
}

static void scratch_append(XentCtx *ctx, XentScratchChunk *chunk) {
	if (!ctx->scratch_head) {
		ctx->scratch_head = chunk;
		return;
	}
	XentScratchChunk *tail = ctx->scratch_head;
	while (tail->next) tail = tail->next;
	tail->next = chunk;
}

static void *take_existing_scratch(XentCtx *ctx, size_t bytes, size_t alignment) {
	for (XentScratchChunk *chunk = ctx->scratch_current; chunk; chunk = chunk->next) {
		size_t offset = chunk_aligned_offset(chunk, alignment);
		if (offset == SIZE_MAX || bytes > chunk->cap - offset) continue;
		ctx->scratch_current           = chunk;
		ctx->profile.temp_allocations += 1u;
		return chunk_take(chunk, offset, bytes);
	}
	return NULL;
}

void *xent_scratch_alloc(XentCtx *ctx, size_t bytes, size_t alignment) {
	if (!ctx || bytes == 0u) return NULL;

	if (alignment == 0u) alignment = sizeof(void *);

	void *existing = take_existing_scratch(ctx, bytes, alignment);
	if (existing) return existing;

	size_t need = scratch_request_size(bytes, alignment);
	if (need == 0u) return NULL;
	size_t default_cap = ctx->scratch_chunk_size ? ctx->scratch_chunk_size : XENT_SCRATCH_CHUNK_SIZE;
	size_t cap         = default_cap > need ? default_cap : need;
	if (cap > SIZE_MAX - sizeof(XentScratchChunk)) return NULL;

	XentScratchChunk *chunk
	  = ( XentScratchChunk * ) xent_alloc_internal(XENT_ALLOC_SCRATCH_ARENA, sizeof(*chunk) + cap);
	if (!chunk) return NULL;
	chunk->next = NULL;
	chunk->cap  = cap;
	chunk->used = 0u;

	scratch_append(ctx, chunk);
	ctx->scratch_current           = chunk;
	ctx->profile.temp_allocations += 1u;

	size_t offset                  = chunk_aligned_offset(chunk, alignment);
	return chunk_take(chunk, offset, bytes);
}

void xent_free_scratch(XentCtx *ctx) {
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
