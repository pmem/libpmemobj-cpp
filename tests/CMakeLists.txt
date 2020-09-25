# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2020, Intel Corporation

include(ctest_helpers.cmake)

add_cppstyle(tests-common ${CMAKE_CURRENT_SOURCE_DIR}/common/*.*pp
						  ${CMAKE_CURRENT_SOURCE_DIR}/*.c
						  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
						  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
						  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
						  ${CMAKE_CURRENT_SOURCE_DIR}/check_is_pmem/*.*pp
						  ${CMAKE_CURRENT_SOURCE_DIR}/container_generic/*.*pp)

add_check_whitespace(tests-common ${CMAKE_CURRENT_SOURCE_DIR}/common/*.*pp
								  ${CMAKE_CURRENT_SOURCE_DIR}/*.c
								  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
								  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
								  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
								  ${CMAKE_CURRENT_SOURCE_DIR}/check_is_pmem/*.*pp
								  ${CMAKE_CURRENT_SOURCE_DIR}/container_generic/*.*pp)

add_check_whitespace(tests-cmake ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)

if(MSVC_VERSION)
	add_flag(-W2)

	# disable warning C4996 - disable checking iterators
	add_flag(-D_SCL_SECURE_NO_WARNINGS)

	# disable warning C4996 - disable deprecation of getenv and strcpy functions
	add_flag(-D_CRT_SECURE_NO_WARNINGS)

	# fix C1128 raised for some test binaries
	add_flag(-bigobj)

	add_flag("-D_FORTIFY_SOURCE=2" RELEASE)
else()
	add_flag(-Wall)
	add_flag("-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2" RELEASE)
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
find_gdb()

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
	add_executable(ex-pman ../examples/pman/pman.cpp)
	target_include_directories(ex-pman PUBLIC ${CURSES_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../examples)
	target_link_libraries(ex-pman ${LIBPMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CURSES_LIBRARIES})
endfunction()

add_executable(check_is_pmem check_is_pmem/check_is_pmem.cpp)
target_link_libraries(check_is_pmem ${LIBPMEM_LIBRARIES})

if(BUILD_EXAMPLES AND NO_GCC_VARIADIC_TEMPLATE_BUG)
	build_example_queue()
	add_test_generic(NAME ex-queue CASE 0 TRACERS none)

	if (CURSES_FOUND AND NOT WIN32)
		build_example_pman()
		add_test_generic(NAME ex-pman CASE 0 TRACERS none)
	else()
		message(WARNING "ncurses not found - pman test won't be build")
	endif()
elseif(BUILD_EXAMPLES)
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

if (NOT WIN32)
	build_test(mutex_posix mutex_posix/mutex_posix.cpp)
	add_test_generic(NAME mutex_posix TRACERS drd helgrind pmemcheck)
endif()

build_test(pool pool/pool.cpp)
add_test_generic(NAME pool CASE 0 TRACERS none)
add_test_generic(NAME pool CASE 1 TRACERS none)
add_test_generic(NAME pool CASE 2 TRACERS none)
add_test_generic(NAME pool CASE 3 TRACERS none)
add_test_generic(NAME pool CASE 4 TRACERS none)
add_test_generic(NAME pool CASE 5 TRACERS none)

build_test(pool_primitives pool_primitives/pool_primitives.cpp)
add_test_generic(NAME pool_primitives CASE 0 TRACERS none pmemcheck
		SCRIPT cmake/common_0.cmake)

build_test(ptr ptr/ptr.cpp)
add_test_generic(NAME ptr CASE 0 TRACERS none pmemcheck
		SCRIPT cmake/common_0.cmake)

build_test(pair pair/pair.cpp)
add_test_generic(NAME pair TRACERS none memcheck)

build_test(ptr_arith ptr_arith/ptr_arith.cpp)
add_test_generic(NAME ptr_arith TRACERS memcheck pmemcheck)
# XXX Bug: incompatibility between asan and custom library
if (NOT USE_ASAN)
	add_test_generic(NAME ptr_arith TRACERS none)
endif()

build_test(p_ext p_ext/p_ext.cpp)
add_test_generic(NAME p_ext TRACERS none pmemcheck)

if (NOT WIN32)
	build_test(shared_mutex_posix shared_mutex_posix/shared_mutex_posix.cpp)
	add_test_generic(NAME shared_mutex_posix TRACERS drd helgrind pmemcheck)
endif()

build_test(transaction transaction/transaction.cpp)
add_test_generic(NAME transaction TRACERS none pmemcheck memcheck)

if (VOLATILE_STATE_PRESENT)
	build_test(volatile_state volatile_state/volatile_state.cpp)
	add_test_generic(NAME volatile_state TRACERS none pmemcheck memcheck drd helgrind)
endif()

if(WIN32)
	build_test(pool_win pool_win/pool_win.cpp)
	add_test_generic(NAME pool_win CASE 0 TRACERS none)
	add_test_generic(NAME pool_win CASE 1 TRACERS none)
	add_test_generic(NAME pool_win CASE 2 TRACERS none)
	add_test_generic(NAME pool_win CASE 3 TRACERS none)
endif()

build_test(v v/v.cpp)
add_test_generic(NAME v CASE 0 TRACERS none memcheck SCRIPT cmake/common_0.cmake)


if(NO_CHRONO_BUG)
	build_test(cond_var cond_var/cond_var.cpp)
	add_test_generic(NAME cond_var TRACERS none)

	if (NOT WIN32)
		build_test(cond_var_posix cond_var_posix/cond_var_posix.cpp)
		add_test_generic(NAME cond_var_posix TRACERS drd helgrind pmemcheck)
	endif()

	build_test(mutex mutex/mutex.cpp)
	add_test_generic(NAME mutex TRACERS none)

	build_test(shared_mutex shared_mutex/shared_mutex.cpp)
	add_test_generic(NAME shared_mutex TRACERS none)

	if (NOT WIN32)
		build_test(timed_mtx_posix timed_mtx_posix/timed_mtx_posix.cpp)
		add_test_generic(NAME timed_mtx_posix TRACERS drd helgrind pmemcheck)
	endif()

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

build_test(pool_cleanup pool_cleanup/pool_cleanup.cpp)
add_test_generic(NAME pool_cleanup TRACERS none pmemcheck memcheck drd helgrind CASE 0)

build_test(iterator_traits iterator_traits/iterator_traits.cpp)
add_test_generic(NAME iterator_traits TRACERS none)

build_test(ctl ctl/ctl.cpp)
add_test_generic(NAME ctl CASE 0 TRACERS none
		SCRIPT ctl/ctl_0.cmake)

build_test(defrag defrag/defrag.cpp)
add_test_generic(NAME defrag TRACERS none pmemcheck memcheck)

build_test(string_view string_view/string_view.cpp)
add_test_generic(NAME string_view TRACERS none memcheck)

build_test(inline_string inline_string/inline_string.cpp)
add_test_generic(NAME inline_string TRACERS none memcheck pmemcheck)

if(WIN32)
	build_test(ctl_win ctl_win/ctl_win.cpp)
	add_test_generic(NAME ctl_win CASE 0 TRACERS none
			SCRIPT ctl/ctl_0.cmake)
endif()

if (TEST_SELF_RELATIVE_POINTER)
	build_test(self_relative_ptr ptr/self_relative_ptr.cpp)
	add_test_generic(NAME self_relative_ptr TRACERS none memcheck pmemcheck)

	build_test(self_relative_ptr_arith ptr_arith/self_relative_ptr_arith.cpp)
	add_test_generic(NAME self_relative_ptr_arith TRACERS none memcheck pmemcheck)

	build_test_atomic(self_relative_ptr_atomic self_relative_ptr_atomic/self_relative_ptr_atomic.cpp)
	add_test_generic(NAME self_relative_ptr_atomic TRACERS none memcheck drd helgrind)

	build_test_atomic(self_relative_ptr_atomic_pmem self_relative_ptr_atomic/self_relative_ptr_atomic_pmem.cpp)
	add_test_generic(NAME self_relative_ptr_atomic_pmem TRACERS none memcheck pmemcheck drd helgrind)
endif()

if (TEST_ARRAY)
	build_test(array_algorithms array_algorithms/array_algorithms.cpp)
	add_test_generic(NAME array_algorithms TRACERS none pmemcheck)

	build_test(array_slice array_slice/array_slice.cpp)
	add_test_generic(NAME array_slice CASE 0 TRACERS none pmemcheck memcheck)

	build_test(array_iterator array_iterator/array_iterator.cpp)
	add_test_generic(NAME array_iterator TRACERS none pmemcheck)

	build_test(array_iterator_noconv array_iterator/array_iterator_noconv.cpp)
	check_cxx_compiler_flag(-Wno-sign-conversion NO_SIGN_CONVERSION_FLAG)
	if (NO_SIGN_CONVERSION_FLAG)
		target_compile_options(array_iterator_noconv PUBLIC -Wno-sign-conversion)
	endif()
	add_test_generic(NAME array_iterator_noconv TRACERS none)

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

if (TEST_VECTOR)
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

	build_test_ext(NAME vector_parameters SRC_FILES vector_parameters/vector_parameters.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_parameters TRACERS none memcheck pmemcheck)

	build_test_ext(NAME vector_layout SRC_FILES vector_layout/vector_layout.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_layout TRACERS none)

	build_test(defrag_vector defrag/defrag_vector.cpp)
	add_test_generic(NAME defrag_vector TRACERS none pmemcheck memcheck)
endif()

if (TEST_STRING)
	build_test(string_access string_access/string_access.cpp)
	add_test_generic(NAME string_access TRACERS none memcheck pmemcheck)

	build_test(string_capacity string_capacity/string_capacity.cpp)
	add_test_generic(NAME string_capacity TRACERS none memcheck pmemcheck)

	build_test(string_exceptions string_exceptions/string_exceptions.cpp)
	add_test_generic(NAME string_exceptions TRACERS none memcheck pmemcheck)

	build_test(string_modifiers string_modifiers/string_modifiers.cpp)
	add_test_generic(NAME string_modifiers TRACERS none memcheck pmemcheck)

	build_test(string_swap string_modifiers/string_swap.cpp)
	add_test_generic(NAME string_swap TRACERS none memcheck pmemcheck)

	build_test(string_snapshot string_snapshot/string_snapshot.cpp)
	add_test_generic(NAME string_snapshot TRACERS none memcheck pmemcheck)

	build_test(string_assign_tx_abort string_assign_tx_abort/string_assign_tx_abort.cpp)
	add_test_generic(NAME string_assign_tx_abort TRACERS none memcheck pmemcheck)

	build_test(string_layout string_layout/string_layout.cpp)
	add_test_generic(NAME string_layout TRACERS none)
endif()

if(TEST_CONCURRENT_HASHMAP)
	build_test(concurrent_hash_map_tx concurrent_hash_map_tx/concurrent_hash_map_tx.cpp)
	add_test_generic(NAME concurrent_hash_map_tx TRACERS none memcheck pmemcheck)

	build_test(concurrent_hash_map_insert_or_assign concurrent_hash_map_insert_or_assign/concurrent_hash_map_insert_or_assign.cpp)
	add_test_generic(NAME concurrent_hash_map_insert_or_assign TRACERS none memcheck pmemcheck helgrind drd)

	build_test(concurrent_hash_map_insert_lookup concurrent_hash_map_insert_lookup/concurrent_hash_map_insert_lookup.cpp)
	add_test_generic(NAME concurrent_hash_map_insert_lookup CASE 0 TRACERS none
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)

	build_test_defrag(concurrent_hash_map_insert_lookup_mock concurrent_hash_map_insert_lookup/concurrent_hash_map_insert_lookup.cpp)
	add_test_generic(NAME concurrent_hash_map_insert_lookup_mock CASE 0 TRACERS memcheck pmemcheck
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)

	if(TESTS_CONCURRENT_HASH_MAP_DRD_HELGRIND AND TESTS_LONG)
		add_test_generic(NAME concurrent_hash_map_insert_lookup_mock CASE 0 TRACERS helgrind drd
				SCRIPT concurrent_hash_map/check_is_pmem.cmake)
	endif()

	build_test(concurrent_hash_map_insert_erase concurrent_hash_map_insert_erase/concurrent_hash_map_insert_erase.cpp)
	add_test_generic(NAME concurrent_hash_map_insert_erase CASE 0 TRACERS none
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)
	add_test_generic(NAME concurrent_hash_map_insert_erase CASE 1 TRACERS none
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)

	build_test_defrag(concurrent_hash_map_insert_erase_mock concurrent_hash_map_insert_erase/concurrent_hash_map_insert_erase.cpp)
	add_test_generic(NAME concurrent_hash_map_insert_erase_mock CASE 0 TRACERS memcheck pmemcheck
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)
	add_test_generic(NAME concurrent_hash_map_insert_erase_mock CASE 1 TRACERS memcheck pmemcheck
			SCRIPT concurrent_hash_map/check_is_pmem_defrag.cmake)

	if(TESTS_CONCURRENT_HASH_MAP_DRD_HELGRIND)
		add_test_generic(NAME concurrent_hash_map_insert_erase_mock CASE 0 TRACERS helgrind drd
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)
		add_test_generic(NAME concurrent_hash_map_insert_erase_mock CASE 1 TRACERS helgrind drd
			SCRIPT concurrent_hash_map/check_is_pmem_defrag.cmake)
	endif()

	# This test should not be run under helgrind due to intermittent failures (most probably false-positive, ref. issue #469)
	build_test(concurrent_hash_map_rehash concurrent_hash_map_rehash/concurrent_hash_map_rehash.cpp)
	add_test_generic(NAME concurrent_hash_map_rehash CASE 0 TRACERS none
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)
	add_test_generic(NAME concurrent_hash_map_rehash CASE 1 TRACERS none
			SCRIPT concurrent_hash_map/check_is_pmem_defrag.cmake)

	build_test_defrag(concurrent_hash_map_rehash_mock concurrent_hash_map_rehash/concurrent_hash_map_rehash.cpp)
	add_test_generic(NAME concurrent_hash_map_rehash_mock CASE 0 TRACERS memcheck pmemcheck
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)
	add_test_generic(NAME concurrent_hash_map_rehash_mock CASE 1 TRACERS memcheck pmemcheck
			SCRIPT concurrent_hash_map/check_is_pmem_defrag.cmake)

	if(TESTS_CONCURRENT_HASH_MAP_DRD_HELGRIND)
		# This test should not be run under helgrind due to intermittent failures (most probably false-positive, ref. issue #469)
		if(TESTS_LONG)
			add_test_generic(NAME concurrent_hash_map_rehash_mock CASE 0 TRACERS drd
					SCRIPT concurrent_hash_map/check_is_pmem.cmake)
			add_test_generic(NAME concurrent_hash_map_rehash_mock CASE 1 TRACERS drd
					SCRIPT concurrent_hash_map/check_is_pmem_defrag.cmake)
		endif()
	endif()

	build_test(concurrent_hash_map_rehash_recursive concurrent_hash_map_rehash_recursive/concurrent_hash_map_rehash_recursive.cpp)
	add_test_generic(NAME concurrent_hash_map_rehash_recursive CASE 0 TRACERS none
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)

	build_test_defrag(concurrent_hash_map_rehash_recursive_mock concurrent_hash_map_rehash_recursive/concurrent_hash_map_rehash_recursive.cpp)
	add_test_generic(NAME concurrent_hash_map_rehash_recursive_mock CASE 0 TRACERS memcheck pmemcheck
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)

	if(TESTS_CONCURRENT_HASH_MAP_DRD_HELGRIND AND TESTS_LONG)
		add_test_generic(NAME concurrent_hash_map_rehash_recursive_mock CASE 0 TRACERS helgrind drd
				SCRIPT concurrent_hash_map/check_is_pmem.cmake)
	endif()

	build_test(concurrent_hash_map_rehash_deadlock concurrent_hash_map_rehash_deadlock/concurrent_hash_map_rehash_deadlock.cpp)
	add_test_generic(NAME concurrent_hash_map_rehash_deadlock CASE 0 TRACERS none
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)

	# this single-threaded test should not be run under helgrind/drd, because a false-positive deadlock can be detected
	build_test_defrag(concurrent_hash_map_rehash_deadlock_mock concurrent_hash_map_rehash_deadlock/concurrent_hash_map_rehash_deadlock.cpp)
	add_test_generic(NAME concurrent_hash_map_rehash_deadlock_mock CASE 0 TRACERS memcheck pmemcheck
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)

	build_test(concurrent_hash_map_defrag concurrent_hash_map_defrag/concurrent_hash_map_defrag.cpp)
	add_test_generic(NAME concurrent_hash_map_defrag TRACERS none)

	# This test can NOT be run under helgrind as it will report wrong lock ordering. Helgrind is right about
	# possible deadlock situation but that could only happen in case of wrong API usage.
	build_test(concurrent_hash_map_deadlock concurrent_hash_map_deadlock/concurrent_hash_map_deadlock.cpp)
	add_test_generic(NAME concurrent_hash_map_deadlock CASE 0 TRACERS none drd memcheck pmemcheck
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)

	build_test(concurrent_hash_map_insert_reopen concurrent_hash_map_insert_reopen/concurrent_hash_map_insert_reopen.cpp)
	add_test_generic(NAME concurrent_hash_map_insert_reopen CASE 0 TRACERS none memcheck pmemcheck
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)

	if(TESTS_CONCURRENT_HASH_MAP_DRD_HELGRIND)
		add_test_generic(NAME concurrent_hash_map_insert_reopen CASE 0 TRACERS drd helgrind
				SCRIPT concurrent_hash_map/check_is_pmem.cmake)
	endif()

	build_test_ext(NAME concurrent_hash_map_insert_reopen_deprecated SRC_FILES concurrent_hash_map_insert_reopen/concurrent_hash_map_insert_reopen.cpp
			BUILD_OPTIONS -DUSE_DEPRECATED_RUNTIME_INITIALIZE)
	check_cxx_compiler_flag(-Wdeprecated-declarations deprecated_declarations)
	if (deprecated_declarations)
		target_compile_options(concurrent_hash_map_insert_reopen_deprecated PUBLIC -Wno-deprecated-declarations)
	endif()
	add_test_generic(NAME concurrent_hash_map_insert_reopen_deprecated TRACERS none)

	build_test(concurrent_hash_map_rehash_check concurrent_hash_map_rehash_check/concurrent_hash_map_rehash_check.cpp)
	add_test_generic(NAME concurrent_hash_map_rehash_check TRACERS none memcheck pmemcheck)

	build_test(concurrent_hash_map_singlethread concurrent_hash_map_singlethread/concurrent_hash_map_singlethread.cpp)
	add_test_generic(NAME concurrent_hash_map_singlethread TRACERS none memcheck pmemcheck)

	build_test(concurrent_hash_map_layout concurrent_hash_map_layout/concurrent_hash_map_layout.cpp)
	add_test_generic(NAME concurrent_hash_map_layout TRACERS none)

	if(GDB_FOUND)
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
	else()
		message(WARNING "Skipping concurrent_hash_map_rehash_break test because GDB was not found")
	endif()

	if(NOT TESTS_TBB)
		message(WARNING "Skipping concurrent_hash_map_tbb tests - disabled by cmake TESTS_TBB option")
	elseif(NOT TBB_FOUND)
		message(FATAL_ERROR "concurrent_hash_map_tbb tests enabled by cmake TESTS_TBB option, but Intel TBB library was not found")
	else()
		build_test_tbb(concurrent_hash_map_tbb_insert_lookup concurrent_hash_map_insert_lookup/concurrent_hash_map_tbb_insert_lookup.cpp)
		add_test_generic(NAME concurrent_hash_map_tbb_insert_lookup TRACERS none
				SCRIPT concurrent_hash_map/check_is_pmem.cmake)

		build_test_tbb_defrag(concurrent_hash_map_tbb_insert_lookup_mock concurrent_hash_map_insert_lookup/concurrent_hash_map_tbb_insert_lookup.cpp)
		add_test_generic(NAME concurrent_hash_map_tbb_insert_lookup_mock TRACERS pmemcheck
				SCRIPT concurrent_hash_map/check_is_pmem.cmake)

		build_test_tbb(concurrent_hash_map_tbb_insert_erase concurrent_hash_map_insert_erase/concurrent_hash_map_tbb_insert_erase.cpp)
		add_test_generic(NAME concurrent_hash_map_tbb_insert_erase TRACERS none
				SCRIPT concurrent_hash_map/check_is_pmem.cmake)

		build_test_tbb_defrag(concurrent_hash_map_tbb_insert_erase_mock concurrent_hash_map_insert_erase/concurrent_hash_map_tbb_insert_erase.cpp)
		add_test_generic(NAME concurrent_hash_map_tbb_insert_erase_mock TRACERS pmemcheck
				SCRIPT concurrent_hash_map/check_is_pmem.cmake)
	endif()

	if(PMREORDER_SUPPORTED)
		build_test(concurrent_hash_map_pmreorder_simple concurrent_hash_map_pmreorder_simple/concurrent_hash_map_pmreorder_simple.cpp)
		add_test_generic(NAME concurrent_hash_map_pmreorder_simple CASE 0 TRACERS none
				SCRIPT cmake/pmreorder.cmake)

		build_test(concurrent_hash_map_pmreorder_multiple_buckets concurrent_hash_map_pmreorder_multiple_buckets/concurrent_hash_map_pmreorder_multiple_buckets.cpp)
		add_test_generic(NAME concurrent_hash_map_pmreorder_multiple_buckets CASE 0 TRACERS none
				SCRIPT cmake/pmreorder.cmake)

		build_test(concurrent_hash_map_pmreorder_erase concurrent_hash_map_pmreorder_erase/concurrent_hash_map_pmreorder_erase.cpp)
		add_test_generic(NAME concurrent_hash_map_pmreorder_erase CASE 0 TRACERS none)

		if(GDB_FOUND)
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
			message(WARNING "Skipping concurrent_hash_map_pmreorder_break_insert test because GDB was not found")
		endif()
	else()
		message(WARNING "Skipping pmreorder tests because of no pmreorder support")
	endif()
endif()

if(TEST_SEGMENT_VECTOR_ARRAY_EXPSIZE)
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
	 When we throwing exceptions in segment vector constructor, destructors is called for all constructed element according to standard.
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

	build_test_ext(NAME segment_vector_array_expsize_parameters SRC_FILES vector_parameters/vector_parameters.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_parameters TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_array_expsize_layout SRC_FILES vector_layout/vector_layout.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_layout TRACERS none)
endif()

if(TEST_SEGMENT_VECTOR_VECTOR_EXPSIZE)
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
	 When we throwing exceptions in segment vector constructor, destructors is called for all constructed element according to standard.
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

	build_test_ext(NAME segment_vector_vector_expsize_parameters SRC_FILES vector_parameters/vector_parameters.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_parameters TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_expsize_layout SRC_FILES vector_layout/vector_layout.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_layout TRACERS none)
endif()

if(TEST_SEGMENT_VECTOR_VECTOR_FIXEDSIZE)
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
	 When we throwing exceptions in segment vector constructor, destructors is called for all constructed element according to standard.
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

	build_test_ext(NAME segment_vector_vector_fixedsize_parameters SRC_FILES vector_parameters/vector_parameters.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_parameters TRACERS none memcheck pmemcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_layout SRC_FILES vector_layout/vector_layout.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_layout TRACERS none)
endif()

if (TEST_ENUMERABLE_THREAD_SPECIFIC)
	build_test(enumerable_thread_specific_access enumerable_thread_specific_access/enumerable_thread_specific_access.cpp)
	add_test_generic(NAME enumerable_thread_specific_access CASE 0 TRACERS none memcheck pmemcheck drd helgrind
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)

	build_test(enumerable_thread_specific_size enumerable_thread_specific_size/enumerable_thread_specific_size.cpp)
	add_test_generic(NAME enumerable_thread_specific_size CASE 0 TRACERS none memcheck pmemcheck drd helgrind
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)

	build_test(enumerable_thread_specific_iterators enumerable_thread_specific_iterators/enumerable_thread_specific_iterators.cpp)
	add_test_generic(NAME enumerable_thread_specific_iterators CASE 0 TRACERS none memcheck pmemcheck drd helgrind
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)

	build_test(enumerable_thread_specific_layout enumerable_thread_specific_layout/enumerable_thread_specific_layout.cpp)
	add_test_generic(NAME enumerable_thread_specific_layout TRACERS none)

	build_test(enumerable_thread_specific_initialize enumerable_thread_specific_initialize/enumerable_thread_specific_initialize.cpp)
	add_test_generic(NAME enumerable_thread_specific_initialize CASE 0 TRACERS none memcheck pmemcheck
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)

	build_test(enumerable_thread_specific_ctor enumerable_thread_specific_ctor/enumerable_thread_specific_ctor.cpp)
	add_test_generic(NAME enumerable_thread_specific_ctor CASE 0 TRACERS none memcheck pmemcheck
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)
endif()

if(TEST_CONCURRENT_MAP)
	build_test(concurrent_map concurrent_map/concurrent_map.cpp)
	# XXX: Add helgrind tracer for this test when we will fix false-positive with lock order
	add_test_generic(NAME concurrent_map TRACERS none memcheck pmemcheck drd)

	build_test(concurrent_map_singlethread concurrent_map_singlethread/concurrent_map_singlethread.cpp)
	add_test_generic(NAME concurrent_map_singlethread TRACERS none memcheck pmemcheck)

	build_test(concurrent_map_tx concurrent_map_tx/concurrent_map_tx.cpp)
	add_test_generic(NAME concurrent_map_tx TRACERS none memcheck pmemcheck)

	# XXX: Fix concurrent_map exceptions
	# build_test_ext(NAME concurrent_map_ctor_exceptions_nopmem SRC_FILES map/map_ctor_exception_nopmem.cpp BUILD_OPTIONS -DLIBPMEMOBJ_CPP_TESTS_CONCURRENT_MAP)
	# add_test_generic(NAME concurrent_map_ctor_exceptions_nopmem TRACERS none memcheck pmemcheck)

	build_test_ext(NAME concurrent_map_ctor_exceptions_notx SRC_FILES map/map_ctor_exception_notx.cpp BUILD_OPTIONS -DLIBPMEMOBJ_CPP_TESTS_CONCURRENT_MAP)
	add_test_generic(NAME concurrent_map_ctor_exceptions_notx TRACERS none memcheck)

	build_test_ext(NAME concurrent_map_txabort SRC_FILES map/map_txabort.cpp BUILD_OPTIONS -DLIBPMEMOBJ_CPP_TESTS_CONCURRENT_MAP)
	add_test_generic(NAME concurrent_map_txabort TRACERS none memcheck pmemcheck)

	build_test_ext(NAME concurrent_map_ctor_and_assignment SRC_FILES map/map_ctor_and_assignment.cpp BUILD_OPTIONS -DLIBPMEMOBJ_CPP_TESTS_CONCURRENT_MAP)
	add_test_generic(NAME concurrent_map_ctor_and_assignment TRACERS none memcheck pmemcheck)

	build_test_ext(NAME concurrent_map_find_lower_lower_eq SRC_FILES map/map_find_lower_lower_eq.cpp BUILD_OPTIONS -DLIBPMEMOBJ_CPP_TESTS_CONCURRENT_MAP)
	add_test_generic(NAME concurrent_map_find_lower_lower_eq TRACERS none memcheck pmemcheck)

	if(TESTS_CONCURRENT_GDB AND GDB_FOUND)
		if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
			build_test(concurrent_map_mt_gdb concurrent_map_mt_gdb/concurrent_map_mt_gdb.cpp)
			add_test_generic(NAME concurrent_map_mt_gdb TRACERS none CASE 0)
			add_test_generic(NAME concurrent_map_mt_gdb TRACERS none CASE 1)
			add_test_generic(NAME concurrent_map_mt_gdb TRACERS none CASE 2)
		else()
			message(WARNING "Skipping concurrent_map_mt_gdb test because it is non-debug build")
		endif()
	elseif(TESTS_CONCURRENT_GDB)
		message(WARNING "Skipping concurrent_map_mt_gdb test because GDB was not found")
	endif()

	build_test(concurrent_map_insert_reopen concurrent_map_insert_reopen/concurrent_map_insert_reopen.cpp)
	# XXX: Add helgrind tracer for this test when we will fix false-positive with lock order
	# XXX: Add drd tracer after issue https://github.com/pmem/libpmemobj-cpp/issues/770 will be resolved
	add_test_generic(NAME concurrent_map_insert_reopen CASE 0 TRACERS none memcheck pmemcheck
			SCRIPT concurrent_hash_map/check_is_pmem.cmake)

	if(PMREORDER_SUPPORTED)
		build_test(concurrent_map_pmreorder_simple concurrent_map_pmreorder_simple/concurrent_map_pmreorder_simple.cpp)
		add_test_generic(NAME concurrent_map_pmreorder_simple CASE 0 TRACERS none
				SCRIPT cmake/pmreorder.cmake)
		build_test(concurrent_map_pmreorder_erase concurrent_map_pmreorder_erase/concurrent_map_pmreorder_erase.cpp)
		add_test_generic(NAME concurrent_map_pmreorder_erase CASE 0 TRACERS none)

		if(GDB_FOUND)
			set(TEST concurrent_map_pmreorder_break_insert)
			build_test(${TEST} concurrent_map_pmreorder_simple/concurrent_map_pmreorder_simple.cpp)

			# add 5 concurrent_map_pmreorder_break_insert test cases
			set(CURRENT_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${TEST})
			set(CMAKE_SCRIPT ${CURRENT_TEST_DIR}/${TEST})
			set(GDB_SCRIPT ${CURRENT_TEST_DIR}/break_before_set_next)
			foreach(case RANGE 4)
				math(EXPR IGNORE "${case} + 1")
				math(EXPR PERSIST "${case} + 2")
				configure_file("${GDB_SCRIPT}.gdb.in" "${GDB_SCRIPT}_${PERSIST}.gdb" @ONLY)
				configure_file("${CMAKE_SCRIPT}.cmake.in" "${CMAKE_SCRIPT}_${case}.cmake" @ONLY)
				add_test_generic(NAME ${TEST} CASE ${case} TRACERS none)
			endforeach()
		else()
			message(WARNING "Skipping concurrent_map_pmreorder_break_insert test because GDB was not found")
		endif()
	else()
		message(WARNING "Skipping pmreorder tests because of no pmreorder support")
	endif()
endif()

if (TEST_RADIX_TREE)
	build_test(radix_tx_abort radix/radix_tx_abort.cpp)
	add_test_generic(NAME radix_tx_abort TRACERS none memcheck pmemcheck)

	build_test(radix_basic radix/radix_basic.cpp)
	add_test_generic(NAME radix_basic TRACERS none memcheck pmemcheck)

	build_test_ext(NAME radix_ctor_exceptions_nopmem SRC_FILES map/map_ctor_exception_nopmem.cpp BUILD_OPTIONS -DLIBPMEMOBJ_CPP_TESTS_RADIX)
	add_test_generic(NAME radix_ctor_exceptions_nopmem TRACERS none memcheck pmemcheck)

	build_test_ext(NAME radix_ctor_exceptions_notx SRC_FILES map/map_ctor_exception_notx.cpp BUILD_OPTIONS -DLIBPMEMOBJ_CPP_TESTS_RADIX)
	add_test_generic(NAME radix_ctor_exceptions_notx TRACERS none memcheck)

	build_test_ext(NAME radix_txabort SRC_FILES map/map_txabort.cpp BUILD_OPTIONS -DLIBPMEMOBJ_CPP_TESTS_RADIX)
	add_test_generic(NAME radix_txabort TRACERS none memcheck pmemcheck)

	build_test_ext(NAME radix_ctor_and_assignment SRC_FILES map/map_ctor_and_assignment.cpp BUILD_OPTIONS -DLIBPMEMOBJ_CPP_TESTS_RADIX)
	add_test_generic(NAME radix_ctor_and_assignment TRACERS none memcheck pmemcheck)

	if (TESTS_LONG)
		build_test(radix_large radix/radix_large.cpp)
		add_test_generic(NAME radix_large TRACERS none)
	endif()
endif()

add_subdirectory(external)

if (TESTS_COMPATIBILITY)
	add_subdirectory(compatibility)
endif()
