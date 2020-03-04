#include "benchmark.h"

#include <sys/mman.h>

static constexpr size_t size = 1024 * 1024; /* 1 MB */

struct Op {
  void operator()() {
    auto *map = ::mmap(nullptr, size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ::munmap(map, size);
  }
};

int main(int argc, char *argv[]) { run_all<SymmetricAction<Op>>(argc, argv); }
