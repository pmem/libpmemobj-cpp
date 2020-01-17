#
# Copyright 2018-2020, Intel Corporation
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

add_cppstyle(tests-common ${CMAKE_CURRENT_SOURCE_DIR}/common/*.*pp ${CMAKE_CURRENT_SOURCE_DIR}/*.c ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/check_is_pmem/*.*pp)
add_check_whitespace(tests-common ${CMAKE_CURRENT_SOURCE_DIR}/common/*.*pp ${CMAKE_CURRENT_SOURCE_DIR}/*.c ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/check_is_pmem/*.*pp)
add_check_whitespace(tests-cmake ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)

if(MSVC_VERSION)
	add_flag(-W3)

	# disable warning C4996 - disable checking iterators
	add_flag(-D_SCL_SECURE_NO_WARNINGS)

	# disable warning C4996 - disable deprecation of getenv and strcpy functions
	add_flag(-D_CRT_SECURE_NO_WARNINGS)

	# fix C1128 raised for some test binaries
	add_flag(-bigobj)
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

find_packages()

add_library(test_backtrace STATIC test_backtrace.c)
if(LIBUNWIND_FOUND)
	target_compile_definitions(test_backtrace PUBLIC USE_LIBUNWIND=1)
endif()

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

add_executable(check_is_pmem check_is_pmem/check_is_pmem.cpp)
target_link_libraries(check_is_pmem ${LIBPMEM_LIBRARIES})

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

	if(NOT CLANG_DESTRUCTOR_REFERENCE_BUG_PRESENT)
		build_test(array_modifiers array_modifiers/array_modifiers.cpp)
		add_test_generic(NAME array_modifiers TRACERS none pmemcheck memcheck)
	else()
		message(WARNING "skipping array_modifiers test - it requires clang >= ${CLANG_REQUIRED_BY_DESTRUCTOR_REFERENCE_BUG}")
	endif()

	if(PMREORDER_SUPPORTED)
		build_test(array_slice_pmreorder array_slice_pmreorder/array_slice_pmreorder.cpp)
		add_test_generic(NAME array_slice_pmreorder CASE 0 TRACERS none)
		add_test_generic(NAME array_slice_pmreorder CASE 1 TRACERS none)
	else()
		message(WARNING "Skipping pmreorder tests because of no pmreorder support")
	endif()
endif()

if (ENABLE_VECTOR)
	build_test_ext(NAME vector_temp_value SRC_FILES temp_value/temp_value.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_temp_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_assign_exceptions_length SRC_FILES vector_assign_exceptions_length/vector_assign_exceptions_length.cpp BUILD_OPTIONS -DVECTOR)

	# array-bounds warning is expected
	check_cxx_compiler_flag(-Wno-array-bounds wno_array_bounds_flag)
	if (wno_array_bounds_flag)
		target_compile_options(vector_assign_exceptions_length PUBLIC -Wno-array-bounds)
	endif()

	add_test_generic(NAME vector_assign_exceptions_length TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_assign_exceptions_oom SRC_FILES vector_assign_exceptions_oom/vector_assign_exceptions_oom.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_assign_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_assign_txabort SRC_FILES vector_assign_txabort/vector_assign_txabort.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_assign_txabort TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_comp_operators SRC_FILES vector_comp_operators/vector_comp_operators.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_comp_operators TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_capacity_exceptions_length SRC_FILES vector_capacity_exceptions_length/vector_capacity_exceptions_length.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_capacity_exceptions_length TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_capacity_exceptions_oom SRC_FILES vector_capacity_exceptions_oom/vector_capacity_exceptions_oom.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_capacity_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_capacity_txabort SRC_FILES vector_capacity_txabort/vector_capacity_txabort.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_capacity_txabort TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_ctor_exceptions_nopmem SRC_FILES vector_ctor_exceptions_nopmem/vector_ctor_exceptions_nopmem.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_ctor_exceptions_nopmem TRACERS none memcheck )

	build_test_ext(NAME vector_ctor_exceptions_notx SRC_FILES vector_ctor_exceptions_notx/vector_ctor_exceptions_notx.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_ctor_exceptions_notx TRACERS none memcheck)

	build_test_ext(NAME vector_ctor_exceptions_oom SRC_FILES vector_ctor_exceptions_oom/vector_ctor_exceptions_oom.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_ctor_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_ctor_move SRC_FILES vector_ctor_move/vector_ctor_move.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_ctor_move TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_ctor_capacity SRC_FILES vector_ctor_capacity/vector_ctor_capacity.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_ctor_capacity TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_dtor SRC_FILES vector_dtor/vector_dtor.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_dtor TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_iterators_access SRC_FILES vector_iterators_access/vector_iterators_access.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_iterators_access TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_data_access SRC_FILES vector_iterators_access/vector_data_access.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_data_access TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_ctor_check_copy SRC_FILES vector_ctor_check_copy/vector_ctor_check_copy.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_ctor_check_copy TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_modifiers_exceptions_oom SRC_FILES vector_modifiers_exceptions_oom/vector_modifiers_exceptions_oom.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_modifiers_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_modifiers_txabort SRC_FILES vector_modifiers_txabort/vector_modifiers_txabort.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_modifiers_txabort TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_modifiers_type_requirements SRC_FILES vector_modifiers_type_requirements/vector_modifiers_type_requirements.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_modifiers_type_requirements TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_std_arg SRC_FILES vector_std_arg/vector_std_arg.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_std_arg TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_range SRC_FILES vector_range/vector_range.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_range TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_layout SRC_FILES vector_layout/vector_layout.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_layout TRACERS none)
endif()

if (ENABLE_STRING)
	build_test(string_access string_access/string_access.cpp)
	add_test_generic(NAME string_access TRACERS none memcheck pmemcheck)

	build_test(string_capacity string_capacity/string_capacity.cpp)
	add_test_generic(NAME string_capacity TRACERS none memcheck pmemcheck)

	build_test(string_exceptions string_exceptions/string_exceptions.cpp)
	add_test_generic(NAME string_exceptions TRACERS none memcheck pmemcheck)

	build_test(string_modifiers string_modifiers/string_modifiers.cpp)
	add_test_generic(NAME string_modifiers TRACERS none memcheck pmemcheck)

	build_test(string_snapshot string_snapshot/string_snapshot.cpp)
	add_test_generic(NAME string_snapshot TRACERS none memcheck pmemcheck)

	build_test(string_assign_tx_abort string_assign_tx_abort/string_assign_tx_abort.cpp)
	add_test_generic(NAME string_assign_tx_abort TRACERS none memcheck pmemcheck)

	build_test(string_layout string_layout/string_layout.cpp)
	add_test_generic(NAME string_layout TRACERS none)
endif()

if(PMEMVLT_PRESENT AND ENABLE_CONCURRENT_HASHMAP)
	build_test(concurrent_hash_map_tx concurrent_hash_map_tx/concurrent_hash_map_tx.cpp)
	add_test_generic(NAME concurrent_hash_map_tx TRACERS none memcheck pmemcheck)

	build_test(concurrent_hash_map_insert_lookup concurrent_hash_map_insert_lookup/concurrent_hash_map_insert_lookup.cpp)
	add_test_generic(NAME concurrent_hash_map_insert_lookup CASE 0 TRACERS none memcheck pmemcheck)

	build_test(concurrent_hash_map_insert_erase concurrent_hash_map_insert_erase/concurrent_hash_map_insert_erase.cpp)
	add_test_generic(NAME concurrent_hash_map_insert_erase CASE 0 TRACERS none memcheck pmemcheck)

	build_test(concurrent_hash_map_rehash concurrent_hash_map_rehash/concurrent_hash_map_rehash.cpp)
	add_test_generic(NAME concurrent_hash_map_rehash CASE 0 TRACERS none memcheck pmemcheck)

	# This test can NOT be run under helgrind as it will report wrong lock ordering. Helgrind is right about
	# possible deadlock situation but that could only happen in case of wrong API usage.
	build_test(concurrent_hash_map_deadlock concurrent_hash_map_deadlock/concurrent_hash_map_deadlock.cpp)
	add_test_generic(NAME concurrent_hash_map_deadlock CASE 0 TRACERS none drd memcheck pmemcheck)

	if(TESTS_CONCURRENT_HASH_MAP_DRD_HELGRIND)
		add_test_generic(NAME concurrent_hash_map_insert_lookup CASE 0 TRACERS helgrind)

		add_test_generic(NAME concurrent_hash_map_insert_erase CASE 0 TRACERS helgrind)

		if (TESTS_LONG)
			add_test_generic(NAME concurrent_hash_map_insert_lookup CASE 0 TRACERS drd)

			add_test_generic(NAME concurrent_hash_map_insert_erase CASE 0 TRACERS drd)
		endif()

		add_test_generic(NAME concurrent_hash_map_rehash CASE 0 TRACERS helgrind)
	endif()

	build_test(concurrent_hash_map_rehash_check concurrent_hash_map_rehash_check/concurrent_hash_map_rehash_check.cpp)
	add_test_generic(NAME concurrent_hash_map_rehash_check TRACERS none memcheck pmemcheck)

	build_test(concurrent_hash_map_singlethread concurrent_hash_map_singlethread/concurrent_hash_map_singlethread.cpp)
	add_test_generic(NAME concurrent_hash_map_singlethread TRACERS none memcheck pmemcheck)

	build_test(concurrent_hash_map_layout concurrent_hash_map_layout/concurrent_hash_map_layout.cpp)
	add_test_generic(NAME concurrent_hash_map_layout TRACERS none)

	build_test(concurrent_hash_map_rehash_break concurrent_hash_map_rehash_break/concurrent_hash_map_rehash_break.cpp)

	if(NOT WIN32)
		# add 2 concurrent_hash_map_rehash_break tests
		set(CURRENT_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/concurrent_hash_map_rehash_break)
		set(CMAKE_SCRIPT ${CURRENT_TEST_DIR}/concurrent_hash_map_rehash_break)
		foreach(TESTCASE RANGE 1)
			configure_file("${CMAKE_SCRIPT}.cmake.in" "${CMAKE_SCRIPT}_${TESTCASE}.cmake" @ONLY)
			add_test_generic(NAME concurrent_hash_map_rehash_break CASE ${TESTCASE} TRACERS none)
		endforeach()
	endif()

	if(NOT TBB_FOUND)
		message(WARNING "Skipping concurrent_hash_map_tbb test because Intel TBB library was not found")
	else()
		build_test_tbb(concurrent_hash_map_tbb_insert_lookup concurrent_hash_map_insert_lookup/concurrent_hash_map_tbb_insert_lookup.cpp)
		add_test_generic(NAME concurrent_hash_map_tbb_insert_lookup TRACERS none pmemcheck)

		build_test_tbb(concurrent_hash_map_tbb_insert_erase concurrent_hash_map_insert_erase/concurrent_hash_map_tbb_insert_erase.cpp)
		add_test_generic(NAME concurrent_hash_map_tbb_insert_erase TRACERS none pmemcheck)
	endif()

	if(PMREORDER_SUPPORTED)
		build_test(concurrent_hash_map_pmreorder_simple concurrent_hash_map_pmreorder_simple/concurrent_hash_map_pmreorder_simple.cpp)
		add_test_generic(NAME concurrent_hash_map_pmreorder_simple CASE 0 TRACERS none)

		build_test(concurrent_hash_map_pmreorder_multiple_buckets concurrent_hash_map_pmreorder_multiple_buckets/concurrent_hash_map_pmreorder_multiple_buckets.cpp)
		add_test_generic(NAME concurrent_hash_map_pmreorder_multiple_buckets CASE 0 TRACERS none)

		build_test(concurrent_hash_map_pmreorder_erase concurrent_hash_map_pmreorder_erase/concurrent_hash_map_pmreorder_erase.cpp)
		add_test_generic(NAME concurrent_hash_map_pmreorder_erase CASE 0 TRACERS none)

		set(TEST concurrent_hash_map_pmreorder_break_insert)
		build_test(${TEST} ${TEST}/${TEST}.cpp)

		# add 5 concurrent_hash_map_pmreorder_break_insert test cases
		set(CURRENT_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${TEST})
		set(CMAKE_SCRIPT ${CURRENT_TEST_DIR}/${TEST})
		set(GDB_SCRIPT ${CURRENT_TEST_DIR}/break_before_persist)
		foreach(case RANGE 4)
			math(EXPR IGNORE "${case} + 1")
			math(EXPR PERSIST "${case} + 2")
			configure_file("${GDB_SCRIPT}.gdb.in" "${GDB_SCRIPT}_${PERSIST}.gdb" @ONLY)
			configure_file("${CMAKE_SCRIPT}.cmake.in" "${CMAKE_SCRIPT}_${case}.cmake" @ONLY)
			add_test_generic(NAME ${TEST} CASE ${case} TRACERS none)
		endforeach()
	else()
		message(WARNING "Skipping pmreorder tests because of no pmreorder support")
	endif()
elseif(NOT PMEMVLT_PRESENT AND ENABLE_CONCURRENT_HASHMAP)
	message(WARNING "Skipping concurrent_hash_map tests because no pmemvlt support found.")
endif()

if(ENABLE_SEGMENT_VECTOR_ARRAY_EXPSIZE)
	build_test_ext(NAME segment_vector_array_expsize_temp_value SRC_FILES temp_value/temp_value.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_temp_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_assign_exceptions_length SRC_FILES vector_assign_exceptions_length/vector_assign_exceptions_length.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_assign_exceptions_length TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_expsize_assign_exceptions_oom SRC_FILES vector_assign_exceptions_oom/vector_assign_exceptions_oom.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_assign_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_expsize_assign_txabort SRC_FILES vector_assign_txabort/vector_assign_txabort.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_assign_txabort TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_expsize_comp_operators SRC_FILES vector_comp_operators/vector_comp_operators.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_comp_operators TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_expsize_capacity_exceptions_length SRC_FILES vector_capacity_exceptions_length/vector_capacity_exceptions_length.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_capacity_exceptions_length TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_expsize_capacity_exceptions_oom SRC_FILES vector_capacity_exceptions_oom/vector_capacity_exceptions_oom.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_capacity_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_expsize_capacity_txabort SRC_FILES vector_capacity_txabort/vector_capacity_txabort.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_capacity_txabort TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_expsize_ctor_exceptions_nopmem SRC_FILES vector_ctor_exceptions_nopmem/vector_ctor_exceptions_nopmem.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_ctor_exceptions_nopmem TRACERS none memcheck )

	build_test_ext(NAME segment_vector_array_expsize_ctor_exceptions_notx SRC_FILES vector_ctor_exceptions_notx/vector_ctor_exceptions_notx.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_ctor_exceptions_notx TRACERS none memcheck)

	#[[ XXX:
	 When we throwing exceptions in segment vector constructor, destructors is called for all constructed element according to standart.
	 In this way for segment with non-zero capacity in the underlying vectors destructor free_data() methods is called.
	 The free_data() run nested transaction which is failed to run when internally calls pmemobj_tx_begin(pool.handle(), nullptr, TX_PARAM_NONE).
	]]
	# build_test_ext(NAME segment_vector_array_expsize_ctor_exceptions_oom SRC_FILES vector_ctor_exceptions_oom/vector_ctor_exceptions_oom.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	# add_test_generic(NAME segment_vector_array_expsize_ctor_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_expsize_ctor_move SRC_FILES vector_ctor_move/vector_ctor_move.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_ctor_move TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_expsize_ctor_capacity SRC_FILES vector_ctor_capacity/vector_ctor_capacity.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_ctor_capacity TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_expsize_dtor SRC_FILES vector_dtor/vector_dtor.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_dtor TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_expsize_iterators_access SRC_FILES vector_iterators_access/vector_iterators_access.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_iterators_access TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_expsize_ctor_check_copy SRC_FILES vector_ctor_check_copy/vector_ctor_check_copy.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_ctor_check_copy TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_expsize_modifiers_exceptions_oom SRC_FILES vector_modifiers_exceptions_oom/vector_modifiers_exceptions_oom.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_modifiers_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_expsize_modifiers_txabort SRC_FILES vector_modifiers_txabort/vector_modifiers_txabort.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_modifiers_txabort TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_expsize_modifiers_type_requirements SRC_FILES vector_modifiers_type_requirements/vector_modifiers_type_requirements.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_modifiers_type_requirements TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_expsize_std_arg SRC_FILES vector_std_arg/vector_std_arg.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_std_arg TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_expsize_layout SRC_FILES vector_layout/vector_layout.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_layout TRACERS none)
endif()

if(ENABLE_SEGMENT_VECTOR_VECTOR_EXPSIZE)
	build_test_ext(NAME segment_vector_vector_expsize_temp_value SRC_FILES temp_value/temp_value.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_temp_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_assign_exceptions_length SRC_FILES vector_assign_exceptions_length/vector_assign_exceptions_length.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_assign_exceptions_length TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_expsize_assign_exceptions_oom SRC_FILES vector_assign_exceptions_oom/vector_assign_exceptions_oom.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_assign_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_expsize_assign_txabort SRC_FILES vector_assign_txabort/vector_assign_txabort.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_assign_txabort TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_expsize_comp_operators SRC_FILES vector_comp_operators/vector_comp_operators.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_comp_operators TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_expsize_capacity_exceptions_length SRC_FILES vector_capacity_exceptions_length/vector_capacity_exceptions_length.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_capacity_exceptions_length TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_expsize_capacity_exceptions_oom SRC_FILES vector_capacity_exceptions_oom/vector_capacity_exceptions_oom.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_capacity_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_expsize_capacity_txabort SRC_FILES vector_capacity_txabort/vector_capacity_txabort.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_capacity_txabort TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_expsize_ctor_exceptions_nopmem SRC_FILES vector_ctor_exceptions_nopmem/vector_ctor_exceptions_nopmem.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_ctor_exceptions_nopmem TRACERS none memcheck )

	build_test_ext(NAME segment_vector_vector_expsize_ctor_exceptions_notx SRC_FILES vector_ctor_exceptions_notx/vector_ctor_exceptions_notx.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_ctor_exceptions_notx TRACERS none memcheck)

	#[[ XXX:
	 When we throwing exceptions in segment vector constructor, destructors is called for all constructed element according to standart.
	 In this way for segment with non-zero capacity in the underlying vectors destructor free_data() methods is called.
	 The free_data() run nested transaction which is failed to run when internally calls pmemobj_tx_begin(pool.handle(), nullptr, TX_PARAM_NONE).
	]]
	# build_test_ext(NAME segment_vector_vector_expsize_ctor_exceptions_oom SRC_FILES vector_ctor_exceptions_oom/vector_ctor_exceptions_oom.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	# add_test_generic(NAME segment_vector_vector_expsize_ctor_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_expsize_ctor_move SRC_FILES vector_ctor_move/vector_ctor_move.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_ctor_move TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_expsize_ctor_capacity SRC_FILES vector_ctor_capacity/vector_ctor_capacity.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_ctor_capacity TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_expsize_dtor SRC_FILES vector_dtor/vector_dtor.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_dtor TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_expsize_iterators_access SRC_FILES vector_iterators_access/vector_iterators_access.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_iterators_access TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_expsize_ctor_check_copy SRC_FILES vector_ctor_check_copy/vector_ctor_check_copy.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_ctor_check_copy TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_expsize_modifiers_exceptions_oom SRC_FILES vector_modifiers_exceptions_oom/vector_modifiers_exceptions_oom.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_modifiers_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_expsize_modifiers_txabort SRC_FILES vector_modifiers_txabort/vector_modifiers_txabort.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_modifiers_txabort TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_expsize_modifiers_type_requirements SRC_FILES vector_modifiers_type_requirements/vector_modifiers_type_requirements.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_modifiers_type_requirements TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_expsize_std_arg SRC_FILES vector_std_arg/vector_std_arg.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_std_arg TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_expsize_layout SRC_FILES vector_layout/vector_layout.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_layout TRACERS none)
endif()

if(ENABLE_SEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	build_test_ext(NAME segment_vector_array_fixedsize_temp_value SRC_FILES temp_value/temp_value.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_temp_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_assign_exceptions_length SRC_FILES vector_assign_exceptions_length/vector_assign_exceptions_length.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_assign_exceptions_length TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_assign_exceptions_oom SRC_FILES vector_assign_exceptions_oom/vector_assign_exceptions_oom.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE_EXT)
	add_test_generic(NAME segment_vector_array_fixedsize_assign_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_assign_txabort SRC_FILES vector_assign_txabort/vector_assign_txabort.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_assign_txabort TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_comp_operators SRC_FILES vector_comp_operators/vector_comp_operators.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_comp_operators TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_capacity_exceptions_length SRC_FILES vector_capacity_exceptions_length/vector_capacity_exceptions_length.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_capacity_exceptions_length TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_capacity_exceptions_oom SRC_FILES vector_capacity_exceptions_oom/vector_capacity_exceptions_oom.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE_EXT)
	add_test_generic(NAME segment_vector_array_fixedsize_capacity_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_capacity_txabort SRC_FILES vector_capacity_txabort/vector_capacity_txabort.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_capacity_txabort TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_ctor_exceptions_nopmem SRC_FILES vector_ctor_exceptions_nopmem/vector_ctor_exceptions_nopmem.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_ctor_exceptions_nopmem TRACERS none memcheck )

	build_test_ext(NAME segment_vector_array_fixedsize_ctor_exceptions_notx SRC_FILES vector_ctor_exceptions_notx/vector_ctor_exceptions_notx.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_ctor_exceptions_notx TRACERS none memcheck)

	#[[ XXX:
	 When we throwing exceptions in segment vector constructor, destructors is called for all constructed element according to standart.
	 In this way for segment with non-zero capacity in the underlying vectors destructor free_data() methods is called.
	 The free_data() run nested transaction which is failed to run when internally calls pmemobj_tx_begin(pool.handle(), nullptr, TX_PARAM_NONE).
	]]
	# build_test_ext(NAME segment_vector_array_fixedsize_ctor_exceptions_oom SRC_FILES vector_ctor_exceptions_oom/vector_ctor_exceptions_oom.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	# add_test_generic(NAME segment_vector_array_fixedsize_ctor_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_ctor_move SRC_FILES vector_ctor_move/vector_ctor_move.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_ctor_move TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_ctor_capacity SRC_FILES vector_ctor_capacity/vector_ctor_capacity.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_ctor_capacity TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_dtor SRC_FILES vector_dtor/vector_dtor.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_dtor TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_iterators_access SRC_FILES vector_iterators_access/vector_iterators_access.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_iterators_access TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_ctor_check_copy SRC_FILES vector_ctor_check_copy/vector_ctor_check_copy.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_ctor_check_copy TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_modifiers_exceptions_oom SRC_FILES vector_modifiers_exceptions_oom/vector_modifiers_exceptions_oom.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE_EXT)
	add_test_generic(NAME segment_vector_array_fixedsize_modifiers_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_modifiers_txabort SRC_FILES vector_modifiers_txabort/vector_modifiers_txabort.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_modifiers_txabort TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_modifiers_type_requirements SRC_FILES vector_modifiers_type_requirements/vector_modifiers_type_requirements.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_modifiers_type_requirements TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_std_arg SRC_FILES vector_std_arg/vector_std_arg.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_std_arg TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_layout SRC_FILES vector_layout/vector_layout.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_layout TRACERS none)
endif()

if(ENABLE_SEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	build_test_ext(NAME segment_vector_vector_fixedsize_temp_value SRC_FILES temp_value/temp_value.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_temp_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_assign_exceptions_length SRC_FILES vector_assign_exceptions_length/vector_assign_exceptions_length.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_assign_exceptions_length TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_assign_exceptions_oom SRC_FILES vector_assign_exceptions_oom/vector_assign_exceptions_oom.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE_EXT)
	add_test_generic(NAME segment_vector_vector_fixedsize_assign_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_assign_txabort SRC_FILES vector_assign_txabort/vector_assign_txabort.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_assign_txabort TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_comp_operators SRC_FILES vector_comp_operators/vector_comp_operators.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_comp_operators TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_capacity_exceptions_length SRC_FILES vector_capacity_exceptions_length/vector_capacity_exceptions_length.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_capacity_exceptions_length TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_capacity_exceptions_oom SRC_FILES vector_capacity_exceptions_oom/vector_capacity_exceptions_oom.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE_EXT)
	add_test_generic(NAME segment_vector_vector_fixedsize_capacity_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_capacity_txabort SRC_FILES vector_capacity_txabort/vector_capacity_txabort.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_capacity_txabort TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_ctor_exceptions_nopmem SRC_FILES vector_ctor_exceptions_nopmem/vector_ctor_exceptions_nopmem.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_ctor_exceptions_nopmem TRACERS none memcheck )

	build_test_ext(NAME segment_vector_vector_fixedsize_ctor_exceptions_notx SRC_FILES vector_ctor_exceptions_notx/vector_ctor_exceptions_notx.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_ctor_exceptions_notx TRACERS none memcheck)

	#[[ XXX:
	 When we throwing exceptions in segment vector constructor, destructors is called for all constructed element according to standart.
	 In this way for segment with non-zero capacity in the underlying vectors destructor free_data() methods is called.
	 The free_data() run nested transaction which is failed to run when internally calls pmemobj_tx_begin(pool.handle(), nullptr, TX_PARAM_NONE).
	]]
	# build_test_ext(NAME segment_vector_vector_fixedsize_ctor_exceptions_oom SRC_FILES vector_ctor_exceptions_oom/vector_ctor_exceptions_oom.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	# add_test_generic(NAME segment_vector_vector_fixedsize_ctor_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_ctor_move SRC_FILES vector_ctor_move/vector_ctor_move.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_ctor_move TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_ctor_capacity SRC_FILES vector_ctor_capacity/vector_ctor_capacity.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_ctor_capacity TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_dtor SRC_FILES vector_dtor/vector_dtor.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_dtor TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_iterators_access SRC_FILES vector_iterators_access/vector_iterators_access.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_iterators_access TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_ctor_check_copy SRC_FILES vector_ctor_check_copy/vector_ctor_check_copy.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_ctor_check_copy TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_modifiers_exceptions_oom SRC_FILES vector_modifiers_exceptions_oom/vector_modifiers_exceptions_oom.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE_EXT)
	add_test_generic(NAME segment_vector_vector_fixedsize_modifiers_exceptions_oom TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_modifiers_txabort SRC_FILES vector_modifiers_txabort/vector_modifiers_txabort.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_modifiers_txabort TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_modifiers_type_requirements SRC_FILES vector_modifiers_type_requirements/vector_modifiers_type_requirements.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_modifiers_type_requirements TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_std_arg SRC_FILES vector_std_arg/vector_std_arg.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_std_arg TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_layout SRC_FILES vector_layout/vector_layout.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_layout TRACERS none)
endif()

add_subdirectory(external)
