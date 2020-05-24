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
#include <functional>
#include <iostream>
#include <optional>
#include <system_error>
#include <thread>
#include <vector>

#include <hdr_histogram.h>
#include <hdr_interval_recorder.h>
#include <hwloc.h>

namespace benchmark {

static uint64_t timespec_to_ns(struct timespec *ts) {
  return ts->tv_sec * 1e9 + ts->tv_nsec;
}

static uint64_t time_diff(struct timespec *start, struct timespec *end) {
  uint64_t start_ns = timespec_to_ns(start);
  uint64_t end_ns = timespec_to_ns(end);
  assert(start_ns < end_ns);
  return end_ns - start_ns;
}

enum class Interference : uint8_t {
  NONE = 0x01,
  LOCAL_CORE = 0x02,
  REMOTE_CORE = 0x04,
  REMOTE_PACKAGE = 0x08,
  ALL = NONE | LOCAL_CORE | REMOTE_CORE | REMOTE_PACKAGE,
};

inline bool operator&(Interference x, Interference y) {
  using Ty = std::underlying_type_t<Interference>;

  return static_cast<Interference>(static_cast<Ty>(x) & static_cast<Ty>(y)) == y;
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

using ThreadVector = std::vector<std::thread>;

struct NoState {
  const ThreadVector& interfering_threads;

  NoState(const ThreadVector& interfering_threads)
	 : interfering_threads{interfering_threads}
  {}
};

template <typename Operation, typename State = NoState>
struct SymmetricAction {
  State make_state(const ThreadVector& interfering_threads) { return State(interfering_threads); }

  void raw_operation(State& state) { Operation()(state); }

  uint64_t measured_operation(State& state) {
    struct timespec start;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
      assert(0);
    }
    raw_operation(state);
    struct timespec end;
    if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
      assert(0);
    }
    return time_diff(&start, &end);
  }

  void other_operation(State& state, size_t tid) { raw_operation(state); }

  bool supports_non_interference() { return true; }

  bool supports_energy_measurement() { return true; }
};

static constexpr int DEFAULT_NR_INTERFERING_THREADS = 1;

struct Config {
  /// Test scenario.
  Scenario scenario;
  /// Number of interfering threads.
  size_t nr_interfering_threads = DEFAULT_NR_INTERFERING_THREADS;
  /// Name of the benchmark.
  std::string benchmark;
};

static std::atomic<bool> sigint_fired = false;

static void signal_handler(int signum) {
  /* SIGINT is sent by user who wants to quit or sent by the measuring thread
     to the interfering threads to make sure they return from
     blocked system calls.  */
  sigint_fired = true;
}

static std::atomic<bool> alarm_fired = false;

