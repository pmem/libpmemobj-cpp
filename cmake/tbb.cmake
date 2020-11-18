# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2017-2020, Intel Corporation

message(STATUS "Checking for module 'tbb'")

if(PKG_CONFIG_FOUND)
	pkg_check_modules(TBB QUIET tbb)
endif()

# Now, if needed, try to find it without pkg-config..
if(NOT TBB_FOUND)
	# find_package without unsetting this var is not working correctly
	unset(TBB_FOUND CACHE)

	find_package(TBB COMPONENTS tbb)
	if(TBB_FOUND)
		set(TBB_LIBRARIES ${TBB_IMPORTED_TARGETS})
		message(STATUS "  Found in: ${TBB_DIR} using CMake's find_package (ver: ${TBB_VERSION})")
	elseif(TESTS_TBB)
		message(FATAL_ERROR "TBB tests are enabled by cmake TESTS_TBB option, but Intel TBB library was not found.")
	endif()
else()
	message(STATUS "  Found in: ${TBB_LIBDIR} using pkg-config (ver: ${TBB_VERSION})")
endif()
