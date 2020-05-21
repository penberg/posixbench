# POSIX Benchmarks

## Getting Started

You need to configure your machine before running the benchmarks.

**Disable TurboBoost.**

**Disable CPU power saving mode:**

```
sudo cpupower --cpu all frequency-set --governor performance
```

**Configure huge pages:**

```
sudo sysctl -w vm.nr_hugepages=512
```

**Kill background processes and services.**

### Benchmarking

**Prerequisites:**

* matplotlib
* pandas

```
make build
make bench
```
