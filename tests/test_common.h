#ifndef XENT_TEST_COMMON_H
#define XENT_TEST_COMMON_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "xent/xent.h"

typedef int (*XentTestFn)(void);

#define TEST_ASSERT(expr)                                                              \
	do {                                                                               \
		if (!(expr)) {                                                                 \
			fprintf(stderr, "ASSERT FAILED: %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
			return 1;                                                                  \
		}                                                                              \
	}                                                                                  \
	while (0)

static inline int test_float_near(float a, float b, float eps) { return fabsf(a - b) <= eps; }

static inline int test_run_all(XentTestFn const *tests, size_t count) {
	for (size_t i = 0; i < count; ++i)
		if (tests [i]() != 0) return 1;
	return 0;
}

#endif
