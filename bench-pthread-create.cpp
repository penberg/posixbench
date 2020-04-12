/* Thread creation benchmark.  */

#include "benchmark.h"

#include <pthread.h>

struct Action {
  NoState make_state() { return NoState(); }

  void raw_operation(NoState& state) {
    std::thread t([] {
    });
    t.join();
  }

  uint64_t measured_operation(NoState& state) {
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
    return timespec_to_ns(&end) - timespec_to_ns(&start);
  }

  void other_operation(NoState& state, size_t tid) {
    raw_operation(state);
  }

  bool supports_non_interference() { return true; }

  bool supports_energy_measurement() { return true; }
};

int main(int argc, char *argv[]) {
  run_all<Action, 100000>(argc, argv);
}
