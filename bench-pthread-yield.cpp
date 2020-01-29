#include "benchmark.h"

struct Op {
  void operator()() { pthread_yield(); }
};

int main() { run_all<SymmetricAction<Op>>(); }
