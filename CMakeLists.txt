project(posix-bench LANGUAGES C CXX)

cmake_minimum_required(VERSION 3.10)

set(LIBS hdr_histogram_static pthread m hwloc)

add_subdirectory(HdrHistogram_c)

add_compile_options(-Wall -Wextra -Wno-unused -O2 -std=c++17 -g)

add_executable(bench-open bench-open.cpp)
target_link_libraries(bench-open ${LIBS})

add_executable(bench-pthread-create bench-pthread-create.cpp)
target_link_libraries(bench-pthread-create ${LIBS})

add_executable(bench-pthread-yield bench-pthread-yield.cpp)
target_link_libraries(bench-pthread-yield ${LIBS})

add_executable(bench-gettime bench-gettime.cpp)
target_link_libraries(bench-gettime ${LIBS})

add_executable(bench-pthread-mutex bench-pthread-mutex.cpp)
target_link_libraries(bench-pthread-mutex ${LIBS})

add_executable(bench-pthread-mutex-adaptive bench-pthread-mutex-adaptive.cpp)
target_link_libraries(bench-pthread-mutex-adaptive ${LIBS})

add_executable(bench-pthread-rwlock-rd bench-pthread-rwlock-rd.cpp)
target_link_libraries(bench-pthread-rwlock-rd ${LIBS})

add_executable(bench-pthread-rwlock-wr bench-pthread-rwlock-wr.cpp)
target_link_libraries(bench-pthread-rwlock-wr ${LIBS})

add_executable(bench-pthread-spinlock bench-pthread-spinlock.cpp)
target_link_libraries(bench-pthread-spinlock ${LIBS})

add_executable(bench-pagefault bench-pagefault.cpp)
target_link_libraries(bench-pagefault ${LIBS})

add_executable(bench-pagefault-large bench-pagefault-large.cpp)
target_link_libraries(bench-pagefault-large ${LIBS})

add_executable(bench-mmap-munmap bench-mmap-munmap.cpp)
target_link_libraries(bench-mmap-munmap ${LIBS})

add_executable(bench-mmap-populate-munmap bench-mmap-populate-munmap.cpp)
target_link_libraries(bench-mmap-populate-munmap ${LIBS})

add_executable(bench-mprotect bench-mprotect.cpp)
target_link_libraries(bench-mprotect ${LIBS})

add_executable(bench-eventfd bench-eventfd.cpp)
target_link_libraries(bench-eventfd ${LIBS})

add_executable(bench-eventfd-nonblock bench-eventfd-nonblock.cpp)
target_link_libraries(bench-eventfd-nonblock ${LIBS})
