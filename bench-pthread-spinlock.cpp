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
  void operator()() {
    pthread_spin_lock(&lock);
    pthread_spin_unlock(&lock);
  }
};

int main(int argc, char *argv[]) {
  init();
  run_all<SymmetricAction<Op>>(argc, argv);
}
