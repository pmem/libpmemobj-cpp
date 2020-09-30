# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2020, Intel Corporation

cmake_minimum_required(VERSION 3.3)
project(libpmemobj-cpp C CXX)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 11)
set(VERSION_PATCH 0)
#set(VERSION_PRERELEASE rc3)

set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR})
if (VERSION_PATCH GREATER 0)
	set(VERSION ${VERSION}.${VERSION_PATCH})
endif()
if (VERSION_PRERELEASE)
	set(VERSION ${VERSION}-${VERSION_PRERELEASE})
endif()

set(LIBPMEMOBJ_REQUIRED_VERSION 1.8)
set(LIBPMEM_REQUIRED_VERSION 1.7)
# Only pmreorder in ver. >= 1.9 guarantees reliable output
set(PMREORDER_REQUIRED_VERSION 1.9)

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# Treat CMAKE_CXX_STANDARD as a requirement
set(CXX_STANDARD_REQUIRED ON)

set(CXX_STANDARD 14 CACHE STRING "C++ language standard")
set(CMAKE_CXX_STANDARD ${CXX_STANDARD})

include(FindPerl)
include(FindThreads)
include(CMakeDependentOption)
include(CMakePackageConfigHelpers)
include(CheckCXXSourceCompiles)
include(CheckCXXCompilerFlag)
include(GNUInstallDirs)
include(${CMAKE_SOURCE_DIR}/cmake/functions.cmake)

option(BUILD_EXAMPLES "build examples" ON)
option(BUILD_TESTS "build tests" ON)
option(BUILD_DOC "build documentation" ON)
option(BUILD_BENCHMARKS "build benchmarks" ON)

option(COVERAGE "run coverage test" OFF)
option(DEVELOPER_MODE "enable developer checks" OFF)
option(CHECK_CPP_STYLE "check code style of C++ sources" OFF)
option(TRACE_TESTS "more verbose test outputs" OFF)
option(USE_ASAN "enable AddressSanitizer (debugging)" OFF)
option(USE_UBSAN "enable UndefinedBehaviorSanitizer (debugging)" OFF)

option(TESTS_USE_FORCED_PMEM "run tests with PMEM_IS_PMEM_FORCE=1" OFF)
option(TESTS_USE_VALGRIND "enable tests with valgrind (if found)" ON)
option(TESTS_PMREORDER "enable tests with pmreorder (if pmreorder found; it requires PMDK ver. >= 1.9)" OFF)
option(TESTS_CONCURRENT_HASH_MAP_DRD_HELGRIND "enable concurrent_hash_map tests with drd and helgrind (can only be run on PMEM)." OFF)
option(TESTS_CONCURRENT_GDB "enable concurrent gdb tests - require 'set scheduler-locking on' support (OS dependent) " OFF)
option(TESTS_LONG "enable long running tests" OFF)
option(TESTS_TBB "enable tests which require TBB" OFF)
option(TESTS_COMPATIBILITY "enable compatibility tests (requires internet connection)" OFF)

option(TEST_ARRAY "enable testing of pmem::obj::array" ON)
option(TEST_VECTOR "enable testing of pmem::obj::vector" ON)
option(TEST_STRING "enable testing of pmem::obj::string (depends on TEST_VECTOR)" ON)
option(TEST_CONCURRENT_HASHMAP "enable testing of pmem::obj::concurrent_hash_map (depends on TEST_STRING)" ON)
option(TEST_SEGMENT_VECTOR_ARRAY_EXPSIZE "enable testing of pmem::obj::segment_vector with array as segment_vector_type and exponential_size_policy" ON)
option(TEST_SEGMENT_VECTOR_VECTOR_EXPSIZE "enable testing of pmem::obj::segment_vector with vector as segment_vector_type and exponential_size_policy" ON)
option(TEST_SEGMENT_VECTOR_VECTOR_FIXEDSIZE "enable testing of pmem::obj::segment_vector with vector as segment_vector_type and fixed_size_policy" ON)
option(TEST_ENUMERABLE_THREAD_SPECIFIC "enable testing of pmem::obj::experimental::enumerable_thread_specific" ON)
option(TEST_CONCURRENT_MAP "enable testing of pmem::obj::experimental::concurrent_map (depends on TEST_STRING)" ON)
option(TEST_SELF_RELATIVE_POINTER "enable testing of pmem::obj::experimental::self_relative_ptr" ON)
option(TEST_RADIX_TREE "enable testing of pmem::obj::experimental::radix_tree" ON)

