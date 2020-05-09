#include "benchmark.h"

#include <sys/mman.h>

const size_t size = 1024 * 1024 * 1024; /* 1 GB */
const size_t page_size = 4096;
const size_t nr_pages = size / page_size;

struct State {
  char *p;
  size_t offset = 0;

  State(const benchmark::ThreadList& ignored)
      : p{reinterpret_cast<char *>(::mmap(NULL, size, PROT_READ | PROT_WRITE,
                                          MAP_ANONYMOUS | MAP_PRIVATE, -1,
                                          0))} {
    assert(p != MAP_FAILED);
    assert(madvise(p, size, MADV_NOHUGEPAGE) == 0);
  }

  ~State() { ::munmap(p, size); }
};

struct Op {
  void operator()(State &state) {
    if (state.offset < size) {
      state.p[state.offset] = '\0';
      state.offset += page_size;
    }
  }
};

int main(int argc, char *argv[]) {
  benchmark::run_all<benchmark::SymmetricAction<Op, State>, nr_pages>(argc, argv);
}
