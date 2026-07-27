#include "../xent_internal.h"

typedef struct XentCacheOps {
	size_t entry_size;
	size_t last_used_offset;
} XentCacheOps;

typedef struct XentCacheStorage {
	void              *entries;
	uint32_t           count;
	uint32_t           capacity;
	uint64_t           clock;
	XentTextCacheStats stats;
} XentCacheStorage;

static XentCacheOps const XENT_TEXT_CACHE_OPS = {sizeof(XentTextCacheEntry), offsetof(XentTextCacheEntry, last_used)};

static bool               float_bits_equal(float a, float b) { return memcmp(&a, &b, sizeof(float)) == 0; }

static void              *cache_entry_at(void *entries, XentCacheOps const *ops, uint32_t slot) {
	return ( uint8_t * ) entries + ( size_t ) slot * ops->entry_size;
}

static XentCachedTextKey *cache_key_at(void *entries, XentCacheOps const *ops, uint32_t slot) {
	return ( XentCachedTextKey * ) cache_entry_at(entries, ops, slot);
}

static uint64_t *cache_last_used_at(void *entry, XentCacheOps const *ops) {
	return ( uint64_t * ) (( uint8_t * ) entry + ops->last_used_offset);
}

static uint64_t hash_bytes(uint64_t hash, void const *data, size_t size) {
	uint8_t const *bytes = ( uint8_t const * ) data;
	for (size_t i = 0; i < size; ++i) {
		hash ^= bytes [i];
		hash *= 1099511628211ull;
	}
	return hash;
}

static uint64_t hash_text_cache_key(XentTextCacheKey const *key) {
	uint64_t hash              = 1469598103934665603ull;
	uint8_t  line_break_policy = ( uint8_t ) key->line_break_policy;
	uint8_t  width_mode        = ( uint8_t ) key->width_mode;
	hash                       = hash_bytes(hash, key->text, strlen(key->text));
	hash                       = hash_bytes(hash, &key->font_size, sizeof(key->font_size));
	hash                       = hash_bytes(hash, &key->font_weight, sizeof(key->font_weight));
	hash                       = hash_bytes(hash, &key->width_constraint, sizeof(key->width_constraint));
	hash                       = hash_bytes(hash, &line_break_policy, sizeof(line_break_policy));
	hash                       = hash_bytes(hash, &width_mode, sizeof(width_mode));
	return hash ? hash : 1ull;
}

static bool cached_text_key_matches(XentCachedTextKey const *cached, XentTextCacheKey const *key, uint64_t key_hash) {
	return cached->hash == key_hash
	    && float_bits_equal(cached->font_size, key->font_size)
	    && cached->font_weight == key->font_weight
	    && float_bits_equal(cached->width_constraint, key->width_constraint)
	    && cached->line_break_policy == ( uint8_t ) key->line_break_policy
	    && cached->width_mode == ( uint8_t ) key->width_mode
	    && strcmp(cached->text, key->text) == 0;
}

static bool cached_text_key_copy(XentCachedTextKey *cached, XentTextCacheKey const *key, uint64_t key_hash) {
	cached->text = xent_strdup(key->text);
	if (!cached->text) return false;

	cached->hash              = key_hash;
	cached->font_size         = key->font_size;
	cached->font_weight       = key->font_weight;
	cached->width_constraint  = key->width_constraint;
	cached->line_break_policy = ( uint8_t ) key->line_break_policy;
	cached->width_mode        = ( uint8_t ) key->width_mode;
	return true;
}

static void cache_clear_entries(void *entries, uint32_t capacity, XentCacheOps const *ops) {
	for (uint32_t i = 0u; i < capacity; ++i) {
		XentCachedTextKey *key = cache_key_at(entries, ops, i);
		if (key->hash == 0u) continue;
		free(key->text);
		memset(cache_entry_at(entries, ops, i), 0, ops->entry_size);
	}
}

