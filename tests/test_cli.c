#include "test_common.h"

#include <string.h>

static int test_json_escapes_control_characters(void) {
	XentContext *ctx = xent_create_context(NULL);
	TEST_ASSERT(ctx != NULL);

	XentNodeId root = xent_create_node(ctx);
	TEST_ASSERT(xent_set_size(ctx, root, (XentSize) {10.0f, 10.0f}));
	TEST_ASSERT(xent_set_text(ctx, root, "a\tb\rc\bd\fe"));
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
	xent_destroy_context(ctx);
	return 0;
}

int main(void) {
	XentTestFn const tests [] = {
	  test_json_escapes_control_characters,
	};
	return test_run_all(tests, sizeof(tests) / sizeof(tests [0]));
}
