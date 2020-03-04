#include "benchmark.h"

#include <sys/mman.h>

static void *map;
static constexpr size_t size = 1024 * 1024; /* 1 MB */

struct Op {
  void operator()() {
    ::mprotect(map, size, PROT_NONE);
    ::mprotect(map, size, PROT_READ | PROT_WRITE);
  }
};

int main(int argc, char *argv[]) {
  map = ::mmap(nullptr, size, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  assert(map != MAP_FAILED);
  run_all<SymmetricAction<Op>>(argc, argv);
  ::munmap(map, size);
}
