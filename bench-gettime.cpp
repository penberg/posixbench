#include "benchmark.h"

struct Op {
  void operator()() {}
};

int main(int argc, char *argv[]) {
  run_all<SymmetricAction<Op>>(argc, argv);
}