option(INSTALL_ARRAY "enable installation of pmem::obj::array" ON)
option(INSTALL_VECTOR "enable installation of pmem::obj::vector" ON)
option(INSTALL_STRING "enable installation of pmem::obj::string (depends on INSTALL_VECTOR)" ON)
option(INSTALL_CONCURRENT_HASHMAP "enable installation of pmem::obj::concurrent_hash_map (depends on INSTALL_STRING and INSTALL_SEGMENT_VECTOR)" ON)
option(INSTALL_SEGMENT_VECTOR "enable installation of pmem::obj::segment_vector" ON)
option(INSTALL_CONCURRENT_MAP "enable installation of pmem::obj::experimental::concurrent_map (depends on INSTALL_STRING)" ON)
option(INSTALL_SELF_RELATIVE_POINTER "enable installation of pmem::obj::experimental::self_relative_ptr" ON)
option(INSTALL_RADIX_TREE "enable testing of pmem::obj::experimental::radix_tree" ON)

# Do not treat include directories from the interfaces
# of consumed Imported Targets as SYSTEM by default.
set(CMAKE_NO_SYSTEM_FROM_IMPORTED 1)

# Required for MSVC to correctly define __cplusplus
add_flag("/Zc:__cplusplus")

set(TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/test
	CACHE STRING "working directory for tests")

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif (NOT CMAKE_BUILD_TYPE)

if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
	execute_process(COMMAND git describe
			OUTPUT_VARIABLE SRCVERSION
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET)
	if(NOT SRCVERSION)
		execute_process(COMMAND git log -1 --format=%h
				OUTPUT_VARIABLE SRCVERSION
				WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
				OUTPUT_STRIP_TRAILING_WHITESPACE)
	endif()
