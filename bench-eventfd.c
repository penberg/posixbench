/*
 * A multi-threaded eventfd benchmark to measure remote thread wakeup latency.
 *
 * In this benchmark, one thread serves as the measuring thread, which does
 * the following:
 *
 *   (1) Query current timestamp
 *   (2) Wake up a remote thread with eventfd_write()
 *   (3) Perform blocking eventfd_read() that reads a timestamp written by
 *       remote thread
 *
 * In addition, there are one or more intefering thread, which do:
 *
 *   (1) Perform a blocking eventfd_read() that reads the remote thread efd
 *   (2) Query current timestamp
 *   (3) Write current timestamp to remote thread with eventfd_read()
 */

#define _GNU_SOURCE
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/eventfd.h>
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

struct config {
	/* The eventfd descriptor of this thread.  */
	int self_efd;
	/* The eventfd descriptors of remote threads.  */
	int *remote_efds;
	size_t nr_remote_efds;
	int nr_cpus;
	int nr_threads_per_cpu;
};

static void *measuring_thread_run(void *arg)
{
	struct hdr_histogram *hist;
	if (hdr_init(1, 10000000, 3, &hist)) {
		assert(0);
	}
	struct config *cfg = arg;
	size_t remote_idx = 0;
	for (size_t i = 0; i < 1000000; i += 1) {
		struct timespec start;
		if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
			assert(0);
		}
		if (eventfd_write(cfg->remote_efds[remote_idx], cfg->self_efd) < 0) {
			assert(0);
		}
		eventfd_t end_ns = 0;
		if (eventfd_read(cfg->self_efd, &end_ns) < 0) {
			assert(0);
		}
		uint64_t start_ns = timespec_to_ns(&start);
		hdr_record_value(hist, end_ns - start_ns);
		remote_idx = (remote_idx + 1) % cfg->nr_remote_efds;
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
	long efd = (long)arg;
	for (;;) {
		eventfd_t fd = 0;
		if (eventfd_read(efd, &fd) < 0) {
			assert(0);
		}
		struct timespec now;
		if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) {
			assert(0);
		}
		if (eventfd_write(fd, timespec_to_ns(&now)) < 0) {
			assert(0);
		}
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
	/* Place the measuring thread on other CPU than CPU0 to avoid measuring
	   kernel background thread interference.  */
	int measuring_cpu = nr_cpus - 1;
	int measuring_thread = -1;

	int max_threads = nr_cpus * nr_threads_per_cpu;
	pthread_t threads[max_threads];

	int nr_remote_efds = max_threads - 1;
	int remote_efds[nr_remote_efds];
	int *remote_efd = remote_efds;

	int measuring_efd = eventfd(0, 0);
	assert(measuring_efd > 0);

	for (int i = 0; i < nr_remote_efds; i++) {
		int efd = eventfd(0, 0);
		assert(efd > 0);
		remote_efds[i] = efd;
	}

	int thread_idx = 0;
	for (int cpu = 0; cpu < nr_cpus; cpu++) {
		int nr_threads = nr_threads_per_cpu;

		if (cpu == measuring_cpu) {
			/* Measuring CPU has one less interfering thread.  */
			nr_threads--;

			struct config *cfg = malloc(sizeof(*cfg));
			assert(cfg != NULL);
			cfg->self_efd = measuring_efd;
			cfg->remote_efds = remote_efds;
			cfg->nr_remote_efds = nr_remote_efds;
			cfg->nr_cpus = nr_cpus;
			cfg->nr_threads_per_cpu = nr_threads_per_cpu;
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			set_attr_affinity(&attr, cpu);
			int err = pthread_create(&threads[thread_idx], &attr, measuring_thread_run, cfg);
			if (err) {
				assert(0);
			}
			measuring_thread = thread_idx;
			thread_idx++;
		}
		for (int i = 0; i < nr_threads; i++) {
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			set_attr_affinity(&attr, cpu);
			int err = pthread_create(&threads[thread_idx], &attr, interfering_thread_run,
						 (void *)(long)*remote_efd++);
			if (err) {
				assert(0);
			}
			thread_idx++;
		}
	}

	int err = pthread_join(threads[measuring_thread], NULL);
	if (err) {
		assert(0);
	}

	for (int i = 0; i < max_threads; i++) {
		if (i == measuring_thread) {
			continue;
		}
		if (pthread_cancel(threads[i]) < 0) {
			assert(0);
		}
		if (pthread_join(threads[i], NULL) < 0) {
			assert(0);
		}
	}

	close(measuring_efd);

	for (int i = 0; i < nr_remote_efds; i++) {
		close(remote_efds[i]);
	}
}

int main(int argc, char *argv[])
{
	printf("cpus,threadspercpu,percentile,time\n");

	int max_cpus = sysconf(_SC_NPROCESSORS_ONLN);

	for (int nr_cpus = 2; nr_cpus <= max_cpus; nr_cpus += 1) {
		measure_action(nr_cpus, 1);
		measure_action(nr_cpus, 2);
	}
	return 0;
}
