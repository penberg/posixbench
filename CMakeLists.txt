project(posix-bench)
cmake_minimum_required(VERSION 3.10)

set(LIBS hdr_histogram_static pthread m)

add_subdirectory(HdrHistogram_c)

add_compile_options(-Wall -Wextra -Wno-unused -O2)

add_executable(bench-yield bench-yield.c)
target_link_libraries(bench-yield ${LIBS})

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

add_executable(bench-pagefault bench-pagefault.c)
target_link_libraries(bench-pagefault ${LIBS})

add_executable(bench-eventfd bench-eventfd.c)
target_link_libraries(bench-eventfd ${LIBS})
