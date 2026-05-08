#include "../xent_internal.h"

static bool     xent_float_bits_equal(float a, float b) { return memcmp(&a, &b, sizeof(float)) == 0; }

static uint64_t xent_hash_bytes(uint64_t hash, void const *data, size_t size) {
	uint8_t const *bytes = ( uint8_t const * ) data;
	for (size_t i = 0; i < size; ++i) {
		hash ^= bytes [i];
		hash *= 1099511628211ull;
	}
	return hash;
}

static uint64_t xent_hash_text_cache_key(XentTextCacheKey const *key) {
	uint64_t hash              = 1469598103934665603ull;
	uint8_t  line_break_policy = ( uint8_t ) key->line_break_policy;
	uint8_t  width_mode        = ( uint8_t ) key->width_mode;
	hash                       = xent_hash_bytes(hash, key->text, strlen(key->text));
	hash                       = xent_hash_bytes(hash, &key->font_size, sizeof(key->font_size));
	hash                       = xent_hash_bytes(hash, &key->width_constraint, sizeof(key->width_constraint));
	hash                       = xent_hash_bytes(hash, &line_break_policy, sizeof(line_break_policy));
	hash                       = xent_hash_bytes(hash, &width_mode, sizeof(width_mode));
	return hash;
}

static bool
xent_cached_text_key_matches(XentCachedTextKey const *cached, XentTextCacheKey const *key, uint64_t key_hash) {
	return cached->hash == key_hash
	    && xent_float_bits_equal(cached->font_size, key->font_size)
	    && xent_float_bits_equal(cached->width_constraint, key->width_constraint)
	    && cached->line_break_policy == ( uint8_t ) key->line_break_policy
	    && cached->width_mode == ( uint8_t ) key->width_mode
	    && strcmp(cached->text, key->text) == 0;
}

static bool xent_cached_text_key_copy(XentCachedTextKey *cached, XentTextCacheKey const *key) {
	cached->text = xent_strdup(key->text);
	if (!cached->text) return false;

	cached->hash              = xent_hash_text_cache_key(key);
	cached->font_size         = key->font_size;
	cached->width_constraint  = key->width_constraint;
	cached->line_break_policy = ( uint8_t ) key->line_break_policy;
	cached->width_mode        = ( uint8_t ) key->width_mode;
	return true;
}

static bool xent_reserve_cache_entries(void **entries, uint32_t *capacity, uint32_t count, size_t entry_size) {
	if (count >= XENT_CACHE_MAX_CAP) return true;
	if (count < *capacity) return true;

	uint32_t next_capacity = *capacity ? *capacity * 2u : 64u;
	if (next_capacity > XENT_CACHE_MAX_CAP) next_capacity = XENT_CACHE_MAX_CAP;
	void *next_entries = realloc(*entries, entry_size * ( size_t ) next_capacity);
	if (!next_entries) return false;

	*entries  = next_entries;
	*capacity = next_capacity;
	return true;
}

static bool xent_reserve_text_cache(XentTextCache *cache) {
	return xent_reserve_cache_entries(
	  ( void ** ) &cache->entries, &cache->capacity, cache->count, sizeof(*cache->entries)
	);
}

static bool xent_reserve_shape_cache(XentShapeCache *cache) {
	return xent_reserve_cache_entries(
	  ( void ** ) &cache->entries, &cache->capacity, cache->count, sizeof(*cache->entries)
	);
}

static XentTextCacheEntry *xent_find_text_entry(XentTextCache *cache, XentTextCacheKey const *key) {
	uint64_t key_hash = xent_hash_text_cache_key(key);
	for (uint32_t i = 0; i < cache->count; ++i) {
		XentTextCacheEntry *entry = &cache->entries [i];
		if (xent_cached_text_key_matches(&entry->key, key, key_hash)) return entry;
	}
	return NULL;
}

static XentShapeCacheEntry *xent_find_shape_entry(XentShapeCache *cache, XentTextCacheKey const *key) {
	uint64_t key_hash = xent_hash_text_cache_key(key);
	for (uint32_t i = 0; i < cache->count; ++i) {
		XentShapeCacheEntry *entry = &cache->entries [i];
		if (xent_cached_text_key_matches(&entry->key, key, key_hash)) return entry;
	}
	return NULL;
}

bool xent_text_cache_init(XentTextCache *cache) {
	if (!cache) return false;
	memset(cache, 0, sizeof(*cache));
	return true;
}

