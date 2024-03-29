# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020-2021, Intel Corporation

if(MSVC_VERSION)
	add_flag(-W2)
	add_flag("-D_FORTIFY_SOURCE=2" RELEASE)
else()
	add_flag(-Wall)
	add_flag("-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2" RELEASE)
endif()
add_flag(-Wpointer-arith)
add_flag(-Wsign-compare)
add_flag(-Wunreachable-code-return)
add_flag(-Wmissing-variable-declarations)
add_flag(-fno-common)
add_flag(-Wunused-macros)
add_flag(-Wsign-conversion)
add_flag(-Wno-maybe-uninitialized)

add_flag(-ggdb DEBUG)
add_flag(-DDEBUG DEBUG)

include_directories(${LIBPMEMOBJ_INCLUDE_DIRS} .)
link_directories(${LIBPMEMOBJ_LIBRARY_DIRS})

function(add_benchmark name)
	set(srcs ${ARGN})
	prepend(srcs ${CMAKE_CURRENT_SOURCE_DIR} ${srcs})
	add_executable(benchmark-${name} ${srcs})
	target_link_libraries(benchmark-${name} ${LIBPMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endfunction()

add_check_whitespace(benchmarks-cmake ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)

add_cppstyle(benchmarks-common ${CMAKE_CURRENT_SOURCE_DIR}/*.*pp)
add_check_whitespace(benchmarks-common ${CMAKE_CURRENT_SOURCE_DIR}/*.*pp)

add_cppstyle(benchmarks-concurrent_hash_map ${CMAKE_CURRENT_SOURCE_DIR}/concurrent_hash_map/*.*pp)
add_check_whitespace(benchmarks-concurrent_hash_map ${CMAKE_CURRENT_SOURCE_DIR}/concurrent_hash_map/*.*pp)

add_cppstyle(benchmarks-self-relative-pointer ${CMAKE_CURRENT_SOURCE_DIR}/self_relative_pointer/*.*pp)
add_check_whitespace(benchmarks-self-relative-pointer ${CMAKE_CURRENT_SOURCE_DIR}/self_relative_pointer/*.*pp)

add_cppstyle(benchmarks-radix_tree ${CMAKE_CURRENT_SOURCE_DIR}/radix/*.*pp)
add_check_whitespace(benchmarks-radix_tree ${CMAKE_CURRENT_SOURCE_DIR}/radix/*.*pp)

if (TEST_CONCURRENT_HASHMAP)
	add_benchmark(concurrent_hash_map_insert_open concurrent_hash_map/insert_open.cpp)
endif()

if (TEST_SELF_RELATIVE_POINTER)
	add_benchmark(self_relative_pointer_get self_relative_pointer/get.cpp)
	add_benchmark(self_relative_pointer_assignment self_relative_pointer/assignment.cpp)
endif()

if (TEST_RADIX_TREE)
	add_benchmark(radix_tree radix/radix_tree.cpp)
endif()
