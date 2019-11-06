#define _GNU_SOURCE
#include <pthread.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

#define BENCHMARK_ACTION() \
	do { \
		pthread_mutex_lock(&lock); \
		pthread_mutex_unlock(&lock); \
	} while (0)

#include "mt-benchmark.h"
