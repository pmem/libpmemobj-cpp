# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2019, Intel Corporation

cmake_minimum_required(VERSION 3.3)
project(pman CXX)

set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)

include(FindThreads)

if(NOT WIN32)
	find_package(PkgConfig QUIET)
endif()

if(PKG_CONFIG_FOUND)
	pkg_check_modules(LIBPMEMOBJ++ REQUIRED libpmemobj++)

	pkg_check_modules(CURSES REQUIRED ncurses)
else()
	find_package(LIBPMEMOBJ++ REQUIRED)

	# Specifies that we want FindCurses to find ncurses and not just any
	# curses library
	set(CURSES_NEED_NCURSES TRUE)
	find_package(Curses REQUIRED)
endif()

link_directories(${LIBPMEMOBJ++_LIBRARY_DIRS})

add_executable(pman pman.cpp)
target_include_directories(pman PUBLIC ${CURSES_INCLUDE_DIR} ${LIBPMEMOBJ++_INCLUDE_DIRS} . ..)
target_link_libraries(pman ${LIBPMEMOBJ++_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CURSES_LIBRARIES})
