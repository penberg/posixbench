/* Thread creation benchmark.  */

#include "benchmark.h"

#include <pthread.h>

struct Action {
  benchmark::NoState make_state(const benchmark::ThreadList& ts) { return benchmark::NoState(ts); }

  void raw_operation(benchmark::NoState& state) {
    std::thread t([] {
    });
    t.join();
  }

  uint64_t measured_operation(benchmark::NoState& state) {
    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
      assert(0);
    }
    std::thread t([&end] {
      if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
        assert(0);
      }
    });
    t.join();
    return benchmark::timespec_to_ns(&end) - benchmark::timespec_to_ns(&start);
  }

  void other_operation(benchmark::NoState& state, size_t tid) {
    raw_operation(state);
  }

  bool supports_non_interference() { return true; }

  bool supports_energy_measurement() { return true; }
};

int main(int argc, char *argv[]) {
  benchmark::run_all<Action, 100000>(argc, argv);
}
