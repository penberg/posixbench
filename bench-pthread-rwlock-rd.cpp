#include "benchmark.h"

struct Op {
  void operator()() {
    static pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;
    pthread_rwlock_rdlock(&lock);
    pthread_rwlock_unlock(&lock);
  }
};

int main() { run_all<SymmetricAction<Op>>(); }
