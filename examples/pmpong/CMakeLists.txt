# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2020, Intel Corporation

cmake_minimum_required(VERSION 3.4)
project(pmpong CXX)

set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)

include(FindThreads)

if(NOT WIN32)
	find_package(PkgConfig QUIET)
endif()

if(PKG_CONFIG_FOUND)
	pkg_check_modules(LIBPMEMOBJ++ REQUIRED libpmemobj++)

	pkg_check_modules(SFML REQUIRED sfml-all>=2.4)
else()
	find_package(LIBPMEMOBJ++ REQUIRED)

	# SFML 2.5 has different cmake interface than <= 2.4 so previous versions are not supported
	find_package(SFML 2.5 REQUIRED COMPONENTS graphics window system)
	set(SFML_LIBRARIES sfml-graphics sfml-window sfml-system)
endif()

link_directories(${LIBPMEMOBJ++_LIBRARY_DIRS})

add_executable(pmpong Ball.cpp GameController.cpp GameOverView.cpp
		GameView.cpp MainGame.cpp MenuView.cpp Paddle.cpp
		PongGameStatus.cpp Pool.cpp)
target_include_directories(pmpong PUBLIC ${SFML_INCLUDE_DIR} ${LIBPMEMOBJ++_INCLUDE_DIRS} .)
target_link_libraries(pmpong ${LIBPMEMOBJ++_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${SFML_LIBRARIES})

if(NOT WIN32)
	find_program(FCLIST NAMES fc-list)
	if(NOT FCLIST)
		message(WARNING "fc-list not found. Install fontconfig to allow examples-pmpong to automatically find fonts.")
	endif()

	execute_process(COMMAND bash -c "fc-list --format='%{file}\n' | head -n1 | tr -d '\n'" OUTPUT_VARIABLE FONT_PATH ERROR_QUIET)
	set(font ${FONT_PATH})
else()
	set(font "C:/Windows/Fonts/Arial.ttf")
endif()

target_compile_options(pmpong PUBLIC -DLIBPMEMOBJ_CPP_PMPONG_FONT_PATH="${font}")
