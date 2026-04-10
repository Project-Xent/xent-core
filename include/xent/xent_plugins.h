#ifndef XENT_PLUGINS_H
#define XENT_PLUGINS_H

#include "xent_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct XentPlugin {
    const char *name;
    void *user_data;
    bool enabled;
    void (*on_register)(XentContext *ctx, const XentPlugin *plugin);
    void (*on_unregister)(XentContext *ctx, const XentPlugin *plugin);
    void (*on_frame_begin)(XentContext *ctx, const XentPlugin *plugin);
    void (*on_frame_end)(XentContext *ctx, const XentPlugin *plugin);
    void (*on_semantic_action)(XentContext *ctx, XentNodeId node, const char *action, const XentPlugin *plugin);
};

bool xent_register_plugin(XentContext *ctx, const XentPlugin *plugin);
bool xent_unregister_plugin(XentContext *ctx, const char *plugin_name);
void xent_emit_semantic_action(XentContext *ctx, XentNodeId node, const char *action);

XentPlugin xent_make_semantics_dump_plugin(FILE *out);
XentPlugin xent_make_ime_stub_plugin(FILE *out);

#ifdef __cplusplus
}
#endif

#endif
