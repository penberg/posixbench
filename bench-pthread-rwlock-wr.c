#define _GNU_SOURCE
#include <pthread.h>

static pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;

#define BENCHMARK_ACTION() \
	do { \
		pthread_rwlock_wrlock(&lock); \
		pthread_rwlock_unlock(&lock); \
	} while (0)

#include "mt-benchmark.h"
