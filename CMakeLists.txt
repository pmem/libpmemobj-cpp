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

cmake_minimum_required(VERSION 3.3)
project(libpmemobj-cpp C CXX)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 5)
set(VERSION_PATCH 1)
set(VERSION_PRERELEASE rc4)

set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR})
if (VERSION_PATCH GREATER 0)
	set(VERSION ${VERSION}.${VERSION_PATCH})
endif()
if (VERSION_PRERELEASE)
	set(VERSION ${VERSION}-${VERSION_PRERELEASE})
endif()

set(LIBPMEMOBJ_REQUIRED_VERSION 1.4)

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# Treat CMAKE_CXX_STANDARD as a requirement
set(CXX_STANDARD_REQUIRED ON)

set(CXX_STANDARD 11 CACHE STRING "C++ language standard")
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
option(COVERAGE "run coverage test" OFF)
option(DEVELOPER_MODE "enable developer checks" OFF)
option(TRACE_TESTS "more verbose test outputs" OFF)
option(USE_ASAN "enable AddressSanitizer (debugging)" OFF)
option(USE_UBSAN "enable UndefinedBehaviorSanitizer (debugging)" OFF)

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

if(BUILD_TESTS OR BUILD_EXAMPLES)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(LIBPMEMOBJ REQUIRED libpmemobj>=1.4)
	else()
		find_package(LIBPMEMOBJ REQUIRED 1.4)
	endif()
endif()

if(PKG_CONFIG_FOUND)
	pkg_check_modules(VALGRIND QUIET valgrind)
else()
	find_package(VALGRIND QUIET)
endif()

if(PKG_CONFIG_FOUND)
	pkg_check_modules(LIBUNWIND QUIET libunwind)
else()
	find_package(LIBUNWIND QUIET)
endif()
if(NOT LIBUNWIND_FOUND)
	message(WARNING "libunwind not found. Stack traces from tests will not be reliable")
endif()

add_executable(check_license EXCLUDE_FROM_ALL utils/check_license/check-license.c)

add_custom_target(checkers ALL)
add_custom_target(cppstyle)
add_custom_target(cppformat)
add_custom_target(check-whitespace)
add_custom_target(check-license
	COMMAND ${CMAKE_SOURCE_DIR}/utils/check_license/check-headers.sh
		${CMAKE_SOURCE_DIR}
		${CMAKE_BINARY_DIR}/check_license
		${CMAKE_SOURCE_DIR}/LICENSE
		-a)
add_dependencies(check-license check_license)

