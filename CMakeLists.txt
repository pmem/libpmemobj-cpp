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

cmake_minimum_required(VERSION 3.3)
project(libpmemobj-cpp C CXX)

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

include(FindThreads)
include(CMakeDependentOption)
include(CheckCXXSourceCompiles)

option(BUILD_EXAMPLES "build examples" ON)

option(BUILD_TESTS "build tests" ON)

option(TRACE_TESTS
	"more verbose test outputs" OFF)

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

include(FindPerl)
if(NOT PERL_FOUND)
	message(FATAL_ERROR "Perl not found")
endif()
if (PERL_VERSION_STRING VERSION_LESS 5.16)
	message(FATAL_ERROR "Too old Perl (<5.16)")
endif()

include(CheckCXXCompilerFlag)
include(GNUInstallDirs)
find_package(PkgConfig QUIET)

include_directories(include)

set(CMAKE_CXX_STANDARD 11)

function(join SEP OUT VALUES)
	string(REPLACE ";" "${SEP}" JOIN_TMP "${VALUES}")
	set(${OUT} "${JOIN_TMP}" PARENT_SCOPE)
endfunction()

# Checks whether flag is supported by current C++ compiler and appends
# it to the relevant cmake variable.
# 1st argument is a flag
# 2nd (optional) argument is a build type (debug, release)
macro(add_flag flag)
	string(REPLACE - _ flag2 ${flag})
	string(REPLACE " " _ flag2 ${flag2})
	string(REPLACE = "_" flag2 ${flag2})
	set(check_name "CXX_HAS_${flag2}")

	check_cxx_compiler_flag(${flag} ${check_name})

	if (${${check_name}})
		if (${ARGC} EQUAL 1)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
		else()
			set(CMAKE_CXX_FLAGS_${ARGV1} "${CMAKE_CXX_FLAGS_${ARGV1}} ${flag}")
		endif()
	endif()
endmacro()

add_flag(-Wall)
add_flag(-Wpointer-arith)
add_flag(-Wunused-macros)
add_flag(-Wsign-conversion)
add_flag(-Wsign-compare)
add_flag(-Wconversion)
add_flag(-Wunreachable-code-return)
add_flag(-Wmissing-variable-declarations)
add_flag(-fno-common)

add_flag(-ggdb DEBUG)
add_flag(-DDEBUG DEBUG)

add_flag("-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2" RELEASE)

option(USE_ASAN "enable AddressSanitizer (debugging)" OFF)
option(USE_UBSAN "enable UndefinedBehaviorSanitizer (debugging)" OFF)

macro(add_sanitizer_flag flag)
	set(SAVED_CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
	set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES} -fsanitize=${flag}")

	check_cxx_compiler_flag("-fsanitize=${flag}" CXX_HAS_ASAN_UBSAN)
	if(CXX_HAS_ASAN_UBSAN)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=${flag}")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=${flag}")
	else()
		message("${flag} sanitizer not supported")
	endif()

	set(CMAKE_REQUIRED_LIBRARIES ${SAVED_CMAKE_REQUIRED_LIBRARIES})
endmacro()

if(USE_ASAN)
	add_sanitizer_flag(address)
endif()
if(USE_UBSAN)
	add_sanitizer_flag(undefined)
endif()

add_executable(check_license EXCLUDE_FROM_ALL utils/check_license/check-license.c)

# Generates cppstyle-$name and cppformat-$name targets and attaches them
# as dependencies of global "cppformat" target.
# cppstyle-$name target verifies C++ style of files in current source dir.
# cppformat-$name target reformats files in current source dir.
# If more arguments are used, then they are used as files to be checked
# instead.
# ${name} must be unique.
function(add_cppstyle name)
	if(NOT CLANG_FORMAT)
		return()
	endif()

	if(${ARGC} EQUAL 1)
		add_custom_target(cppstyle-${name}
			COMMAND ${PERL_EXECUTABLE}
				${CMAKE_SOURCE_DIR}/utils/cppstyle
				${CLANG_FORMAT}
				check
				${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
				${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
			)
		add_custom_target(cppformat-${name}
			COMMAND ${PERL_EXECUTABLE}
				${CMAKE_SOURCE_DIR}/utils/cppstyle
				${CLANG_FORMAT}
				format
				${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
				${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
			)
	else()
		add_custom_target(cppstyle-${name}
			COMMAND ${PERL_EXECUTABLE}
				${CMAKE_SOURCE_DIR}/utils/cppstyle
				${CLANG_FORMAT}
				check
				${ARGN}
			)
		add_custom_target(cppformat-${name}
			COMMAND ${PERL_EXECUTABLE}
				${CMAKE_SOURCE_DIR}/utils/cppstyle
				${CLANG_FORMAT}
				format
				${ARGN}
			)
	endif()

	add_dependencies(cppstyle cppstyle-${name})
	add_dependencies(cppformat cppformat-${name})
endfunction()

# Generates check-whitespace-$name target and attaches it as a dependency
# of global "check-whitespace" target.
# ${name} must be unique.
function(add_check_whitespace name)
	add_custom_target(check-whitespace-${name}
		COMMAND ${PERL_EXECUTABLE}
			${CMAKE_SOURCE_DIR}/utils/check_whitespace ${ARGN})

	add_dependencies(check-whitespace check-whitespace-${name})
endfunction()

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

option(DEVELOPER_MODE "enable developer checks" OFF)
if(DEVELOPER_MODE)
	# add_flag(-Werror) XXX : fix warnings
	find_program(CLANG_FORMAT NAMES clang-format-3.9 clang-format-3.8)
	# XXX : clang-format (no postfix in newer versions)
	if (NOT CLANG_FORMAT)
		message(WARNING "clang-format not found - C++ sources will not be checked (needed version: 3.8 or 3.9)")
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

if(PKG_CONFIG_FOUND)
	pkg_check_modules(PMEMOBJ REQUIRED libpmemobj>=1.4)
else()
	find_package(PMEMOBJ REQUIRED)
endif()

if(PKG_CONFIG_FOUND)
	pkg_check_modules(VALGRIND QUIET valgrind)
else()
	find_package(VALGRIND QUIET)
endif()

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

if(PKG_CONFIG_FOUND)
	pkg_check_modules(LIBUNWIND QUIET libunwind)
else()
	find_package(LIBUNWIND QUIET)
endif()
if(NOT LIBUNWIND_FOUND)
	message(WARNING "libunwind not found. Stack traces from tests will not be reliable")
endif()

set(SAVED_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})

# Check for issues with older gcc compilers which do not expand variadic template
# variables in lambda expressions.
set(CMAKE_REQUIRED_FLAGS "--std=c++11 -c")
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

set(CMAKE_REQUIRED_FLAGS ${SAVED_CMAKE_REQUIRED_FLAGS})

if(BUILD_TESTS)
	if(TEST_DIR)
		enable_testing()
	else()
		message(WARNING "TEST_DIR is empty - 'make test' will not work")
	endif()
endif()

if(BUILD_TESTS)
	add_subdirectory(tests)
endif()

# XXX : doc and examples
