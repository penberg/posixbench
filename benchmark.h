#pragma once

#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include <array>
#include <atomic>
#include <cassert>
#include <fstream>
#include <iostream>
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
  /// No interfering thread.
  NO_INTERFERENCE,
  /// Interfering thread is running on a remote package (NUMA).
  REMOTE_PACKAGE,
  /// Interfering thread is running on a remote core (SMP).
  REMOTE_CORE,
  /// Interfering thread is running on the same core (SMT).
  LOCAL_CORE,
};

static const char *to_string(Scenario scenario) {
  switch (scenario) {
    case NO_INTERFERENCE:
      return "No interference";
    case REMOTE_PACKAGE:
      return "NUMA interference";
    case REMOTE_CORE:
      return "Multicore interference";
    case LOCAL_CORE:
      return "SMT interference";
  }
  assert(0);
}

template <typename Operation, bool SupportsEnergyMeasurement = true>
struct SymmetricAction {
  void init() {}

  void release() {}

  void raw_operation() { Operation()(); }

  uint64_t measured_operation() {
    struct timespec start;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
      assert(0);
    }
    raw_operation();
    struct timespec end;
    if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
      assert(0);
    }
    return time_diff(&start, &end);
  }

  void other_operation(size_t tid) { raw_operation(); }

  bool supports_non_interference() { return true; }

  bool supports_energy_measurement() { return SupportsEnergyMeasurement; }
};

template <typename Operation, typename State>
struct SymmetricActionWithState {
  State state;

  void init() {}

  void release() {}

  void raw_operation() { Operation()(state); }

  uint64_t measured_operation() {
    struct timespec start;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
      assert(0);
    }
    raw_operation();
    struct timespec end;
    if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
      assert(0);
    }
    return time_diff(&start, &end);
  }

  void other_operation(size_t tid) { raw_operation(); }

  bool supports_non_interference() { return true; }

  bool supports_energy_measurement() { return true; }
};

struct Config {
  /// Test scenario.
  Scenario scenario;
  /// Number of interfering threads.
  size_t nr_threads = 1;
  /// Number of iterations.
  size_t nr_iter;
  /// Name of the benchmark.
  std::string benchmark;
};

static void signal_handler(int signum) {
  /* Nothing to do. We send signals to threads to make sure they return from
     blocked system calls.  */
}

inline std::optional<hwloc_obj_t> find_package(hwloc_obj_t obj) {
  while ((obj = obj->parent)) {
    if (obj->type == HWLOC_OBJ_PACKAGE) {
      return obj;
    }
  }
  return std::nullopt;
}

