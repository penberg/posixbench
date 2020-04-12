#include "benchmark.h"

struct Op {
  void operator()(NoState& state) {
    static pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;
    pthread_rwlock_rdlock(&lock);
    pthread_rwlock_unlock(&lock);
  }
};

int main(int argc, char *argv[]) {
  run_all<SymmetricAction<Op>>(argc, argv);
}
