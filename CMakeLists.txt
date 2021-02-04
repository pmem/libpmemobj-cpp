# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2021, Intel Corporation

cmake_minimum_required(VERSION 3.3)
project(libpmemobj-cpp C CXX)

# ----------------------------------------------------------------- #
## Set required and useful variables
# ----------------------------------------------------------------- #
set(VERSION_MAJOR 1)
set(VERSION_MINOR 12)
set(VERSION_PATCH 0)
set(VERSION_PRERELEASE rc1)

set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR})
if(VERSION_PATCH GREATER 0)
	set(VERSION ${VERSION}.${VERSION_PATCH})
endif()
if(VERSION_PRERELEASE)
	set(VERSION ${VERSION}-${VERSION_PRERELEASE})
endif()

set(LIBPMEMOBJCPP_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LIBPMEMOBJ_REQUIRED_VERSION 1.9)
set(LIBPMEM_REQUIRED_VERSION 1.7)
# Only pmreorder in ver. >= 1.9 guarantees reliable output
set(PMREORDER_REQUIRED_VERSION 1.9)

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# Treat CMAKE_CXX_STANDARD as a requirement
set(CXX_STANDARD_REQUIRED ON)

set(CXX_STANDARD 14 CACHE STRING "C++ language standard")
set(CMAKE_CXX_STANDARD ${CXX_STANDARD})

# Do not treat include directories from the interfaces
# of consumed Imported Targets as SYSTEM by default.
set(CMAKE_NO_SYSTEM_FROM_IMPORTED 1)

set(TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/test CACHE STRING "working directory for tests")
message(STATUS "TEST_DIR set to: \"${TEST_DIR}\"")

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${LIBPMEMOBJCPP_ROOT_DIR}/cmake)

# ----------------------------------------------------------------- #
## CMake build options
# ----------------------------------------------------------------- #
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
option(USE_CCACHE "Use ccache if it is available in the system" ON)

option(TESTS_USE_FORCED_PMEM "run tests with PMEM_IS_PMEM_FORCE=1 - it speeds up tests execution on emulated pmem" OFF)
option(TESTS_USE_VALGRIND "enable tests with valgrind (fail build if Valgrind not found)" ON)
option(TESTS_PMREORDER "enable tests with pmreorder (if pmreorder found; it requires PMDK ver. >= 1.9)" OFF)
option(TESTS_CONCURRENT_HASH_MAP_DRD_HELGRIND "enable concurrent_hash_map tests with drd and helgrind (can only be run on PMEM)" OFF)
option(TESTS_CONCURRENT_GDB "enable concurrent gdb tests - require 'set scheduler-locking on' support (OS dependent)" OFF)
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
option(TEST_ENUMERABLE_THREAD_SPECIFIC "enable testing of pmem::obj::enumerable_thread_specific" ON)
option(TEST_CONCURRENT_MAP "enable testing of pmem::obj::experimental::concurrent_map (depends on TEST_STRING)" ON)
option(TEST_SELF_RELATIVE_POINTER "enable testing of pmem::obj::experimental::self_relative_ptr" ON)
option(TEST_RADIX_TREE "enable testing of pmem::obj::experimental::radix_tree" ON)

# ----------------------------------------------------------------- #
## Setup environment, find packages, set compiler's flags,
## add additional custom targets, ...
# ----------------------------------------------------------------- #
include(FindPerl)
include(FindThreads)
include(CMakeDependentOption)
include(CMakePackageConfigHelpers)
include(CheckCXXSourceCompiles)
include(CheckCXXCompilerFlag)
include(GNUInstallDirs)

include(${LIBPMEMOBJCPP_ROOT_DIR}/cmake/functions.cmake)
# set SRCVERSION, it's more accurate and "current" than VERSION
set_source_ver(SRCVERSION)

if(WIN32)
	# Required for MSVC to correctly define __cplusplus
	add_flag("/Zc:__cplusplus")
endif()

if(NOT WIN32)
	find_package(PkgConfig QUIET)
endif()

# Configure the ccache as compiler launcher
find_program(CCACHE_FOUND ccache)
if(USE_CCACHE AND CCACHE_FOUND)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif()

# Find Valgrind
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

# Find Clang
find_program(CLANG NAMES clang)
if(CLANG)
	message(STATUS "Found clang: ${CLANG} (version: ${CLANG_VERSION})")
else()
	message(STATUS "clang not found")
endif()

if(BUILD_TESTS OR BUILD_EXAMPLES OR BUILD_BENCHMARKS)
	# Find libpmem and libpmemobj (PMDK libraries)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(LIBPMEMOBJ REQUIRED libpmemobj>=${LIBPMEMOBJ_REQUIRED_VERSION})
		pkg_check_modules(LIBPMEM REQUIRED libpmem>=${LIBPMEM_REQUIRED_VERSION})
	else()
		find_package(LIBPMEMOBJ REQUIRED ${LIBPMEMOBJ_REQUIRED_VERSION})
		find_package(LIBPMEM REQUIRED ${LIBPMEM_REQUIRED_VERSION})
	endif()

	# Some tests and examples require clang >= 8.0, because of the bug
	# (https://bugs.llvm.org/show_bug.cgi?id=28280), which is fixed in clang v8.0.
	set(CLANG_REQUIRED_BY_DESTRUCTOR_REFERENCE_BUG "8.0")
	if(CLANG)
		get_program_version_major_minor(${CLANG} CLANG_VERSION)
		if(CLANG_VERSION VERSION_LESS CLANG_REQUIRED_BY_DESTRUCTOR_REFERENCE_BUG)
			set(CLANG_DESTRUCTOR_REFERENCE_BUG_PRESENT 1)
		endif()
	endif()