inline void alarm_signal_handler(int signum) {
  alarm_fired = true;
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
  ThreadVector _interfering_threads;

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
    Action action;
    if (cfg.scenario == NO_INTERFERENCE && !action.supports_non_interference()) {
        return;
    }
    std::cout << "Measuring latency for " << cfg.benchmark << " (" << to_string(cfg.scenario) << ") ..." << std::endl;
    std::atomic<bool> stop = false;
    if (other_pu) {
      _interfering_threads.resize(cfg.nr_interfering_threads);
      for (size_t tid = 0; tid < cfg.nr_interfering_threads; tid++) {
        std::thread interfering_thread([this, &cfg, &topology, &other_pu, &stop, &action, tid]() {
          hwloc_set_cpubind(topology, (*other_pu)->cpuset, HWLOC_CPUBIND_THREAD);

          auto state = action.make_state(_interfering_threads);

          while (!stop.load(std::memory_order_relaxed)) {
            action.other_operation(state, tid);
          }
        });
        _interfering_threads[tid] = std::move(interfering_thread);
      }
    }
    std::thread t([this, &cfg, &topology, &pu, &action, &stop, &out]() {
      hwloc_set_cpubind(topology, pu->cpuset, HWLOC_CPUBIND_THREAD);
      auto state = action.make_state(_interfering_threads);
      struct hdr_histogram *hist;
      if (hdr_init(1, 1000000, 3, &hist)) {
        assert(0);
      }

      alarm_fired = false;
      ::alarm(5);

      while (!sigint_fired && !alarm_fired) {
        auto diff = action.measured_operation(state);
        hdr_record_value(hist, diff);
      }
      stop.store(true);
      out << to_string(cfg.scenario);
      out << ",";
      out << "mean";
      out << ",";
      out << hdr_mean(hist);
      out << std::endl;
      out << to_string(cfg.scenario);
      out << ",";
      out << "stddev";
      out << ",";
      out << hdr_stddev(hist);
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

    t.join();

    for (std::thread &t : _interfering_threads) {
      ::pthread_kill(t.native_handle(), SIGINT);
    }
    for (std::thread &t : _interfering_threads) {
      t.join();
    }
    fflush(stdout);
    hwloc_topology_destroy(topology);
  }
};

template <typename T>
static void run_latency_benchmark(Scenario scenario, std::ostream& out) {
  Config cfg;
  cfg.scenario = scenario;
  LatencyBenchmark<T> benchmark;
  benchmark.run(cfg, out);
}

template <typename T>
static void run_latency_benchmarks(Interference interference, std::ostream& out) {
  out << "scenario,percentile,time" << std::endl;
  if (interference & Interference::REMOTE_PACKAGE) {
    run_latency_benchmark<T>(Scenario::REMOTE_PACKAGE, out);
  }
  if (interference & Interference::REMOTE_CORE) {
    run_latency_benchmark<T>(Scenario::REMOTE_CORE, out);
  }
  if (interference & Interference::LOCAL_CORE) {
    run_latency_benchmark<T>(Scenario::LOCAL_CORE, out);
  }
  if (interference & Interference::NONE) {
    run_latency_benchmark<T>(Scenario::NO_INTERFERENCE, out);
  }
}

static void usage(std::string program)
{
  std::cout << "usage: " << program << "[-i <interference>] [-l <path>]" << std::endl;
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

/* Measure mean energy per operation (EPO) by running an operation for 1 second
   and dividing the total amount of energy by the number of operations
   executed.  */
template<typename Action, typename State>
inline uint64_t measure_energy(Action& action, State& state, int msr_offset, double energy_unit)
{
  uint64_t iterations = 0;

  alarm_fired = false;
  ::alarm(1);

  struct timespec start;
  if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
    assert(0);
  }
  wait_for_rapl_sampling_period_begin();

  uint64_t energy_begin = read_msr(msr_offset);
  while (!alarm_fired) {
    action.raw_operation(state);
    iterations++;
  }
  uint64_t energy_end = read_msr(msr_offset);

  struct timespec end;
  if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
    assert(0);
  }

  printf("Measured energy for %f ms\n", double(time_diff(&start, &end)) / 1e6);

  return uint64_t((energy_end - energy_begin) * energy_unit * 1e9 / double(iterations));
}

struct EnergyConfig {
  /// Test scenario.
  Scenario scenario;
  /// Number of interfering threads.
  size_t nr_interfering_threads = DEFAULT_NR_INTERFERING_THREADS;
  /// Name of the benchmark.
  std::string benchmark;
  /// Number of energy measurement samples.
  int nr_samples;
};

template <class Action>
class EnergyBenchmark {
  ThreadVector _interfering_threads;

