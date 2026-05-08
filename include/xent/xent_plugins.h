#ifndef XENT_PLUGINS_H
#define XENT_PLUGINS_H

#include "xent_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct XentPlugin {
	char const *name;
	void       *user_data;
	bool        enabled;
	void        (*on_register)(XentContext *ctx, XentPlugin const *plugin);
	void        (*on_unregister)(XentContext *ctx, XentPlugin const *plugin);
	void        (*on_frame_begin)(XentContext *ctx, XentPlugin const *plugin);
	void        (*on_frame_end)(XentContext *ctx, XentPlugin const *plugin);
	void        (*on_semantic_action)(XentContext *ctx, XentNodeId node, char const *action, XentPlugin const *plugin);
};

bool       xent_register_plugin(XentContext *ctx, XentPlugin const *plugin);
bool       xent_unregister_plugin(XentContext *ctx, char const *plugin_name);
void       xent_emit_semantic_action(XentContext *ctx, XentNodeId node, char const *action);

XentPlugin xent_make_semantics_dump_plugin(FILE *out);
XentPlugin xent_make_ime_stub_plugin(FILE *out);

#ifdef __cplusplus
}
#endif

#endif
