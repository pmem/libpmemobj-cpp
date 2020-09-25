# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2020, Intel Corporation

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

add_custom_target(examples)

include_directories(${LIBPMEMOBJ_INCLUDE_DIRS} .)
link_directories(${LIBPMEMOBJ_LIBRARY_DIRS})

add_cppstyle(examples-common ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
add_check_whitespace(examples-common ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
add_check_whitespace(examples-cmake ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)

add_cppstyle(examples-array ${CMAKE_CURRENT_SOURCE_DIR}/array/*.*pp)
add_check_whitespace(examples-array ${CMAKE_CURRENT_SOURCE_DIR}/array/*.*)

add_cppstyle(examples-map_cli ${CMAKE_CURRENT_SOURCE_DIR}/map_cli/*.*pp)
add_check_whitespace(examples-map_cli ${CMAKE_CURRENT_SOURCE_DIR}/map_cli/*.*)

add_cppstyle(examples-panaconda ${CMAKE_CURRENT_SOURCE_DIR}/panaconda/*.*pp)
add_check_whitespace(examples-panaconda ${CMAKE_CURRENT_SOURCE_DIR}/panaconda/*.*)

add_cppstyle(examples-pman ${CMAKE_CURRENT_SOURCE_DIR}/pman/*.*pp)
add_check_whitespace(examples-pman ${CMAKE_CURRENT_SOURCE_DIR}/pman/*.*)

add_cppstyle(examples-pmpong ${CMAKE_CURRENT_SOURCE_DIR}/pmpong/*.*pp)
add_check_whitespace(examples-pmpong ${CMAKE_CURRENT_SOURCE_DIR}/pmpong/*.*)

add_cppstyle(examples-queue ${CMAKE_CURRENT_SOURCE_DIR}/queue/*.*pp)
add_check_whitespace(examples-queue ${CMAKE_CURRENT_SOURCE_DIR}/queue/*.*)

add_cppstyle(examples-simplekv ${CMAKE_CURRENT_SOURCE_DIR}/simplekv/*.*pp)
add_check_whitespace(examples-simplekv ${CMAKE_CURRENT_SOURCE_DIR}/simplekv/*.*)

add_cppstyle(examples-simplekv_rebuild ${CMAKE_CURRENT_SOURCE_DIR}/simplekv_rebuild/*.*pp)
add_check_whitespace(examples-simplekv_rebuild ${CMAKE_CURRENT_SOURCE_DIR}/simplekv_rebuild/*.*)

add_cppstyle(examples-slab ${CMAKE_CURRENT_SOURCE_DIR}/slab/*.*pp)
add_check_whitespace(examples-slab ${CMAKE_CURRENT_SOURCE_DIR}/slab/*.*)

add_cppstyle(examples-string ${CMAKE_CURRENT_SOURCE_DIR}/string/*.*pp)
add_check_whitespace(examples-string ${CMAKE_CURRENT_SOURCE_DIR}/string/*.*)

add_cppstyle(examples-v ${CMAKE_CURRENT_SOURCE_DIR}/v/*.*pp)
add_check_whitespace(examples-v ${CMAKE_CURRENT_SOURCE_DIR}/v/*.*)

add_cppstyle(examples-segment_vector ${CMAKE_CURRENT_SOURCE_DIR}/segment_vector/*.*pp)
add_check_whitespace(examples-segment_vector ${CMAKE_CURRENT_SOURCE_DIR}/segment_vector/*.*)

add_cppstyle(examples-concurrent_hash_map ${CMAKE_CURRENT_SOURCE_DIR}/concurrent_hash_map/*.*pp)
add_check_whitespace(examples-concurrent_hash_map ${CMAKE_CURRENT_SOURCE_DIR}/concurrent_hash_map/*.*)

add_cppstyle(examples-defrag ${CMAKE_CURRENT_SOURCE_DIR}/defrag/*.*pp)
add_check_whitespace(examples-defrag ${CMAKE_CURRENT_SOURCE_DIR}/defrag/*.*)

add_cppstyle(examples-pool ${CMAKE_CURRENT_SOURCE_DIR}/pool/*.*pp)
add_check_whitespace(examples-pool ${CMAKE_CURRENT_SOURCE_DIR}/pool/*.*)

add_cppstyle(examples-mutex ${CMAKE_CURRENT_SOURCE_DIR}/mutex/*.*pp)
add_check_whitespace(examples-mutex ${CMAKE_CURRENT_SOURCE_DIR}/mutex/*.*)

add_cppstyle(examples-make_persistent ${CMAKE_CURRENT_SOURCE_DIR}/make_persistent/*.*pp)
add_check_whitespace(examples-make_persistent ${CMAKE_CURRENT_SOURCE_DIR}/make_persistent/*.*)

add_cppstyle(examples-persistent ${CMAKE_CURRENT_SOURCE_DIR}/persistent/*.*pp)
add_check_whitespace(examples-persistent ${CMAKE_CURRENT_SOURCE_DIR}/persistent/*.*)

add_cppstyle(examples-transaction ${CMAKE_CURRENT_SOURCE_DIR}/transaction/*.*pp)
add_check_whitespace(examples-transaction ${CMAKE_CURRENT_SOURCE_DIR}/transaction/*.*)

add_cppstyle(examples-inline_string ${CMAKE_CURRENT_SOURCE_DIR}/inline_string/*.*pp)
add_check_whitespace(examples-inline_string ${CMAKE_CURRENT_SOURCE_DIR}/inline_string/*.*)

add_cppstyle(examples-radix_tree ${CMAKE_CURRENT_SOURCE_DIR}/radix_tree/*.*pp)
add_check_whitespace(examples-radix_tree ${CMAKE_CURRENT_SOURCE_DIR}/radix_tree/*.*)

function(add_example name)
	set(srcs ${ARGN})
	prepend(srcs ${CMAKE_CURRENT_SOURCE_DIR} ${srcs})
	add_executable(example-${name} ${srcs})
	target_link_libraries(example-${name} ${LIBPMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
	add_dependencies(examples example-${name})
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

if (TEST_ARRAY)
	add_example(queue queue/queue.cpp)
endif()

if (TEST_VECTOR)
	add_example(slab slab/slab.cpp)
	add_example(simplekv simplekv/simplekv.cpp)
	add_example(defrag defrag/defrag.cpp)
	if(NOT CLANG_DESTRUCTOR_REFERENCE_BUG_PRESENT)
		add_example(simplekv_rebuild simplekv_rebuild/simplekv_rebuild.cpp)
	else()
		message(WARNING "skipping simplekv_rebuild example - it requires clang >= ${CLANG_REQUIRED_BY_DESTRUCTOR_REFERENCE_BUG}")
	endif()
else()
	message(WARNING "skipping defrag example - it requires vector enabled")
endif()

if (TEST_STRING)
	add_example(string string/string.cpp)
endif()

if (TEST_RADIX_TREE)
	add_example(radix_tree_dot radix_tree/radix_tree_dot.cpp)
	add_example(radix_tree_basic radix_tree/radix_tree_basic.cpp)
	add_example(radix_tree_custom_key radix_tree/radix_tree_custom_key.cpp)
	add_example(radix_tree_inline_string radix_tree/radix_tree_inline_string.cpp)
	add_example(radix_tree_inline_string_uint8t_key radix_tree/radix_tree_inline_string_uint8t_key.cpp)
endif()

if(CURSES_FOUND)
	add_example(pman pman/pman.cpp)
	target_include_directories(example-pman PUBLIC ${CURSES_INCLUDE_DIR})
	target_link_libraries(example-pman ${CURSES_LIBRARIES})
else()
	message(WARNING "ncurses not found - pman won't be build")
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

if(CURSES_FOUND)
	add_example(panaconda panaconda/panaconda.cpp)
	target_include_directories(example-panaconda PUBLIC ${CURSES_INCLUDE_DIR})
	target_link_libraries(example-panaconda ${CURSES_LIBRARIES})
else()
	message(WARNING "ncurses not found - panaconda won't be build")
endif()

add_example(map_cli map_cli/map_cli.cpp)

add_example(array array/array.cpp)

add_example(v v/v.cpp)

add_example(inline_string inline_string/inline_string.cpp)

add_example(segment_vector segment_vector/segment_vector.cpp)

add_example(concurrent_hash_map concurrent_hash_map/concurrent_hash_map.cpp)

add_example(pool pool/pool.cpp)

add_example(mutex mutex/mutex.cpp)

add_example(make_persistent make_persistent/make_persistent.cpp)

add_example(persistent persistent/persistent.cpp)

add_example(transaction transaction/transaction.cpp)
