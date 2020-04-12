/* Eventfd benchmark.

   This benchmark measures the time to wake up a thread using eventfd.  The
   measuring thread writes to an eventfd descriptor and waits for the other
   thread to read from the descriptor and write back to it.

   The time elapsed between writing to the descriptor and the other thread
   reading it is used as the measure of wakeup delay. That is, the measuring
   thread queries timestap before writing and the other thread queries timestamp
   after reading.  */

#include "benchmark.h"

#include <sys/eventfd.h>

#include <vector>

static int local_efd;
static std::vector<int> remote_efds;

struct Action {
  /// The running index of remote thread to wake up.
  size_t remote_idx = 0;

  void init() {
    local_efd = eventfd(0, 0);
    remote_efds.push_back(eventfd(0, 0));
  }

  void release() {
    for (size_t i = 0; i < remote_efds.size(); i++) {
      ::close(remote_efds[i]);
    }
    remote_efds.clear();
  }

  void raw_operation() {
    if (eventfd_write(remote_efds[remote_idx], local_efd) < 0) {
      assert(0);
    }
    eventfd_t end_ns = 0;
    if (eventfd_read(local_efd, &end_ns) < 0) {
      assert(0);
    }
    remote_idx = (remote_idx + 1) % remote_efds.size();
  }

  uint64_t measured_operation() {
    struct timespec start;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
      assert(0);
    }
    if (eventfd_write(remote_efds[remote_idx], local_efd) < 0) {
      assert(0);
    }
    eventfd_t end_ns = 0;
    if (eventfd_read(local_efd, &end_ns) < 0) {
      assert(0);
    }
    remote_idx = (remote_idx + 1) % remote_efds.size();
    uint64_t start_ns = timespec_to_ns(&start);
    return end_ns - start_ns;
  }

  void other_operation(size_t tid) {
    eventfd_t fd = 0;
    if (eventfd_read(remote_efds[tid], &fd) < 0) {
      if (errno == EINTR) {
        /* The benchmark harness sent a signal to wake up blocked system calls.
         */
        return;
      }
      assert(0);
    }
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) {
      assert(0);
    }
    if (eventfd_write(fd, timespec_to_ns(&now)) < 0) {
      assert(0);
    }
  }

  bool supports_non_interference() { return false; }

  bool supports_energy_measurement() { return true; }
};

int main(int argc, char *argv[]) {
  run_all<Action>(argc, argv);
}
