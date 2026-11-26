#ifndef XENT_DIRTY_H
#define XENT_DIRTY_H

#include <stdbool.h>
#include <stdint.h>

#include "xent/xent.h"

#define XENT_DIRTY_LAYOUT_MASK (XENT_DIRTY_SELF | XENT_DIRTY_SUBTREE | XENT_DIRTY_LAYOUT)

enum
{
	XENT_QUEUE_LAYOUT = 1u << 0,
	XENT_QUEUE_SEM    = 1u << 1,
};

static bool inline xent_dirty_direct(uint32_t flags) { return (flags & XENT_DIRTY_LAYOUT) != 0u; }

void xent_mark_dirty(XentCtx *ctx, XentNodeId node, uint32_t flags);
void xent_dirty_clear_order(XentCtx *ctx);
void xent_compact_dirty_nodes(XentCtx *ctx);

#endif