static bool cache_slot(
  XentCacheStorage const *cache, XentCacheOps const *ops, XentTextCacheKey const *key, uint64_t key_hash,
  uint32_t *out_slot, bool *out_found
) {
	if (cache->capacity == 0u) return false;
	uint32_t mask = cache->capacity - 1u;
	uint32_t slot = ( uint32_t ) (key_hash & mask);
	for (uint32_t probe = 0u; probe < cache->capacity; ++probe) {
		XentCachedTextKey *cached = cache_key_at(cache->entries, ops, slot);
		if (cached->hash == 0u) {
			*out_slot  = slot;
			*out_found = false;
			return true;
		}
		if (cached_text_key_matches(cached, key, key_hash)) {
			*out_slot  = slot;
			*out_found = true;
			return true;
		}
		slot = (slot + 1u) & mask;
	}
	return false;
}

static void cache_place_existing(void *entries, uint32_t capacity, XentCacheOps const *ops, void const *entry) {
	uint32_t           mask = capacity - 1u;
	XentCachedTextKey *key  = ( XentCachedTextKey * ) entry;
	uint32_t           slot = ( uint32_t ) (key->hash & mask);
	while (cache_key_at(entries, ops, slot)->hash != 0u) slot = (slot + 1u) & mask;
	memcpy(cache_entry_at(entries, ops, slot), entry, ops->entry_size);
}

static bool cache_rehash(XentCacheStorage *cache, XentCacheOps const *ops, uint32_t new_cap) {
	void *new_entries = calloc(new_cap, ops->entry_size);
	if (!new_entries) return false;

	for (uint32_t i = 0u; i < cache->capacity; ++i) {
		void *entry = cache_entry_at(cache->entries, ops, i);
		if ((( XentCachedTextKey * ) entry)->hash != 0u) cache_place_existing(new_entries, new_cap, ops, entry);
	}

	free(cache->entries);
	cache->entries  = new_entries;
	cache->capacity = new_cap;
	return true;
}

static int compare_u64_asc(void const *a, void const *b) {
	uint64_t av = *( uint64_t const * ) a;
	uint64_t bv = *( uint64_t const * ) b;
	return (av > bv) - (av < bv);
}

static void cache_evict_all(XentCacheStorage *cache, XentCacheOps const *ops) {
	uint32_t before = cache->count;
	cache_clear_entries(cache->entries, cache->capacity, ops);
	cache->count            = 0u;
	cache->stats.evictions += before;
}

static bool cache_lru_cutoff(XentCacheStorage const *cache, XentCacheOps const *ops, uint32_t target, uint64_t *out) {
	uint64_t *ages = ( uint64_t * ) malloc(sizeof(*ages) * ( size_t ) cache->count);
	if (!ages) return false;

	uint32_t age_count = 0u;
	for (uint32_t i = 0u; i < cache->capacity; ++i) {
		void *entry = cache_entry_at(cache->entries, ops, i);
		if ((( XentCachedTextKey * ) entry)->hash != 0u) ages [age_count++] = *cache_last_used_at(entry, ops);
	}
	qsort(ages, age_count, sizeof(*ages), compare_u64_asc);
	*out = ages [age_count - target - 1u];
	free(ages);
	return true;
}

static uint32_t
cache_retain_newer(void *old_entries, void *new_entries, uint32_t capacity, XentCacheOps const *ops, uint64_t cutoff) {
	uint32_t kept = 0u;
	for (uint32_t i = 0u; i < capacity; ++i) {
		void              *entry = cache_entry_at(old_entries, ops, i);
		XentCachedTextKey *key   = ( XentCachedTextKey * ) entry;
		if (key->hash == 0u) continue;
		if (*cache_last_used_at(entry, ops) <= cutoff) {
			free(key->text);
			continue;
		}
		cache_place_existing(new_entries, capacity, ops, entry);
		kept += 1u;
	}
	return kept;
}

static void cache_evict_half(XentCacheStorage *cache, XentCacheOps const *ops) {
	uint32_t target = XENT_CACHE_MAX_CAP / 2u;
	if (cache->count <= target) return;

	uint64_t cutoff = 0u;
	if (!cache_lru_cutoff(cache, ops, target, &cutoff)) {
		cache_evict_all(cache, ops);
		return;
	}

	void *new_entries = calloc(cache->capacity, ops->entry_size);
	if (!new_entries) {
		cache_evict_all(cache, ops);
		return;
	}

	uint32_t before = cache->count;
	uint32_t kept   = cache_retain_newer(cache->entries, new_entries, cache->capacity, ops, cutoff);
	free(cache->entries);
	cache->entries          = new_entries;
	cache->count            = kept;
	cache->stats.evictions += before - kept;
}

