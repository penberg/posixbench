project(posix-bench)
cmake_minimum_required(VERSION 3.13)

set(LIBS hdr_histogram_static pthread m)

add_subdirectory(HdrHistogram_c)

add_compile_options(-Wall -Wextra -Wno-unused -O2)

add_executable(bench-pthread-nop bench-pthread-nop.c)
target_link_libraries(bench-pthread-nop ${LIBS})

add_executable(bench-pthread-yield bench-pthread-yield.c)
target_link_libraries(bench-pthread-yield ${LIBS})

add_executable(bench-pthread-mutex bench-pthread-mutex.c)
target_link_libraries(bench-pthread-mutex ${LIBS})

add_executable(bench-pthread-rwlock-rd bench-pthread-rwlock-rd.c)
target_link_libraries(bench-pthread-rwlock-rd ${LIBS})

add_executable(bench-pthread-rwlock-wr bench-pthread-rwlock-wr.c)
target_link_libraries(bench-pthread-rwlock-wr ${LIBS})

add_executable(bench-pthread-spinlock bench-pthread-spinlock.c)
target_link_libraries(bench-pthread-spinlock ${LIBS})
