BENCHMARKS += bench-yield
BENCHMARKS += bench-pthread-nop
BENCHMARKS += bench-pthread-yield
BENCHMARKS += bench-pthread-mutex
BENCHMARKS += bench-pthread-rwlock-rd
BENCHMARKS += bench-pthread-rwlock-wr
BENCHMARKS += bench-pthread-spinlock
BENCHMARKS += bench-pagefault
BENCHMARKS += bench-eventfd

CPU_MODEL=$(shell scripts/cpuinfo.sh)

RESULTS=results/$(CPU_MODEL)
RESULTS_TEMPLATE=results/template

all: $(BENCHMARKS)

$(BENCHMARKS): build report
	./build/$@ > "$(RESULTS)/$@.csv"
	./plot.py "$(RESULTS)/$@.csv"

report:
	mkdir -p "$(RESULTS)"
	cp "$(RESULTS_TEMPLATE)/README.md" "$(RESULTS)/"
.PHONY: build

build:
	@git submodule update --init --force --recursive
	@mkdir -p build && cmake -S . -B build && make --quiet --directory build
.PHONY: build
