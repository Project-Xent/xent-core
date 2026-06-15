#include "../xent_internal.h"
#include "xent/xent_cli.h"

static void semantics_dump_on_frame_end(XentContext *ctx, XentPlugin const *plugin) {
	FILE      *out  = plugin && plugin->user_data ? ( FILE * ) plugin->user_data : stdout;
	XentNodeId root = xent_get_last_layout_root(ctx);
	if (root != XENT_NODE_INVALID) {
		fprintf(out, "[semantics_dump] frame=%llu\n", ( unsigned long long ) ctx->frame_index);
		xent_dump_semantics_text(ctx, root, out);
	}
}

XentPlugin xent_make_semantics_dump_plugin(FILE *out) {
	XentPlugin plugin;
	memset(&plugin, 0, sizeof(plugin));
	plugin.name         = "semantics_dump";
	plugin.enabled      = true;
	plugin.user_data    = out ? ( void * ) out : ( void * ) stdout;
	plugin.on_frame_end = semantics_dump_on_frame_end;
	return plugin;
}
