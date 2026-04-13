#include <math.h>
#include <stdio.h>
#include <time.h>

#include "xent/xent.h"

static double now_ms(void) {
    return (double)clock() * 1000.0 / (double)CLOCKS_PER_SEC;
}

static XentNodeId build_many_children(XentContext *ctx, XentProtocol protocol, uint32_t node_count) {
    XentNodeId root = xent_create_node(ctx);
    xent_set_protocol(ctx, root, protocol);
    xent_set_size(ctx, root, 1200.0f, 720.0f);
    xent_set_gap(ctx, root, 1.0f);

    if (protocol == XENT_PROTOCOL_FLEX) {
        xent_set_flex_direction(ctx, root, XENT_FLEX_COLUMN);
    } else if (protocol == XENT_PROTOCOL_SWIFTSTACK) {
        xent_set_stack_axis(ctx, root, XENT_AXIS_VERTICAL);
    }

    for (uint32_t i = 0; i < node_count; ++i) {
        XentNodeId child = xent_create_node(ctx);
        xent_set_size(ctx, child, NAN, 18.0f);
        xent_set_text(ctx, child, "node");
        if (protocol == XENT_PROTOCOL_FLEX) {
            xent_set_flex_shrink(ctx, child, 1.0f);
        } else if (protocol == XENT_PROTOCOL_SWIFTSTACK) {
            xent_set_layout_priority(ctx, child, (i % 3 == 0) ? 1.0f : 0.0f);
        }
        xent_append_child(ctx, root, child);
    }
    return root;
}

static void run_case(const char *name, XentProtocol protocol, uint32_t nodes) {
    XentContext *ctx = xent_create_context(NULL);
    XentNodeId root = build_many_children(ctx, protocol, nodes);

    const int iterations = 20;
    xent_profile_reset(ctx);
    double start = now_ms();
    for (int i = 0; i < iterations; ++i) {
        xent_layout(ctx, root, 1200.0f, 720.0f);
    }
    double elapsed = now_ms() - start;
    printf("%s nodes=%u iterations=%d total_ms=%.3f avg_ms=%.3f\n",
           name,
           nodes,
           iterations,
           elapsed,
           elapsed / (double)iterations);

    if (protocol == XENT_PROTOCOL_SWIFTSTACK) {
        XentProfileStats p = xent_profile_get(ctx);
        printf("  swiftstack_profile total=%.3fms collect=%.3fms sort=%.3fms text=%.3fms allocs=%llu sorts=%llu scans=%llu text_calls=%llu\n",
               p.swiftstack_total_ms,
               p.swiftstack_collect_ms,
               p.swiftstack_sort_ms,
               p.swiftstack_text_ms,
               (unsigned long long)p.temp_allocations,
               (unsigned long long)p.sort_calls,
               (unsigned long long)p.sibling_scans,
               (unsigned long long)p.text_measure_calls);
    }

    xent_destroy_context(ctx);
}

int main(void) {
    printf("runtime simd=%s\n", xent_is_simd_enabled() ? "enabled" : "disabled");
    uint32_t sizes[] = {100u, 1000u, 10000u};
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        run_case("xent_flex", XENT_PROTOCOL_FLEX, sizes[i]);
        run_case("xent_swiftstack", XENT_PROTOCOL_SWIFTSTACK, sizes[i]);
    }
    return 0;
}
