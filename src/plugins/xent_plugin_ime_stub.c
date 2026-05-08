#include "../xent_internal.h"

static void ime_stub_on_register(XentContext *ctx, XentPlugin const *plugin) {
	( void ) ctx;
	FILE *out = plugin && plugin->user_data ? ( FILE * ) plugin->user_data : stdout;
	fprintf(out, "[ime_stub] registered (no platform IME integration in MVP)\n");
}

static void
ime_stub_on_semantic_action(XentContext *ctx, XentNodeId node, char const *action, XentPlugin const *plugin) {
	( void ) ctx;
	FILE *out = plugin && plugin->user_data ? ( FILE * ) plugin->user_data : stdout;
	fprintf(out, "[ime_stub] semantic action node=%u action=%s (stub)\n", node, action ? action : "<null>");
}

XentPlugin xent_make_ime_stub_plugin(FILE *out) {
	XentPlugin plugin;
	memset(&plugin, 0, sizeof(plugin));
	plugin.name               = "ime_stub";
	plugin.enabled            = true;
	plugin.user_data          = out ? ( void * ) out : ( void * ) stdout;
	plugin.on_register        = ime_stub_on_register;
	plugin.on_semantic_action = ime_stub_on_semantic_action;
	return plugin;
}
