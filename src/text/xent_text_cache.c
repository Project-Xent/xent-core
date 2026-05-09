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
	return hash ? hash : 1ull;
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

static bool xent_cached_text_key_copy(XentCachedTextKey *cached, XentTextCacheKey const *key, uint64_t key_hash) {
	cached->text = xent_strdup(key->text);
	if (!cached->text) return false;

	cached->hash              = key_hash;
	cached->font_size         = key->font_size;
	cached->width_constraint  = key->width_constraint;
	cached->line_break_policy = ( uint8_t ) key->line_break_policy;
	cached->width_mode        = ( uint8_t ) key->width_mode;
	return true;
}

static void xent_text_cache_clear_entries(XentTextCacheEntry *entries, uint32_t capacity) {
	for (uint32_t i = 0; i < capacity; ++i) {
		if (entries [i].key.hash != 0) {
			free(entries [i].key.text);
			entries [i].key.hash = 0;
			entries [i].key.text = NULL;
		}
	}
}

static void xent_shape_cache_clear_entries(XentShapeCacheEntry *entries, uint32_t capacity) {
	for (uint32_t i = 0; i < capacity; ++i) {
		if (entries [i].key.hash != 0) {
			free(entries [i].key.text);
			entries [i].key.hash = 0;
			entries [i].key.text = NULL;
		}
	}
}

static bool xent_text_cache_rehash(XentTextCache *cache, uint32_t new_cap) {
	XentTextCacheEntry *new_entries = ( XentTextCacheEntry * ) calloc(new_cap, sizeof(*new_entries));
	if (!new_entries) return false;

	uint32_t mask = new_cap - 1u;
	for (uint32_t i = 0; i < cache->capacity; ++i) {
		XentTextCacheEntry *old = &cache->entries [i];
		if (old->key.hash == 0) continue;

		uint32_t slot = ( uint32_t ) (old->key.hash & mask);
		while (new_entries [slot].key.hash != 0) slot = (slot + 1u) & mask;
		new_entries [slot] = *old;
	}

	free(cache->entries);
	cache->entries  = new_entries;
	cache->capacity = new_cap;
	return true;
}

static bool xent_shape_cache_rehash(XentShapeCache *cache, uint32_t new_cap) {
	XentShapeCacheEntry *new_entries = ( XentShapeCacheEntry * ) calloc(new_cap, sizeof(*new_entries));
	if (!new_entries) return false;

	uint32_t mask = new_cap - 1u;
	for (uint32_t i = 0; i < cache->capacity; ++i) {
		XentShapeCacheEntry *old = &cache->entries [i];
		if (old->key.hash == 0) continue;

		uint32_t slot = ( uint32_t ) (old->key.hash & mask);
		while (new_entries [slot].key.hash != 0) slot = (slot + 1u) & mask;
		new_entries [slot] = *old;
	}

	free(cache->entries);
	cache->entries  = new_entries;
	cache->capacity = new_cap;
	return true;
}

static bool xent_text_cache_ensure_space(XentTextCache *cache) {
	if (cache->capacity == 0) {
		cache->entries = ( XentTextCacheEntry * ) calloc(XENT_CACHE_INIT_CAP, sizeof(*cache->entries));
		if (!cache->entries) return false;
		cache->capacity = XENT_CACHE_INIT_CAP;
		return true;
	}

	if (cache->count * 4u < cache->capacity * 3u) return true;

	if (cache->capacity >= XENT_CACHE_MAX_CAP) {
		xent_text_cache_clear_entries(cache->entries, cache->capacity);
		cache->stats.evictions += cache->count;
		cache->count            = 0;
		return true;
	}

	uint32_t new_cap = cache->capacity * 2u;
	if (new_cap > XENT_CACHE_MAX_CAP) new_cap = XENT_CACHE_MAX_CAP;
	return xent_text_cache_rehash(cache, new_cap);
}

static bool xent_shape_cache_ensure_space(XentShapeCache *cache) {
	if (cache->capacity == 0) {
		cache->entries = ( XentShapeCacheEntry * ) calloc(XENT_CACHE_INIT_CAP, sizeof(*cache->entries));
		if (!cache->entries) return false;
		cache->capacity = XENT_CACHE_INIT_CAP;
		return true;
	}

	if (cache->count * 4u < cache->capacity * 3u) return true;

	if (cache->capacity >= XENT_CACHE_MAX_CAP) {
		xent_shape_cache_clear_entries(cache->entries, cache->capacity);
		cache->stats.evictions += cache->count;
		cache->count            = 0;
		return true;
	}

	uint32_t new_cap = cache->capacity * 2u;
	if (new_cap > XENT_CACHE_MAX_CAP) new_cap = XENT_CACHE_MAX_CAP;
	return xent_shape_cache_rehash(cache, new_cap);
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
	if (cache->entries) xent_text_cache_clear_entries(cache->entries, cache->capacity);
	free(cache->entries);
	memset(cache, 0, sizeof(*cache));
}

