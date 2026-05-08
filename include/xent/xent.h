#ifndef XENT_H
#define XENT_H

#include "xent_cli.h"
#include "xent_layout.h"
#include "xent_plugins.h"
#include "xent_text.h"
#include "xent_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

XentContext *xent_create_context(XentConfig const *config);
void         xent_destroy_context(XentContext *ctx);

bool         xent_begin_frame(XentContext *ctx);
bool         xent_end_frame(XentContext *ctx);

XentNodeId   xent_create_node(XentContext *ctx);
bool         xent_destroy_node(XentContext *ctx, XentNodeId node);

bool         xent_append_child(XentContext *ctx, XentNodeId parent, XentNodeId child);
bool         xent_remove_child(XentContext *ctx, XentNodeId parent, XentNodeId child);

/** Register one context-wide node lifecycle callback. Passing NULL clears it. */
bool         xent_set_node_lifecycle_callback(XentContext *ctx, XentNodeLifecycleFn callback, void *userdata);
/** Attach one typed external payload slot to a node. The destroy callback is optional and owned by xent. */
bool         xent_set_node_payload(
  XentContext *ctx, XentNodeId node, uint32_t payload_type, void *payload, XentNodePayloadDestroyFn destroy,
  void *destroy_userdata
);
/** Clear a node payload and run its destroy callback when present. */
bool            xent_clear_node_payload(XentContext *ctx, XentNodeId node);
/** Return the node payload when expected_payload_type matches, or when expected_payload_type is zero. */
void           *xent_get_node_payload(XentContext const *ctx, XentNodeId node, uint32_t expected_payload_type);
/** Return zero when the node has no typed payload. */
uint32_t        xent_get_node_payload_type(XentContext const *ctx, XentNodeId node);

bool            xent_set_semantic_role(XentContext *ctx, XentNodeId node, XentSemanticRole role);
bool            xent_set_semantic_label(XentContext *ctx, XentNodeId node, char const *label);
bool            xent_set_semantic_flags(XentContext *ctx, XentNodeId node, uint32_t flags);

bool            xent_set_userdata(XentContext *ctx, XentNodeId node, void *data);
void           *xent_get_userdata(XentContext const *ctx, XentNodeId node);

bool            xent_set_control_type(XentContext *ctx, XentNodeId node, XentControlType type);
XentControlType xent_get_control_type(XentContext const *ctx, XentNodeId node);

bool            xent_set_semantic_checked(XentContext *ctx, XentNodeId node, uint8_t state);
bool            xent_set_semantic_enabled(XentContext *ctx, XentNodeId node, bool enabled);
bool            xent_set_semantic_expanded(XentContext *ctx, XentNodeId node, bool expanded);
bool            xent_set_semantic_selected(XentContext *ctx, XentNodeId node, bool selected);
bool            xent_set_semantic_value(XentContext *ctx, XentNodeId node, float value, float min, float max);

uint8_t         xent_get_semantic_checked(XentContext const *ctx, XentNodeId node);
bool            xent_get_semantic_enabled(XentContext const *ctx, XentNodeId node);
bool            xent_get_semantic_expanded(XentContext const *ctx, XentNodeId node);
bool            xent_get_semantic_selected(XentContext const *ctx, XentNodeId node);
bool xent_get_semantic_value(XentContext const *ctx, XentNodeId node, float *out_value, float *out_min, float *out_max);

bool xent_is_valid_node(XentContext const *ctx, XentNodeId node);
XentNodeId       xent_get_parent(XentContext const *ctx, XentNodeId node);
XentNodeId       xent_get_first_child(XentContext const *ctx, XentNodeId node);
/** Return the last child in append order, or XENT_NODE_INVALID. */
XentNodeId       xent_get_last_child(XentContext const *ctx, XentNodeId node);
XentNodeId       xent_get_next_sibling(XentContext const *ctx, XentNodeId node);
/** Return the previous sibling in append order, or XENT_NODE_INVALID. */
XentNodeId       xent_get_prev_sibling(XentContext const *ctx, XentNodeId node);
uint32_t         xent_get_child_count(XentContext const *ctx, XentNodeId node);
XentProtocol     xent_get_protocol(XentContext const *ctx, XentNodeId node);
uint32_t         xent_get_dirty_flags(XentContext const *ctx, XentNodeId node);
XentSemanticRole xent_get_semantic_role(XentContext const *ctx, XentNodeId node);
char const      *xent_get_semantic_label(XentContext const *ctx, XentNodeId node);
char const      *xent_get_text(XentContext const *ctx, XentNodeId node);
float            xent_get_layout_priority(XentContext const *ctx, XentNodeId node);
XentNodeId       xent_get_last_layout_root(XentContext const *ctx);

void             xent_profile_reset(XentContext *ctx);
XentProfileStats xent_profile_get(XentContext const *ctx);
void             xent_profile_dump(XentContext const *ctx, FILE *out);
bool             xent_is_simd_enabled(void);

bool             xent_set_focusable(XentContext *ctx, XentNodeId node, bool focusable);
bool             xent_get_focusable(XentContext const *ctx, XentNodeId node);
bool             xent_set_tab_index(XentContext *ctx, XentNodeId node, int32_t tab_index);
int32_t          xent_get_tab_index(XentContext const *ctx, XentNodeId node);

#ifdef __cplusplus
}
#endif

#endif
