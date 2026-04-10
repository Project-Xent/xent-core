#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "test_common.h"

static uint32_t lcg_next(uint32_t *state) {
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

int main(void) {
    XentContext *ctx = xent_create_context(NULL);
    TEST_ASSERT(ctx != NULL);

    const uint32_t groups = 120u;
    const uint32_t leaves_per_group = 120u;
    const uint32_t leaf_count = groups * leaves_per_group;
    XentNodeId *leaves = (XentNodeId *)malloc(sizeof(XentNodeId) * (size_t)leaf_count);
    TEST_ASSERT(leaves != NULL);

    uint32_t seed = 0xC0FFEEu;
    uint32_t write = 0u;

    XentNodeId root = xent_create_node(ctx);
    xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
    xent_set_flex_direction(ctx, root, XENT_FLEX_COLUMN);
    xent_set_size(ctx, root, 1920.0f, 1080.0f);
    xent_set_gap(ctx, root, 1.0f);

    for (uint32_t g = 0; g < groups; ++g) {
        XentNodeId group = xent_create_node(ctx);
        xent_set_protocol(ctx, group, (g % 2u == 0u) ? XENT_PROTOCOL_FLEX : XENT_PROTOCOL_SWIFTSTACK);
        if (g % 2u == 0u) {
            xent_set_flex_direction(ctx, group, XENT_FLEX_ROW);
        } else {
            xent_set_stack_axis(ctx, group, XENT_AXIS_HORIZONTAL);
        }
        xent_set_size(ctx, group, NAN, 18.0f);
        xent_set_flex_grow(ctx, group, 1.0f);
        xent_set_gap(ctx, group, 1.0f);
        xent_append_child(ctx, root, group);

        for (uint32_t i = 0; i < leaves_per_group; ++i) {
            XentNodeId leaf = xent_create_node(ctx);
            leaves[write++] = leaf;

            uint32_t r = lcg_next(&seed);
            bool is_text = (r & 3u) == 0u;
            if (is_text) {
                xent_set_text(ctx, leaf, ((r & 1u) == 0u) ? "stress-text-alpha" : "stress-text-beta");
                xent_set_size(ctx, leaf, NAN, 16.0f);
            } else {
                float w = 8.0f + (float)(r % 48u);
                xent_set_size(ctx, leaf, w, 16.0f);
            }

            if (g % 2u == 0u) {
                xent_set_flex_shrink(ctx, leaf, 1.0f);
                xent_set_flex_grow(ctx, leaf, (r % 5u == 0u) ? 1.0f : 0.0f);
            } else {
                xent_set_layout_priority(ctx, leaf, (float)(r % 4u));
                xent_set_is_spacer(ctx, leaf, (r % 17u == 0u));
            }
            xent_append_child(ctx, group, leaf);
        }
    }

    TEST_ASSERT(write == leaf_count);
    TEST_ASSERT(xent_layout(ctx, root, 1920.0f, 1080.0f));

    const uint32_t iterations = 24u;
    for (uint32_t it = 0; it < iterations; ++it) {
        for (uint32_t i = 0; i < leaf_count; i += 97u) {
            XentNodeId leaf = leaves[(i + it * 13u) % leaf_count];
            if (((i + it) & 1u) == 0u) {
                xent_set_text(ctx, leaf, (it & 1u) ? "mut-a" : "mut-b");
                xent_set_size(ctx, leaf, NAN, 16.0f);
            } else {
                float w = 10.0f + (float)((i + it) % 64u);
                xent_set_size(ctx, leaf, w, 16.0f);
            }
        }

        TEST_ASSERT(xent_layout(ctx, root, 1920.0f, 1080.0f));

        for (uint32_t i = 0; i < leaf_count; i += 503u) {
            XentRect rect = {0};
            TEST_ASSERT(xent_get_layout_rect(ctx, leaves[i], &rect));
            TEST_ASSERT(isfinite(rect.x));
            TEST_ASSERT(isfinite(rect.y));
            TEST_ASSERT(isfinite(rect.width));
            TEST_ASSERT(isfinite(rect.height));
            TEST_ASSERT(rect.width >= 0.0f);
            TEST_ASSERT(rect.height >= 0.0f);
        }
    }

    free(leaves);
    xent_destroy_context(ctx);
    return 0;
}
