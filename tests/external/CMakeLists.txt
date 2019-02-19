#
# Copyright 2018-2019, Intel Corporation
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

include(../ctest_helpers.cmake)

function(add_test_default name tracer)
	if ("${ARGN}" STREQUAL "")
		set(testcase "0")
	else()
		set(testcase "${ARGN}")
	endif()

	set(cmake_script ${CMAKE_CURRENT_SOURCE_DIR}/run_default.cmake)

	add_test_common(${name} ${tracer} ${testcase} ${cmake_script})
endfunction()

function(add_test_expect_failure name)
	set(srcs ${ARGN})
	prepend(srcs ${CMAKE_CURRENT_SOURCE_DIR} ${srcs})

	add_cppstyle(tests-external-${name} ${srcs})
	add_check_whitespace(tests-external-${name} ${srcs})

	add_executable(${name}_fail_expected ${srcs})
	set_target_properties(${name}_fail_expected PROPERTIES
				EXCLUDE_FROM_ALL TRUE
				EXCLUDE_FROM_DEFAULT_BUILD TRUE)

	add_test(NAME ${name}_fail_expected
		COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${name}_fail_expected
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

	set_tests_properties(${name}_fail_expected PROPERTIES WILL_FAIL TRUE)
endfunction()

build_test(array_at libcxx/array/at.pass.cpp)
add_test_default(array_at none)

build_test(array_begin libcxx/array/begin.pass.cpp)
add_test_default(array_begin none)

build_test(array_compare libcxx/array/compare.pass.cpp)
add_test_default(array_compare none)

build_test(array_contiguous libcxx/array/contiguous.pass.cpp)
add_test_default(array_contiguous none)

build_test(array_empty libcxx/array/empty.pass.cpp)
add_test_default(array_empty none)

build_test(array_front_back libcxx/array/front_back.pass.cpp)
add_test_default(array_front_back none)

build_test(array_indexing libcxx/array/indexing.pass.cpp)
add_test_default(array_indexing none)

build_test(array_max_size libcxx/array/max_size.pass.cpp)
add_test_default(array_max_size none)

if (MAX_ALIGN_TYPE_EXISTS)
	build_test(array_size_and_alignment libcxx/array/size_and_alignment.pass.cpp)
	add_test_default(array_size_and_alignment none)
endif()

build_test(array_types libcxx/array/types.pass.cpp)
add_test_default(array_types none)

add_test_expect_failure(array_compare_0 libcxx/array/compare.fail_0.cpp)
add_test_expect_failure(array_compare_1 libcxx/array/compare.fail_1.cpp)
add_test_expect_failure(array_compare_2 libcxx/array/compare.fail_2.cpp)

build_test(array_cons_defualt libcxx/array/array.cons/default.pass.cpp)
add_test_default(array_cons_defualt none)

add_test_expect_failure(array_copy_assignment libcxx/array/array.cons/copy_assignment.fail.cpp)

build_test(array_cons_implicit_copy libcxx/array/array.cons/implicit_copy.pass.cpp)
add_test_default(array_cons_implicit_copy none)

build_test(array_initializer_list libcxx/array/array.cons/initializer_list.pass.cpp)
add_test_default(array_initializer_list none)

if (MAX_ALIGN_TYPE_EXISTS)
	build_test(array_data_const libcxx/array/array.data/data_const.pass.cpp)
	add_test_default(array_data_const none)
endif()

if (MAX_ALIGN_TYPE_EXISTS)
	build_test(array_data libcxx/array/array.data/data.pass.cpp)
	add_test_default(array_data none)
endif()

add_test_expect_failure(array_fill libcxx/array/array.fill/fill.fail.cpp)

build_test(array_fill libcxx/array/array.fill/fill.pass.cpp)
add_test_default(array_fill none)

build_test(array_size libcxx/array/array.size/size.pass.cpp)
add_test_default(array_size none)

build_test(array_swap libcxx/array/array.special/swap.pass.cpp)
add_test_default(array_swap none)

add_test_expect_failure(array_swap libcxx/array/array.swap/swap.fail.cpp)

build_test(array_swap_2 libcxx/array/array.swap/swap.pass.cpp)
add_test_default(array_swap_2 none)

build_test(array_get_const_rv libcxx/array/array.tuple/get_const_rv.pass.cpp)
add_test_default(array_get_const_rv none)

build_test(array_get_const libcxx/array/array.tuple/get_const.pass.cpp)
add_test_default(array_get_const none)

build_test(array_get_rv libcxx/array/array.tuple/get_rv.pass.cpp)
add_test_default(array_get_rv none)

build_test(array_get libcxx/array/array.tuple/get.pass.cpp)
add_test_default(array_get none)

add_test_expect_failure(array_get libcxx/array/array.tuple/get.fail.cpp)
