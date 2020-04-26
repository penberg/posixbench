#include "benchmark.h"

#include <unistd.h>

struct Op {
  void operator()() {
    ::getuid();
  }
};

int main(int argc, char *argv[]) {
  run_all<SymmetricAction<Op>>(argc, argv);
}
