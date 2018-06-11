#
# Copyright 2018, Intel Corporation
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

set(GLOBAL_TEST_ARGS
	-DPERL_EXECUTABLE=${PERL_EXECUTABLE}
	-DMATCH_SCRIPT=${PROJECT_SOURCE_DIR}/tests/match
	-DPARENT_DIR=${TEST_DIR}
	-DTESTS_USE_FORCED_PMEM=${TESTS_USE_FORCED_PMEM})

if(TRACE_TESTS)
	set(GLOBAL_TEST_ARGS ${GLOBAL_TEST_ARGS} --trace-expand)
endif()

set(INCLUDE_DIRS ${PMEMOBJ_INCLUDE_DIRS} common/ .. .)
set(LIBS_DIRS ${PMEMOBJ_LIBRARY_DIRS})

if(USE_LLVM_LIBCPP)
	set(INCLUDE_DIRS ${INCLUDE_DIRS} ${LIBCPP_INCDIR})
	set(LIBS_DIRS ${LIBS_DIRS} ${LIBCPP_LIBDIR})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdinc++ -lc++ -lc++abi")
	# XXX: -lcxxrt instead of -lc++abi on FreeBSD?
endif()

if(USE_CUSTOM_GCC)
	set(INCLUDE_DIRS ${INCLUDE_DIRS} ${GCC_INCDIR})
	set(LIBS_DIRS ${LIBS_DIRS} ${GCC_LIBDIR})
endif()

include_directories(${INCLUDE_DIRS})
link_directories(${LIBS_DIRS})

if(NOT WIN32)
	set(SAVED_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
	set(SAVED_CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES})

	# Check for issues with older clang compilers which assert on delete persistent<[][]>.
	set(CMAKE_REQUIRED_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/../include ${PMEMOBJ_INCLUDE_DIRS})
	set(CMAKE_REQUIRED_FLAGS "--std=c++11 -c")
	CHECK_CXX_SOURCE_COMPILES(
		"#include <libpmemobj++/make_persistent_array.hpp>
		using namespace pmem::obj;
		int main() {
			delete_persistent<int[][3]>(make_persistent<int[][3]>(2), 2);
			return 0;
		}"
		NO_CLANG_TEMPLATE_BUG)

	# This is a workaround for older incompatible versions of libstdc++ and clang.
	# Please see https://llvm.org/bugs/show_bug.cgi?id=15517 for more info.
	set(CMAKE_REQUIRED_FLAGS "--std=c++11 -include future")
	CHECK_CXX_SOURCE_COMPILES(
		"int main() { return 0; }"
		NO_CHRONO_BUG)

	set(CMAKE_REQUIRED_FLAGS ${SAVED_CMAKE_REQUIRED_FLAGS})
	set(CMAKE_REQUIRED_INCLUDES ${SAVED_CMAKE_REQUIRED_INCLUDES})
else()
	set(NO_CLANG_TEMPLATE_BUG TRUE)
	set(NO_CHRONO_BUG TRUE)
endif()

