#pragma once

#include <pthread.h>
#include <signal.h>
#include <time.h>

#include <array>
#include <atomic>
#include <cassert>
#include <list>
#include <optional>
#include <system_error>
#include <thread>

#include <hdr_histogram.h>
#include <hdr_interval_recorder.h>
#include <hwloc.h>

static uint64_t timespec_to_ns(struct timespec *ts) {
  return ts->tv_sec * 1e9 + ts->tv_nsec;
}

static uint64_t time_diff(struct timespec *start, struct timespec *end) {
  uint64_t start_ns = timespec_to_ns(start);
  uint64_t end_ns = timespec_to_ns(end);
  assert(start_ns < end_ns);
  return end_ns - start_ns;
}

enum Scenario {
  /// Interfering thread is running on a remote package (NUMA).
  REMOTE_PACKAGE,
  /// Interfering thread is running on a remote core (SMP).
  REMOTE_CORE,
  /// Interfering thread is running on the same core (SMT).
  LOCAL_CORE,
};

static const char *to_string(Scenario scenario) {
  switch (scenario) {
    case REMOTE_PACKAGE:
      return "NUMA";
    case REMOTE_CORE:
      return "Multicore";
    case LOCAL_CORE:
      return "SMT";
  }
  assert(0);
}

template <typename Operation>
struct SymmetricAction {
  void init() {}

  void release() {}

  uint64_t measured_operation() {
    struct timespec start;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
      assert(0);
    }
    Operation()();
    struct timespec end;
    if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
      assert(0);
    }
    return time_diff(&start, &end);
  }

  void other_operation(size_t tid) { Operation()(); }
};

template <typename Operation, typename State>
struct SymmetricActionWithState {
  State state;

  void init() {}

  void release() {}

  uint64_t measured_operation() {
    struct timespec start;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
      assert(0);
    }
    Operation()(state);
    struct timespec end;
    if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
      assert(0);
    }
    return time_diff(&start, &end);
  }

  void other_operation(size_t tid) { Operation()(state); }
};

struct Config {
  /// Test scenario.
  Scenario scenario;
  /// Number of interfering threads.
  size_t nr_threads = 1;
  /// Number of iterations.
  size_t nr_iter;
};

static void signal_handler(int signum) {
  /* Nothing to do. We send signals to threads to make sure they return from
     blocked system calls.  */
}

template <class Action>
class LatencyBenchmark {
  std::list<std::thread> _threads;

 public:
  void run(const Config &cfg) {
#if 0
    printf("# %s: Measuring thread on CPU %d, intefering thread on CPU %d\n",
           to_string(cfg.scenario), pu->os_index, (*other_pu)->os_index);
#endif
    hwloc_topology_t topology;
    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);
    hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, 0);
    auto other_pu = find_other_pu(topology, pu, cfg.scenario);
    if (!other_pu) {
      printf("Unable to find other PU\n");
      return;
    }
    Action action; /* FIXME: potentially wrong placement of data */
    action.init();
    std::atomic<bool> stop = false;
    std::thread t([&cfg, &topology, &pu, &action, &stop]() {
      hwloc_set_cpubind(topology, pu->cpuset, HWLOC_CPUBIND_THREAD);
      struct hdr_histogram *hist;
      if (hdr_init(1, 1000000, 3, &hist)) {
        assert(0);
      }
      for (size_t i = 0; i < cfg.nr_iter; i++) {
        auto diff = action.measured_operation();
        hdr_record_value(hist, diff);
      }
      stop.store(true);
      for (size_t percentile = 1; percentile < 100; percentile++) {
        printf("%s,%f,%ld\n", to_string(cfg.scenario), (double)percentile,
               hdr_value_at_percentile(hist, percentile));
      }
      std::array<double, 4> tail_percentiles = {
          99.9,
          99.99,
          99.999,
          100,
      };
      for (auto percentile : tail_percentiles) {
        printf("%s,%f,%ld\n", to_string(cfg.scenario), percentile,
               hdr_value_at_percentile(hist, percentile));
      }
    });

    for (size_t tid = 0; tid < cfg.nr_threads; tid++) {
      std::thread interfering_thread([&cfg, &topology, &other_pu, &stop,
                                      tid]() {
        /* Set up a signal handler that does not restart system calls. When the
           benchmark harness is about to stop, it sends a signal to all
           intefering threads to return from any blocking system calls.  */
        struct ::sigaction sa;
        sa.sa_handler = signal_handler;
        sa.sa_flags = 0;
        ::sigemptyset(&sa.sa_mask);
        ::sigaction(SIGINT, &sa, nullptr);

        hwloc_set_cpubind(topology, (*other_pu)->cpuset, HWLOC_CPUBIND_THREAD);

        Action action;
        while (!stop.load(std::memory_order_relaxed)) {
          action.other_operation(tid);
        }
      });
      _threads.push_back(std::move(interfering_thread));
    }

    t.join();

    for (std::thread &t : _threads) {
      ::pthread_kill(t.native_handle(), SIGINT);
    }
    for (std::thread &t : _threads) {
      t.join();
    }
    action.release();
    fflush(stdout);
    hwloc_topology_destroy(topology);
  }

 private:
  std::optional<hwloc_obj_t> find_other_pu(hwloc_topology_t topology,
                                           hwloc_obj_t pu, Scenario scenario) {
    hwloc_obj_t core = pu->parent;
    auto package = find_package(core);
    if (!package) {
      return std::nullopt;
    }
    while ((pu = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_PU, pu))) {
      hwloc_obj_t other_core = pu->parent;
      auto other_package = find_package(other_core);
      if (!other_package) {
        return std::nullopt;
      }
      switch (scenario) {
        case REMOTE_PACKAGE:
          if (package != other_package) {
            return pu;
          }
          break;
        case REMOTE_CORE:
          if (package == other_package && core != other_core) {
            return pu;
          }
          break;
        case LOCAL_CORE:
          if (core == other_core) {
            return pu;
          }
          break;
      }
    }
    return std::nullopt;
  }

  std::optional<hwloc_obj_t> find_package(hwloc_obj_t obj) {
    while ((obj = obj->parent)) {
      if (obj->type == HWLOC_OBJ_PACKAGE) {
        return obj;
      }
    }
    return std::nullopt;
  }
};

template <typename T, size_t nr_iter>
static void run_latency_benchmark(Scenario scenario) {
  Config cfg;
  cfg.nr_iter = nr_iter;
  cfg.scenario = scenario;
  LatencyBenchmark<T> benchmark;
  benchmark.run(cfg);
}

template <typename T, size_t nr_iter>
static void run_latency_benchmarks() {
  printf("scenario,percentile,time\n");
  run_latency_benchmark<T, nr_iter>(REMOTE_PACKAGE);
  run_latency_benchmark<T, nr_iter>(REMOTE_CORE);
  run_latency_benchmark<T, nr_iter>(LOCAL_CORE);
}

template <typename T, size_t nr_iter = 1000000>
static void run_all() {
  run_latency_benchmarks<T, nr_iter>();
}
