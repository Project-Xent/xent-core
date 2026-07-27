#include <math.h>
#include <stdio.h>

#include "xent/xent_cli.h"
#include "xent/xent.h"

int main(int argc, char **argv) {
	XentCtx *ctx = xent_ctx_create(NULL);
	if (!ctx) return 1;

	XentNodeId root      = xent_node_create(ctx);
	XentNodeId header    = xent_node_create(ctx);
	XentNodeId content   = xent_node_create(ctx);
	XentNodeId paragraph = xent_node_create(ctx);

	xent_setproto(ctx, root, XENT_PROTOCOL_FLEX);
	xent_setflexdir(ctx, root, XENT_FLEX_COLUMN);
	xent_setsize(ctx, root, (XentSize) {640.0f, 360.0f});
	xent_setgap(ctx, root, 8.0f);
	xent_setp(ctx, root, (XentInsets) {8.0f, 8.0f, 8.0f, 8.0f});
	xent_sem_setlabel(ctx, root, "json_root");

	xent_setsize(ctx, header, (XentSize) {NAN, 40.0f});
	xent_sem_setlabel(ctx, header, "header");

	xent_setgrow(ctx, content, 1.0f);
	xent_setproto(ctx, content, XENT_PROTOCOL_ABSOLUTE);
	xent_sem_setlabel(ctx, content, "content");

	xent_settext(ctx, paragraph, "JSON dump from xent-core runtime");
	xent_setsize(ctx, paragraph, (XentSize) {NAN, NAN});
	xent_sem_setrole(ctx, paragraph, XENT_SEM_ROLE_TEXT);
	xent_sem_setlabel(ctx, paragraph, "paragraph");

	xent_node_append(ctx, root, header);
	xent_node_append(ctx, root, content);
	xent_node_append(ctx, content, paragraph);

	xent_layout(ctx, root, 640.0f, 360.0f);

	FILE *out = stdout;
	if (argc > 1) {
		FILE *file = NULL;
#if defined(_MSC_VER)
		if (fopen_s(&file, argv [1], "wb") != 0) file = NULL;
#else
		file = fopen(argv [1], "wb");
#endif
		out = file;
		if (!out) {
			xent_ctx_destroy(ctx);
			return 2;
		}
	}

	if (!xent_dump_layout_json(ctx, root, out)) {
		if (out != stdout) fclose(out);
		xent_ctx_destroy(ctx);
		return 3;
	}
	fputc('\n', out);

	if (out != stdout) fclose(out);
	xent_ctx_destroy(ctx);
	return 0;
}
