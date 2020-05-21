#include "benchmark.h"

#include <sys/mman.h>

const size_t page_size = 4096;

static thread_local uint64_t end_ns;

static void sigaction_segv(int signal, siginfo_t *si, void *arg) {
  struct timespec end;

  if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
    assert(0);
  }
  end_ns = benchmark::timespec_to_ns(&end);

  ucontext_t *ctx = (ucontext_t *) arg;

  /* Let's jump over the "mov (%rax), %rax" instruction in raw_operation() */
  ctx->uc_mcontext.gregs[REG_RIP] += 3;
}

struct Action {
  void *map = nullptr;

  Action() {
    map = ::mmap(nullptr, page_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(map != MAP_FAILED);
  }

  ~Action() {
    ::munmap(map, page_size);
  }

  benchmark::NoState make_state(const benchmark::ThreadVector& ts) {
    struct ::sigaction sa;
    sa.sa_sigaction = sigaction_segv;
    sa.sa_flags = SA_SIGINFO;
    ::sigemptyset(&sa.sa_mask);
    ::sigaction(SIGSEGV, &sa, nullptr);
    return benchmark::NoState(ts);
  }

  void raw_operation(benchmark::NoState& state) {
    volatile char *p = reinterpret_cast<char*>(map);
    asm volatile("movq (%0), %0\n\t" : : "a"(p) : "memory");
  }

  uint64_t measured_operation(benchmark::NoState& state) {
    struct timespec start;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
      assert(0);
    }
    raw_operation(state);
    uint64_t start_ns = benchmark::timespec_to_ns(&start);
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

