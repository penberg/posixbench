#include "benchmark.h"

struct Op {
  void operator()(NoState& state) {}
};

int main(int argc, char *argv[]) {
  run_all<SymmetricAction<Op>>(argc, argv);
}
