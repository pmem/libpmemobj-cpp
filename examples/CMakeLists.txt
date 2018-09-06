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

if(MSVC_VERSION)
	add_flag(-W4)
else()
	add_flag(-Wall)
endif()
add_flag(-Wpointer-arith)
add_flag(-Wsign-compare)
add_flag(-Wunreachable-code-return)
add_flag(-Wmissing-variable-declarations)
add_flag(-fno-common)
#add_flag(-Wunused-macros)
#add_flag(-Wsign-conversion)

add_flag(-ggdb DEBUG)
add_flag(-DDEBUG DEBUG)

add_flag("-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2" RELEASE)

if(USE_ASAN)
	add_sanitizer_flag(address)
endif()
if(USE_UBSAN)
	add_sanitizer_flag(undefined)
endif()

if(COVERAGE)
       set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -coverage")
endif()

include_directories(${PMEMOBJ_INCLUDE_DIRS} .)
link_directories(${PMEMOBJ_LIBRARY_DIRS})

add_cppstyle(examples-common ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
add_check_whitespace(examples-common ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
add_check_whitespace(examples-cmake ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)

function(add_example name)
	set(srcs ${ARGN})
	prepend(srcs ${CMAKE_CURRENT_SOURCE_DIR} ${srcs})
	add_executable(example-${name} ${srcs})
	add_cppstyle(examples-${name} ${srcs})
	add_check_whitespace(examples-${name} ${srcs})
endfunction()

if(PKG_CONFIG_FOUND)
	pkg_check_modules(CURSES QUIET ncurses)
else()
	# Specifies that we want FindCurses to find ncurses and not just any
	# curses library
	set(CURSES_NEED_NCURSES TRUE)
	find_package(Curses QUIET)
endif()

if(PKG_CONFIG_FOUND)
	pkg_check_modules(SFML QUIET sfml-all>=2.4)
else()
	# SFML 2.5 has different cmake interface than <= 2.4 so previous versions are not supported
	find_package(SFML 2.5 QUIET COMPONENTS graphics window system)
	set(SFML_LIBRARIES sfml-graphics sfml-window sfml-system)
endif()

add_example(queue queue/queue.cpp)
target_link_libraries(example-queue ${PMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

if(CURSES_FOUND)
	add_example(pman pman/pman.cpp)
	target_include_directories(example-pman PUBLIC ${CURSES_INCLUDE_DIR})
	target_link_libraries(example-pman ${PMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CURSES_LIBRARIES})
else()
	message(WARNING "ncurses not found - pman won't be build")
endif()

if(SFML_FOUND)
	# XXX: this can only be run in Release mode - in Debug SFML doesn't add all dependencies automatically
	add_example(pmpong pmpong/Ball.cpp pmpong/GameController.cpp pmpong/GameOverView.cpp
			pmpong/GameView.cpp pmpong/MainGame.cpp pmpong/MenuView.cpp pmpong/Paddle.cpp
			pmpong/PongGameStatus.cpp pmpong/Pool.cpp)
	target_include_directories(example-pmpong PUBLIC ${SFML_INCLUDE_DIR})
	target_link_libraries(example-pmpong ${PMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${SFML_LIBRARIES})

	if(NOT WIN32)
		execute_process(COMMAND uname -s OUTPUT_VARIABLE SYSTEM_TYPE)
		if(SYSTEM_TYPE STREQUAL "FreeBSD")
			set(FONTDIR "/usr/local/share/fonts")
		else()
			set(FONTDIR "/usr/share/fonts")
		endif()
		file(GLOB_RECURSE fonts ${FONTDIR}/*.ttf)
		list(GET fonts 0 font)
		file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/fontConf ${font})
	endif()
else()
	message(WARNING "SFML 2.4 or newer not found - pmpong won't be build")
endif()

if(CURSES_FOUND)
	add_example(panaconda panaconda/panaconda.cpp)
	target_include_directories(example-panaconda PUBLIC ${CURSES_INCLUDE_DIR})
	target_link_libraries(example-panaconda ${PMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CURSES_LIBRARIES})
else()
	message(WARNING "ncurses not found - panaconda won't be build")
endif()

add_example(map_cli map_cli/map_cli.cpp)
target_link_libraries(example-map_cli ${PMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_example(array array/array.cpp)
target_link_libraries(example-array ${PMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

if(PMEMVLT_PRESENT)
	add_library(doc_snippets_v OBJECT doc_snippets/v.cpp)
endif()
add_library(doc_snippets_persistent OBJECT doc_snippets/persistent.cpp)
add_library(doc_snippets_make_persistent OBJECT doc_snippets/make_persistent.cpp)
add_library(doc_snippets_mutex OBJECT doc_snippets/mutex.cpp)
add_library(doc_snippets_pool OBJECT doc_snippets/pool.cpp)
add_library(doc_snippets_transaction OBJECT doc_snippets/transaction.cpp)
