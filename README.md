# POSIX Benchmarks

## Getting Started

### System configuration

Before you run the benchmark, you must configure the following things on your system for benchmark isolation.

**Disable TurboBoost.**

**Disable CPU power saving mode:**

```
sudo cpupower --cpu all frequency-set --governor performance
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
