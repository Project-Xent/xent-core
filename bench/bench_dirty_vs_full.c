#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "xent/xent.h"

typedef struct BenchTree {
    XentNodeId root;
    XentNodeId groups[100];
    XentNodeId leaves[100][100];
} BenchTree;

static double now_ms(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static void build_tree(XentContext *ctx, BenchTree *tree) {
    tree->root = xent_create_node(ctx);
    xent_set_protocol(ctx, tree->root, XENT_PROTOCOL_FLEX);
    xent_set_flex_direction(ctx, tree->root, XENT_FLEX_COLUMN);
    xent_set_size(ctx, tree->root, 1200.0f, 800.0f);
    xent_set_gap(ctx, tree->root, 1.0f);

    for (uint32_t g = 0; g < 100u; ++g) {
        XentNodeId group = xent_create_node(ctx);
        tree->groups[g] = group;
        xent_set_protocol(ctx, group, XENT_PROTOCOL_FLEX);
        xent_set_flex_direction(ctx, group, XENT_FLEX_ROW);
        xent_set_flex_grow(ctx, group, 1.0f);
        xent_set_gap(ctx, group, 1.0f);

        xent_append_child(ctx, tree->root, group);
        for (uint32_t i = 0; i < 100u; ++i) {
            XentNodeId leaf = xent_create_node(ctx);
            tree->leaves[g][i] = leaf;
            xent_set_text(ctx, leaf, "node");
            xent_set_size(ctx, leaf, NAN, 16.0f);
            xent_append_child(ctx, group, leaf);
        }
    }

    xent_layout(ctx, tree->root, 1200.0f, 800.0f);
}

static void mutate_leafs(XentContext *ctx,
                         const BenchTree *tree,
                         uint32_t groups_to_touch,
                         uint32_t leaves_per_group,
                         uint32_t tick) {
    const char *text = (tick & 1u) ? "node" : "node_updated";
    for (uint32_t g = 0; g < groups_to_touch; ++g) {
        for (uint32_t i = 0; i < leaves_per_group; ++i) {
            xent_set_text(ctx, tree->leaves[g][i], text);
        }
    }
}

static double run_full(XentContext *ctx,
                       const BenchTree *tree,
                       uint32_t groups_to_touch,
                       uint32_t leaves_per_group,
                       uint32_t iterations) {
    double start = now_ms();
    for (uint32_t it = 0; it < iterations; ++it) {
        mutate_leafs(ctx, tree, groups_to_touch, leaves_per_group, it);
        /* Force root dirty to benchmark true full-tree recompute cost. */
        xent_set_size(ctx, tree->root, 1200.0f, 800.0f);
        xent_layout(ctx, tree->root, 1200.0f, 800.0f);
    }
    return (now_ms() - start) / (double)iterations;
}

static double run_subtree(XentContext *ctx,
                          const BenchTree *tree,
                          uint32_t groups_to_touch,
                          uint32_t leaves_per_group,
                          uint32_t iterations) {
    double start = now_ms();
    for (uint32_t it = 0; it < iterations; ++it) {
        mutate_leafs(ctx, tree, groups_to_touch, leaves_per_group, it);
        for (uint32_t g = 0; g < groups_to_touch; ++g) {
            XentRect rect = {0};
            if (xent_get_layout_rect(ctx, tree->groups[g], &rect)) {
                xent_layout(ctx, tree->groups[g], rect.width, rect.height);
            }
        }
    }
    return (now_ms() - start) / (double)iterations;
}

static void run_scenario(const char *name, uint32_t groups_to_touch, uint32_t leaves_per_group) {
    XentContext *ctx = xent_create_context(NULL);
    BenchTree tree = {0};
    build_tree(ctx, &tree);

    const uint32_t iterations = 80u;
    double full_avg = run_full(ctx, &tree, groups_to_touch, leaves_per_group, iterations);
    double dirty_avg = run_subtree(ctx, &tree, groups_to_touch, leaves_per_group, iterations);
    double speedup = (dirty_avg > 0.0) ? (full_avg / dirty_avg) : 0.0;
    uint32_t dirty_nodes = groups_to_touch * leaves_per_group;

    printf("scenario=%s dirty_nodes=%u full_avg_ms=%.4f subtree_avg_ms=%.4f speedup=%.2fx\n",
           name,
           dirty_nodes,
           full_avg,
           dirty_avg,
           speedup);

    xent_destroy_context(ctx);
}

int main(void) {
    printf("bench_dirty_vs_full (100 groups x 100 leaves = 10k leaves)\n");
    run_scenario("leaf-1", 1u, 1u);
    run_scenario("group-100", 1u, 100u);
    run_scenario("ten-groups-1000", 10u, 100u);
    run_scenario("fifty-groups-5000", 50u, 100u);
    run_scenario("all-groups-10000", 100u, 100u);
    return 0;
}