endif()

add_custom_target(checkers ALL)
add_custom_target(cppstyle)
add_custom_target(cppformat)
add_custom_target(check-whitespace)
add_custom_target(check-license
	COMMAND ${LIBPMEMOBJCPP_ROOT_DIR}/utils/check_license/check-headers.sh
			${LIBPMEMOBJCPP_ROOT_DIR}
			BSD-3-Clause)

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

	add_dependencies(checkers cppstyle)
endif()

if(DEVELOPER_MODE)
	# treat compiler warnings as errors
	if(WIN32)
		add_flag(-WX)
	else()
		add_flag(-Werror)

		# check for required programs for whitespace and license checks and add dependencies to ALL
		if(NOT PERL_FOUND)
			message(FATAL_ERROR "Perl not found")
		endif()
		if(PERL_VERSION_STRING VERSION_LESS 5.16)
			message(FATAL_ERROR "Too old Perl (<5.16)")
		endif()
		execute_process(COMMAND ${PERL_EXECUTABLE} -MText::Diff -e ""
						ERROR_QUIET
						RESULT_VARIABLE PERL_TEXT_DIFF_STATUS)
		if(PERL_TEXT_DIFF_STATUS)
			message(FATAL_ERROR "Text::Diff Perl module not found (install libtext-diff-perl or perl-Text-Diff)")
		endif()

		add_dependencies(checkers check-whitespace)
		add_dependencies(checkers check-license)
	endif()
endif()

add_cppstyle(include ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/*.hpp)
add_cppstyle(include-container ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/container/*.hpp)
add_cppstyle(include-container-detail ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/container/detail/*.hpp)
add_cppstyle(include-detail ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/detail/*.hpp)
add_cppstyle(include-experimental ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/experimental/*.hpp)

add_check_whitespace(main ${LIBPMEMOBJCPP_ROOT_DIR}/utils/check_license/*.sh
						${LIBPMEMOBJCPP_ROOT_DIR}/*.md)
add_check_whitespace(include ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/*.hpp)
add_check_whitespace(include-container ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/container/*.hpp)
add_check_whitespace(include-container-detail ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/container/detail/*.hpp)
add_check_whitespace(include-detail ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/detail/*.hpp)
add_check_whitespace(include-experimental ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/experimental/*.hpp)
add_check_whitespace(cmake-main ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)
add_check_whitespace(cmake-helpers ${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.cmake)

# ----------------------------------------------------------------- #
## Configure make install/uninstall and packages
# ----------------------------------------------------------------- #
configure_file(${LIBPMEMOBJCPP_ROOT_DIR}/cmake/version.hpp.in
	${CMAKE_CURRENT_BINARY_DIR}/version.hpp @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/version.hpp
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libpmemobj++)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	FILES_MATCHING PATTERN "*.hpp")

install(DIRECTORY examples/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples
	FILES_MATCHING PATTERN "*.*pp")

configure_file(${LIBPMEMOBJCPP_ROOT_DIR}/cmake/libpmemobj++.pc.in
		${CMAKE_CURRENT_BINARY_DIR}/libpmemobj++.pc @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpmemobj++.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

configure_file(
	"${LIBPMEMOBJCPP_ROOT_DIR}/cmake/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)

add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

configure_package_config_file(${LIBPMEMOBJCPP_ROOT_DIR}/cmake/libpmemobj++-config.cmake.in
		${CMAKE_CURRENT_BINARY_DIR}/libpmemobj++-config.cmake
		INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/libpmemobj++/cmake
		PATH_VARS CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR)

write_basic_package_version_file(libpmemobj++-config-version.cmake
				VERSION ${VERSION}
				COMPATIBILITY AnyNewerVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpmemobj++-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/libpmemobj++-config-version.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/libpmemobj++/cmake)


# set up project's include dir (our source 'include' dir)
include_directories(include)

# Run checks for known issues, it's required to enable/disable some tests and examples.
# It has to be executed (included) here, when environment is fully set up,
# all packages are found and all paths/variables are set.
include(${LIBPMEMOBJCPP_ROOT_DIR}/cmake/check_compiling_issues.cmake)

# ----------------------------------------------------------------- #
## Add/include sub-directories if build options enabled them
# ----------------------------------------------------------------- #
if(BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

if(BUILD_DOC)
	add_subdirectory(doc)
endif()

if(BUILD_BENCHMARKS)
	add_subdirectory(benchmarks)
endif()

if(BUILD_EXAMPLES AND NO_GCC_VARIADIC_TEMPLATE_BUG)
	add_subdirectory(examples)
elseif(BUILD_EXAMPLES)
	message(WARNING "Skipping building of examples because of compiler issue")
endif()

if(NOT "${CPACK_GENERATOR}" STREQUAL "")
	include(${LIBPMEMOBJCPP_ROOT_DIR}/cmake/packages.cmake)
endif()
