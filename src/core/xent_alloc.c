#include "../xent_alloc_internal.h"

#include <stdbool.h>
#include <stdlib.h>

#if defined(XENT_ENABLE_FAULT_INJECTION)
typedef struct XentAllocFailure {
	XentAllocSite site;
	uint32_t      remaining;
	bool          armed;
} XentAllocFailure;

static XentAllocFailure xent_alloc_failure;

static bool             alloc_should_fail(XentAllocSite site) {
	if (!xent_alloc_failure.armed || xent_alloc_failure.site != site) return false;
	if (xent_alloc_failure.remaining > 0u) {
		xent_alloc_failure.remaining--;
		return false;
	}
	xent_alloc_failure.armed = false;
	return true;
}

void xent_alloc_fail_after(XentAllocSite site, uint32_t successful_allocations) {
	xent_alloc_failure = (XentAllocFailure) {site, successful_allocations, true};
}

void xent_test_alloc_reset(void) { xent_alloc_failure = (XentAllocFailure) {0}; }
#else
static bool alloc_should_fail(XentAllocSite site) {
	( void ) site;
	return false;
}
#endif

void *xent_alloc_internal(XentAllocSite site, size_t size) {
	if (alloc_should_fail(site)) return NULL;
	return malloc(size);
}

void *xent_realloc_internal(XentAllocSite site, void *ptr, size_t size) {
	if (alloc_should_fail(site)) return NULL;
	return realloc(ptr, size);
}
