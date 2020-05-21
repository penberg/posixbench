#include "benchmark.h"

#include <sys/eventfd.h>

#include <deque>
#include <vector>

static std::vector<uint64_t> remote_timestamps;
static std::deque<bool> signaled; /* protected by "mutexes" */
static std::vector<pthread_cond_t> cond_vars;
static std::vector<pthread_mutex_t> mutexes;

struct State {
};

struct Action {
  size_t remote_tid = 0;

  Action() {
  }

  ~Action() {
  }

  benchmark::NoState make_state(const benchmark::ThreadVector& ts) {
    return benchmark::NoState(ts);
  }

  void raw_operation(benchmark::NoState& state) {
    auto other_thread = const_cast<std::thread&>(state.interfering_threads[remote_tid]).native_handle();
    ::pthread_kill(other_thread, SIGUSR1);
    ::pthread_mutex_lock(&mutexes[remote_tid]);
    while (!signaled[remote_tid]) {
      ::pthread_cond_wait(&cond_vars[remote_tid], &mutexes[remote_tid]);
    }
    signaled[remote_tid] = false;
    ::pthread_mutex_unlock(&mutexes[remote_tid]);
    remote_tid = (remote_tid + 1) % remote_timestamps.size();
  }

  uint64_t measured_operation(benchmark::NoState& state) {
    auto other_thread = const_cast<std::thread&>(state.interfering_threads[remote_tid]).native_handle();
    struct timespec start;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
      assert(0);
    }
    ::pthread_kill(other_thread, SIGUSR1);
    ::pthread_mutex_lock(&mutexes[remote_tid]);
    while (!signaled[remote_tid]) {
      ::pthread_cond_wait(&cond_vars[remote_tid], &mutexes[remote_tid]);
    }
    signaled[remote_tid] = false;
    ::pthread_mutex_unlock(&mutexes[remote_tid]);
    uint64_t start_ns = benchmark::timespec_to_ns(&start);
    uint64_t end_ns = remote_timestamps[remote_tid];
    remote_tid = (remote_tid + 1) % remote_timestamps.size();
    return end_ns - start_ns;
  }

  void other_operation(benchmark::NoState& state, size_t tid) {
    ::sigset_t set;
    ::sigemptyset(&set);
    ::sigaddset(&set, SIGALRM);
    ::sigaddset(&set, SIGINT);
    ::sigaddset(&set, SIGUSR1);
    int sig;
    ::sigwait(&set, &sig);
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) {
      assert(0);
    }
    assert(sig == SIGALRM || sig == SIGUSR1 || sig == SIGINT);
    remote_timestamps[tid] = benchmark::timespec_to_ns(&now);
    ::pthread_mutex_lock(&mutexes[tid]);
    signaled[tid] = true;
    ::pthread_cond_signal(&cond_vars[tid]);
    ::pthread_mutex_unlock(&mutexes[tid]);
  }

  bool supports_non_interference() { return false; }

  bool supports_energy_measurement() { return true; }
};

static void init(size_t nr_threads) {
  for (unsigned i = 0; i < nr_threads; i++) {
    remote_timestamps.push_back(0);
    signaled.push_back(false);
    cond_vars.push_back(PTHREAD_COND_INITIALIZER);
    mutexes.push_back(PTHREAD_MUTEX_INITIALIZER);
  }
}

int main(int argc, char *argv[]) {
  ::sigset_t blocked_sigs;
#if 0
  ::sigfillset(&blocked_sigs);
  ::sigdelset(&blocked_sigs, SIGALRM);
  ::sigdelset(&blocked_sigs, SIGINT);
#endif
  ::sigaddset(&blocked_sigs, SIGUSR1);
  assert(::sigprocmask(SIG_SETMASK, &blocked_sigs, nullptr) == 0);
  benchmark::run_all<Action>(argc, argv, init);
}
