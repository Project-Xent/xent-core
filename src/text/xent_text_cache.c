#include "../xent_internal.h"

static uint64_t xent_fnv1a(const char *text,
                           float font_size,
                           float width_constraint,
                           XentLineBreakPolicy line_break_policy,
                           XentMeasureMode width_mode) {
    const uint64_t fnv_offset = 1469598103934665603ull;
    const uint64_t fnv_prime = 1099511628211ull;

    uint64_t hash = fnv_offset;
    const uint8_t *bytes = (const uint8_t *)text;
    while (*bytes) {
        hash ^= *bytes++;
        hash *= fnv_prime;
    }

    const uint8_t *size_bytes = (const uint8_t *)&font_size;
    const uint8_t *width_bytes = (const uint8_t *)&width_constraint;
    for (size_t i = 0; i < sizeof(float); ++i) {
        hash ^= size_bytes[i];
        hash *= fnv_prime;
    }
    for (size_t i = 0; i < sizeof(float); ++i) {
        hash ^= width_bytes[i];
        hash *= fnv_prime;
    }
    hash ^= (uint8_t)line_break_policy;
    hash *= fnv_prime;
    hash ^= (uint8_t)width_mode;
    hash *= fnv_prime;

    return hash;
}

bool xent_text_cache_init(XentTextCache *cache) {
    if (!cache) {
        return false;
    }
    memset(cache, 0, sizeof(*cache));
    return true;
}

bool xent_shape_cache_init(XentShapeCache *cache) {
    if (!cache) {
        return false;
    }
    memset(cache, 0, sizeof(*cache));
    return true;
}

void xent_text_cache_destroy(XentTextCache *cache) {
    if (!cache) {
        return;
    }
    for (uint32_t i = 0; i < cache->count; ++i) {
        free(cache->entries[i].text);
        cache->entries[i].text = NULL;
    }
    free(cache->entries);
    memset(cache, 0, sizeof(*cache));
}

void xent_shape_cache_destroy(XentShapeCache *cache) {
    if (!cache) {
        return;
    }
    for (uint32_t i = 0; i < cache->count; ++i) {
        free(cache->entries[i].text);
        cache->entries[i].text = NULL;
    }
    free(cache->entries);
    memset(cache, 0, sizeof(*cache));
}

bool xent_text_cache_lookup(XentTextCache *cache,
                            const char *text,
                            float font_size,
                            float width_constraint,
                            XentLineBreakPolicy line_break_policy,
                            XentMeasureMode width_mode,
                            XentTextMetrics *out_metrics) {
    if (!cache || !text || !out_metrics) {
        return false;
    }

    uint64_t hash = xent_fnv1a(text, font_size, width_constraint, line_break_policy, width_mode);
    for (uint32_t i = 0; i < cache->count; ++i) {
        XentTextCacheEntry *entry = &cache->entries[i];
        if (entry->hash == hash &&
            entry->font_size == font_size &&
            entry->width_constraint == width_constraint &&
            entry->line_break_policy == (uint8_t)line_break_policy &&
            entry->width_mode == (uint8_t)width_mode &&
            strcmp(entry->text, text) == 0) {
            *out_metrics = entry->metrics;
            cache->stats.hits += 1u;
            return true;
        }
    }

    cache->stats.misses += 1u;
    return false;
}

bool xent_shape_cache_lookup(XentShapeCache *cache,
                             const char *text,
                             float font_size,
                             float width_constraint,
                             XentLineBreakPolicy line_break_policy,
                             XentMeasureMode width_mode,
                             XentShapingResult *out_result) {
    if (!cache || !text || !out_result) {
        return false;
    }

    uint64_t hash = xent_fnv1a(text, font_size, width_constraint, line_break_policy, width_mode);
    for (uint32_t i = 0; i < cache->count; ++i) {
        XentShapeCacheEntry *entry = &cache->entries[i];
        if (entry->hash == hash &&
            entry->font_size == font_size &&
            entry->width_constraint == width_constraint &&
            entry->line_break_policy == (uint8_t)line_break_policy &&
            entry->width_mode == (uint8_t)width_mode &&
            strcmp(entry->text, text) == 0) {
            *out_result = entry->result;
            cache->stats.hits += 1u;
            return true;
        }
    }

    cache->stats.misses += 1u;
    return false;
}

void xent_text_cache_insert(XentTextCache *cache,
                            const char *text,
                            float font_size,
                            float width_constraint,
                            XentLineBreakPolicy line_break_policy,
                            XentMeasureMode width_mode,
                            const XentTextMetrics *metrics) {
    if (!cache || !text || !metrics) {
        return;
    }

    if (cache->count == cache->capacity) {
        uint32_t new_cap = cache->capacity ? cache->capacity * 2u : 64u;
        XentTextCacheEntry *new_entries =
            (XentTextCacheEntry *)realloc(cache->entries, sizeof(XentTextCacheEntry) * (size_t)new_cap);
        if (!new_entries) {
            return;
        }
        cache->entries = new_entries;
        cache->capacity = new_cap;
    }

    char *text_copy = xent_strdup(text);
    if (!text_copy) {
        return;
    }

    XentTextCacheEntry *entry = &cache->entries[cache->count++];
    entry->hash = xent_fnv1a(text, font_size, width_constraint, line_break_policy, width_mode);
    entry->text = text_copy;
    entry->font_size = font_size;
    entry->width_constraint = width_constraint;
    entry->line_break_policy = (uint8_t)line_break_policy;
    entry->width_mode = (uint8_t)width_mode;
    entry->metrics = *metrics;
    cache->stats.inserts += 1u;
}

void xent_shape_cache_insert(XentShapeCache *cache,
                             const char *text,
                             float font_size,
                             float width_constraint,
                             XentLineBreakPolicy line_break_policy,
                             XentMeasureMode width_mode,
                             const XentShapingResult *result) {
    if (!cache || !text || !result) {
        return;
    }

    if (cache->count == cache->capacity) {
        uint32_t new_cap = cache->capacity ? cache->capacity * 2u : 64u;
        XentShapeCacheEntry *new_entries =
            (XentShapeCacheEntry *)realloc(cache->entries, sizeof(XentShapeCacheEntry) * (size_t)new_cap);
        if (!new_entries) {
            return;
        }
        cache->entries = new_entries;
        cache->capacity = new_cap;
    }

    char *text_copy = xent_strdup(text);
    if (!text_copy) {
        return;
    }

    XentShapeCacheEntry *entry = &cache->entries[cache->count++];
    entry->hash = xent_fnv1a(text, font_size, width_constraint, line_break_policy, width_mode);
    entry->text = text_copy;
    entry->font_size = font_size;
    entry->width_constraint = width_constraint;
    entry->line_break_policy = (uint8_t)line_break_policy;
    entry->width_mode = (uint8_t)width_mode;
    entry->result = *result;
    cache->stats.inserts += 1u;
}
