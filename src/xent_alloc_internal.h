#ifndef XENT_ALLOC_INTERNAL_H
#define XENT_ALLOC_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

typedef enum XentAllocSite
{
	XENT_ALLOC_NODE_GROW,
	XENT_ALLOC_SCRATCH_ARENA,
	XENT_ALLOC_TOPOLOGY_MUTATION,
} XentAllocSite;

void *xent_alloc_internal(XentAllocSite site, size_t size);
void *xent_realloc_internal(XentAllocSite site, void *ptr, size_t size);

#if defined(XENT_ENABLE_FAULT_INJECTION)
void xent_alloc_fail_after(XentAllocSite site, uint32_t successful_allocations);
void xent_test_alloc_reset(void);
#endif

#endif
