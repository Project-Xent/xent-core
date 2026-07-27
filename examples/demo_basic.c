#include <math.h>
#include <stdio.h>

#include "xent/xent_cli.h"
#include "xent/xent.h"

int main(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	if (!ctx) {
		fprintf(stderr, "failed to create context\n");
		return 1;
	}

	XentNodeId root    = xent_node_create(ctx);
	XentNodeId sidebar = xent_node_create(ctx);
	XentNodeId content = xent_node_create(ctx);
	XentNodeId toolbar = xent_node_create(ctx);
	XentNodeId body    = xent_node_create(ctx);
	XentNodeId text    = xent_node_create(ctx);

	xent_sem_setlabel(ctx, root, "root");
	xent_sem_setlabel(ctx, sidebar, "sidebar");
	xent_sem_setlabel(ctx, content, "content");
	xent_sem_setlabel(ctx, toolbar, "toolbar");
	xent_sem_setlabel(ctx, body, "body");
	xent_sem_setlabel(ctx, text, "title");

	xent_setproto(ctx, root, XENT_PROTOCOL_FLEX);
	xent_setflexdir(ctx, root, XENT_FLEX_ROW);
	xent_setsize(ctx, root, (XentSize) {800.0f, 600.0f});
	xent_setgap(ctx, root, 8.0f);
	xent_setp(ctx, root, (XentInsets) {8.0f, 8.0f, 8.0f, 8.0f});

	xent_setsize(ctx, sidebar, (XentSize) {200.0f, 580.0f});
	xent_setshrink(ctx, sidebar, 0.0f);

	xent_setproto(ctx, content, XENT_PROTOCOL_FLEX);
	xent_setflexdir(ctx, content, XENT_FLEX_COLUMN);
	xent_setgrow(ctx, content, 1.0f);
	xent_setgap(ctx, content, 8.0f);

	xent_setsize(ctx, toolbar, (XentSize) {NAN, 48.0f});
	xent_setshrink(ctx, toolbar, 0.0f);

	xent_setgrow(ctx, body, 1.0f);

	xent_settext(ctx, text, "Hello from xent-core CLI demo");
	xent_setsize(ctx, text, (XentSize) {NAN, NAN});
	xent_sem_setrole(ctx, text, XENT_SEM_ROLE_TEXT);

	xent_node_append(ctx, root, sidebar);
	xent_node_append(ctx, root, content);
	xent_node_append(ctx, content, toolbar);
	xent_node_append(ctx, content, body);
	xent_node_append(ctx, body, text);

	xent_layout(ctx, root, 800.0f, 600.0f);
	xent_dump_layout_text(ctx, root, stdout);

	xent_ctx_destroy(ctx);
	return 0;
}
