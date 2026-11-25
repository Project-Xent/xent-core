#ifndef XENT_SCRATCH_H
#define XENT_SCRATCH_H

#include <stddef.h>

#include "xent/xent.h"

#define XENT_SCRATCH_CHUNK_SIZE (64u * 1024u)

typedef struct XentScratchChunk XentScratchChunk;

typedef struct XentScratchMark {
	XentScratchChunk *chunk;
	size_t            used;
} XentScratchMark;

XentScratchMark xent_scratch_mark(XentCtx const *ctx);
void            xent_scratch_rewind(XentCtx *ctx, XentScratchMark mark);
void            xent_scratch_reset(XentCtx *ctx);
void           *xent_scratch_alloc(XentCtx *ctx, size_t bytes, size_t alignment);
void            xent_free_scratch(XentCtx *ctx);

#endif