void xent_shape_cache_destroy(XentShapeCache *cache) {
	if (!cache) return;
	if (cache->entries) xent_shape_cache_clear_entries(cache->entries, cache->capacity);
	free(cache->entries);
	memset(cache, 0, sizeof(*cache));
}

bool xent_text_cache_lookup(XentTextCache *cache, XentTextCacheKey const *key, XentTextMetrics *out_metrics) {
	if (!cache || !key || !key->text || !out_metrics) return false;
	if (cache->capacity == 0) {
		cache->stats.misses += 1u;
		return false;
	}

	uint64_t key_hash = xent_hash_text_cache_key(key);
	uint32_t mask     = cache->capacity - 1u;
	uint32_t slot     = ( uint32_t ) (key_hash & mask);

	for (;;) {
		XentTextCacheEntry *entry = &cache->entries [slot];
		if (entry->key.hash == 0) break;
		if (xent_cached_text_key_matches(&entry->key, key, key_hash)) {
			*out_metrics       = entry->metrics;
			cache->stats.hits += 1u;
			return true;
		}
		slot = (slot + 1u) & mask;
	}

	cache->stats.misses += 1u;
	return false;
}

bool xent_shape_cache_lookup(XentShapeCache *cache, XentTextCacheKey const *key, XentShapingResult *out_result) {
	if (!cache || !key || !key->text || !out_result) return false;
	if (cache->capacity == 0) {
		cache->stats.misses += 1u;
		return false;
	}

	uint64_t key_hash = xent_hash_text_cache_key(key);
	uint32_t mask     = cache->capacity - 1u;
	uint32_t slot     = ( uint32_t ) (key_hash & mask);

	for (;;) {
		XentShapeCacheEntry *entry = &cache->entries [slot];
		if (entry->key.hash == 0) break;
		if (xent_cached_text_key_matches(&entry->key, key, key_hash)) {
			*out_result        = entry->result;
			cache->stats.hits += 1u;
			return true;
		}
		slot = (slot + 1u) & mask;
	}

	cache->stats.misses += 1u;
	return false;
}

void xent_text_cache_insert(XentTextCache *cache, XentTextCacheKey const *key, XentTextMetrics const *metrics) {
	if (!cache || !key || !key->text || !metrics) return;
	if (!xent_text_cache_ensure_space(cache)) return;

	uint64_t key_hash = xent_hash_text_cache_key(key);
	uint32_t mask     = cache->capacity - 1u;
	uint32_t slot     = ( uint32_t ) (key_hash & mask);

	while (cache->entries [slot].key.hash != 0) {
		if (xent_cached_text_key_matches(&cache->entries [slot].key, key, key_hash)) return;
		slot = (slot + 1u) & mask;
	}

	XentTextCacheEntry *entry = &cache->entries [slot];
	if (!xent_cached_text_key_copy(&entry->key, key, key_hash)) return;

	entry->metrics        = *metrics;
	cache->count         += 1u;
	cache->stats.inserts += 1u;
}

void xent_shape_cache_insert(XentShapeCache *cache, XentTextCacheKey const *key, XentShapingResult const *result) {
	if (!cache || !key || !key->text || !result) return;
	if (!xent_shape_cache_ensure_space(cache)) return;

	uint64_t key_hash = xent_hash_text_cache_key(key);
	uint32_t mask     = cache->capacity - 1u;
	uint32_t slot     = ( uint32_t ) (key_hash & mask);

	while (cache->entries [slot].key.hash != 0) {
		if (xent_cached_text_key_matches(&cache->entries [slot].key, key, key_hash)) return;
		slot = (slot + 1u) & mask;
	}

	XentShapeCacheEntry *entry = &cache->entries [slot];
	if (!xent_cached_text_key_copy(&entry->key, key, key_hash)) return;

	entry->result         = *result;
	cache->count         += 1u;
	cache->stats.inserts += 1u;
}
