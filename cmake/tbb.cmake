# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2017-2021, Intel Corporation

message(STATUS "Checking for module 'tbb'")

# TBB was found without pkg-config:
# print status message and set global variable with tbb libs.
function(handle_tbb_found_no_pkgconf)
	set(TBB_LIBRARIES ${TBB_IMPORTED_TARGETS} PARENT_SCOPE)
	message(STATUS "  Found in dir '${TBB_DIR}' using CMake's find_package (ver: ${TBB_VERSION})")
endfunction()

# Fail cmake build if TBB not found and TBB tests are enabled.
function(fail_tbb_not_found)
	message(FATAL_ERROR "TBB tests are enabled by cmake TESTS_TBB option, but Intel TBB library was not found.")
endfunction()

# CMake param TBB_DIR is priortized. This is the best to use
# while developing/testing pmemkv with custom TBB installation.
if(TBB_DIR)
	message(STATUS "  CMake param TBB_DIR is set, look ONLY in there (${TBB_DIR})!")
	find_package(TBB QUIET COMPONENTS tbb NO_DEFAULT_PATH)
	if(TBB_FOUND)
		handle_tbb_found_no_pkgconf()
	else()
		message(WARNING "TBB_DIR is set, but does not contain a path to TBB. "
			"Either set this var to a dir with TBBConfig.cmake (or tbb-config.cmake), "
			"or unset it and let CMake find TBB in the system (using e.g. pkg-config).")
		if(TESTS_TBB)
			fail_tbb_not_found()
		endif()
	endif()
else()
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(TBB QUIET tbb)
		if(TBB_FOUND)
			message(STATUS "  Found in dir '${TBB_LIBDIR}' using pkg-config (ver: ${TBB_VERSION})")
			return()
		endif()
	endif()

	# find_package (run after pkg-config) without unsetting this var is not working correctly
	unset(TBB_FOUND CACHE)

	find_package(TBB COMPONENTS tbb)
	if(TBB_FOUND)
		handle_tbb_found_no_pkgconf()
	elseif(TESTS_TBB)
		fail_tbb_not_found()
	endif()
endif()