static bool cache_ensure_space(XentCacheStorage *cache, XentCacheOps const *ops) {
	if (cache->count >= XENT_CACHE_MAX_CAP) cache_evict_half(cache, ops);
	if (cache->capacity == 0u) {
		cache->entries = calloc(64u, ops->entry_size);
		if (!cache->entries) return false;
		cache->capacity = 64u;
		return true;
	}
	if (cache->count * 4u < cache->capacity * 3u || cache->capacity >= XENT_CACHE_MAX_CAP) return true;

	uint32_t new_cap = cache->capacity * 2u;
	if (new_cap > XENT_CACHE_MAX_CAP) new_cap = XENT_CACHE_MAX_CAP;
	return cache_rehash(cache, ops, new_cap);
}

static void *cache_lookup_entry(XentCacheStorage *cache, XentCacheOps const *ops, XentTextCacheKey const *key) {
	uint32_t slot     = 0u;
	bool     found    = false;
	uint64_t key_hash = hash_text_cache_key(key);
	if (!cache_slot(cache, ops, key, key_hash, &slot, &found) || !found) {
		cache->stats.misses += 1u;
		return NULL;
	}

	void *entry                      = cache_entry_at(cache->entries, ops, slot);
	*cache_last_used_at(entry, ops)  = ++cache->clock;
	cache->stats.hits               += 1u;
	return entry;
}

static void *cache_insert_entry(XentCacheStorage *cache, XentCacheOps const *ops, XentTextCacheKey const *key) {
	if (!cache_ensure_space(cache, ops)) return NULL;

	uint32_t slot     = 0u;
	bool     found    = false;
	uint64_t key_hash = hash_text_cache_key(key);
	if (!cache_slot(cache, ops, key, key_hash, &slot, &found)) return NULL;

	void *entry = cache_entry_at(cache->entries, ops, slot);
	if (!found && !cached_text_key_copy(( XentCachedTextKey * ) entry, key, key_hash)) return NULL;

	*cache_last_used_at(entry, ops) = ++cache->clock;
	if (!found) {
		cache->count         += 1u;
		cache->stats.inserts += 1u;
	}
	return entry;
}

static XentCacheStorage text_cache_storage(XentTextCache const *cache) {
	XentCacheStorage storage = {cache->entries, cache->count, cache->capacity, cache->clock, cache->stats};
	return storage;
}

static void text_cache_apply_storage(XentTextCache *cache, XentCacheStorage const *storage) {
	cache->entries  = ( XentTextCacheEntry * ) storage->entries;
	cache->count    = storage->count;
	cache->capacity = storage->capacity;
	cache->clock    = storage->clock;
	cache->stats    = storage->stats;
}

bool xent_text_cache_init(XentTextCache *cache) {
	if (!cache) return false;
	memset(cache, 0, sizeof(*cache));
	return true;
}

void xent_text_cache_destroy(XentTextCache *cache) {
	if (!cache) return;
	if (cache->entries) cache_clear_entries(cache->entries, cache->capacity, &XENT_TEXT_CACHE_OPS);
	free(cache->entries);
	memset(cache, 0, sizeof(*cache));
}

bool xent_text_cache_lookup(XentTextCache *cache, XentTextCacheKey const *key, XentTextMetrics *out_metrics) {
	if (!cache || !key || !key->text || !out_metrics) return false;

	XentCacheStorage    storage = text_cache_storage(cache);
	XentTextCacheEntry *entry   = ( XentTextCacheEntry * ) cache_lookup_entry(&storage, &XENT_TEXT_CACHE_OPS, key);
	text_cache_apply_storage(cache, &storage);
	if (!entry) return false;
	*out_metrics = entry->metrics;
	return true;
}

void xent_text_cache_insert(XentTextCache *cache, XentTextCacheKey const *key, XentTextMetrics const *metrics) {
	if (!cache || !key || !key->text || !metrics) return;

	XentCacheStorage    storage = text_cache_storage(cache);
	XentTextCacheEntry *entry   = ( XentTextCacheEntry * ) cache_insert_entry(&storage, &XENT_TEXT_CACHE_OPS, key);
	text_cache_apply_storage(cache, &storage);
	if (entry) entry->metrics = *metrics;
}
