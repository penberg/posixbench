V =
ifeq ($(strip $(V)),)
        E = @echo
        Q = @
else
        E = @\#
        Q =
endif
export E Q

BENCHMARKS += bench-gettime
BENCHMARKS += bench-getuid
BENCHMARKS += bench-open
BENCHMARKS += bench-pthread-create
BENCHMARKS += bench-pthread-yield
BENCHMARKS += bench-pthread-kill
BENCHMARKS += bench-pthread-mutex
BENCHMARKS += bench-pthread-mutex-adaptive
BENCHMARKS += bench-pthread-rwlock-rd
BENCHMARKS += bench-pthread-rwlock-wr
BENCHMARKS += bench-pthread-spinlock
BENCHMARKS += bench-pagefault-large
BENCHMARKS += bench-pagefault-signal
BENCHMARKS += bench-pagefault-small
BENCHMARKS += bench-mmap-2mb
BENCHMARKS += bench-mmap-4kb
BENCHMARKS += bench-munmap-2mb
BENCHMARKS += bench-munmap-4kb
BENCHMARKS += bench-mmap-populate-2mb
BENCHMARKS += bench-mmap-populate-4kb
BENCHMARKS += bench-munmap-populated-2mb
BENCHMARKS += bench-munmap-populated-4kb
BENCHMARKS += bench-mprotect
BENCHMARKS += bench-eventfd
BENCHMARKS += bench-eventfd-nonblock

OS=$(shell uname -s)
CPU=$(shell scripts/cpuinfo.sh)

RESULTS_OUT=results/$(OS)/$(CPU)
RESULTS_PERF=results/$(OS)/$(CPU)/perf

REPORT=posixbench-report.md

TARBALL=posixbench.tar.gz

all: build bench report
.PHONY: all

build:
	@git submodule update --init --force --recursive
	@mkdir -p build && cd build && cmake .. && make --quiet
.PHONY: build

bench: $(BENCHMARKS)
.PHONY: bench

$(BENCHMARKS):
	$(E) "  BENCH   " $@
	$(Q) mkdir -p "$(RESULTS_OUT)"
	$(Q)./build/$@ -l "$(RESULTS_OUT)/$@.csv"
	$(Q)./build/$@ -e "$(RESULTS_OUT)/$@-energy.csv"

perf:
	$(E) "  PERF"
	$(Q) mkdir -p "$(RESULTS_PERF)"
	$(Q) $(foreach interference,none smt mc numa,$(foreach benchmark,$(BENCHMARKS),perf record -g ./build/$(benchmark) -i $(interference) -l tmp -d 5 && perf script -f > "$(RESULTS_PERF)/$(benchmark)-$(interference).out";))
.PHONY: perf

report:
	$(E) "  GEN     " $(REPORT)
	$(Q) UNAME="$(shell uname -a)" CPUINFO="$(shell ./scripts/cpuinfo.sh)" envsubst < posixbench-report.md.in > "$(RESULTS_OUT)/$(REPORT)"
	$(Q) $(foreach benchmark,$(BENCHMARKS),./scripts/plot-latency.py "$(RESULTS_OUT)/$(benchmark).csv";)
	$(Q) $(foreach benchmark,$(BENCHMARKS),./scripts/plot-energy.py "$(RESULTS_OUT)/$(benchmark)-energy.csv";)
.PHONY: report

tarball: $(TARBALL)
.PHONY: tarball

$(TARBALL):
	$(E) "  TAR     " $@
	$(Q) tar -czf "$@" "$(RESULTS_OUT)"
