#ifndef XENT_H
#define XENT_H

#include <stdio.h>

#include "xent_display.h"
#include "xent_layout.h"
#include "xent_measure.h"
#include "xent_text.h"
#include "xent_types.h"

XENT_NODISCARD XentCtx   *xent_ctx_create(XentCfg const *config);
void                      xent_ctx_destroy(XentCtx *ctx);
bool                      xent_node_reserve(XentCtx *ctx, uint32_t capacity);
bool                      xent_simd_enabled(void);

/** Fails while an observer is running. */
XENT_NODISCARD XentNodeId xent_node_create(XentCtx *ctx);
/** Destroys the subtree. Fails while an observer is running. */
bool                      xent_node_destroy(XentCtx *ctx, XentNodeId node);

/** Detaches `child` from its current parent and appends it. Fails while an observer is running, or if `child` is an ancestor of `parent`. */
bool                      xent_node_append(XentCtx *ctx, XentNodeId parent, XentNodeId child);
/** Fails while an observer is running. */
bool                      xent_node_remove(XentCtx *ctx, XentNodeId parent, XentNodeId child);

/** `before == XENT_NODE_INVALID` appends. `child` must have no parent. Fails while an observer is running, or if `child` is an ancestor of `parent`. */
bool                      xent_node_insert(XentCtx *ctx, XentNodeId parent, XentNodeId child, XentNodeId before);
/** `before == XENT_NODE_INVALID` moves to the end. `child` must already be under `parent`. Fails while an observer is running. */
bool                      xent_node_move(XentCtx *ctx, XentNodeId parent, XentNodeId child, XentNodeId before);
/** `before == XENT_NODE_INVALID` appends. `child` must already have a different parent. Fails while an observer is running, or if `child` is an ancestor of `new_parent`. */
bool                      xent_node_reparent(XentCtx *ctx, XentNodeId new_parent, XentNodeId child, XentNodeId before);

/** Borrows `observer->userdata` until `xent_node_delobs` or `xent_ctx_destroy`. */
XENT_NODISCARD XentObsId  xent_node_addobs(XentCtx *ctx, XentNodeObs const *observer);
bool                      xent_node_delobs(XentCtx *ctx, XentObsId observer);

bool                      xent_sem_setrole(XentCtx *ctx, XentNodeId node, XentSemRole role);
/** Copies `label`. NULL is stored as an empty string. The caller may free it. */
bool                      xent_sem_setlabel(XentCtx *ctx, XentNodeId node, char const *label);
bool                      xent_sem_setflags(XentCtx *ctx, XentNodeId node, uint32_t flags);

bool                      xent_sem_setchecked(XentCtx *ctx, XentNodeId node, uint8_t state);
bool                      xent_sem_setenabled(XentCtx *ctx, XentNodeId node, bool enabled);
bool                      xent_sem_setexpanded(XentCtx *ctx, XentNodeId node, bool expanded);
bool                      xent_sem_setselected(XentCtx *ctx, XentNodeId node, bool selected);
bool                      xent_sem_setvalue(XentCtx *ctx, XentNodeId node, float value, float min, float max);

uint8_t                   xent_sem_checked(XentCtx const *ctx, XentNodeId node);
bool                      xent_sem_enabled(XentCtx const *ctx, XentNodeId node);
bool                      xent_sem_expanded(XentCtx const *ctx, XentNodeId node);
bool                      xent_sem_selected(XentCtx const *ctx, XentNodeId node);
bool          xent_sem_value(XentCtx const *ctx, XentNodeId node, float *out_value, float *out_min, float *out_max);

bool          xent_node_valid(XentCtx const *ctx, XentNodeId node);
XentNodeId    xent_node_parent(XentCtx const *ctx, XentNodeId node);
XentNodeId    xent_node_first(XentCtx const *ctx, XentNodeId node);
XentNodeId    xent_node_last(XentCtx const *ctx, XentNodeId node);
XentNodeId    xent_node_next(XentCtx const *ctx, XentNodeId node);
XentNodeId    xent_node_prev(XentCtx const *ctx, XentNodeId node);
uint32_t      xent_node_nchild(XentCtx const *ctx, XentNodeId node);
XentProtocol  xent_node_proto(XentCtx const *ctx, XentNodeId node);
uint32_t      xent_node_dirty(XentCtx const *ctx, XentNodeId node);
void          xent_mark_paint_dirty(XentCtx *ctx, XentNodeId node);
void          xent_clear_dirty(XentCtx *ctx, XentNodeId node, uint32_t mask);
uint32_t      xent_sem_drain_dirty(XentCtx *ctx, XentNodeId *out_nodes, uint32_t max_count);
XentSemRole   xent_sem_role(XentCtx const *ctx, XentNodeId node);
/** Borrowed until the next `xent_sem_setlabel` on this node, or until the node is destroyed. */
char const   *xent_sem_label(XentCtx const *ctx, XentNodeId node);
/** Borrowed until the next `xent_settext` on this node, or until the node is destroyed. */
char const   *xent_node_text(XentCtx const *ctx, XentNodeId node);
float         xent_node_priority(XentCtx const *ctx, XentNodeId node);
XentNodeId    xent_layout_root(XentCtx const *ctx);

void          xent_profile_reset(XentCtx *ctx);
XentProfStats xent_profile_get(XentCtx const *ctx);
void          xent_profile_dump(XentCtx const *ctx, FILE *out);

bool          xent_set_focusable(XentCtx *ctx, XentNodeId node, bool focusable);
bool          xent_get_focusable(XentCtx const *ctx, XentNodeId node);
bool          xent_set_tabindex(XentCtx *ctx, XentNodeId node, int32_t tab_index);
int32_t       xent_get_tabindex(XentCtx const *ctx, XentNodeId node);

#endif
