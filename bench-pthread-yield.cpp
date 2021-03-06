#include "benchmark.h"

struct Op {
  void operator()(benchmark::NoState& state) { pthread_yield(); }
};

int main(int argc, char *argv[]) {
  benchmark::run_all<benchmark::SymmetricAction<Op>>(argc, argv);
}
