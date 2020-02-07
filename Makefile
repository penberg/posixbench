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
BENCHMARKS += bench-pthread-create
BENCHMARKS += bench-pthread-yield
BENCHMARKS += bench-pthread-mutex
BENCHMARKS += bench-pthread-rwlock-rd
BENCHMARKS += bench-pthread-rwlock-wr
BENCHMARKS += bench-pthread-spinlock
BENCHMARKS += bench-pagefault
BENCHMARKS += bench-eventfd
BENCHMARKS += bench-eventfd-nonblock

OS=$(shell uname -s)
CPU=$(shell scripts/cpuinfo.sh)

RESULTS_OUT=results/$(OS)/$(CPU)
RESULTS_TEMPLATE=results/template

all: build
.PHONY: all

build:
	@git submodule update --init --force --recursive
	@mkdir -p build && cd build && cmake .. && make --quiet
.PHONY: build

bench: $(BENCHMARKS)
	$(Q) cp "$(RESULTS_TEMPLATE)/README.md" "$(RESULTS_OUT)/"
	$(Q) cp "$(RESULTS_TEMPLATE)/README.html" "$(RESULTS_OUT)/"
.PHONY: bench

$(BENCHMARKS):
	$(E) "  BENCH   " $@
	$(Q) mkdir -p "$(RESULTS_OUT)"
	$(Q)./build/$@ > "$(RESULTS_OUT)/$@.csv"
	$(Q)./plot.py "$(RESULTS_OUT)/$@.csv"
