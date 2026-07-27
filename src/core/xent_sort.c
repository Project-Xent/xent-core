#if defined(__linux__) || defined(__GLIBC__)
  #ifndef _GNU_SOURCE
	#define _GNU_SOURCE
  #endif
#endif

#include "../xent_internal.h"

typedef struct XentSortThunk {
	XentSortCompareFn compare;
	void             *context;
} XentSortThunk;

#if defined(_MSC_VER)
static int sort_compare_msvc(void *context, void const *a, void const *b) {
	XentSortThunk *thunk = ( XentSortThunk * ) context;
	return thunk->compare(a, b, thunk->context);
}
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
static int sort_compare_bsd(void *context, void const *a, void const *b) {
	XentSortThunk *thunk = ( XentSortThunk * ) context;
	return thunk->compare(a, b, thunk->context);
}
#elif defined(__GLIBC__) || defined(__linux__)
static int sort_compare_gnu(void const *a, void const *b, void *context) {
	XentSortThunk *thunk = ( XentSortThunk * ) context;
	return thunk->compare(a, b, thunk->context);
}
#else
static void sort_swap(uint8_t *a, uint8_t *b, uint8_t *tmp, size_t size) {
	memcpy(tmp, a, size);
	memcpy(a, b, size);
	memcpy(b, tmp, size);
}

static void sort_fallback(void *base, size_t count, size_t size, XentSortCompareFn compare, void *context) {
	uint8_t *items = ( uint8_t * ) base;
	uint8_t *tmp   = ( uint8_t * ) malloc(size);
	if (!tmp) return;

	for (size_t i = 1u; i < count; ++i) {
		size_t j = i;
		while (j > 0u && compare(items + j * size, items + (j - 1u) * size, context) < 0) {
			sort_swap(items + j * size, items + (j - 1u) * size, tmp, size);
			j -= 1u;
		}
	}

	free(tmp);
}
#endif

void xent_sort_r(void *base, size_t count, size_t size, XentSortCompareFn compare, void *context) {
	if (!base || !compare || count < 2u || size == 0u) return;

	XentSortThunk thunk = {compare, context};
#if defined(_MSC_VER)
	qsort_s(base, count, size, sort_compare_msvc, &thunk);
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	qsort_r(base, count, size, &thunk, sort_compare_bsd);
#elif defined(__GLIBC__) || defined(__linux__)
	qsort_r(base, count, size, sort_compare_gnu, &thunk);
#else
	sort_fallback(base, count, size, compare, context);
#endif
}
