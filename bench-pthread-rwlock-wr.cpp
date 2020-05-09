#include "benchmark.h"

#include <pthread.h>

struct Op {
  void operator()(benchmark::NoState& state) {
    static pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;
    pthread_rwlock_wrlock(&lock);
    pthread_rwlock_unlock(&lock);
  }
};

int main(int argc, char *argv[]) {
  benchmark::run_all<benchmark::SymmetricAction<Op>>(argc, argv);
}
