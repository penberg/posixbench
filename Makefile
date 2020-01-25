V =
ifeq ($(strip $(V)),)
        E = @echo
        Q = @
else
        E = @\#
        Q =
endif
export E Q

BENCHMARKS += bench-yield
BENCHMARKS += bench-pthread-nop
BENCHMARKS += bench-pthread-yield
BENCHMARKS += bench-pthread-mutex
BENCHMARKS += bench-pthread-rwlock-rd
BENCHMARKS += bench-pthread-rwlock-wr
BENCHMARKS += bench-pthread-spinlock
BENCHMARKS += bench-pagefault
BENCHMARKS += bench-eventfd

OS=$(shell uname -s)
CPU=$(shell scripts/cpuinfo.sh)

RESULTS_OUT=results/$(OS)/$(CPU)
RESULTS_TEMPLATE=results/template

all: build
.PHONY: all

build:
	@git submodule update --init --force --recursive
	@mkdir -p build && cmake -S . -B build && make --quiet --directory build
.PHONY: build

bench: $(BENCHMARKS)
	$(Q) cp "$(RESULTS_TEMPLATE)/README.md" "$(RESULTS_OUT)/"
.PHONY: bench

$(BENCHMARKS):
	$(E) "  BENCH   " $@
	$(Q) mkdir -p "$(RESULTS_OUT)"
	$(Q)./build/$@ > "$(RESULTS_OUT)/$@.csv"
	$(Q)./plot.py "$(RESULTS_OUT)/$@.csv"
