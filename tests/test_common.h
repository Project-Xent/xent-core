#ifndef XENT_TEST_COMMON_H
#define XENT_TEST_COMMON_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "xent/xent.h"

#define TEST_ASSERT(expr)                                                                                                 \
    do {                                                                                                                   \
        if (!(expr)) {                                                                                                     \
            fprintf(stderr, "ASSERT FAILED: %s (%s:%d)\n", #expr, __FILE__, __LINE__);                                   \
            return 1;                                                                                                      \
        }                                                                                                                  \
    } while (0)

static inline int test_float_near(float a, float b, float eps) {
    return fabsf(a - b) <= eps;
}

#endif
