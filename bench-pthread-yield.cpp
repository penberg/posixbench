#include "benchmark.h"

struct Op {
  void operator()() { pthread_yield(); }
};

int main(int argc, char *argv[]) {
  run_all<SymmetricAction<Op>>(argc, argv);
}
