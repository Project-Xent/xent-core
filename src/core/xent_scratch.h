#ifndef XENT_SCRATCH_H
#define XENT_SCRATCH_H

#include <stddef.h>

#include "xent/xent.h"

#define XENT_SCRATCH_CHUNK_SIZE (64u * 1024u)

typedef struct XentScratchChunk XentScratchChunk;

void                            xent_scratch_reset(XentCtx *ctx);
void                           *xent_scratch_alloc(XentCtx *ctx, size_t bytes, size_t alignment);
void                            xent_free_scratch(XentCtx *ctx);

#endif
