#include "benchmark.h"

#include <sys/mman.h>

const size_t large_page_size =
#if defined(__x86_64__)
    2 * 1024 * 1024; /* 2 MB */
#else
#error "Large page size is unknown"
#endif

const size_t size = 1024 * 1024 * 1024; /* 1 GB */
const size_t nr_pages = size / large_page_size;

struct State {
  char *p;
  size_t offset = 0;

  State(const benchmark::ThreadVector& ignored)
      : p{reinterpret_cast<char *>(::mmap(NULL, size, PROT_READ | PROT_WRITE,
                                          MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1,
                                          0))} {
    assert(p != MAP_FAILED);
  }

  ~State() { ::munmap(p, size); }
};

struct Op {
  void operator()(State &state) {
    if (state.offset < size) {
      state.p[state.offset] = '\0';
      state.offset += large_page_size;
    }
  }
};

int main(int argc, char *argv[]) {
  benchmark::run_all<benchmark::SymmetricAction<Op, State>, nr_pages>(argc, argv);
}
