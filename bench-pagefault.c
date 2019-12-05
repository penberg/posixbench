#define _GNU_SOURCE
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <hdr_histogram.h>
#include <hdr_interval_recorder.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

static uint64_t timespec_to_ns(struct timespec *ts)
{
	return ts->tv_sec * 1e9 + ts->tv_nsec;
}

static uint64_t time_diff(struct timespec *start, struct timespec *end)
{
	uint64_t start_ns = timespec_to_ns(start);
	uint64_t end_ns = timespec_to_ns(end);
	assert(start_ns < end_ns);
	return end_ns - start_ns;
}

struct measuring_thread_config {
	int nr_cpus;
	int nr_threads_per_cpu;
};

const size_t size = 1024 * 1024; /* 1 MB */

static void *measuring_thread_run(void *arg)
{
	char *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	assert(p != MAP_FAILED);

	struct hdr_histogram *hist;
	if (hdr_init(1, 1000000, 3, &hist)) {
		assert(0);
	}
	struct measuring_thread_config *cfg = arg;
	for (size_t i = 0; i < size; i += 4096) {
		struct timespec start;
		if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
			assert(0);
		}
		p[i] = '\0';
		struct timespec end;
		if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
			assert(0);
		}
		hdr_record_value(hist, time_diff(&start, &end));
	}
	for (size_t percentile = 1; percentile < 100; percentile++) {
		printf("%d,%d,%f,%ld\n", cfg->nr_cpus, cfg->nr_threads_per_cpu, (double)percentile,
		       hdr_value_at_percentile(hist, percentile));
	}
	double tail_percentiles[] = {
	    99.9,
	    99.99,
	    99.999,
	    100,
	};
	for (size_t i = 0; i < ARRAY_SIZE(tail_percentiles); i++) {
		printf("%d,%d,%f,%ld\n", cfg->nr_cpus, cfg->nr_threads_per_cpu, tail_percentiles[i],
		       hdr_value_at_percentile(hist, tail_percentiles[i]));
	}
	return NULL;
}

static void *interfering_thread_run(void *arg)
{
	char *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	assert(p != MAP_FAILED);

	for (size_t i = 0; i < size; i += 4096) {
		p[i] = '\0';
	}
	return 0;
}

static void set_attr_affinity(pthread_attr_t *attr, int cpu)
{
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);
	pthread_attr_setaffinity_np(attr, sizeof(cpu_set_t), &cpuset);
}

static void measure_action(int nr_cpus, int nr_threads_per_cpu)
{
	struct measuring_thread_config cfg = {
	    .nr_cpus = nr_cpus,
	    .nr_threads_per_cpu = nr_threads_per_cpu,
	};

	/* Place the measuring thread on other CPU than CPU0 to avoid measuring
	   kernel background thread interference.  */
	int measuring_cpu = nr_cpus - 1;

	pthread_t threads[nr_cpus * nr_threads_per_cpu];
	int thread_count = 0;

	for (int cpu = 0; cpu < nr_cpus; cpu++) {
		int nr_threads = nr_threads_per_cpu;

		/* Measuring CPU has one less interfering thread.  */
		if (cpu == measuring_cpu) {
			nr_threads--;
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			set_attr_affinity(&attr, cpu);
			int err = pthread_create(&threads[thread_count++], &attr, measuring_thread_run, &cfg);
			if (err) {
				assert(0);
			}
		}
		for (int i = 0; i < nr_threads; i++) {
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			set_attr_affinity(&attr, cpu);
			int err = pthread_create(&threads[thread_count++], &attr, interfering_thread_run, NULL);
			if (err) {
				assert(0);
			}
		}
	}

	for (int i = 0; i < thread_count; i++) {
		int err = pthread_join(threads[i], NULL);
		if (err) {
			assert(0);
		}
	}
}

int main(int argc, char *argv[])
{
	printf("cpus,threadspercpu,percentile,time\n");

	int max_cpus = sysconf(_SC_NPROCESSORS_ONLN);

	for (int nr_cpus = 1; nr_cpus <= max_cpus; nr_cpus += 1) {
		measure_action(nr_cpus, 1);
	}
	return 0;
}