inline std::optional<hwloc_obj_t> find_other_pu(hwloc_topology_t topology, hwloc_obj_t pu, Scenario scenario) {
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
      case NO_INTERFERENCE:
        return std::nullopt;
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

template <class Action>
class LatencyBenchmark {
  std::list<std::thread> _threads;

 public:
  void run(const Config &cfg, std::ostream& out) {
#if 0
    printf("# %s: Measuring thread on CPU %d, intefering thread on CPU %d\n",
           to_string(cfg.scenario), pu->os_index, (*other_pu)->os_index);
#endif
    hwloc_topology_t topology;
    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);
    hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, 0);
    std::optional<hwloc_obj_t> other_pu;
    if (cfg.scenario != NO_INTERFERENCE) {
        other_pu = find_other_pu(topology, pu, cfg.scenario);
        if (!other_pu) {
          std::cerr << "warning: Unable to find other PU for scenario: " << to_string(cfg.scenario) << std::endl;;
          return;
        }
    }
    Action action; /* FIXME: potentially wrong placement of data */
    if (cfg.scenario == NO_INTERFERENCE && !action.supports_non_interference()) {
        return;
    }
    action.init();
    std::atomic<bool> stop = false;
    std::thread t([&cfg, &topology, &pu, &action, &stop, &out]() {
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
      out << to_string(cfg.scenario);
      out << ",";
      out << "mean";
      out << ",";
      out << hdr_mean(hist);
      out << std::endl;
      for (size_t percentile = 1; percentile < 100; percentile++) {
        out << to_string(cfg.scenario);
	out << ",";
	out << (double)percentile;
	out << ",";
	out << hdr_value_at_percentile(hist, percentile);
	out << std::endl;
      }
      std::array<double, 4> tail_percentiles = {
          99.9,
          99.99,
          99.999,
          100,
      };
      for (auto percentile : tail_percentiles) {
        out << to_string(cfg.scenario);
	out << ",";
	out << (double)percentile;
	out << ",";
	out << hdr_value_at_percentile(hist, percentile);
	out << std::endl;
      }
    });

    if (other_pu) {
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
};

template <typename T, size_t nr_iter>
static void run_latency_benchmark(Scenario scenario, std::ostream& out) {
  Config cfg;
  cfg.nr_iter = nr_iter;
  cfg.scenario = scenario;
  LatencyBenchmark<T> benchmark;
  benchmark.run(cfg, out);
}

template <typename T, size_t nr_iter>
static void run_latency_benchmarks(std::ostream& out) {
  out << "scenario,percentile,time" << std::endl;
  run_latency_benchmark<T, nr_iter>(REMOTE_PACKAGE, out);
  run_latency_benchmark<T, nr_iter>(REMOTE_CORE, out);
  run_latency_benchmark<T, nr_iter>(LOCAL_CORE, out);
  run_latency_benchmark<T, nr_iter>(NO_INTERFERENCE, out);
}

static void usage(std::string program)
{
  std::cout << "usage: " << program << " [-l <path>]" << std::endl;
}

/* RAPL MSRs  */
enum {
  MSR_PKG_ENERGY_STATUS  = 0x611,
  MSR_RAPL_POWER_UNIT    = 0x606,
  MSR_PP0_ENERGY_STATUS  = 0x639,
  MSR_PP1_ENERGY_STATUS  = 0x641,
  MSR_DRAM_ENERGY_STATUS = 0x619,
};

static int msr_fd = -1;

static uint64_t read_msr(int msr_offset) {
  uint64_t ret;
  if (pread(msr_fd, &ret, sizeof(ret), msr_offset) != sizeof(ret)) {
    assert(0);
  }
  return ret;
}

/* Wait for the beginning of the next RAPL counter sampling period (1 ms
   granularity).  */
inline void wait_for_rapl_sampling_period_begin() {
  struct timespec start;
  if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
    assert(0);
  }
  for (;;) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) {
      assert(0);
    }
    if ((start.tv_nsec / 1000000) != (now.tv_nsec / 1000000)) {
      break;
    }
  }
}

template<typename Action>
inline uint64_t measure_energy(Action& action, int msr_offset, uint64_t iterations, double energy_unit)
{
  struct timespec start;
  if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
    assert(0);
  }
  wait_for_rapl_sampling_period_begin();

  uint64_t energy_begin = read_msr(msr_offset);
  for (uint64_t i = 0; i < iterations; i++) {
    action.raw_operation();
  }
  uint64_t energy_end = read_msr(msr_offset);

  struct timespec end;
  if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
    assert(0);
  }

  printf("Measured energy for %f ms\n", double(time_diff(&start, &end)) / 1e6);

  return uint64_t((energy_end - energy_begin) * energy_unit * 1e9 / double(iterations));
}

static std::atomic<bool> alarm_fired = false;

inline void alarm_signal_handler(int signum) {
  alarm_fired = true;
}

template <class Action>
class EnergyBenchmark {
  std::list<std::thread> _threads;

