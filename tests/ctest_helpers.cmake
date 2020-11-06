# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2020, Intel Corporation

#
# ctest_helpers.cmake - helper functions for tests/CMakeLists.txt
#

set(TEST_ROOT_DIR ${PROJECT_SOURCE_DIR}/tests)

set(GLOBAL_TEST_ARGS
	-DPERL_EXECUTABLE=${PERL_EXECUTABLE}
	-DMATCH_SCRIPT=${PROJECT_SOURCE_DIR}/tests/match
	-DPARENT_DIR=${TEST_DIR}
	-DTESTS_USE_FORCED_PMEM=${TESTS_USE_FORCED_PMEM}
	-DTEST_ROOT_DIR=${TEST_ROOT_DIR})

if(TRACE_TESTS)
	set(GLOBAL_TEST_ARGS ${GLOBAL_TEST_ARGS} --trace-expand)
endif()

set(INCLUDE_DIRS ${LIBPMEMOBJ_INCLUDE_DIRS} ${LIBPMEM_INCLUDE_DIRS} common/ .. .)
set(LIBS_DIRS ${LIBPMEMOBJ_LIBRARY_DIRS} ${LIBPMEM_LIBRARY_DIRS})

include_directories(${INCLUDE_DIRS})
link_directories(${LIBS_DIRS})

function(find_gdb)
	execute_process(COMMAND gdb --help
			RESULT_VARIABLE GDB_RET
			OUTPUT_QUIET
			ERROR_QUIET)
	if(GDB_RET)
		set(GDB_FOUND 0 CACHE INTERNAL "")
		message(WARNING "GDB NOT found, some tests will be skipped")
	else()
		set(GDB_FOUND 1 CACHE INTERNAL "")
	endif()
endfunction()

function(find_packages)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(CURSES QUIET ncurses)
	else()
		# Specifies that we want FindCurses to find ncurses and not just any
		# curses library
		set(CURSES_NEED_NCURSES TRUE)
		find_package(Curses QUIET)
	endif()

	if(PKG_CONFIG_FOUND)
		pkg_check_modules(LIBUNWIND QUIET libunwind)
	else()
		find_package(LIBUNWIND QUIET)
	endif()
	if(NOT LIBUNWIND_FOUND)
		message(WARNING "libunwind not found. Stack traces from tests will not be reliable")
	endif()

	# XXX: if pmreorder not supported (e.g. on Win) - print one message "tests will be skipped"
	if(NOT WIN32)
		if(VALGRIND_FOUND AND TESTS_PMREORDER)
			if((NOT(PMEMCHECK_VERSION LESS 1.0)) AND PMEMCHECK_VERSION LESS 2.0)
				find_program(PMREORDER names pmreorder HINTS ${LIBPMEMOBJ_PREFIX}/bin)
				get_program_version_major_minor(${PMREORDER} PMREORDER_VERSION)
				message(STATUS "pmreorder found: ${PMREORDER}, in version: ${PMREORDER_VERSION}")

				if(PMREORDER_VERSION EQUAL PMREORDER_REQUIRED_VERSION OR PMREORDER_VERSION GREATER PMREORDER_REQUIRED_VERSION)
					set(PROPER_PMREORDER_VERSION 1)
				endif()

				if(PMREORDER AND PROPER_PMREORDER_VERSION)
					set(ENV{PATH} ${LIBPMEMOBJ_PREFIX}/bin:$ENV{PATH})
					set(PMREORDER_SUPPORTED true CACHE INTERNAL "pmreorder support")
				elseif(NOT PMREORDER)
					message(WARNING "Pmreorder not found - pmreorder tests will not be performed.")
				elseif(NOT PROPER_PMREORDER_VERSION)
					message(WARNING "Pmreorder should be in version >= ${PMREORDER_REQUIRED_VERSION} - pmreorder tests will not be performed.")
				endif()
			else()
				message(WARNING "Pmemcheck must be installed in version 1.X for pmreorder to work - pmreorder tests will not be performed.")
			endif()
		elseif(NOT VALGRIND_FOUND AND TESTS_USE_VALGRIND)
			message(FATAL_ERROR "Valgrind not found, but tests are enabled. To disable them set CMake option TESTS_USE_VALGRIND=OFF.")
		endif()
	elseif(TESTS_USE_VALGRIND)
		message(WARNING "Valgrind tests can't be run on Windows - they will be skipped.")
	endif()
endfunction()