add_custom_target(check-whitespace-main
		COMMAND ${PERL_EXECUTABLE}
			${CMAKE_SOURCE_DIR}/utils/check_whitespace
			${CMAKE_SOURCE_DIR}/utils/check_license/*.sh
			${CMAKE_SOURCE_DIR}/README.md)

add_dependencies(check-whitespace check-whitespace-main)

add_custom_target(tests)

if(DEVELOPER_MODE)
	add_flag(-Werror) # XXX: WX for windows
	find_program(CLANG_FORMAT NAMES clang-format-6.0)
	if (NOT CLANG_FORMAT)
		message(WARNING "clang-format not found - C++ sources will not be checked (needed version: 6.0)")
	endif()

	execute_process(COMMAND ${PERL_EXECUTABLE} -MText::Diff -e ""
			ERROR_QUIET
			RESULT_VARIABLE PERL_TEXT_DIFF_STATUS)
	if (PERL_TEXT_DIFF_STATUS)
		message(FATAL_ERROR "Text::Diff Perl module not found (install libtext-diff-perl or perl-Text-Diff)")
	endif()

	add_dependencies(checkers cppstyle)
	add_dependencies(checkers check-whitespace)
	add_dependencies(checkers check-license)
endif(DEVELOPER_MODE)

add_cppstyle(include ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/*.hpp)
add_cppstyle(include-detail ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/detail/*.hpp)
add_cppstyle(include-experimental ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/experimental/*.hpp)
add_check_whitespace(include ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/*.hpp)
add_check_whitespace(include-detail ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/detail/*.hpp)
add_check_whitespace(include-experimental ${CMAKE_CURRENT_SOURCE_DIR}/include/libpmemobj++/experimental/*.hpp)
add_check_whitespace(cmake-main ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)
add_check_whitespace(cmake-helpers ${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.cmake)

# Check for existence of pmemvlt (introduced after 1.4 release)
set(SAVED_CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
set(CMAKE_REQUIRED_INCLUDES ${LIBPMEMOBJ_INCLUDE_DIRS})
CHECK_CXX_SOURCE_COMPILES(
	"#include <libpmemobj.h>
	struct pmemvlt vlt;
	int main() {}"
	PMEMVLT_PRESENT)
set(CMAKE_REQUIRED_INCLUDES ${SAVED_CMAKE_REQUIRED_INCLUDES})

if(NOT PMEMVLT_PRESENT)
	message(WARNING "pmemvlt support in libpmemobj not found (to enable - use libpmemobj version > 1.4")
endif()

configure_file(${CMAKE_SOURCE_DIR}/cmake/version.hpp.in
		${CMAKE_SOURCE_DIR}/include/libpmemobj++/version.hpp @ONLY)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	FILES_MATCHING PATTERN "*.hpp")

install(DIRECTORY examples/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples
	FILES_MATCHING PATTERN "*.*pp")

configure_file(${CMAKE_SOURCE_DIR}/cmake/libpmemobj++.pc.in
		${CMAKE_CURRENT_BINARY_DIR}/libpmemobj++.pc @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpmemobj++.pc
	CONFIGURATIONS Release Debug
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

if(NOT WIN32)
    if(VALGRIND_FOUND)
	    set(ENV{PATH} ${VALGRIND_PREFIX}/bin:$ENV{PATH})
	    execute_process(COMMAND valgrind --tool=pmemcheck --help
			    RESULT_VARIABLE VALGRIND_PMEMCHECK_NOT_FOUND
			    OUTPUT_QUIET
			    ERROR_QUIET)
	    if(VALGRIND_PMEMCHECK_NOT_FOUND)
		    message(WARNING "Valgrind pmemcheck NOT found. Pmemcheck tests will not be performed.")
	    endif()
    else()
	    message(WARNING "Valgrind not found. Valgrind tests will not be performed.")
    endif()
endif()


if(NOT MSVC_VERSION)
	set(SAVED_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})

	# Check for issues with older gcc compilers which do not expand variadic template
	# variables in lambda expressions.
	set(CMAKE_REQUIRED_FLAGS "--std=c++11 -Wno-error -c")
	CHECK_CXX_SOURCE_COMPILES(
		"void print() {}
		template<typename...Args, typename T>
		void print(const T&, const Args &...arg) {
			auto f = [&]{ print(arg...);};
		}
		int main() {
			print(1, 2, 3);
			return 0;
		}"
		NO_GCC_VARIADIC_TEMPLATE_BUG)

	# Check for issues with older gcc compilers if "inline" aggregate initialization
	# works for array class members https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65815
	CHECK_CXX_SOURCE_COMPILES(
		"struct array {
			int data[2];
		};
		struct X {
			array a = { 1, 2 };
		};
		int main() {
			return 0;
		}"
		NO_GCC_AGGREGATE_INITIALIZATION_BUG)
	set(CMAKE_REQUIRED_FLAGS ${SAVED_CMAKE_REQUIRED_FLAGS})
else()
	set(NO_GCC_VARIADIC_TEMPLATE_BUG TRUE)
	set(NO_GCC_AGGREGATE_INITIALIZATION_BUG TRUE)
endif()

include_directories(include)

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

if(BUILD_EXAMPLES AND NO_GCC_VARIADIC_TEMPLATE_BUG)
	add_subdirectory(examples)
elseif(BUILD_EXAMPLES)
	message(WARNING "Skipping build of examples because of compiler issue")
endif()

if(NOT "${CPACK_GENERATOR}" STREQUAL "")
	include(${CMAKE_SOURCE_DIR}/cmake/packages.cmake)
endif()
