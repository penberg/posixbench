#include "benchmark.h"

#include <unistd.h>

struct Op {
  void operator()(benchmark::NoState& tate) {
    ::getuid();
  }
};

int main(int argc, char *argv[]) {
  benchmark::run_all<benchmark::SymmetricAction<Op>>(argc, argv);
}
