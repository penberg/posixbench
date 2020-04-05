#include "benchmark.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

static char filename[PATH_MAX];

struct Action {
  void init() {
    ::strncpy(filename, "tmp-bench-openXXXXXX", PATH_MAX);
    int fd = mkostemp(filename, O_CREAT);
    if (fd < 0) {
      assert(0);
    }
    if (::close(fd) < 0) {
      assert(0);
    }
  }

  void release() {
    if (::unlink(filename) < 0) {
      assert(0);
    }
  }

  uint64_t measured_operation() {
    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
      assert(0);
    }
    int fd = ::open(filename, O_RDWR);
    if (fd < 0) {
      assert(0);
    }
    if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
      assert(0);
    }
    if (::close(fd) < 0) {
      assert(0);
    }
    uint64_t start_ns = timespec_to_ns(&start);
    uint64_t end_ns = timespec_to_ns(&end);
    return end_ns - start_ns;
  }

  void other_operation(size_t tid) {
    int fd = ::open(filename, O_RDWR);
    if (fd < 0) {
      assert(0);
    }
    if (::close(fd) < 0) {
      assert(0);
    }
  }

  bool supports_non_interference() { return true; }
};

int main(int argc, char *argv[]) { run_all<Action>(argc, argv); }
