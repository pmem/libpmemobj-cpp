# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2021, Intel Corporation

#
# functions.cmake - helper functions for CMakeLists.txt
#

function(join SEP OUT VALUES)
	string(REPLACE ";" "${SEP}" JOIN_TMP "${VALUES}")
	set(${OUT} "${JOIN_TMP}" PARENT_SCOPE)
endfunction()

# prepends prefix to list of strings
function(prepend var prefix)
	set(listVar "")
	foreach(f ${ARGN})
		list(APPEND listVar "${prefix}/${f}")
	endforeach(f)
	set(${var} "${listVar}" PARENT_SCOPE)
endfunction()

# Checks whether flag is supported by current C++ compiler
macro(check_flag flag OUT_NAME)
	string(REPLACE - _ flag2 ${flag})
	string(REPLACE " " _ flag2 ${flag2})
	string(REPLACE = "_" flag2 ${flag2})
	set(check_name "CXX_HAS_${flag2}")

	check_cxx_compiler_flag(${flag} ${check_name})
	set(${OUT_NAME} "${check_name}")
endmacro()

# Checks flag and appends it to the relevant cmake variable, parameters:
# 1st: a flag
# 2nd: (optional) a build type (DEBUG, RELEASE, ...), by default appends to common variable
macro(add_flag flag)
	check_flag("${flag}" check_name)

	if (${${check_name}})
		if (${ARGC} EQUAL 1)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
		else()
			string(TOUPPER "${ARGV1}" BUILD)
			set(CMAKE_CXX_FLAGS_${BUILD} "${CMAKE_CXX_FLAGS_${BUILD}} ${flag}")
		endif()
	endif()
endmacro()

