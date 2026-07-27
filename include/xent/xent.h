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
bool                      xent_is_simd_enabled(void);

XENT_NODISCARD XentNodeId xent_node_create(XentCtx *ctx);
bool                      xent_node_destroy(XentCtx *ctx, XentNodeId node);

bool                      xent_node_append(XentCtx *ctx, XentNodeId parent, XentNodeId child);
bool                      xent_node_remove(XentCtx *ctx, XentNodeId parent, XentNodeId child);

/** Insert a detached child before `before`, or append when `before` is invalid. */
bool                      xent_node_insert(XentCtx *ctx, XentNodeId parent, XentNodeId child, XentNodeId before);
/** Reorder an existing child of `parent` before `before`, or to the end when invalid. */
bool                      xent_node_move(XentCtx *ctx, XentNodeId parent, XentNodeId child, XentNodeId before);
/** Move `child` under `new_parent` before `before`. Fails if already under `new_parent`. */
bool                      xent_node_reparent(XentCtx *ctx, XentNodeId new_parent, XentNodeId child, XentNodeId before);

/** Observers are called in registration order. Adding or removing one during
 * notification is safe; a newly added observer starts with the next event.
 * Core topology and context mutation is rejected while observers run. */
XENT_NODISCARD XentObsId  xent_node_addobs(XentCtx *ctx, XentNodeObs const *observer);
bool                      xent_node_delobs(XentCtx *ctx, XentObsId observer);

bool                      xent_sem_setrole(XentCtx *ctx, XentNodeId node, XentSemRole role);
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
/** Return the last child in append order, or XENT_NODE_INVALID. */
XentNodeId    xent_node_last(XentCtx const *ctx, XentNodeId node);
XentNodeId    xent_node_next(XentCtx const *ctx, XentNodeId node);
/** Return the previous sibling in append order, or XENT_NODE_INVALID. */
XentNodeId    xent_node_prev(XentCtx const *ctx, XentNodeId node);
uint32_t      xent_node_nchild(XentCtx const *ctx, XentNodeId node);
XentProtocol  xent_node_proto(XentCtx const *ctx, XentNodeId node);
uint32_t      xent_node_dirty(XentCtx const *ctx, XentNodeId node);
XentSemRole   xent_sem_role(XentCtx const *ctx, XentNodeId node);
char const   *xent_sem_label(XentCtx const *ctx, XentNodeId node);
char const   *xent_node_text(XentCtx const *ctx, XentNodeId node);
float         xent_node_priority(XentCtx const *ctx, XentNodeId node);
XentNodeId    xent_layout_root(XentCtx const *ctx);

void          xent_profile_reset(XentCtx *ctx);
XentProfStats xent_profile_get(XentCtx const *ctx);
void          xent_profile_dump(XentCtx const *ctx, FILE *out);

bool          xent_set_focusable(XentCtx *ctx, XentNodeId node, bool focusable);
bool          xent_get_focusable(XentCtx const *ctx, XentNodeId node);
bool          xent_set_tab_index(XentCtx *ctx, XentNodeId node, int32_t tab_index);
int32_t       xent_get_tab_index(XentCtx const *ctx, XentNodeId node);


#endif