 public:
  void run(const Config &cfg, std::ostream &out) {
#if 0
    printf("# %s: Measuring thread on CPU %d, intefering thread on CPU %d\n",
           to_string(cfg.scenario), pu->os_index, (*other_pu)->os_index);
#endif
    hwloc_topology_t topology;
    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);
    hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, 0);
    std::optional<hwloc_obj_t> other_pu;
    if (cfg.scenario != NO_INTERFERENCE) {
      other_pu = find_other_pu(topology, pu, cfg.scenario);
      if (!other_pu) {
        std::cerr << "warning: Unable to find other PU for scenario: " << to_string(cfg.scenario) << std::endl;;
        return;
      }
    }
    Action action; /* FIXME: potentially wrong placement of data */
    if (cfg.scenario == NO_INTERFERENCE && !action.supports_non_interference()) {
      return;
    }
    if (!action.supports_energy_measurement()) {
      return;
    }
    action.init();
    std::atomic<bool> stop = false;
    std::thread t([&cfg, &topology, &pu, &action, &stop, &out]() {
      int cpu = pu->os_index; /* FIXME: is this correct CPU? */
      char msr_path[PATH_MAX];
      snprintf(msr_path, PATH_MAX, "/dev/cpu/%d/msr", cpu);
      msr_fd = open(msr_path, O_RDONLY);
      if (msr_fd < 0) {
        assert(0);
      }
      uint64_t power_unit = read_msr(MSR_RAPL_POWER_UNIT);
      double energy_unit = pow(0.5, (double)((power_unit >> 8) & 0x1f));

      hwloc_set_cpubind(topology, pu->cpuset, HWLOC_CPUBIND_THREAD);

      struct ::sigaction sa;
      sa.sa_handler = alarm_signal_handler;
      sa.sa_flags = 0;
      ::sigemptyset(&sa.sa_mask);
      ::sigaction(SIGALRM, &sa, nullptr);

      alarm_fired = false;
      ::alarm(1);

      struct timespec start_ts;
      if (clock_gettime(CLOCK_MONOTONIC, &start_ts) < 0) {
        assert(0);
      }

      uint64_t nr_ops = 0;
      while (!alarm_fired) {
        action.raw_operation();
	nr_ops++;
      }
      struct timespec end_ts;
      if (clock_gettime(CLOCK_MONOTONIC, &end_ts) < 0) {
        assert(0);
      }

      double ns_per_op = time_diff(&start_ts, &end_ts) / double(nr_ops);

      size_t nr_samples = 10;

      /* Aim for 100 ms for the energy measurement period.  */
      uint64_t iterations = uint64_t(1e8 / ns_per_op);

      printf("ns/op = %f, iterations = %lu\n", ns_per_op, iterations);

      for (size_t j = 0; j < nr_samples; j++) {
        uint64_t pkg_energy  = measure_energy(action, MSR_PKG_ENERGY_STATUS, iterations, energy_unit);
        uint64_t p0_energy   = measure_energy(action, MSR_PP0_ENERGY_STATUS, iterations, energy_unit);
        uint64_t p1_energy   = measure_energy(action, MSR_PP1_ENERGY_STATUS, iterations, energy_unit);
        uint64_t dram_energy = measure_energy(action, MSR_DRAM_ENERGY_STATUS, iterations, energy_unit);

        out << cfg.benchmark;
        out << ",";
        out << to_string(cfg.scenario);
        out << ",";
        out << pkg_energy;
        out << ",";
        out << p0_energy;
        out << ",";
        out << p1_energy;
        out << ",";
        out << dram_energy;
        out << std::endl;
      }
      stop.store(true);
    });

    if (other_pu) {
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
};

template <typename T, size_t nr_iter>
static void run_energy_benchmark(std::string benchmark, Scenario scenario,
                                 std::ostream& out) {
  Config cfg;
  cfg.benchmark = benchmark;
  cfg.nr_iter = nr_iter;
  cfg.scenario = scenario;
  EnergyBenchmark<T> bench;
  bench.run(cfg, out);
}

template <typename T, size_t nr_iter>
static void run_energy_benchmarks(std::string benchmark, std::ostream &out) {
  out << "Benchmark,Scenario,PackageEnergyPerOperation(nJ),PowerPlane0EnergyPerOperation(nJ),PowerPlane1EnergyPerOperation(nJ),DRAMEnergyPerOperation(nJ)" << std::endl;
  run_energy_benchmark<T, nr_iter>(benchmark, REMOTE_PACKAGE, out);
  run_energy_benchmark<T, nr_iter>(benchmark, REMOTE_CORE, out);
  run_energy_benchmark<T, nr_iter>(benchmark, LOCAL_CORE, out);
  run_energy_benchmark<T, nr_iter>(benchmark, NO_INTERFERENCE, out);
}

template <typename T, size_t nr_iter = 1000000>
static void run_all(int argc, char *argv[]) {
  std::string program = ::basename(argv[0]);
  std::optional<std::string> latency_output;
  std::optional<std::string> energy_output;
  int c;
  while ((c = getopt(argc, argv, "l:e:")) != -1) {
    switch (c) {
      case 'l':
        latency_output = optarg;
        break;
      case 'e':
        energy_output = optarg;
        break;
      default:
        usage(program);
        exit(1);
    }
  }
  if (latency_output) {
    std::ofstream output;
    output.open(*latency_output);
    run_latency_benchmarks<T, nr_iter>(output);
  }
  if (energy_output) {
    std::ofstream output;
    output.open(*energy_output);
    run_energy_benchmarks<T, nr_iter>(program, output);
  }
}
