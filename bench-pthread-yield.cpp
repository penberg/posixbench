#include "benchmark.h"

struct Op {
  void operator()(NoState& state) { pthread_yield(); }
};

int main(int argc, char *argv[]) {
  run_all<SymmetricAction<Op>>(argc, argv);
}
