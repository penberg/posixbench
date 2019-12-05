#define _GNU_SOURCE
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
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

static int nr_iter = 100000;

static void *measuring_thread_run(void *arg)
{
	struct hdr_histogram *hist;
	if (hdr_init(1, 1000000, 3, &hist)) {
		assert(0);
	}
	struct measuring_thread_config *cfg = arg;
	for (int i = 0; i < nr_iter / 2; i++) {
		struct timespec start;
		if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
			assert(0);
		}
		BENCHMARK_ACTION();
		struct timespec end;
		if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
			assert(0);
		}
		hdr_record_value(hist, time_diff(&start, &end));
	}
	for (size_t percentile = 1; percentile < 100; percentile++) {
		printf("%d,%d,%f,%ld\n", cfg->nr_cpus, cfg->nr_threads_per_cpu, (double)percentile, hdr_value_at_percentile(hist, percentile));
	}
	double tail_percentiles[] = {
	    99.9,
	    99.99,
	    99.999,
	    100,
	};
	for (size_t i = 0; i < ARRAY_SIZE(tail_percentiles); i++) {
		printf("%d,%d,%f,%ld\n", cfg->nr_cpus, cfg->nr_threads_per_cpu, tail_percentiles[i], hdr_value_at_percentile(hist, tail_percentiles[i]));
	}
	fflush(stdout);
	return NULL;
}

static void *interfering_thread_run(void *arg)
{
	for (int i = 0; i < nr_iter; i++) {
		BENCHMARK_ACTION();
	}
	return 0;
}

static void set_attr_affinity(int cpu)
{
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

static void measure_action(int nr_cpus)
{
	struct measuring_thread_config cfg = {
		.nr_cpus = nr_cpus,
		.nr_threads_per_cpu = 1,
	};

	/* Place the measuring thread on other CPU than CPU0 to avoid measuring
	   kernel background thread interference.  */
	int measuring_cpu = nr_cpus - 1;

	pid_t threads[nr_cpus];
	int thread_count = 0;

	for (int cpu = 0; cpu < nr_cpus; cpu++) {
		pid_t pid = fork();
		if (pid != 0) {
			/* parent */
			threads[cpu] = pid;
			continue;
		}
		/* child */
		set_attr_affinity(cpu);
		if (cpu == measuring_cpu) {
			measuring_thread_run(&cfg);
			exit(0);
		} else {
			interfering_thread_run(NULL);
			exit(0);
		}
	}

	wait(NULL);
#if 0
	for (int i = 0; i < thread_count; i++) {
		int err = waitpid(threads[i], NULL, 1);
		if (err < 0) {
			assert(0);
		}
	}
#endif
}

int main(int argc, char *argv[])
{
	printf("cpus,threadspercpu,percentile,time\n");
	fflush(stdout);

	int max_cpus = sysconf(_SC_NPROCESSORS_ONLN);

	for (int nr_cpus = 1; nr_cpus <= max_cpus; nr_cpus += 1) {
		measure_action(nr_cpus);
	}
	return 0;
}
