#include "test_common.h"

#include <string.h>

#include "xent/xent_cli.h"

static int test_json_escapes_control_characters(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root = xent_node_create(ctx);
	TEST_ASSERT(xent_setsize(ctx, root, (XentSize) {10.0f, 10.0f}));
	TEST_ASSERT(xent_settext(ctx, root, "a\tb\rc\bd\fe"));
	TEST_ASSERT(xent_layout(ctx, root, 10.0f, 10.0f));

	FILE *tmp = tmpfile();
	TEST_ASSERT(tmp != NULL);
	TEST_ASSERT(xent_dump_layout_json(ctx, root, tmp));
	TEST_ASSERT(fflush(tmp) == 0);
	TEST_ASSERT(fseek(tmp, 0, SEEK_SET) == 0);

	char   buffer [512] = {0};
	size_t read         = fread(buffer, 1u, sizeof(buffer) - 1u, tmp);
	TEST_ASSERT(read > 0u);
	TEST_ASSERT(strstr(buffer, "\\t") != NULL);
	TEST_ASSERT(strstr(buffer, "\\r") != NULL);
	TEST_ASSERT(strstr(buffer, "\\b") != NULL);
	TEST_ASSERT(strstr(buffer, "\\f") != NULL);

	fclose(tmp);
	xent_ctx_destroy(ctx);
	return 0;
}

static int test_semantics_dump_direct(void) {
	XentCtx *ctx = xent_ctx_create(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root  = xent_node_create(ctx);
	XentNodeId child = xent_node_create(ctx);
	TEST_ASSERT(root != XENT_NODE_INVALID && child != XENT_NODE_INVALID);
	TEST_ASSERT(xent_node_append(ctx, root, child));
	TEST_ASSERT(xent_setsize(ctx, root, (XentSize) {40.0f, 20.0f}));
	TEST_ASSERT(xent_sem_setrole(ctx, root, XENT_SEM_ROLE_ROOT));
	TEST_ASSERT(xent_sem_setlabel(ctx, root, "root-label"));
	TEST_ASSERT(xent_sem_setrole(ctx, child, XENT_SEM_ROLE_BUTTON));
	TEST_ASSERT(xent_layout(ctx, root, 40.0f, 20.0f));

	FILE *tmp = tmpfile();
	TEST_ASSERT(tmp != NULL);
	xent_dump_semantics_text(ctx, root, tmp);
	TEST_ASSERT(fflush(tmp) == 0);
	TEST_ASSERT(fseek(tmp, 0, SEEK_SET) == 0);

	char   buffer [512] = {0};
	size_t read         = fread(buffer, 1u, sizeof(buffer) - 1u, tmp);
	TEST_ASSERT(read > 0u);
	TEST_ASSERT(strstr(buffer, "role=root") != NULL);
	TEST_ASSERT(strstr(buffer, "label=\"root-label\"") != NULL);
	TEST_ASSERT(strstr(buffer, "role=button") != NULL);

	fclose(tmp);
	xent_ctx_destroy(ctx);
	return 0;
}

int main(void) {
	XentTestFn const tests [] = {
	  test_json_escapes_control_characters,
	  test_semantics_dump_direct,
	};
	return test_run_all(tests, sizeof(tests) / sizeof(tests [0]));
}
