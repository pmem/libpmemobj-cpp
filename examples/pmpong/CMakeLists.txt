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
project(pmpong CXX)

set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)

include(FindThreads)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(PMEMOBJ++ REQUIRED libpmemobj++)

	pkg_check_modules(SFML REQUIRED sfml-all>=2.4)
else()
	find_package(PMEMOBJ++ REQUIRED)

	# SFML 2.5 has different cmake interface than <= 2.4 so previous versions are not supported
	find_package(SFML 2.5 REQUIRED COMPONENTS graphics window system)
	set(SFML_LIBRARIES sfml-graphics sfml-window sfml-system)
endif()

link_directories(${PMEMOBJ++_LIBRARY_DIRS})

add_executable(pmpong Ball.cpp GameController.cpp GameOverView.cpp
		GameView.cpp MainGame.cpp MenuView.cpp Paddle.cpp
		PongGameStatus.cpp Pool.cpp)
target_include_directories(pmpong PUBLIC ${SFML_INCLUDE_DIR} ${PMEMOBJ++_INCLUDE_DIRS} .)
target_link_libraries(pmpong ${PMEMOBJ++_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${SFML_LIBRARIES})
