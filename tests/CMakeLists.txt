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

include(ctest_helpers.cmake)

add_cppstyle(tests-common ${CMAKE_CURRENT_SOURCE_DIR}/common/*.*pp ${CMAKE_CURRENT_SOURCE_DIR}/*.c ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
add_cppstyle(tests-pmemobj_cow ${CMAKE_CURRENT_SOURCE_DIR}/pmemobj_check_cow/*.*cpp)
add_check_whitespace(tests-pmemobj_cow ${CMAKE_CURRENT_SOURCE_DIR}/pmemobj_check_cow/*.*cpp)
add_check_whitespace(tests-common ${CMAKE_CURRENT_SOURCE_DIR}/common/*.*pp ${CMAKE_CURRENT_SOURCE_DIR}/*.c ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
add_check_whitespace(tests-cmake ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)

if(MSVC_VERSION)
	add_flag(-W4)
	add_flag(-bigobj) # fix C1128 raised for some test binaries
else()
	add_flag(-Wall)
endif()
add_flag(-Wpointer-arith)
add_flag(-Wunused-macros)
add_flag(-Wsign-conversion)
add_flag(-Wsign-compare)
add_flag(-Wunreachable-code-return)
add_flag(-Wmissing-variable-declarations)
add_flag(-fno-common)

add_flag(-ggdb DEBUG)
add_flag(-DDEBUG DEBUG)

add_flag("-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2" RELEASE)

if(USE_ASAN)
       add_sanitizer_flag(address)
endif()
if(USE_UBSAN)
       add_sanitizer_flag(undefined)
endif()

if(COVERAGE)
       set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -coverage")
endif()

add_library(test_backtrace STATIC test_backtrace.c)
add_library(valgrind_internal STATIC valgrind_internal.cpp)

function(build_example_queue)
	add_executable(ex-queue ../examples/queue/queue.cpp)
	target_include_directories(ex-queue PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../examples)
	target_link_libraries(ex-queue ${LIBPMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endfunction()

function(build_example_pman)
	if(CURSES_FOUND AND NOT WIN32)
		add_executable(ex-pman ../examples/pman/pman.cpp)
		target_include_directories(ex-pman PUBLIC ${CURSES_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../examples)
		target_link_libraries(ex-pman ${LIBPMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CURSES_LIBRARIES})
	else()
		message(WARNING "ncurses not found - pman test won't be build")
	endif()
endfunction()

find_packages()

if(VALGRIND_FOUND)
	add_flag(-DLIBPMEMOBJ_CPP_VG_MEMCHECK_ENABLED=1)
	add_flag(-DLIBPMEMOBJ_CPP_VG_DRD_ENABLED=1)
	add_flag(-DLIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED=1)

	if(VALGRIND_PMEMCHECK_FOUND)
		add_flag(-DLIBPMEMOBJ_CPP_VG_PMEMCHECK_ENABLED=1)
	endif()
endif()

if(NO_GCC_VARIADIC_TEMPLATE_BUG)
	build_example_queue()
	add_test_generic(NAME ex-queue CASE 0 TRACERS none)

	build_example_pman()
	add_test_generic(NAME ex-pman CASE 0 TRACERS none)
else()
	message(WARNING "Skipping examples tests because of gcc variadic template bug")
	skip_test("examples_tests" "SKIPPED_BECAUSE_OF_GCC_VARIADIC_TEMPLATE_BUG")
endif()

if(AGGREGATE_INITIALIZATION_AVAILABLE)
	build_test(aggregate_initialization aggregate_initialization/aggregate_initialization.cpp)
	add_test_generic(NAME aggregate_initialization TRACERS none pmemcheck)
else()
	message(WARNING "Skipping aggregate initialization test because of no compiler support")
	skip_test("aggregate_initialization" "SKIPPED_BECAUSE_OF_NO_COMPILER_SUPPORT")
endif()

build_test(allocator allocator/allocator.cpp)
add_test_generic(NAME allocator TRACERS none memcheck pmemcheck)

build_test(detail_common detail_common/detail_common.cpp)
add_test_generic(NAME detail_common TRACERS none)

build_test(make_persistent make_persistent/make_persistent.cpp)
add_test_generic(NAME make_persistent TRACERS none pmemcheck)

build_test(make_persistent_atomic make_persistent_atomic/make_persistent_atomic.cpp)
add_test_generic(NAME make_persistent_atomic TRACERS none pmemcheck)

build_test(mutex_posix mutex_posix/mutex_posix.cpp)
add_test_generic(NAME mutex_posix TRACERS drd helgrind pmemcheck)

build_test(pool pool/pool.cpp)
add_test_generic(NAME pool CASE 0 TRACERS none)
add_test_generic(NAME pool CASE 1 TRACERS none)
add_test_generic(NAME pool CASE 2 TRACERS none)
add_test_generic(NAME pool CASE 3 TRACERS none)
add_test_generic(NAME pool CASE 4 TRACERS none)
add_test_generic(NAME pool CASE 5 TRACERS none)

build_test(pool_primitives pool_primitives/pool_primitives.cpp)
add_test_generic(NAME pool_primitives CASE 0 TRACERS none pmemcheck)

build_test(ptr ptr/ptr.cpp)
add_test_generic(NAME ptr CASE 0 TRACERS none pmemcheck)

build_test(ptr_arith ptr_arith/ptr_arith.cpp)
add_test_generic(NAME ptr_arith TRACERS memcheck pmemcheck)
# XXX Bug: incompatibility between asan and custom library
if (NOT USE_ASAN)
	add_test_generic(NAME ptr_arith TRACERS none)
endif()

build_test(p_ext p_ext/p_ext.cpp)
add_test_generic(NAME p_ext TRACERS none pmemcheck)

build_test(shared_mutex_posix shared_mutex_posix/shared_mutex_posix.cpp)
add_test_generic(NAME shared_mutex_posix TRACERS drd helgrind pmemcheck)

build_test(transaction transaction/transaction.cpp)
add_test_generic(NAME transaction TRACERS none pmemcheck)

if(WIN32)
	build_test(pool_win pool_win/pool_win.cpp)
	add_test_generic(NAME pool_win CASE 0 TRACERS none)
	add_test_generic(NAME pool_win CASE 1 TRACERS none)
	add_test_generic(NAME pool_win CASE 2 TRACERS none)
	add_test_generic(NAME pool_win CASE 3 TRACERS none)
endif()

if(PMEMVLT_PRESENT)
	build_test(v v/v.cpp)
	add_test_generic(NAME v CASE 0 TRACERS none memcheck)
else()
	message(WARNING "Skipping v test because no pmemvlt support found")
	skip_test("v" "SKIPPED_BECAUSE_OF_MISSING_PMEMVLT")
endif()

if(NO_CHRONO_BUG)
	build_test(cond_var cond_var/cond_var.cpp)
	add_test_generic(NAME cond_var TRACERS none)

	build_test(cond_var_posix cond_var_posix/cond_var_posix.cpp)
	add_test_generic(NAME cond_var_posix TRACERS drd helgrind pmemcheck)

	build_test(mutex mutex/mutex.cpp)
	add_test_generic(NAME mutex TRACERS none)

	build_test(shared_mutex shared_mutex/shared_mutex.cpp)
	add_test_generic(NAME shared_mutex TRACERS none)

	build_test(timed_mtx_posix timed_mtx_posix/timed_mtx_posix.cpp)
	add_test_generic(NAME timed_mtx_posix TRACERS drd helgrind pmemcheck)

	build_test(timed_mtx timed_mtx/timed_mtx.cpp)
	add_test_generic(NAME timed_mtx TRACERS none)
else()
	message(WARNING "Skipping chrono tests because of compiler/stdc++ issues")
	skip_test("chrono_tests" "SKIPPED_BECAUSE_OF_COMPILER_CHRONO_BUG")
endif()

if(NO_CLANG_TEMPLATE_BUG)
	build_test(make_persistent_array make_persistent_array/make_persistent_array.cpp)
	add_test_generic(NAME make_persistent_array TRACERS none pmemcheck)

	build_test(make_persistent_array_atomic make_persistent_array_atomic/make_persistent_array_atomic.cpp)
	add_test_generic(NAME make_persistent_array_atomic TRACERS none pmemcheck)
else()
	message(WARNING "Skipping array tests because of clang template bug")
	skip_test("make_persistent_array" "SKIPPED_BECAUSE_OF_CLANG_TEMPLATE_BUG")
endif()

build_test(iterator_traits iterator_traits/iterator_traits.cpp)
add_test_generic(NAME iterator_traits TRACERS none)

build_test(ctl ctl/ctl.cpp)
add_test_generic(NAME ctl CASE 0 TRACERS none)

if(WIN32)
	build_test(ctl_win ctl_win/ctl_win.cpp)
	add_test_generic(NAME ctl_win CASE 0 TRACERS none)
endif()

if (ENABLE_ARRAY)
	build_test(array_algorithms array_algorithms/array_algorithms.cpp)
	add_test_generic(NAME array_algorithms TRACERS none pmemcheck)

	build_test(array_slice array_slice/array_slice.cpp)
	add_test_generic(NAME array_slice CASE 0 TRACERS none pmemcheck memcheck)

	build_test(array_iterator array_iterator/array_iterator.cpp)
	add_test_generic(NAME array_iterator TRACERS none pmemcheck)

	build_test(array_modifiers array_modifiers/array_modifiers.cpp)
	add_test_generic(NAME array_modifiers TRACERS none pmemcheck memcheck)

	if(PMREORDER_SUPPORTED)
		build_test(array_slice_pmreorder array_slice_pmreorder/array_slice_pmreorder.cpp)
		add_test_generic(NAME array_slice_pmreorder CASE 0 TRACERS none)
		add_test_generic(NAME array_slice_pmreorder CASE 1 TRACERS none)
	else()
		message(WARNING "Skipping pmreorder tests because of no pmreorder support")
	endif()
endif()

if (ENABLE_VECTOR)
	build_test(temp_value temp_value/temp_value.cpp)
	add_test_generic(NAME temp_value TRACERS none pmemcheck memcheck)

	build_test(vector_assign_exceptions_length vector_assign_exceptions_length/vector_assign_exceptions_length.cpp)
	add_test_generic(NAME vector_assign_exceptions_length TRACERS none memcheck pmemcheck)

	build_test(vector_assign_exceptions_oom vector_assign_exceptions_oom/vector_assign_exceptions_oom.cpp)
	add_test_generic(NAME vector_assign_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test(vector_assign_txabort vector_assign_txabort/vector_assign_txabort.cpp)
	add_test_generic(NAME vector_assign_txabort TRACERS none memcheck pmemcheck)

	build_test(vector_comp_operators vector_comp_operators/vector_comp_operators.cpp)
	add_test_generic(NAME vector_comp_operators TRACERS none memcheck pmemcheck)

	build_test(vector_capacity_exceptions_length vector_capacity_exceptions_length/vector_capacity_exceptions_length.cpp)
	add_test_generic(NAME vector_capacity_exceptions_length TRACERS none memcheck pmemcheck)

	build_test(vector_capacity_exceptions_oom vector_capacity_exceptions_oom/vector_capacity_exceptions_oom.cpp)
	add_test_generic(NAME vector_capacity_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test(vector_capacity_txabort vector_capacity_txabort/vector_capacity_txabort.cpp)
	add_test_generic(NAME vector_capacity_txabort TRACERS none memcheck pmemcheck)

	build_test(vector_ctor_exceptions_nopmem vector_ctor_exceptions_nopmem/vector_ctor_exceptions_nopmem.cpp)
	add_test_generic(NAME vector_ctor_exceptions_nopmem TRACERS none memcheck )

	build_test(vector_ctor_exceptions_notx vector_ctor_exceptions_notx/vector_ctor_exceptions_notx.cpp)
	add_test_generic(NAME vector_ctor_exceptions_notx TRACERS none memcheck)

	build_test(vector_ctor_exceptions_oom vector_ctor_exceptions_oom/vector_ctor_exceptions_oom.cpp)
	add_test_generic(NAME vector_ctor_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test(vector_ctor_move vector_ctor_move/vector_ctor_move.cpp)
	add_test_generic(NAME vector_ctor_move TRACERS none memcheck pmemcheck)

	build_test(vector_ctor_capacity vector_ctor_capacity/vector_ctor_capacity.cpp)
	add_test_generic(NAME vector_ctor_capacity TRACERS none memcheck pmemcheck)

	build_test(vector_dtor vector_dtor/vector_dtor.cpp)
	add_test_generic(NAME vector_dtor TRACERS none memcheck pmemcheck)

	build_test(vector_iterators_access vector_iterators_access/vector_iterators_access.cpp)
	add_test_generic(NAME vector_iterators_access TRACERS none memcheck pmemcheck)

	build_test(vector_ctor_check_copy vector_ctor_check_copy/vector_ctor_check_copy.cpp)
	add_test_generic(NAME vector_ctor_check_copy TRACERS none memcheck pmemcheck)

	build_test(vector_modifiers_exceptions_oom vector_modifiers_exceptions_oom/vector_modifiers_exceptions_oom.cpp)
	add_test_generic(NAME vector_modifiers_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test(vector_modifiers_txabort vector_modifiers_txabort/vector_modifiers_txabort.cpp)
	add_test_generic(NAME vector_modifiers_txabort TRACERS none memcheck pmemcheck)

	build_test(vector_modifiers_type_requirements vector_modifiers_type_requirements/vector_modifiers_type_requirements.cpp)
	add_test_generic(NAME vector_modifiers_type_requirements TRACERS none memcheck pmemcheck)

	build_test(vector_std_arg vector_std_arg/vector_std_arg.cpp)
	add_test_generic(NAME vector_std_arg TRACERS none memcheck pmemcheck)

	build_test(vector_range vector_range/vector_range.cpp)
	add_test_generic(NAME vector_range TRACERS none memcheck pmemcheck)
endif()

if (ENABLE_STRING)
	build_test(string_access string_access/string_access.cpp)
	add_test_generic(NAME string_access TRACERS none memcheck pmemcheck)

	build_test(string_exceptions string_exceptions/string_exceptions.cpp)
	add_test_generic(NAME string_exceptions TRACERS none memcheck pmemcheck)

	build_test(string_snapshot string_snapshot/string_snapshot.cpp)
	add_test_generic(NAME string_snapshot TRACERS none memcheck pmemcheck)

	build_test(string_assign_tx_abort string_assign_tx_abort/string_assign_tx_abort.cpp)
	add_test_generic(NAME string_assign_tx_abort TRACERS none memcheck pmemcheck)
endif()

add_subdirectory(external)