 public:
  void run(const EnergyConfig &cfg, std::ostream &out) {
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
    Action action;
    if (cfg.scenario == NO_INTERFERENCE && !action.supports_non_interference()) {
      return;
    }
    if (!action.supports_energy_measurement()) {
      return;
    }
    std::cout << "Measuring energy for " << cfg.benchmark << " (" << to_string(cfg.scenario) << ") ..." << std::endl;
    std::atomic<bool> stop = false;
    if (other_pu) {
      _interfering_threads.resize(cfg.nr_interfering_threads);
      for (size_t tid = 0; tid < cfg.nr_interfering_threads; tid++) {
        std::thread interfering_thread([this, &cfg, &topology, &other_pu, &stop, &action, tid]() {
          hwloc_set_cpubind(topology, (*other_pu)->cpuset, HWLOC_CPUBIND_THREAD);
    
	  auto state = action.make_state(_interfering_threads);

          while (!stop.load(std::memory_order_relaxed)) {
            action.other_operation(state, tid);
          }
        });
        _interfering_threads[tid] = std::move(interfering_thread);
      }
    }
    std::thread t([this, &cfg, &topology, &pu, &action, &stop, &out]() {
      int cpu = pu->os_index;
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

      for (int j = 0; j < cfg.nr_samples; j++) {
        auto state_pkg = action.make_state(_interfering_threads);
        uint64_t pkg_energy  = measure_energy(action, state_pkg, MSR_PKG_ENERGY_STATUS, energy_unit);
#if 0
        uint64_t p0_energy   = measure_energy(action, state, MSR_PP0_ENERGY_STATUS, energy_unit);
        uint64_t p1_energy   = measure_energy(action, state, MSR_PP1_ENERGY_STATUS, energy_unit);
#endif
        uint64_t p0_energy   = 0;
        uint64_t p1_energy   = 0;
        auto state_dram = action.make_state(_interfering_threads);
        uint64_t dram_energy = measure_energy(action, state_dram, MSR_DRAM_ENERGY_STATUS, energy_unit);

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

    t.join();

    for (std::thread &t : _interfering_threads) {
      ::pthread_kill(t.native_handle(), SIGINT);
    }
    for (std::thread &t : _interfering_threads) {
      t.join();
    }
    fflush(stdout);
    hwloc_topology_destroy(topology);
  }
};

template <typename T>
static void run_energy_benchmark(std::string benchmark, Scenario scenario, int nr_samples, std::ostream& out) {
  EnergyConfig cfg;
  cfg.benchmark = benchmark;
  cfg.scenario = scenario;
  cfg.nr_samples = nr_samples;
  EnergyBenchmark<T> bench;
  bench.run(cfg, out);
}

template <typename T>
static void run_energy_benchmarks(std::string benchmark, Interference interference, int nr_samples, std::ostream &out) {
  out << "Benchmark,Scenario,PackageEnergyPerOperation(nJ),PowerPlane0EnergyPerOperation(nJ),PowerPlane1EnergyPerOperation(nJ),DRAMEnergyPerOperation(nJ)" << std::endl;
  if (interference & Interference::REMOTE_PACKAGE) {
    run_energy_benchmark<T>(benchmark, Scenario::REMOTE_PACKAGE, nr_samples, out);
  }
  if (interference & Interference::REMOTE_CORE) {
    run_energy_benchmark<T>(benchmark, Scenario::REMOTE_CORE, nr_samples, out);
  }
  if (interference & Interference::LOCAL_CORE) {
    run_energy_benchmark<T>(benchmark, Scenario::LOCAL_CORE, nr_samples, out);
  }
  if (interference & Interference::NONE) {
    run_energy_benchmark<T>(benchmark, Scenario::NO_INTERFERENCE, nr_samples, out);
  }
}

static Interference parse_interference(const std::string& raw_interference)
{
  if (raw_interference == "all") {
    return Interference::ALL;
  } else if (raw_interference == "none") {
    return Interference::NONE;
  } else if (raw_interference == "smt") {
    return Interference::LOCAL_CORE;
  } else if (raw_interference == "mc") {
    return Interference::REMOTE_CORE;
  } else if (raw_interference == "numa") {
    return Interference::REMOTE_PACKAGE;
  } else {
    throw std::invalid_argument("unknown '" + raw_interference + "' interference option");
  }
}

template <typename T>
static void run_all(int argc, char *argv[], std::optional<std::function<void(size_t)>> init = std::nullopt) {
  /* Set up a signal handler that does not restart system calls. When
     the user or the benchmark harness is wants to stop, they send a
     signal to all intefering threads to return from any blocking system
     calls.  */
  struct ::sigaction sa_int;
  sa_int.sa_handler = signal_handler;
  sa_int.sa_flags = 0;
  ::sigemptyset(&sa_int.sa_mask);
  ::sigaction(SIGINT, &sa_int, nullptr);
     
  struct ::sigaction sa_alrm;
  sa_alrm.sa_handler = alarm_signal_handler;
  sa_alrm.sa_flags = 0;
  ::sigemptyset(&sa_alrm.sa_mask);
  ::sigaction(SIGALRM, &sa_alrm, nullptr);

  std::string program = ::basename(argv[0]);
	std::string raw_interference = "all";
  std::optional<std::string> latency_output;
  std::optional<std::string> energy_output;
  std::optional<int> nr_samples;
  int c;
  while ((c = getopt(argc, argv, "i:l:e:s:")) != -1) {
    switch (c) {
      case 'i':
        raw_interference = optarg;
        break;
      case 'l':
        latency_output = optarg;
        break;
      case 'e':
        energy_output = optarg;
        break;
      case 's':
        nr_samples = strtol(optarg, nullptr, 10);
        break;
      default:
        usage(program);
        exit(1);
    }
  }
  if (init) {
    (*init)(DEFAULT_NR_INTERFERING_THREADS);
  }
  try {
    auto interference = parse_interference(raw_interference); 
    if (latency_output) {
      std::ofstream output;
      output.open(*latency_output);
      run_latency_benchmarks<T>(interference, output);
    }
    if (energy_output) {
      constexpr int DEFAULT_NR_SAMPLES = 30;
      std::ofstream output;
      output.open(*energy_output);
      run_energy_benchmarks<T>(program, interference, nr_samples.value_or(DEFAULT_NR_SAMPLES), output);
    }
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << std::endl;
  }
}

}
