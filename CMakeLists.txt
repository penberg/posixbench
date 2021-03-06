project(posix-bench LANGUAGES C CXX)

cmake_minimum_required(VERSION 3.10)

set(LIBS hdr_histogram_static pthread m hwloc)

add_subdirectory(HdrHistogram_c)

add_compile_options(-Wall -Wextra -Wno-unused -O2 -std=c++17 -g)

#
# syscall
#

add_executable(bench-getuid bench-getuid.cpp)
target_link_libraries(bench-getuid ${LIBS})

#
# vDSO
#

add_executable(bench-gettime bench-gettime.cpp)
target_link_libraries(bench-gettime ${LIBS})

#
# filesystem
#

add_executable(bench-open bench-open.cpp)
target_link_libraries(bench-open ${LIBS})

add_executable(bench-close bench-close.cpp)
target_link_libraries(bench-close ${LIBS})

#
# pthreads
#

add_executable(bench-pthread-create bench-pthread-create.cpp)
target_link_libraries(bench-pthread-create ${LIBS})

add_executable(bench-pthread-yield bench-pthread-yield.cpp)
target_link_libraries(bench-pthread-yield ${LIBS})

add_executable(bench-pthread-kill bench-pthread-kill.cpp)
target_link_libraries(bench-pthread-kill ${LIBS})

add_executable(bench-pthread-mutex bench-pthread-mutex.cpp)
target_link_libraries(bench-pthread-mutex ${LIBS})

#
# pthread locking
#

add_executable(bench-pthread-mutex-adaptive bench-pthread-mutex-adaptive.cpp)
target_link_libraries(bench-pthread-mutex-adaptive ${LIBS})

add_executable(bench-pthread-rwlock-rd bench-pthread-rwlock-rd.cpp)
target_link_libraries(bench-pthread-rwlock-rd ${LIBS})

add_executable(bench-pthread-rwlock-wr bench-pthread-rwlock-wr.cpp)
target_link_libraries(bench-pthread-rwlock-wr ${LIBS})

add_executable(bench-pthread-spinlock bench-pthread-spinlock.cpp)
target_link_libraries(bench-pthread-spinlock ${LIBS})

#
# pagefaults
#

add_executable(bench-pagefault-small bench-pagefault-small.cpp)
target_link_libraries(bench-pagefault-small ${LIBS})

add_executable(bench-pagefault-large bench-pagefault-large.cpp)
target_link_libraries(bench-pagefault-large ${LIBS})

add_executable(bench-pagefault-signal bench-pagefault-signal.cpp)
target_link_libraries(bench-pagefault-signal ${LIBS})

#
# mmap
#

add_executable(bench-mmap-munmap-4kb bench-mmap-munmap-4kb.cpp)
target_link_libraries(bench-mmap-munmap-4kb ${LIBS})

add_executable(bench-mmap-munmap-2mb bench-mmap-munmap-2mb.cpp)
target_link_libraries(bench-mmap-munmap-2mb ${LIBS})

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
add_executable(bench-mmap-populate-munmap-4kb bench-mmap-populate-munmap-4kb.cpp)
target_link_libraries(bench-mmap-populate-munmap-4kb ${LIBS})

add_executable(bench-mmap-populate-munmap-2mb bench-mmap-populate-munmap-2mb.cpp)
target_link_libraries(bench-mmap-populate-munmap-2mb ${LIBS})
endif()

add_executable(bench-mmap-4kb bench-mmap-4kb.cpp)
target_link_libraries(bench-mmap-4kb ${LIBS})

add_executable(bench-munmap-4kb bench-munmap-4kb.cpp)
target_link_libraries(bench-munmap-4kb ${LIBS})

add_executable(bench-mmap-2mb bench-mmap-2mb.cpp)
target_link_libraries(bench-mmap-2mb ${LIBS})

add_executable(bench-munmap-2mb bench-munmap-2mb.cpp)
target_link_libraries(bench-munmap-2mb ${LIBS})

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
add_executable(bench-mmap-populate-4kb bench-mmap-populate-4kb.cpp)
target_link_libraries(bench-mmap-populate-4kb ${LIBS})

add_executable(bench-munmap-populated-4kb bench-munmap-populated-4kb.cpp)
target_link_libraries(bench-munmap-populated-4kb ${LIBS})

add_executable(bench-mmap-populate-2mb bench-mmap-populate-2mb.cpp)
target_link_libraries(bench-mmap-populate-2mb ${LIBS})

add_executable(bench-munmap-populated-2mb bench-munmap-populated-2mb.cpp)
target_link_libraries(bench-munmap-populated-2mb ${LIBS})
endif()

#
# mprotect
#

add_executable(bench-mprotect bench-mprotect.cpp)
target_link_libraries(bench-mprotect ${LIBS})

#
# eventfd
#

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
add_executable(bench-eventfd bench-eventfd.cpp)
target_link_libraries(bench-eventfd ${LIBS})

add_executable(bench-eventfd-nonblock bench-eventfd-nonblock.cpp)
target_link_libraries(bench-eventfd-nonblock ${LIBS})
endif()