add_cppstyle("tests-common" ${CMAKE_CURRENT_SOURCE_DIR}/common/*.*pp)
add_check_whitespace("tests-common" ${CMAKE_CURRENT_SOURCE_DIR}/common/*.*pp)

function(build_test name srcs)
	# skip posix tests
	if(${name} MATCHES "posix$" AND WIN32)
		return()
	endif()

	add_cppstyle(tests-${name} ${CMAKE_CURRENT_SOURCE_DIR}/${srcs})
	add_check_whitespace(tests-${name} ${CMAKE_CURRENT_SOURCE_DIR}/${srcs})

	add_executable(${name} ${srcs})
	target_link_libraries(${name} ${PMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
	add_dependencies(tests ${name})
endfunction()

build_test(allocator allocator/allocator.cpp)
build_test(make_persistent make_persistent/make_persistent.cpp)
build_test(make_persistent_atomic make_persistent_atomic/make_persistent_atomic.cpp)
build_test(pool_primitives pool_primitives/pool_primitives.cpp)
build_test(mutex_posix mutex_posix/mutex_posix.cpp)
build_test(p_ext p_ext/p_ext.cpp)
build_test(pool pool/pool.cpp)
build_test(ptr ptr/ptr.cpp)
build_test(shared_mutex_posix shared_mutex_posix/shared_mutex_posix.cpp)
build_test(transaction transaction/transaction.cpp)
build_test(ptr_arith ptr_arith/ptr_arith.cpp)

#build_test(pool_win pool_win/pool_win.cpp) XXX: add toUTF8
#build_test(v v/v.cpp) XXX: fix compile error

if(NO_CHRONO_BUG)
	build_test(cond_var cond_var/cond_var.cpp)
	build_test(cond_var_posix cond_var_posix/cond_var_posix.cpp)
	build_test(mutex mutex/mutex.cpp)
	build_test(shared_mutex shared_mutex/shared_mutex.cpp)
	build_test(timed_mtx_posix timed_mtx_posix/timed_mtx_posix.cpp)
	build_test(timed_mtx timed_mtx/timed_mtx.cpp)
else()
	message(WARNING "Skipping chrono tests because of compiler/stdc++ issues")
endif()

if(NO_GCC_VARIADIC_TEMPLATE_BUG)
	# XXX: Ex libpmemobj cpp
else()
	message(WARNING "Skipping examples tests because of compiler issue")
endif()

if(NO_CLANG_TEMPLATE_BUG)
	build_test(make_persistent_array make_persistent_array/make_persistent_array.cpp)
	build_test(make_persistent_array_atomic make_persistent_array_atomic/make_persistent_array_atomic.cpp)
else()
	message(WARNING "Skipping array tests because of compiler issue")
endif()

if(USE_LLVM_LIBCPP OR USE_CUSTOM_GCC)
	build_test(deque deque/deque.cpp)
	build_test(priority_queue priority_queue/priority_queue.cpp)
	build_test(queue queue/queue.cpp)
	build_test(set set/set.cpp)
	build_test(stack stack/stack.cpp)
	build_test(list list/list.cpp)
	build_test(map map/map.cpp)
	build_test(multimap multimap/multimap.cpp)
	build_test(multiset multiset/multiset.cpp)
	build_test(unordered_map unordered_map/unordered_map.cpp)
	build_test(unordered_multimap unordered_multimap/unordered_multimap.cpp)
	build_test(unordered_multiset unordered_multiset/unordered_multiset.cpp)
	build_test(unordered_set unordered_set/unordered_set.cpp)
	build_test(vector vector/vector.cpp)
else()
	message(WARNING "Skipping container tests because a custom version of the standard library is not used")
endif()

# Configures testcase ${name} ${testcase} using tracer ${tracer}
function(add_testcase name tracer testcase)
	if ("${testcase}" STREQUAL "")
		set(testcase "0")
	endif()

	set(executable ${name})
	set(vg_tracers memcheck helgrind drd pmemcheck)

	if ((NOT VALGRIND_FOUND) AND ${tracer} IN_LIST vg_tracers)
		add_test(NAME ${name}_${testcase}_${tracer}_SKIPPED_BECAUSE_OF_MISSING_VALGRIND
				COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/true.cmake)
		return()
	endif()

	if(${tracer} STREQUAL pmemcheck)
		if (VALGRIND_PMEMCHECK_NOT_FOUND)
			add_test(NAME ${name}_${testcase}_${tracer}_SKIPPED_BECAUSE_OF_MISSING_PMEMCHECK
				COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/true.cmake)
			return()
		endif()
	endif()

	if (NOT TARGET ${name}) # if test was not build (because of compiler issue)
		add_test(NAME ${name}_${testcase}_${tracer}_SKIPPED_BECAUSE_OF_COMPILER_ISSUE
				COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/true.cmake)
		return()
	endif()

	if ((USE_ASAN OR USE_UBSAN) AND ${tracer} IN_LIST vg_tracers)
		return()
	endif()

	add_test(NAME ${executable}_${testcase}_${tracer}
			COMMAND ${CMAKE_COMMAND}
			${GLOBAL_TEST_ARGS}
			-DTEST_NAME=${executable}_${testcase}_${tracer}
			-DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}/${name}
			-DBIN_DIR=${CMAKE_CURRENT_BINARY_DIR}/${executable}_${testcase}_${tracer}
			-DTEST_EXECUTABLE=$<TARGET_FILE:${executable}>
			-DTRACER=${tracer}
			-DLONG_TESTS=${LONG_TESTS}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/${name}/${name}_${testcase}.cmake)

	set_tests_properties(${name}_${testcase}_${tracer} PROPERTIES
			ENVIRONMENT "LC_ALL=C;PATH=$ENV{PATH};"
			FAIL_REGULAR_EXPRESSION Sanitizer)

	# pmemcheck is a special snowflake and it doesn't set exit code when it
	# detects an error, so we have to look at its output
	if (${tracer} STREQUAL pmemcheck)
		set_tests_properties(${name}_${testcase}_${tracer} PROPERTIES
				PASS_REGULAR_EXPRESSION "ERROR SUMMARY: 0")
		# XXX: if we use FATAL_ERROR in test.cmake - pmemcheck passes anyway
		set_tests_properties(${name}_${testcase}_${tracer} PROPERTIES
				FAIL_REGULAR_EXPRESSION "CMake Error")
	endif()

	if (${tracer} STREQUAL pmemcheck)
		set_tests_properties(${name}_${testcase}_${tracer} PROPERTIES
				COST 100)
	elseif(${tracer} IN_LIST vg_tracers)
		set_tests_properties(${name}_${testcase}_${tracer} PROPERTIES
				COST 50)
	endif()
endfunction()

# adds test with name, tracer and optional testcase number
function(add_test_generic name tracer)
	if(${tracer} STREQUAL "")
	    set(tracer none)
	endif()

	# skip all tests with pmemcheck/memcheck/drd on windows
	if(${tracer} STREQUAL none OR NOT WIN32)
		add_testcase(${name} ${tracer} "${ARGN}")
	endif()
endfunction()

add_test_generic(allocator memcheck)
add_test_generic(allocator none)
add_test_generic(allocator pmemcheck)

add_test_generic(cond_var none)

add_test_generic(cond_var_posix drd)
add_test_generic(cond_var_posix helgrind)
add_test_generic(cond_var_posix pmemcheck)

add_test_generic(deque none)
add_test_generic(deque pmemcheck)

add_test_generic(list none)
add_test_generic(list pmemcheck)

add_test_generic(make_persistent none)
add_test_generic(make_persistent pmemcheck)

add_test_generic(make_persistent_array none)
add_test_generic(make_persistent_array pmemcheck)

add_test_generic(make_persistent_array_atomic none)
add_test_generic(make_persistent_array_atomic pmemcheck)

add_test_generic(make_persistent_atomic none)
add_test_generic(make_persistent_atomic pmemcheck)

add_test_generic(map none)
add_test_generic(map pmemcheck)

add_test_generic(multimap none)
add_test_generic(multimap pmemcheck)

add_test_generic(multiset none)
add_test_generic(multiset pmemcheck)

add_test_generic(mutex none)

add_test_generic(mutex_posix drd)
add_test_generic(mutex_posix helgrind)
add_test_generic(mutex_posix pmemcheck)

add_test_generic(pool none 0)
add_test_generic(pool none 1)
add_test_generic(pool none 2)
add_test_generic(pool none 3)

add_test_generic(pool_primitives none)
add_test_generic(pool_primitives pmemcheck)

add_test_generic(priority_queue none)
add_test_generic(priority_queue pmemcheck)

add_test_generic(ptr none)
add_test_generic(ptr pmemcheck)

add_test_generic(ptr_arith memcheck)
add_test_generic(ptr_arith pmemcheck)

# XXX Bug: incompatibility between asan and custom library
if (NOT USE_ASAN)
	add_test_generic(ptr_arith none)
endif()

add_test_generic(p_ext none)
add_test_generic(p_ext pmemcheck)

add_test_generic(queue none)
add_test_generic(queue pmemcheck)

add_test_generic(set none)
add_test_generic(set pmemcheck)

add_test_generic(shared_mutex none)

add_test_generic(shared_mutex_posix drd)
add_test_generic(shared_mutex_posix helgrind)
add_test_generic(shared_mutex_posix pmemcheck)

add_test_generic(stack none)
add_test_generic(stack pmemcheck)

add_test_generic(timed_mtx none)

add_test_generic(timed_mtx_posix drd)
add_test_generic(timed_mtx_posix helgrind)
add_test_generic(timed_mtx_posix pmemcheck)

add_test_generic(transaction none)
add_test_generic(transaction pmemcheck)

add_test_generic(unordered_map none)
add_test_generic(unordered_map pmemcheck)

add_test_generic(unordered_multimap none)
add_test_generic(unordered_multimap pmemcheck)

add_test_generic(unordered_multiset none)
add_test_generic(unordered_multiset pmemcheck)

add_test_generic(unordered_set none)
add_test_generic(unordered_set pmemcheck)

add_test_generic(v none)

add_test_generic(vector none)
add_test_generic(vector pmemcheck)
