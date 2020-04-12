#include "benchmark.h"

#include <sys/mman.h>

static constexpr size_t size = 1024 * 1024; /* 1 MB */

struct Action {
  void *map = nullptr;

  void init() {
    map = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(map != MAP_FAILED);
  }

  void release() {
    ::munmap(map, size);
  }

  void raw_operation() {
    ::mprotect(map, size, PROT_NONE);
    ::mprotect(map, size, PROT_READ | PROT_WRITE);
  }

  uint64_t measured_operation() {
    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
      assert(0);
    }
    ::mprotect(map, size, PROT_NONE);
    if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
      assert(0);
    }
    ::mprotect(map, size, PROT_READ | PROT_WRITE);
    uint64_t start_ns = timespec_to_ns(&start);
    uint64_t end_ns = timespec_to_ns(&end);
    return end_ns - start_ns;
  }

  void other_operation(size_t tid) {
    raw_operation();
  }

  bool supports_non_interference() { return true; }

  bool supports_energy_measurement() { return true; }
};


int main(int argc, char *argv[]) {
  run_all<Action>(argc, argv);
}
