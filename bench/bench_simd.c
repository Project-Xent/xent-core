#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "xent/xent.h"

float xent_simd_sum_f32(float const *values, unsigned int count);
void  xent_simd_fill_f32(float *values, unsigned int count, float value);
bool  xent_is_simd_enabled(void);

#ifdef _WIN32
  #include <windows.h>

static double now_ns(void) {
	static double freq = 0.0;
	if (freq == 0.0) {
		LARGE_INTEGER f;
		QueryPerformanceFrequency(&f);
		freq = ( double ) f.QuadPart;
	}
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	return ( double ) t.QuadPart / freq * 1.0e9;
}
#else
static double now_ns(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ( double ) ts.tv_sec * 1.0e9 + ( double ) ts.tv_nsec;
}
#endif

static void fill_random(float *buf, unsigned int count) {
	for (unsigned int i = 0; i < count; i++) buf [i] = ( float ) rand() / ( float ) RAND_MAX * 200.0f - 100.0f;
}

typedef struct {
	double min_ns;
	double med_ns;
	double avg_ns;
} BenchResult;

static int compare_double_ascending(void const *lhs, void const *rhs) {
	double a = *( double const * ) lhs;
	double b = *( double const * ) rhs;
	return (a > b) - (a < b);
}

static void sort_samples(double *samples, int iterations) {
	qsort(samples, ( size_t ) iterations, sizeof(double), compare_double_ascending);
}

static BenchResult bench_sum(float *buf, unsigned int count, int iterations) {
	double        *samples = ( double * ) malloc(sizeof(double) * ( size_t ) iterations);

	volatile float sink    = 0.0f;
	for (int w = 0; w < 3; w++) sink += xent_simd_sum_f32(buf, count);
	( void ) sink;

	for (int i = 0; i < iterations; i++) {
		double         t0 = now_ns();
		volatile float s  = xent_simd_sum_f32(buf, count);
		double         t1 = now_ns();
		( void ) s;
		samples [i] = t1 - t0;
	}

	sort_samples(samples, iterations);

	BenchResult r;
	r.min_ns = samples [0];
	r.med_ns = samples [iterations / 2];
	r.avg_ns = 0.0;
	for (int i = 0; i < iterations; i++) r.avg_ns += samples [i];
	r.avg_ns /= ( double ) iterations;

	free(samples);
	return r;
}

static BenchResult bench_fill(float *buf, unsigned int count, float value, int iterations) {
	double *samples = ( double * ) malloc(sizeof(double) * ( size_t ) iterations);

	for (int w = 0; w < 3; w++) xent_simd_fill_f32(buf, count, value);

	for (int i = 0; i < iterations; i++) {
		double t0 = now_ns();
		xent_simd_fill_f32(buf, count, value);
		double t1   = now_ns();
		samples [i] = t1 - t0;
	}

	sort_samples(samples, iterations);

	BenchResult r;
	r.min_ns = samples [0];
	r.med_ns = samples [iterations / 2];
	r.avg_ns = 0.0;
	for (int i = 0; i < iterations; i++) r.avg_ns += samples [i];
	r.avg_ns /= ( double ) iterations;

	free(samples);
	return r;
}

static char const *fmt_ns(double ns, char *buf, size_t bufsz) {
	if (ns < 1000.0) snprintf(buf, bufsz, "%7.1f ns", ns);
	else if (ns < 1.0e6) snprintf(buf, bufsz, "%7.2f us", ns / 1.0e3);
	else snprintf(buf, bufsz, "%7.2f ms", ns / 1.0e6);
	return buf;
}

static char const *fmt_throughput(unsigned int count, double ns, char *buf, size_t bufsz) {
	double bytes    = ( double ) count * sizeof(float);
	double gb_per_s = bytes / ns;
	snprintf(buf, bufsz, "%6.2f GB/s", gb_per_s);
	return buf;
}

int main(void) {
	srand(42);

	printf("=== xent-core SIMD microbenchmark ===\n");
	printf("backend: %s\n\n", xent_is_simd_enabled() ? "ISPC (sse4+avx2)" : "scalar fallback");

	unsigned int sizes []  = {64, 256, 1024, 4096, 16384, 65536, 262144, 1048576};
	int          nsizes    = ( int ) (sizeof(sizes) / sizeof(sizes [0]));
	int          iters     = 500;

	unsigned int max_count = sizes [nsizes - 1];
	float       *buf       = ( float * ) malloc(sizeof(float) * max_count);
	if (!buf) {
		fprintf(stderr, "allocation failed\n");
		return 1;
	}

	printf("--- sum_f32 ---\n");
	printf("  %10s  %12s  %12s  %12s  %12s\n", "count", "min", "median", "avg", "throughput");
	for (int si = 0; si < nsizes; si++) {
		unsigned int count = sizes [si];
		fill_random(buf, count);
		BenchResult r = bench_sum(buf, count, iters);
		char        b1 [32], b2 [32], b3 [32], b4 [32];
		printf(
		  "  %10u  %12s  %12s  %12s  %12s\n", count, fmt_ns(r.min_ns, b1, sizeof(b1)), fmt_ns(r.med_ns, b2, sizeof(b2)),
		  fmt_ns(r.avg_ns, b3, sizeof(b3)), fmt_throughput(count, r.med_ns, b4, sizeof(b4))
		);
	}
	printf("\n");

	printf("--- fill_f32 ---\n");
	printf("  %10s  %12s  %12s  %12s  %12s\n", "count", "min", "median", "avg", "throughput");
	for (int si = 0; si < nsizes; si++) {
		unsigned int count = sizes [si];
		memset(buf, 0, sizeof(float) * count);
		BenchResult r = bench_fill(buf, count, 3.14f, iters);
		char        b1 [32], b2 [32], b3 [32], b4 [32];
		printf(
		  "  %10u  %12s  %12s  %12s  %12s\n", count, fmt_ns(r.min_ns, b1, sizeof(b1)), fmt_ns(r.med_ns, b2, sizeof(b2)),
		  fmt_ns(r.avg_ns, b3, sizeof(b3)), fmt_throughput(count, r.med_ns, b4, sizeof(b4))
		);
	}
	printf("\n");

	free(buf);
	return 0;
}
