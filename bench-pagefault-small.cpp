#include "benchmark.h"
#include "memory.hh"

#include <sys/mman.h>

static constexpr size_t size = 4 * 1024; /* 4 KB */

struct Action {
  benchmark::NoState make_state(const benchmark::ThreadVector& ts) { return benchmark::NoState(ts); }

  void raw_operation(benchmark::NoState& state) {
    void *map = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(map != MAP_FAILED);
    auto *p = reinterpret_cast<volatile unsigned long*>(map);
    memory::force_read(p);
    ::munmap(map, size);
  }

  uint64_t measured_operation(benchmark::NoState& state) {
    void *map = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(map != MAP_FAILED);
    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
      assert(0);
    }
    auto *p = reinterpret_cast<volatile unsigned long*>(map);
    memory::force_read(p);
    if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
      assert(0);
    }
    ::munmap(map, size);
    uint64_t start_ns = benchmark::timespec_to_ns(&start);
    uint64_t end_ns = benchmark::timespec_to_ns(&end);
    return end_ns - start_ns;
  }

  void other_operation(benchmark::NoState& state, size_t tid) {
    raw_operation(state);
  }

  bool supports_non_interference() { return true; }

  bool supports_energy_measurement() { return true; }
};


int main(int argc, char *argv[]) {
  benchmark::run_all<Action>(argc, argv);
}
