# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020-2021, Intel Corporation

cmake_minimum_required(VERSION 3.3)
project(radix_tree CXX)

set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(LIBPMEMOBJ++ REQUIRED libpmemobj++)
else()
	find_package(LIBPMEMOBJ++ REQUIRED)
endif()

link_directories(${LIBPMEMOBJ++_LIBRARY_DIRS})

add_executable(radix_tree_dot radix_tree_dot.cpp)
target_include_directories(radix_tree_dot PUBLIC ${LIBPMEMOBJ++_INCLUDE_DIRS} . ..)
target_link_libraries(radix_tree_dot ${LIBPMEMOBJ++_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(radix_tree_basic radix_tree_basic.cpp)
target_include_directories(radix_tree_basic PUBLIC ${LIBPMEMOBJ++_INCLUDE_DIRS} . ..)
target_link_libraries(radix_tree_basic ${LIBPMEMOBJ++_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(radix_tree_custom_key radix_tree_custom_key.cpp)
target_include_directories(radix_tree_custom_key PUBLIC ${LIBPMEMOBJ++_INCLUDE_DIRS} . ..)
target_link_libraries(radix_tree_custom_key ${LIBPMEMOBJ++_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(radix_tree_inline_string radix_tree_inline_string.cpp)
target_include_directories(radix_tree_inline_string PUBLIC ${LIBPMEMOBJ++_INCLUDE_DIRS} . ..)
target_link_libraries(radix_tree_inline_string ${LIBPMEMOBJ++_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(radix_tree_inline_string_uint8t_key radix_tree_inline_string_uint8t_key.cpp)
target_include_directories(radix_tree_inline_string_uint8t_key PUBLIC ${LIBPMEMOBJ++_INCLUDE_DIRS} . ..)
target_link_libraries(radix_tree_inline_string_uint8t_key ${LIBPMEMOBJ++_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(radix_tree_complex_value radix_tree_complex_value.cpp)
target_include_directories(radix_tree_complex_value PUBLIC ${LIBPMEMOBJ++_INCLUDE_DIRS} . ..)
target_link_libraries(radix_tree_complex_value ${LIBPMEMOBJ++_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
