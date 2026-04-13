#ifndef XENT_H
#define XENT_H

#include "xent_cli.h"
#include "xent_layout.h"
#include "xent_plugins.h"
#include "xent_text.h"
#include "xent_types.h"

#ifdef __cplusplus
extern "C" {
#endif

XentContext *xent_create_context(const XentConfig *config);
void xent_destroy_context(XentContext *ctx);

bool xent_begin_frame(XentContext *ctx);
bool xent_end_frame(XentContext *ctx);

XentNodeId xent_create_node(XentContext *ctx);
bool xent_destroy_node(XentContext *ctx, XentNodeId node);

bool xent_append_child(XentContext *ctx, XentNodeId parent, XentNodeId child);
bool xent_remove_child(XentContext *ctx, XentNodeId parent, XentNodeId child);

bool xent_set_semantic_role(XentContext *ctx, XentNodeId node, XentSemanticRole role);
bool xent_set_semantic_label(XentContext *ctx, XentNodeId node, const char *label);
bool xent_set_semantic_flags(XentContext *ctx, XentNodeId node, uint32_t flags);

bool  xent_set_userdata(XentContext *ctx, XentNodeId node, void *data);
void *xent_get_userdata(const XentContext *ctx, XentNodeId node);

bool            xent_set_control_type(XentContext *ctx, XentNodeId node, XentControlType type);
XentControlType xent_get_control_type(const XentContext *ctx, XentNodeId node);

bool xent_set_semantic_checked  (XentContext *ctx, XentNodeId node, uint8_t state);
bool xent_set_semantic_enabled  (XentContext *ctx, XentNodeId node, bool enabled);
bool xent_set_semantic_expanded (XentContext *ctx, XentNodeId node, bool expanded);
bool xent_set_semantic_selected (XentContext *ctx, XentNodeId node, bool selected);
bool xent_set_semantic_value    (XentContext *ctx, XentNodeId node,
                                 float value, float min, float max);

uint8_t xent_get_semantic_checked  (const XentContext *ctx, XentNodeId node);
bool    xent_get_semantic_enabled  (const XentContext *ctx, XentNodeId node);
bool    xent_get_semantic_expanded (const XentContext *ctx, XentNodeId node);
bool    xent_get_semantic_selected (const XentContext *ctx, XentNodeId node);
bool    xent_get_semantic_value    (const XentContext *ctx, XentNodeId node,
                                    float *out_value, float *out_min, float *out_max);

bool xent_is_valid_node(const XentContext *ctx, XentNodeId node);
XentNodeId xent_get_parent(const XentContext *ctx, XentNodeId node);
XentNodeId xent_get_first_child(const XentContext *ctx, XentNodeId node);
XentNodeId xent_get_next_sibling(const XentContext *ctx, XentNodeId node);
uint32_t xent_get_child_count(const XentContext *ctx, XentNodeId node);
XentProtocol xent_get_protocol(const XentContext *ctx, XentNodeId node);
uint32_t xent_get_dirty_flags(const XentContext *ctx, XentNodeId node);
XentSemanticRole xent_get_semantic_role(const XentContext *ctx, XentNodeId node);
const char *xent_get_semantic_label(const XentContext *ctx, XentNodeId node);
const char *xent_get_text(const XentContext *ctx, XentNodeId node);
float xent_get_layout_priority(const XentContext *ctx, XentNodeId node);
XentNodeId xent_get_last_layout_root(const XentContext *ctx);

void xent_profile_reset(XentContext *ctx);
XentProfileStats xent_profile_get(const XentContext *ctx);
void xent_profile_dump(const XentContext *ctx, FILE *out);
bool xent_is_simd_enabled(void);

bool     xent_set_focusable(XentContext *ctx, XentNodeId node, bool focusable);
bool     xent_get_focusable(const XentContext *ctx, XentNodeId node);
bool     xent_set_tab_index(XentContext *ctx, XentNodeId node, int32_t tab_index);
int32_t  xent_get_tab_index(const XentContext *ctx, XentNodeId node);

#ifdef __cplusplus
}
#endif

#endif
