#include "benchmark.h"

#include <assert.h>
#include <pthread.h>

static pthread_spinlock_t lock;

static void init() {
  if (pthread_spin_init(&lock, 0) != 0) {
    assert(0);
  }
}

struct Op {
  void operator()(benchmark::NoState& state) {
    pthread_spin_lock(&lock);
    pthread_spin_unlock(&lock);
  }
};

int main(int argc, char *argv[]) {
  init();
  benchmark::run_all<benchmark::SymmetricAction<Op>>(argc, argv);
}
