# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2021, Intel Corporation

#
# examples/CMakeLists.txt - CMake file for building all examples along with
#	the current libpmemobj-cpp sources, for verification purposes.
#	To build an example as a standalone application (with libpmemobj-cpp
#	from the system) see a CMake file in any of the subdirectories.
#
add_custom_target(examples)

# ----------------------------------------------------------------- #
## Setup examples
# ----------------------------------------------------------------- #
if(MSVC_VERSION)
	add_flag(-W2)
	add_flag("-D_FORTIFY_SOURCE=2" RELEASE)
else()
	add_flag(-Wall)
	add_flag("-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2" RELEASE)
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

if(USE_ASAN)
	add_sanitizer_flag(address)
endif()
if(USE_UBSAN)
	add_sanitizer_flag(undefined)
endif()

if(COVERAGE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -coverage")
endif()

include_directories(${LIBPMEMOBJ_INCLUDE_DIRS} .)
link_directories(${LIBPMEMOBJ_LIBRARY_DIRS})

# Add developer checks
add_cppstyle(examples ${CMAKE_CURRENT_SOURCE_DIR}/*/*.*pp)
add_check_whitespace(examples ${CMAKE_CURRENT_SOURCE_DIR}/*.*)

add_cppstyle(examples-common ${CMAKE_CURRENT_SOURCE_DIR}/*.*pp)
add_check_whitespace(examples-common ${CMAKE_CURRENT_SOURCE_DIR}/*.*)

# Find required packages for examples
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

function(add_example name)
	set(srcs ${ARGN})
	prepend(srcs ${CMAKE_CURRENT_SOURCE_DIR} ${srcs})
	add_executable(example-${name} ${srcs})
	target_link_libraries(example-${name} ${LIBPMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
	add_dependencies(examples example-${name})
endfunction()

# ----------------------------------------------------------------- #
## Add examples (some under the 'if', required for specific example)
# ----------------------------------------------------------------- #
if(TEST_ARRAY)
	add_example(queue queue/queue.cpp)
endif()

if(TEST_VECTOR)
	add_example(slab slab/slab.cpp)
	add_example(simplekv simplekv/simplekv.cpp)
	add_example(defrag defrag/defrag.cpp)
	if(NOT CLANG_DESTRUCTOR_REFERENCE_BUG_PRESENT)
		add_example(simplekv_rebuild simplekv_rebuild/simplekv_rebuild.cpp)
		add_example(simplekv_rebuild_string simplekv_rebuild/simplekv_rebuild_string.cpp)
	else()
		message(WARNING "skipping simplekv_rebuild example - it requires clang >= ${CLANG_REQUIRED_BY_DESTRUCTOR_REFERENCE_BUG}")
	endif()
else()
	message(WARNING "skipping defrag example - it requires vector enabled")
endif()

if(TEST_STRING)
	add_example(string string/string.cpp)
endif()

if(TEST_RADIX_TREE)
	add_example(radix_tree_dot radix_tree/radix_tree_dot.cpp)
	add_example(radix_tree_basic radix_tree/radix_tree_basic.cpp)
	add_example(radix_tree_complex_value radix_tree/radix_tree_complex_value.cpp)
	add_example(radix_tree_custom_key radix_tree/radix_tree_custom_key.cpp)
	add_example(radix_tree_inline_string radix_tree/radix_tree_inline_string.cpp)
	add_example(radix_tree_inline_string_uint8t_key radix_tree/radix_tree_inline_string_uint8t_key.cpp)
endif()

if(CURSES_FOUND)
	add_example(pman pman/pman.cpp)
	target_include_directories(example-pman PUBLIC ${CURSES_INCLUDE_DIR})
	target_link_libraries(example-pman ${CURSES_LIBRARIES})

	add_example(panaconda panaconda/panaconda.cpp)
	target_include_directories(example-panaconda PUBLIC ${CURSES_INCLUDE_DIR})
	target_link_libraries(example-panaconda ${CURSES_LIBRARIES})
elseif(NOT WIN32)
	message(WARNING "ncurses not found - pman and panaconda examples won't be build")
endif()

if(SFML_FOUND)
	# XXX: this can only be run in Release mode - in Debug SFML doesn't add all dependencies automatically
	add_example(pmpong pmpong/Ball.cpp pmpong/GameController.cpp pmpong/GameOverView.cpp
			pmpong/GameView.cpp pmpong/MainGame.cpp pmpong/MenuView.cpp pmpong/Paddle.cpp
			pmpong/PongGameStatus.cpp pmpong/Pool.cpp)
	target_include_directories(example-pmpong PUBLIC ${SFML_INCLUDE_DIR})
	target_link_libraries(example-pmpong ${SFML_LIBRARIES})

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

	target_compile_options(example-pmpong PUBLIC -DLIBPMEMOBJ_CPP_PMPONG_FONT_PATH="${font}")
else()
	message(WARNING "SFML 2.4 or newer not found - pmpong won't be build")
endif()

add_example(map_cli map_cli/map_cli.cpp)

add_example(array array/array.cpp)

add_example(v v/v.cpp)

add_example(inline_string inline_string/inline_string.cpp)

add_example(segment_vector segment_vector/segment_vector.cpp)

add_example(concurrent_hash_map concurrent_hash_map/concurrent_hash_map.cpp)

add_example(concurrent_hash_map_string concurrent_hash_map/concurrent_hash_map_string.cpp)

add_example(pool pool/pool.cpp)

add_example(pool_as_class_member pool/pool_as_class_member.cpp)

add_example(mutex mutex/mutex.cpp)

add_example(make_persistent make_persistent/make_persistent.cpp)

add_example(persistent persistent/persistent.cpp)

add_example(transaction transaction/transaction.cpp)
