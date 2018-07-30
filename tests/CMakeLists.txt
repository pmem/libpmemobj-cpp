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

include(ctest_helpers.cmake)

function(build_example_queue)
	add_executable(ex-queue ../examples/queue/queue.cpp)
	target_include_directories(ex-queue PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../examples)
	target_link_libraries(ex-queue ${PMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endfunction()

function(build_example_pman)
	if(CURSES_FOUND AND NOT WIN32)
		add_executable(ex-pman ../examples/pman/pman.cpp)
		target_include_directories(ex-pman PUBLIC ${CURSES_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../examples)
		target_link_libraries(ex-pman ${PMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CURSES_LIBRARIES})
	else()
		message(WARNING "ncurses not found - pman test won't be build")
	endif()
endfunction()

if(NO_GCC_VARIADIC_TEMPLATE_BUG)
	build_example_queue()
	add_test_generic(ex-queue none)

	build_example_pman()
	add_test_generic(ex-pman none)
else()
	message(WARNING "Skipping examples tests because of gcc variadic template bug")
	skip_test("examples_tests" "SKIPPED_BECAUSE_OF_GCC_VARIADIC_TEMPLATE_BUG")
endif()

build_test(allocator allocator/allocator.cpp)
add_test_generic(allocator memcheck)
add_test_generic(allocator none)
add_test_generic(allocator pmemcheck)

build_test(make_persistent make_persistent/make_persistent.cpp)
add_test_generic(make_persistent none)
add_test_generic(make_persistent pmemcheck)

build_test(make_persistent_atomic make_persistent_atomic/make_persistent_atomic.cpp)
add_test_generic(make_persistent_atomic none)
add_test_generic(make_persistent_atomic pmemcheck)

build_test(mutex_posix mutex_posix/mutex_posix.cpp)
add_test_generic(mutex_posix drd)
add_test_generic(mutex_posix helgrind)
add_test_generic(mutex_posix pmemcheck)

build_test(pool pool/pool.cpp)
add_test_generic(pool none 0)
add_test_generic(pool none 1)
add_test_generic(pool none 2)
add_test_generic(pool none 3)

build_test(pool_primitives pool_primitives/pool_primitives.cpp)
add_test_generic(pool_primitives none)
add_test_generic(pool_primitives pmemcheck)

build_test(ptr ptr/ptr.cpp)
add_test_generic(ptr none)
add_test_generic(ptr pmemcheck)

build_test(ptr_arith ptr_arith/ptr_arith.cpp)
add_test_generic(ptr_arith memcheck)
add_test_generic(ptr_arith pmemcheck)
# XXX Bug: incompatibility between asan and custom library
if (NOT USE_ASAN)
	add_test_generic(ptr_arith none)
endif()

build_test(p_ext p_ext/p_ext.cpp)
add_test_generic(p_ext none)
add_test_generic(p_ext pmemcheck)

build_test(shared_mutex_posix shared_mutex_posix/shared_mutex_posix.cpp)
add_test_generic(shared_mutex_posix drd)
add_test_generic(shared_mutex_posix helgrind)
add_test_generic(shared_mutex_posix pmemcheck)

build_test(transaction transaction/transaction.cpp)
add_test_generic(transaction none)
add_test_generic(transaction pmemcheck)

if(WIN32)
	build_test(pool_win pool_win/pool_win.cpp)
	add_test_generic(pool_win none 0)
	add_test_generic(pool_win none 1)
	add_test_generic(pool_win none 2)
	add_test_generic(pool_win none 3)
endif()

if(PMEMVLT_PRESENT)
	build_test(v v/v.cpp)
	add_test_generic(v none)
else()
	message(WARNING "Skipping v test because no pmemvlt support found")
	skip_test("v" "SKIPPED_BECAUSE_OF_MISSING_PMEMVLT")
endif()

if(NO_CHRONO_BUG)
	build_test(cond_var cond_var/cond_var.cpp)
	add_test_generic(cond_var none)

	build_test(cond_var_posix cond_var_posix/cond_var_posix.cpp)
	add_test_generic(cond_var_posix drd)
	add_test_generic(cond_var_posix helgrind)
	add_test_generic(cond_var_posix pmemcheck)

	build_test(mutex mutex/mutex.cpp)
	add_test_generic(mutex none)

	build_test(shared_mutex shared_mutex/shared_mutex.cpp)
	add_test_generic(shared_mutex none)

	build_test(timed_mtx_posix timed_mtx_posix/timed_mtx_posix.cpp)
	add_test_generic(timed_mtx_posix drd)
	add_test_generic(timed_mtx_posix helgrind)
	add_test_generic(timed_mtx_posix pmemcheck)

	build_test(timed_mtx timed_mtx/timed_mtx.cpp)
	add_test_generic(timed_mtx none)
else()
	message(WARNING "Skipping chrono tests because of compiler/stdc++ issues")
	skip_test("chrono_tests" "SKIPPED_BECAUSE_OF_COMPILER_CHRONO_BUG")
endif()

if(NO_CLANG_TEMPLATE_BUG)
	build_test(make_persistent_array make_persistent_array/make_persistent_array.cpp)
	add_test_generic(make_persistent_array none)
	add_test_generic(make_persistent_array pmemcheck)

	build_test(make_persistent_array_atomic make_persistent_array_atomic/make_persistent_array_atomic.cpp)
	add_test_generic(make_persistent_array_atomic none)
	add_test_generic(make_persistent_array_atomic pmemcheck)
else()
	message(WARNING "Skipping array tests because of clang template bug")
	skip_test("make_persistent_array" "SKIPPED_BECAUSE_OF_CLANG_TEMPLATE_BUG")
endif()

if(USE_LLVM_LIBCPP OR USE_CUSTOM_GCC)
	build_test(deque deque/deque.cpp)
	add_test_generic(deque none)
	add_test_generic(deque pmemcheck)

	build_test(list list/list.cpp)
	add_test_generic(list none)
	add_test_generic(list pmemcheck)

	build_test(priority_queue priority_queue/priority_queue.cpp)
	add_test_generic(priority_queue none)
	add_test_generic(priority_queue pmemcheck)

	build_test(map map/map.cpp)
	add_test_generic(map none)
	add_test_generic(map pmemcheck)

	build_test(multimap multimap/multimap.cpp)
	add_test_generic(multimap none)
	add_test_generic(multimap pmemcheck)

	build_test(multiset multiset/multiset.cpp)
	add_test_generic(multiset none)
	add_test_generic(multiset pmemcheck)

	build_test(queue queue/queue.cpp)
	add_test_generic(queue none)
	add_test_generic(queue pmemcheck)

	build_test(set set/set.cpp)
	add_test_generic(set none)
	add_test_generic(set pmemcheck)

	build_test(stack stack/stack.cpp)
	add_test_generic(stack none)
	add_test_generic(stack pmemcheck)

	build_test(unordered_map unordered_map/unordered_map.cpp)
	add_test_generic(unordered_map none)
	add_test_generic(unordered_map pmemcheck)

	build_test(unordered_multimap unordered_multimap/unordered_multimap.cpp)
	add_test_generic(unordered_multimap none)
	add_test_generic(unordered_multimap pmemcheck)

	build_test(unordered_multiset unordered_multiset/unordered_multiset.cpp)
	add_test_generic(unordered_multiset none)
	add_test_generic(unordered_multiset pmemcheck)

	build_test(unordered_set unordered_set/unordered_set.cpp)
	add_test_generic(unordered_set none)
	add_test_generic(unordered_set pmemcheck)

	build_test(vector vector/vector.cpp)
	add_test_generic(vector none)
	add_test_generic(vector pmemcheck)
elseif(NOT WIN32)
	message(WARNING "Skipping container tests because custom standard library was not provided")
	skip_test("containers" "SKIPPED_BECAUSE_OF_NO_CUSTOM_STL")
endif()

if(TBB_FOUND)
	build_test(persistent_concurrent_hash_map persistent_concurrent_hash_map/persistent_concurrent_hash_map.cpp)
	add_test_generic(persistent_concurrent_hash_map none)
else()
	message(WARNING "Skipping persistent_concurrent_hash_map test because Intel TBB library not found")
endif()
