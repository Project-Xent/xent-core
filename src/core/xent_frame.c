#include "../xent_internal.h"

bool xent_begin_frame(XentContext *ctx) {
    if (!ctx || ctx->in_frame) {
        return false;
    }
    ctx->in_frame = true;
    ctx->frame_index += 1u;

    for (uint32_t i = 0; i < ctx->plugin_count; ++i) {
        XentPlugin *plugin = &ctx->plugins[i];
        if (plugin->enabled && plugin->on_frame_begin) {
            plugin->on_frame_begin(ctx, plugin);
        }
    }
    return true;
}

bool xent_end_frame(XentContext *ctx) {
    if (!ctx || !ctx->in_frame) {
        return false;
    }

    for (uint32_t i = 0; i < ctx->plugin_count; ++i) {
        XentPlugin *plugin = &ctx->plugins[i];
        if (plugin->enabled && plugin->on_frame_end) {
            plugin->on_frame_end(ctx, plugin);
        }
    }

    ctx->in_frame = false;
    return true;
}