bool xent_shape_cache_init(XentShapeCache *cache) {
	if (!cache) return false;
	memset(cache, 0, sizeof(*cache));
	return true;
}

void xent_text_cache_destroy(XentTextCache *cache) {
	if (!cache) return;
	for (uint32_t i = 0; i < cache->count; ++i) free(cache->entries [i].key.text);
	free(cache->entries);
	memset(cache, 0, sizeof(*cache));
}

void xent_shape_cache_destroy(XentShapeCache *cache) {
	if (!cache) return;
	for (uint32_t i = 0; i < cache->count; ++i) free(cache->entries [i].key.text);
	free(cache->entries);
	memset(cache, 0, sizeof(*cache));
}

bool xent_text_cache_lookup(XentTextCache *cache, XentTextCacheKey const *key, XentTextMetrics *out_metrics) {
	if (!cache || !key || !key->text || !out_metrics) return false;

	XentTextCacheEntry *entry = xent_find_text_entry(cache, key);
	if (!entry) {
		cache->stats.misses += 1u;
		return false;
	}

	*out_metrics       = entry->metrics;
	entry->last_used   = ++cache->clock;
	cache->stats.hits += 1u;
	return true;
}

bool xent_shape_cache_lookup(XentShapeCache *cache, XentTextCacheKey const *key, XentShapingResult *out_result) {
	if (!cache || !key || !key->text || !out_result) return false;

	XentShapeCacheEntry *entry = xent_find_shape_entry(cache, key);
	if (!entry) {
		cache->stats.misses += 1u;
		return false;
	}

	*out_result        = entry->result;
	entry->last_used   = ++cache->clock;
	cache->stats.hits += 1u;
	return true;
}

static uint32_t xent_oldest_text_entry(XentTextCache const *cache) {
	uint32_t oldest = 0u;
	for (uint32_t i = 1u; i < cache->count; ++i)
		if (cache->entries [i].last_used < cache->entries [oldest].last_used) oldest = i;
	return oldest;
}

static uint32_t xent_oldest_shape_entry(XentShapeCache const *cache) {
	uint32_t oldest = 0u;
	for (uint32_t i = 1u; i < cache->count; ++i)
		if (cache->entries [i].last_used < cache->entries [oldest].last_used) oldest = i;
	return oldest;
}

static void xent_remove_text_entry(XentTextCache *cache, uint32_t index) {
	free(cache->entries [index].key.text);
	cache->entries [index]  = cache->entries [cache->count - 1u];
	cache->count           -= 1u;
}

static void xent_remove_shape_entry(XentShapeCache *cache, uint32_t index) {
	free(cache->entries [index].key.text);
	cache->entries [index]  = cache->entries [cache->count - 1u];
	cache->count           -= 1u;
}

static void xent_text_cache_evict_half(XentTextCache *cache) {
	uint32_t target = XENT_CACHE_MAX_CAP / 2u;
	while (cache->count > target) xent_remove_text_entry(cache, xent_oldest_text_entry(cache));
}

static void xent_shape_cache_evict_half(XentShapeCache *cache) {
	uint32_t target = XENT_CACHE_MAX_CAP / 2u;
	while (cache->count > target) xent_remove_shape_entry(cache, xent_oldest_shape_entry(cache));
}

void xent_text_cache_insert(XentTextCache *cache, XentTextCacheKey const *key, XentTextMetrics const *metrics) {
	if (!cache || !key || !key->text || !metrics) return;
	if (cache->count >= XENT_CACHE_MAX_CAP) xent_text_cache_evict_half(cache);
	if (!xent_reserve_text_cache(cache)) return;

	XentTextCacheEntry *entry = &cache->entries [cache->count];
	if (!xent_cached_text_key_copy(&entry->key, key)) return;

	entry->metrics        = *metrics;
	entry->last_used      = ++cache->clock;
	cache->count         += 1u;
	cache->stats.inserts += 1u;
}

void xent_shape_cache_insert(XentShapeCache *cache, XentTextCacheKey const *key, XentShapingResult const *result) {
	if (!cache || !key || !key->text || !result) return;
	if (cache->count >= XENT_CACHE_MAX_CAP) xent_shape_cache_evict_half(cache);
	if (!xent_reserve_shape_cache(cache)) return;

	XentShapeCacheEntry *entry = &cache->entries [cache->count];
	if (!xent_cached_text_key_copy(&entry->key, key)) return;

	entry->result         = *result;
	entry->last_used      = ++cache->clock;
	cache->count         += 1u;
	cache->stats.inserts += 1u;
}