# Function to build test with custom build options (e.g. passing defines)
# Example: build_test_ext(NAME ... SRC_FILES ....cpp BUILD_OPTIONS -D...)
function(build_test_ext)
	set(oneValueArgs NAME)
	set(multiValueArgs SRC_FILES BUILD_OPTIONS)
	cmake_parse_arguments(TEST "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	build_test(${TEST_NAME} ${TEST_SRC_FILES})
	target_compile_definitions(${TEST_NAME} PRIVATE ${TEST_BUILD_OPTIONS})
endfunction()

function(build_test name)
	set(srcs ${ARGN})
	prepend(srcs ${CMAKE_CURRENT_SOURCE_DIR} ${srcs})

	add_cppstyle(tests-${name} ${srcs})
	add_check_whitespace(tests-${name} ${srcs})

	add_executable(${name} ${srcs})
	target_link_libraries(${name} ${LIBPMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} test_backtrace valgrind_internal)
	if(LIBUNWIND_FOUND)
		target_link_libraries(${name} ${LIBUNWIND_LIBRARIES} ${CMAKE_DL_LIBS})
	endif()
	if(WIN32)
		target_link_libraries(${name} dbghelp)
	endif()

	add_dependencies(tests ${name})
endfunction()

# Function to build a test with mocked pmemobj_defrag() function
function(build_test_defrag name)
	build_test(${name} ${ARGN})
	if (NOT WIN32)
		# target_link_options() should be used below,
		# but it is available since CMake v3.13
		target_link_libraries(${name} "-Wl,--wrap=pmemobj_defrag")
	endif()
endfunction()

function(build_test_tbb name)
	build_test(${name} ${ARGN})
	target_link_libraries(${name} ${TBB_LIBRARIES})
endfunction()

# Function to build test with atomic
function(build_test_atomic name)
	build_test(${name} ${ARGN})
	if(CLANG)
		target_link_libraries(${name} atomic)
	endif()
endfunction()

# Function to build a TBB test with mocked pmemobj_defrag() function
function(build_test_tbb_defrag name)
	build_test_tbb(${name} ${ARGN})
	if (NOT WIN32)
		# target_link_options() should be used below,
		# but it is available since CMake v3.13
		target_link_libraries(${name} "-Wl,--wrap=pmemobj_defrag")
	endif()
endfunction()

set(vg_tracers memcheck helgrind drd pmemcheck)

# Configures testcase ${name} ${testcase} using tracer ${tracer}, cmake_script is used to run test
function(add_testcase name tracer testcase cmake_script)
	set(executable ${name})
	add_test(NAME ${executable}_${testcase}_${tracer}
			COMMAND ${CMAKE_COMMAND}
			${GLOBAL_TEST_ARGS}
			-DTEST_NAME=${executable}_${testcase}_${tracer}
			-DTESTCASE=${testcase}
			-DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}/${name}
			-DBIN_DIR=${CMAKE_CURRENT_BINARY_DIR}/${executable}_${testcase}_${tracer}
			-DTEST_EXECUTABLE=$<TARGET_FILE:${executable}>
			-DTRACER=${tracer}
			-DLONG_TESTS=${LONG_TESTS}
			-P ${cmake_script})

	set_tests_properties(${name}_${testcase}_${tracer} PROPERTIES
			ENVIRONMENT "LC_ALL=C;PATH=$ENV{PATH};"
			FAIL_REGULAR_EXPRESSION Sanitizer)

	if (${tracer} STREQUAL pmemcheck)
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
	else()
		set_tests_properties(${name}_${testcase}_${tracer} PROPERTIES
				COST 10)
	endif()
endfunction()

function(skip_test name message)
	add_test(NAME ${name}_${message}
		COMMAND ${CMAKE_COMMAND} -P ${TEST_ROOT_DIR}/true.cmake)

	set_tests_properties(${name}_${message} PROPERTIES COST 0)
endfunction()

# adds testcase only if tracer is found and target is build, skips adding test otherwise
function(add_test_common name tracer testcase cmake_script)
	if(${tracer} STREQUAL "")
	    set(tracer none)
	endif()

	# Valgrind tests won't work on Windows
	if(NOT WIN32 AND ${tracer} IN_LIST vg_tracers)
		if(TESTS_USE_VALGRIND)
			# pmemcheck is not necessarily required (because it's not always delivered with Valgrind)
			if(${tracer} STREQUAL "pmemcheck" AND (NOT VALGRIND_PMEMCHECK_FOUND))
				skip_test(${name}_${testcase}_${tracer} "SKIPPED_BECAUSE_OF_MISSING_PMEMCHECK")
				return()
			# Valgrind on the other hand is required, because we enabled Valgrind tests
			elseif(NOT VALGRIND_FOUND)
				message(FATAL_ERROR "Valgrind not found, but tests are enabled. To disable them set CMake option TESTS_USE_VALGRIND=OFF.")
			endif()
		else()
			# TESTS_USE_VALGRIND=OFF so just don't run Valgrind tests
			return()
		endif()

		if(USE_ASAN OR USE_UBSAN)
			skip_test(${name}_${testcase}_${tracer} "SKIPPED_BECAUSE_SANITIZER_USED")
			return()
		endif()
	endif()

	# test was not build
	if (NOT TARGET ${name})
		message(FATAL_ERROR "${executable} not build.")
	endif()

	# skip all Valgrind tests on Windows
	if (WIN32 AND ${tracer} IN_LIST vg_tracers)
		return()
	endif()

	add_testcase(${name} ${tracer} ${testcase} ${cmake_script})
endfunction()

# adds testscase with optional TRACERS and SCRIPT parameters
function(add_test_generic)
	set(oneValueArgs NAME CASE SCRIPT)
	set(multiValueArgs TRACERS)
	cmake_parse_arguments(TEST "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	if("${TEST_SCRIPT}" STREQUAL "")
		if("${TEST_CASE}" STREQUAL "")
			set(TEST_CASE "0")
			set(cmake_script ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_default.cmake)
		else()
			set(cmake_script ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}/${TEST_NAME}_${TEST_CASE}.cmake)
		endif()
	else()
		if("${TEST_CASE}" STREQUAL "")
			set(TEST_CASE "0")
		endif()
		set(cmake_script ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SCRIPT})
	endif()

	foreach(tracer ${TEST_TRACERS})
		add_test_common(${TEST_NAME} ${tracer} ${TEST_CASE} ${cmake_script})
	endforeach()
endfunction()
