# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation

cmake_minimum_required(VERSION 3.3)
project(transaction CXX)

set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)

include(FindThreads)

if(NOT WIN32)
	find_package(PkgConfig QUIET)
endif()

if(PKG_CONFIG_FOUND)
	pkg_check_modules(LIBPMEMOBJ++ REQUIRED libpmemobj++)
else()
	find_package(LIBPMEMOBJ++ REQUIRED)
endif()

link_directories(${LIBPMEMOBJ++_LIBRARY_DIRS})

add_executable(transaction transaction.cpp)
target_include_directories(transaction PUBLIC ${LIBPMEMOBJ++_INCLUDE_DIRS} . ..)
target_link_libraries(transaction ${LIBPMEMOBJ++_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
