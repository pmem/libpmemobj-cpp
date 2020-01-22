#
# Copyright 2020, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

if(MSVC_VERSION)
	add_flag(-W4)
else()
	add_flag(-Wall)
endif()
add_flag(-Wpointer-arith)
add_flag(-Wsign-compare)
add_flag(-Wunreachable-code-return)
add_flag(-Wmissing-variable-declarations)
add_flag(-fno-common)
add_flag(-Wunused-macros)
add_flag(-Wsign-conversion)

add_flag(-ggdb DEBUG)
add_flag(-DDEBUG DEBUG)

add_flag("-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2" RELEASE)

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

if (TEST_CONCURRENT_HASHMAP)
	add_benchmark(concurrent_hash_map_insert_open concurrent_hash_map/insert_open.cpp)
endif()
