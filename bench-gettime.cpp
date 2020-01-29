#include "benchmark.h"

struct Op {
  void operator()() {}
};

int main() { run_all<SymmetricAction<Op>>(); }
