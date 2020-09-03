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
sudo ./scripts/hugepages.sh
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
