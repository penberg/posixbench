#include "benchmark.h"

#include <unistd.h>

struct Op {
  void operator()(NoState& tate) {
    ::getuid();
  }
};

int main(int argc, char *argv[]) {
  run_all<SymmetricAction<Op>>(argc, argv);
}
