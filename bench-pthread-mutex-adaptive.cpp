#include "benchmark.h"

#include <pthread.h>

static pthread_mutexattr_t mutex_attr;

struct Op {
  void operator()(NoState& state) {
    static pthread_mutex_t lock = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;
    pthread_mutex_lock(&lock);
    pthread_mutex_unlock(&lock);
  }
};

int main(int argc, char *argv[]) {
  run_all<SymmetricAction<Op>>(argc, argv);
}
