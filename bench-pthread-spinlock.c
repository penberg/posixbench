#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>

static pthread_spinlock_t lock;

__attribute__((constructor))
static void init(void)
{
    if (pthread_spin_init( &lock, 0) != 0) {
       assert(0);
    }
}

#define BENCHMARK_ACTION() \
	do { \
		pthread_spin_lock(&lock); \
		pthread_spin_unlock(&lock); \
	} while (0)

#include "mt-benchmark.h"
