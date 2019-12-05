BENCHMARKS += bench-yield
BENCHMARKS += bench-pthread-nop
BENCHMARKS += bench-pthread-yield
BENCHMARKS += bench-pthread-mutex
BENCHMARKS += bench-pthread-rwlock-rd
BENCHMARKS += bench-pthread-rwlock-wr
BENCHMARKS += bench-pthread-spinlock
BENCHMARKS += bench-pagefault
BENCHMARKS += bench-eventfd

all: $(BENCHMARKS)

$(BENCHMARKS): build
	./build/$@ > results/$@.csv
	./plot.py results/$@.csv

build:
	@git submodule update --init --force --recursive
	@mkdir -p build && cmake -S . -B build && make --quiet --directory build
.PHONY: build
