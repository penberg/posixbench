#include "benchmark.h"

#include <sys/mman.h>

static constexpr size_t size = 2 * 1024 * 1024; /* 2 MB */

struct Op {
  void operator()(benchmark::NoState& state) {
    auto *map = ::mmap(nullptr, size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    ::munmap(map, size);
  }
};

int main(int argc, char *argv[]) {
  benchmark::run_all<benchmark::SymmetricAction<Op>, 100000>(argc, argv);
}
