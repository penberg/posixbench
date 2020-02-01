/* Thread creation benchmark.  */

#include "benchmark.h"

#include <pthread.h>

struct Action {
  void init() {
  }

  void release() {
  }

  uint64_t measured_operation() {
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

  void other_operation(size_t tid) {
    std::thread t([] {
    });
    t.join();
  }
};

int main() { run_all<Action, 100000>(); }