else()
	execute_process(COMMAND cat .version
			OUTPUT_VARIABLE SRCVERSION
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

if(NOT WIN32)
	find_package(PkgConfig QUIET)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

if(NOT PERL_FOUND)
	message(FATAL_ERROR "Perl not found")
endif()
if (PERL_VERSION_STRING VERSION_LESS 5.16)
	message(FATAL_ERROR "Too old Perl (<5.16)")
endif()

# Some tests and examples require clang >= 8.0
# because of the following bug:
# https://bugs.llvm.org/show_bug.cgi?id=28280
# which is fixed in clang v8.0.
set(CLANG_REQUIRED_BY_DESTRUCTOR_REFERENCE_BUG "8.0")
find_program(CLANG NAMES clang)
if(CLANG)
	get_program_version_major_minor(${CLANG} CLANG_VERSION)
	message(STATUS "Found clang: ${CLANG} (version: ${CLANG_VERSION})")
	if(CLANG_VERSION VERSION_LESS CLANG_REQUIRED_BY_DESTRUCTOR_REFERENCE_BUG)
		set(CLANG_DESTRUCTOR_REFERENCE_BUG_PRESENT 1)
	endif()
else()
	message(STATUS "clang not found")
endif()

if(BUILD_TESTS OR BUILD_EXAMPLES OR BUILD_BENCHMARKS)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(LIBPMEMOBJ REQUIRED libpmemobj>=${LIBPMEMOBJ_REQUIRED_VERSION})
		pkg_check_modules(LIBPMEM REQUIRED libpmem>=${LIBPMEM_REQUIRED_VERSION})
	else()
		find_package(LIBPMEMOBJ REQUIRED ${LIBPMEMOBJ_REQUIRED_VERSION})
		find_package(LIBPMEM REQUIRED ${LIBPMEM_REQUIRED_VERSION})
	endif()

	if (LIBPMEMOBJ_VERSION AND LIBPMEMOBJ_VERSION MATCHES "[0-9]+[.][0-9]+.*")
		string(REGEX REPLACE "\\+git.*" "" LIBPMEMOBJ_VERSION_SHORT ${LIBPMEMOBJ_VERSION})
		string(REGEX REPLACE "-rc.*" "" LIBPMEMOBJ_VERSION_SHORT ${LIBPMEMOBJ_VERSION_SHORT})
		string(REPLACE "." ";" VERSION_LIST ${LIBPMEMOBJ_VERSION_SHORT})
		list(GET VERSION_LIST 0 LIBPMEMOBJ_VERSION_MAJOR)
		list(GET VERSION_LIST 1 LIBPMEMOBJ_VERSION_MINOR)
		list(LENGTH VERSION_LIST OBJ_VER_COMPS)
		if (${OBJ_VER_COMPS} LESS 3)
			list(APPEND VERSION_LIST "0")
		endif()
		list(GET VERSION_LIST 2 LIBPMEMOBJ_VERSION_PATCH)
	else()
		message(WARNING "cannot detect libpmemobj version, some tests will be skipped")
		# assume 0.0.0
		set(LIBPMEMOBJ_VERSION_MAJOR 0)
		set(LIBPMEMOBJ_VERSION_MINOR 0)
		set(LIBPMEMOBJ_VERSION_PATCH 0)
	endif()

	math(EXPR LIBPMEMOBJ_VERSION_NUM "${LIBPMEMOBJ_VERSION_PATCH} + ${LIBPMEMOBJ_VERSION_MINOR} * 100 + ${LIBPMEMOBJ_VERSION_MAJOR} * 10000")

	include(tbb)
endif()

add_custom_target(checkers ALL)
add_custom_target(cppstyle)
add_custom_target(cppformat)
add_custom_target(check-whitespace)
add_custom_target(check-license
	COMMAND ${CMAKE_SOURCE_DIR}/utils/check_license/check-headers.sh
		${CMAKE_SOURCE_DIR}
		BSD-3-Clause)

add_custom_target(check-whitespace-main
		COMMAND ${PERL_EXECUTABLE}
			${CMAKE_SOURCE_DIR}/utils/check_whitespace
			${CMAKE_SOURCE_DIR}/utils/check_license/*.sh
			${CMAKE_SOURCE_DIR}/README.md)

add_dependencies(check-whitespace check-whitespace-main)

add_custom_target(tests)

if(CHECK_CPP_STYLE)
	find_program(CLANG_FORMAT NAMES clang-format clang-format-9 clang-format-9.0)
	set(CLANG_FORMAT_REQUIRED "9.0")
	if(CLANG_FORMAT)
		get_program_version_major_minor(${CLANG_FORMAT} CLANG_FORMAT_VERSION)
		message(STATUS "Found clang-format: ${CLANG_FORMAT} (version: ${CLANG_FORMAT_VERSION})")
		if(NOT (CLANG_FORMAT_VERSION VERSION_EQUAL CLANG_FORMAT_REQUIRED))
			message(FATAL_ERROR "required clang-format version is ${CLANG_FORMAT_REQUIRED}")
		endif()
	else()
		message(FATAL_ERROR "CHECK_CPP_STYLE=ON, but clang-format not found (required version: ${CLANG_FORMAT_REQUIRED})")
	endif()
endif()

if(DEVELOPER_MODE)
	if(WIN32)
		add_flag(-WX)
	else()
		add_flag(-Werror)
		execute_process(COMMAND ${PERL_EXECUTABLE} -MText::Diff -e ""
				ERROR_QUIET
				RESULT_VARIABLE PERL_TEXT_DIFF_STATUS)
		if (PERL_TEXT_DIFF_STATUS)
			message(FATAL_ERROR "Text::Diff Perl module not found (install libtext-diff-perl or perl-Text-Diff)")
		endif()

		add_dependencies(checkers cppstyle)
		add_dependencies(checkers check-whitespace)
		add_dependencies(checkers check-license)
	endif()
endif(DEVELOPER_MODE)

add_cppstyle(include ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/*.hpp)
add_cppstyle(include-container ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/container/*.hpp)
add_cppstyle(include-container-detail ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/container/detail/*.hpp)
add_cppstyle(include-detail ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/detail/*.hpp)
add_cppstyle(include-experimental ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/experimental/*.hpp)
add_check_whitespace(include ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/*.hpp)
add_check_whitespace(include-container ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/container/*.hpp)
add_check_whitespace(include-container-detail ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/container/detail/*.hpp)
add_check_whitespace(include-detail ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/detail/*.hpp)
add_check_whitespace(include-experimental ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/experimental/*.hpp)
add_check_whitespace(cmake-main ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)
add_check_whitespace(cmake-helpers ${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.cmake)

configure_file(${CMAKE_SOURCE_DIR}/cmake/version.hpp.in
		${CMAKE_SOURCE_DIR}/include/libpmemobj++/version.hpp @ONLY)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	FILES_MATCHING PATTERN "*.hpp"
	PATTERN "array.hpp" EXCLUDE
	PATTERN "vector.hpp" EXCLUDE
	PATTERN "string.hpp" EXCLUDE
	PATTERN "basic_string.hpp" EXCLUDE
	PATTERN "contiguous_iterator.hpp" EXCLUDE
	PATTERN "slice.hpp" EXCLUDE
	PATTERN "concurrent_hash_map.hpp" EXCLUDE
	PATTERN "segment_vector.hpp" EXCLUDE
	PATTERN "enumerable_thread_specific.hpp" EXCLUDE
	PATTERN "concurrent_map.hpp" EXCLUDE
	PATTERN "self_relative_ptr.hpp" EXCLUDE
	PATTERN "self_relative_ptr_base.hpp" EXCLUDE
	PATTERN "inline_string.hpp" EXCLUDE
	PATTERN "bytes_view.hpp" EXCLUDE
	PATTERN "radix_tree.hpp" EXCLUDE
	PATTERN "self_relative_ptr_base_impl.hpp" EXCLUDE
	PATTERN "atomic_self_relative_ptr.hpp" EXCLUDE
	)

if(INSTALL_ARRAY)
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "array.hpp")
endif()

if(INSTALL_VECTOR)
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "vector.hpp")
endif()

if(INSTALL_STRING)
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "basic_string.hpp")
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "string.hpp")
endif()

if(INSTALL_CONCURRENT_HASHMAP)
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "concurrent_hash_map.hpp")
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "enumerable_thread_specific.hpp")
endif()

if(INSTALL_ARRAY OR INSTALL_VECTOR OR INSTALL_STRING)
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "contiguous_iterator.hpp")
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "slice.hpp")
endif()

if(INSTALL_SEGMENT_VECTOR)
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "segment_vector.hpp")
endif()

if(INSTALL_CONCURRENT_MAP)
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "concurrent_map.hpp")
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "enumerable_thread_specific.hpp")
endif()

if(INSTALL_SELF_RELATIVE_POINTER)
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "self_relative_ptr.hpp")
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "self_relative_ptr_base.hpp")
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "self_relative_ptr_base_impl.hpp")
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "atomic_self_relative_ptr.hpp")
endif()

if(INSTALL_RADIX_TREE)
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "bytes_view.hpp")
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "inline_string.hpp")
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "radix_tree.hpp")
endif()


install(DIRECTORY examples/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples
	FILES_MATCHING PATTERN "*.*pp")

configure_file(${CMAKE_SOURCE_DIR}/cmake/libpmemobj++.pc.in
		${CMAKE_CURRENT_BINARY_DIR}/libpmemobj++.pc @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpmemobj++.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

configure_file(
	"${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)

add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

configure_package_config_file(${CMAKE_SOURCE_DIR}/cmake/libpmemobj++-config.cmake.in
		${CMAKE_CURRENT_BINARY_DIR}/libpmemobj++-config.cmake
		INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/libpmemobj++/cmake
		PATH_VARS CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR)

write_basic_package_version_file(libpmemobj++-config-version.cmake
				VERSION ${VERSION}
				COMPATIBILITY AnyNewerVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpmemobj++-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/libpmemobj++-config-version.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/libpmemobj++/cmake)

include_directories(include)

include(${CMAKE_SOURCE_DIR}/cmake/check_compiling_issues.cmake)

if(PKG_CONFIG_FOUND)
	pkg_check_modules(VALGRIND QUIET valgrind)
else()
	find_package(VALGRIND QUIET)
endif()

if(VALGRIND_FOUND)
	add_flag(-DLIBPMEMOBJ_CPP_VG_MEMCHECK_ENABLED=1)
	add_flag(-DLIBPMEMOBJ_CPP_VG_DRD_ENABLED=1)
	add_flag(-DLIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED=1)

	include_directories(${VALGRIND_INCLUDE_DIRS})
	find_pmemcheck()

	if(VALGRIND_PMEMCHECK_FOUND)
		add_flag(-DLIBPMEMOBJ_CPP_VG_PMEMCHECK_ENABLED=1)
	endif()
endif()

if(BUILD_TESTS)
	if(TEST_DIR)
		enable_testing()
	else()
		message(WARNING "TEST_DIR is empty - 'make test' will not work")
	endif()

	add_subdirectory(tests)
endif()

if(BUILD_DOC)
	add_subdirectory(doc)
endif()

if (BUILD_BENCHMARKS)
	add_subdirectory(benchmarks)
endif()

if(BUILD_EXAMPLES AND NO_GCC_VARIADIC_TEMPLATE_BUG)
	add_subdirectory(examples)
elseif(BUILD_EXAMPLES)
	message(WARNING "Skipping build of examples because of compiler issue")
endif()

if(NOT "${CPACK_GENERATOR}" STREQUAL "")
	include(${CMAKE_SOURCE_DIR}/cmake/packages.cmake)
endif()
