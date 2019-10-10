#define _GNU_SOURCE
#include <pthread.h>

static pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;

#define BENCHMARK_ACTION() \
	do { \
		pthread_rwlock_rdlock(&lock); \
		pthread_rwlock_unlock(&lock); \
	} while (0)

#include "benchmark.h"