# Checks flag and tries to replace found regex with the flag. If regex wasn't found,
# it adds it. In both cases, only if the flag is supported by compiler.
# Useful in Windows builds, where doubled flag produces warning. Parameters:
# 1st: a flag
# 2nd: a regex to be replaced, by the flag - NOTE: be precise, partial match is undiserable!
# 3rd: (optional) a build type (DEBUG, RELEASE, ...), by default replaces/adds in common variable
macro(replace_or_add_flag flag regex)
	check_flag("${flag}" check_name)

	if (${${check_name}})
		if (${ARGC} EQUAL 2)
			string(REGEX MATCHALL "${regex}" MATCHES "${CMAKE_CXX_FLAGS}")
			if(MATCHES)
				string(REGEX REPLACE "${regex}" "${flag}" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
			else()
				add_flag(${flag})
			endif()
		else()
			string(TOUPPER "${ARGV2}" BUILD)
			string(REGEX MATCHALL "${regex}" MATCHES "${CMAKE_CXX_FLAGS_${BUILD}}")
			if(MATCHES)
				string(REGEX REPLACE "${regex}" "${flag}" CMAKE_CXX_FLAGS_${BUILD} "${CMAKE_CXX_FLAGS_${BUILD}}")
			else()
				add_flag(${flag} ${BUILD})
			endif()
		endif()
	endif()
endmacro()

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
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cppstyle-${name}-status
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
				${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
			COMMAND ${PERL_EXECUTABLE}
				${LIBPMEMOBJCPP_ROOT_DIR}/utils/cppstyle
				${CLANG_FORMAT}
				check
				${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
				${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
			COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/cppstyle-${name}-status
			)

		add_custom_target(cppformat-${name}
			COMMAND ${PERL_EXECUTABLE}
				${LIBPMEMOBJCPP_ROOT_DIR}/utils/cppstyle
				${CLANG_FORMAT}
				format
				${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
				${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
			)
	else()
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cppstyle-${name}-status
			DEPENDS ${ARGN}
			COMMAND ${PERL_EXECUTABLE}
				${LIBPMEMOBJCPP_ROOT_DIR}/utils/cppstyle
				${CLANG_FORMAT}
				check
				${ARGN}
			COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/cppstyle-${name}-status
			)

		add_custom_target(cppformat-${name}
			COMMAND ${PERL_EXECUTABLE}
				${LIBPMEMOBJCPP_ROOT_DIR}/utils/cppstyle
				${CLANG_FORMAT}
				format
				${ARGN}
			)
	endif()

	add_custom_target(cppstyle-${name}
			DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/cppstyle-${name}-status)

	add_dependencies(cppstyle cppstyle-${name})
	add_dependencies(cppformat cppformat-${name})
endfunction()

# Generates check-whitespace-$name target and attaches it as a dependency
# of global "check-whitespace" target.
# ${name} must be unique.
function(add_check_whitespace name)
	add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/check-whitespace-${name}-status
		DEPENDS ${ARGN}
		COMMAND ${PERL_EXECUTABLE}
			${LIBPMEMOBJCPP_ROOT_DIR}/utils/check_whitespace ${ARGN}
		COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/check-whitespace-${name}-status)

	add_custom_target(check-whitespace-${name}
			DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/check-whitespace-${name}-status)
	add_dependencies(check-whitespace check-whitespace-${name})
endfunction()

# Sets ${ret} to version of program specified by ${name} in major.minor format
function(get_program_version_major_minor name ret)
	execute_process(COMMAND ${name} --version
		OUTPUT_VARIABLE cmd_ret
		ERROR_QUIET)
	STRING(REGEX MATCH "([0-9]+.)([0-9]+)" VERSION ${cmd_ret})
	SET(${ret} ${VERSION} PARENT_SCOPE)
endfunction()

function(find_pmemcheck)
	set(ENV{PATH} ${VALGRIND_PREFIX}/bin:$ENV{PATH})
	execute_process(COMMAND valgrind --tool=pmemcheck --help
			RESULT_VARIABLE VALGRIND_PMEMCHECK_RET
			OUTPUT_QUIET
			ERROR_QUIET)
	if(VALGRIND_PMEMCHECK_RET)
		set(VALGRIND_PMEMCHECK_FOUND 0 CACHE INTERNAL "")
	else()
		set(VALGRIND_PMEMCHECK_FOUND 1 CACHE INTERNAL "")
	endif()

	if(VALGRIND_PMEMCHECK_FOUND)
		execute_process(COMMAND valgrind --tool=pmemcheck true
				ERROR_VARIABLE PMEMCHECK_OUT
				OUTPUT_QUIET)

		string(REGEX MATCH ".*pmemcheck-([0-9.]*),.*" PMEMCHECK_OUT "${PMEMCHECK_OUT}")
		set(PMEMCHECK_VERSION ${CMAKE_MATCH_1} CACHE INTERNAL "")
	else()
		message(WARNING "Valgrind pmemcheck NOT found. Pmemcheck tests will not be performed.")
	endif()
endfunction()

# src version shows the current version, as reported by git describe
# unless git is not available, then it's set to the recently released VERSION
function(set_source_ver SRCVERSION)
	# if there's version file commited, use it
	if(EXISTS "${LIBPMEMOBJCPP_ROOT_DIR}/.version")
		file(STRINGS ${LIBPMEMOBJCPP_ROOT_DIR}/.version FILE_VERSION)
		set(SRCVERSION ${FILE_VERSION} PARENT_SCOPE)
		return()
	endif()

	# otherwise take it from git
	execute_process(COMMAND git describe
		OUTPUT_VARIABLE GIT_VERSION
		WORKING_DIRECTORY ${LIBPMEMOBJCPP_ROOT_DIR}
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_QUIET)
	if(GIT_VERSION)
		# 1.5-rc1-19-gb8f78a329 -> 1.5-rc1.git19.gb8f78a329
		string(REGEX MATCHALL
			"([0-9.]*)-rc([0-9]*)-([0-9]*)-([0-9a-g]*)"
			MATCHES
			${GIT_VERSION})
		if(MATCHES)
			set(SRCVERSION
				"${CMAKE_MATCH_1}-rc${CMAKE_MATCH_2}.git${CMAKE_MATCH_3}.${CMAKE_MATCH_4}"
				PARENT_SCOPE)
			return()
		endif()

		# 1.5-19-gb8f78a329 -> 1.5-git19.gb8f78a329
		string(REGEX MATCHALL
			"([0-9.]*)-([0-9]*)-([0-9a-g]*)"
			MATCHES
			${GIT_VERSION})
		if(MATCHES)
			set(SRCVERSION
				"${CMAKE_MATCH_1}-git${CMAKE_MATCH_2}.${CMAKE_MATCH_3}"
				PARENT_SCOPE)
			return()
		endif()
	else()
		execute_process(COMMAND git log -1 --format=%h
			OUTPUT_VARIABLE GIT_COMMIT
			WORKING_DIRECTORY ${LIBPMEMOBJCPP_ROOT_DIR}
			OUTPUT_STRIP_TRAILING_WHITESPACE)
		set(SRCVERSION ${GIT_COMMIT} PARENT_SCOPE)

		# CPack may complain about commit sha being a package version
		if(NOT "${CPACK_GENERATOR}" STREQUAL "")
			message(WARNING "It seems this is a shallow clone. SRCVERSION is set to: \"${GIT_COMMIT}\". "
				"CPack may complain about setting it as a package version. Unshallow this repo before making a package.")
		endif()
		return()
	endif()

	# last chance: use version set up in the top-level CMake
	set(SRCVERSION ${VERSION} PARENT_SCOPE)
endfunction()
