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
        xent_set_text_line_break_policy(ctx, child, XENT_LINE_BREAK_CHAR_WRAP);
        if (protocol == XENT_PROTOCOL_FLEX) {
            xent_set_flex_shrink(ctx, child, 1.0f);
        } else if (protocol == XENT_PROTOCOL_SWIFTSTACK) {
            xent_set_layout_priority(ctx, child, (i % 3u == 0u) ? 1.0f : 0.0f);
        }
        xent_append_child(ctx, root, child);
    }
    return root;
}

static double run_layout_case_avg_ms(XentProtocol protocol, uint32_t nodes, uint32_t iterations) {
    XentContext *ctx = xent_create_context(NULL);
    XentNodeId root = build_many_children(ctx, protocol, nodes);

    double start = now_ms();
    for (uint32_t i = 0; i < iterations; ++i) {
        xent_layout(ctx, root, 1200.0f, 720.0f);
    }
    double avg = (now_ms() - start) / (double)iterations;

    xent_destroy_context(ctx);
    return avg;
}

static void build_dirty_tree(XentContext *ctx, BenchTree *tree) {
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
            xent_set_text_line_break_policy(ctx, leaf, XENT_LINE_BREAK_CHAR_WRAP);
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

static double run_dirty_full_avg(XentContext *ctx,
                                 const BenchTree *tree,
                                 uint32_t groups_to_touch,
                                 uint32_t leaves_per_group,
                                 uint32_t iterations) {
    double start = now_ms();
    for (uint32_t it = 0; it < iterations; ++it) {
        mutate_leafs(ctx, tree, groups_to_touch, leaves_per_group, it);
        xent_set_size(ctx, tree->root, 1200.0f, 800.0f);
        xent_layout(ctx, tree->root, 1200.0f, 800.0f);
    }
    return (now_ms() - start) / (double)iterations;
}

static double run_dirty_subtree_avg(XentContext *ctx,
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

static bool gate_layout_relative_perf(void) {
    double flex_10k = run_layout_case_avg_ms(XENT_PROTOCOL_FLEX, 10000u, 20u);
    double swift_10k = run_layout_case_avg_ms(XENT_PROTOCOL_SWIFTSTACK, 10000u, 20u);

    double limit = (flex_10k * 4.0) + 0.25;
    bool pass = swift_10k <= limit;
    printf("[gate] layout-relative swift<=4x-flex+0.25: flex=%.4f swift=%.4f limit=%.4f => %s\n",
           flex_10k,
           swift_10k,
           limit,
           pass ? "PASS" : "FAIL");
    return pass;
}

static bool gate_dirty_scheduler(void) {
    XentContext *ctx = xent_create_context(NULL);
    BenchTree tree = {0};
    build_dirty_tree(ctx, &tree);

    double leaf_full = run_dirty_full_avg(ctx, &tree, 1u, 1u, 60u);
    double leaf_subtree = run_dirty_subtree_avg(ctx, &tree, 1u, 1u, 60u);
    double all_full = run_dirty_full_avg(ctx, &tree, 100u, 100u, 60u);
    double all_subtree = run_dirty_subtree_avg(ctx, &tree, 100u, 100u, 60u);

    bool leaf_pass = leaf_subtree < (leaf_full * 0.80);
    bool all_pass = all_full < (all_subtree * 1.10);
    printf("[gate] dirty-leaf subtree<0.8x full: full=%.4f subtree=%.4f => %s\n",
           leaf_full,
           leaf_subtree,
           leaf_pass ? "PASS" : "FAIL");
    printf("[gate] dirty-all full<1.1x subtree: full=%.4f subtree=%.4f => %s\n",
           all_full,
           all_subtree,
           all_pass ? "PASS" : "FAIL");

    xent_destroy_context(ctx);
    return leaf_pass && all_pass;
}

static bool gate_layout_strategy_transitions(void) {
    XentContext *ctx = xent_create_context(NULL);
    XentNodeId root = xent_create_node(ctx);
    XentNodeId a = xent_create_node(ctx);
    XentNodeId b = xent_create_node(ctx);

    xent_set_protocol(ctx, root, XENT_PROTOCOL_FLEX);
    xent_set_flex_direction(ctx, root, XENT_FLEX_ROW);
    xent_set_size(ctx, root, 200.0f, 80.0f);
    xent_set_size(ctx, a, 50.0f, 20.0f);
    xent_set_size(ctx, b, NAN, 20.0f);
    xent_set_text(ctx, b, "node");
    xent_set_text_line_break_policy(ctx, b, XENT_LINE_BREAK_CHAR_WRAP);
    xent_append_child(ctx, root, a);
    xent_append_child(ctx, root, b);

    xent_layout(ctx, root, 200.0f, 80.0f);
    bool pass_full = xent_get_last_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_FULL_TREE;

    xent_layout(ctx, root, 200.0f, 80.0f);
    bool pass_none = xent_get_last_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_NONE;

    xent_set_text(ctx, b, "node2");
    xent_layout(ctx, root, 200.0f, 80.0f);
    bool pass_subtree = xent_get_last_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_DIRTY_SUBTREE;

    xent_set_size(ctx, root, 220.0f, 80.0f);
    xent_layout(ctx, root, 220.0f, 80.0f);
    bool pass_full_again = xent_get_last_layout_strategy(ctx) == XENT_LAYOUT_STRATEGY_FULL_TREE;

    bool pass = pass_full && pass_none && pass_subtree && pass_full_again;
    printf("[gate] strategy-transition full->none->subtree->full => %s\n", pass ? "PASS" : "FAIL");

    xent_destroy_context(ctx);
    return pass;
}

int main(void) {
    bool ok = true;
    ok = gate_layout_relative_perf() && ok;
    ok = gate_dirty_scheduler() && ok;
    ok = gate_layout_strategy_transitions() && ok;
    return ok ? 0 : 1;
}
