#include "../xent_internal.h"

static bool xent_ensure_plugin_capacity(XentContext *ctx, uint32_t needed) {
    if (needed <= ctx->plugin_capacity) {
        return true;
    }
    uint32_t new_cap = ctx->plugin_capacity ? ctx->plugin_capacity * 2u : 8u;
    while (new_cap < needed) {
        new_cap *= 2u;
    }
    XentPlugin *new_plugins = (XentPlugin *)realloc(ctx->plugins, sizeof(XentPlugin) * (size_t)new_cap);
    if (!new_plugins) {
        return false;
    }
    ctx->plugins = new_plugins;
    ctx->plugin_capacity = new_cap;
    return true;
}

bool xent_register_plugin(XentContext *ctx, const XentPlugin *plugin) {
    if (!ctx || !plugin || !plugin->name) {
        return false;
    }

    for (uint32_t i = 0; i < ctx->plugin_count; ++i) {
        if (strcmp(ctx->plugins[i].name, plugin->name) == 0) {
            return false;
        }
    }

    if (!xent_ensure_plugin_capacity(ctx, ctx->plugin_count + 1u)) {
        return false;
    }

    ctx->plugins[ctx->plugin_count] = *plugin;
    if (ctx->plugins[ctx->plugin_count].on_register) {
        ctx->plugins[ctx->plugin_count].on_register(ctx, &ctx->plugins[ctx->plugin_count]);
    }
    ctx->plugin_count += 1u;
    return true;
}

bool xent_unregister_plugin(XentContext *ctx, const char *plugin_name) {
    if (!ctx || !plugin_name) {
        return false;
    }

    for (uint32_t i = 0; i < ctx->plugin_count; ++i) {
        if (strcmp(ctx->plugins[i].name, plugin_name) == 0) {
            if (ctx->plugins[i].on_unregister) {
                ctx->plugins[i].on_unregister(ctx, &ctx->plugins[i]);
            }

            for (uint32_t j = i + 1u; j < ctx->plugin_count; ++j) {
                ctx->plugins[j - 1u] = ctx->plugins[j];
            }
            ctx->plugin_count -= 1u;
            return true;
        }
    }

    return false;
}

void xent_emit_semantic_action(XentContext *ctx, XentNodeId node, const char *action) {
    if (!ctx || !xent_is_valid_node(ctx, node) || !action) {
        return;
    }

    for (uint32_t i = 0; i < ctx->plugin_count; ++i) {
        XentPlugin *plugin = &ctx->plugins[i];
        if (plugin->enabled && plugin->on_semantic_action) {
            plugin->on_semantic_action(ctx, node, action, plugin);
        }
    }
}
