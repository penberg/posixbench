#include "benchmark.h"

#include <pthread.h>

struct Op {
  void operator()(NoState& state) {
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&lock);
    pthread_mutex_unlock(&lock);
  }
};

int main(int argc, char *argv[]) {
  run_all<SymmetricAction<Op>>(argc, argv);
}
