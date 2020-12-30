# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020-2021, Intel Corporation

cmake_minimum_required(VERSION 3.4)
project(concurrent_hash_map CXX)

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

add_executable(concurrent_hash_map concurrent_hash_map.cpp)
target_include_directories(concurrent_hash_map PUBLIC ${LIBPMEMOBJ++_INCLUDE_DIRS} . ..)
target_link_libraries(concurrent_hash_map ${LIBPMEMOBJ++_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(concurrent_hash_map_string concurrent_hash_map_string.cpp)
target_include_directories(concurrent_hash_map_string PUBLIC ${LIBPMEMOBJ++_INCLUDE_DIRS} . ..)
target_link_libraries(concurrent_hash_map_string ${LIBPMEMOBJ++_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
