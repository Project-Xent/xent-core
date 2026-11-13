#ifndef XENT_DIRTY_H
#define XENT_DIRTY_H

#include <stdbool.h>
#include <stdint.h>

#include "xent/xent.h"

static bool inline xent_dirty_direct(uint32_t flags) { return (flags & (XENT_DIRTY_LAYOUT | XENT_DIRTY_SELF)) != 0u; }

void xent_mark_dirty(XentCtx *ctx, XentNodeId node, uint32_t flags);
void xent_dirty_clear_order(XentCtx *ctx);
void xent_compact_dirty_nodes(XentCtx *ctx);

#endif
