## POSIX Benchmark Results

### No operation 

The time to do nothing. This benchmark shows the baseline measurement overhead.

![pthread-nop](bench-pthread-nop.png)

[[Benchmark](https://github.com/penberg/posixbench/blob/master//bench-pthread-nop.c)] [[CSV](bench-pthread-nop.csv)]

### Yield the processor (process)

The time it takes for a process to run again after yielding the processor with `pthread_yield`.

![pthread-yield](bench-yield.png)

[[Benchmark](https://github.com/penberg/posixbench/blob/master/bench-yield.c)] [[CSV](bench-yield.csv)]

### Yield the processor (thread)

The time it takes for a thread to run again after yielding the processor with `pthread_yield`.

![pthread-yield](bench-pthread-yield.png)

[[Benchmark](https://github.com/penberg/posixbench/blob/master//bench-pthread-yield.c)] [[CSV](bench-pthread-yield.csv)]

### Acquire and release a mutex

The time it takes for a thread to acquire and release a mutex that is under contention.

![pthread-mutex](bench-pthread-mutex.png)

[[Benchmark](https://github.com/penberg/posixbench/blob/master//bench-pthread-mutex.c)] [[CSV](bench-pthread-mutex.csv)]

### Acquire and release a rwlock

### Read lock

The time it takes for a thread to acquire and release a rwlock that is under contention for _reading_.

![pthread-rwlock](bench-pthread-rwlock-rd.png)

[[Benchmark](https://github.com/penberg/posixbench/blob/master//bench-pthread-rwlock-rd.c)] [[CSV](bench-pthread-rwlock-rd.csv)]

### Write lock

The time it takes for a thread to acquire and release a rwlock that is under contention for _writing_.

![pthread-rwlock](bench-pthread-rwlock-wr.png)

[[Benchmark](https://github.com/penberg/posixbench/blob/master//bench-pthread-rwlock-wr.c)] [[CSV](bench-pthread-rwlock-wr.csv)]

### Acquire and release a spinlock

The time it takes for a thread to acquire and release a spinlock that is under contention.

![pthread-spinlock](bench-pthread-spinlock.png)

[[Benchmark](https://github.com/penberg/posixbench/blob/master//bench-pthread-spinlock.c)] [[CSV](bench-pthread-spinlock.csv)]

### Thread notification (eventfd)

The time it takes for a thread to wake up when another thread notifies it via eventfd.

![pthread-eventfd](bench-eventfd.png)

[[Benchmark](https://github.com/penberg/posixbench/blob/master/bench-eventfd.c)] [[CSV](bench-eventfd.csv)]