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
BENCHMARKS += bench-pthread-mutex
BENCHMARKS += bench-pthread-mutex-adaptive
BENCHMARKS += bench-pthread-rwlock-rd
BENCHMARKS += bench-pthread-rwlock-wr
BENCHMARKS += bench-pthread-spinlock
BENCHMARKS += bench-pagefault
BENCHMARKS += bench-pagefault-large
BENCHMARKS += bench-mmap-munmap
BENCHMARKS += bench-mmap-populate-munmap
BENCHMARKS += bench-mprotect
BENCHMARKS += bench-eventfd
BENCHMARKS += bench-eventfd-nonblock

OS=$(shell uname -s)
CPU=$(shell scripts/cpuinfo.sh)

RESULTS_OUT=results/$(OS)/$(CPU)

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
