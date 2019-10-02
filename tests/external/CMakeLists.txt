#
# Copyright 2018-2019, Intel Corporation
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

include(../ctest_helpers.cmake)

function(add_test_expect_failure name)
	set(srcs ${ARGN})
	prepend(srcs ${CMAKE_CURRENT_SOURCE_DIR} ${srcs})

	add_cppstyle(tests-external-${name} ${srcs})
	add_check_whitespace(tests-external-${name} ${srcs})

	add_executable(${name}_fail_expected ${srcs})
	set_target_properties(${name}_fail_expected PROPERTIES
				EXCLUDE_FROM_ALL TRUE
				EXCLUDE_FROM_DEFAULT_BUILD TRUE)

	add_test(NAME ${name}_fail_expected
		COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${name}_fail_expected
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

	set_tests_properties(${name}_fail_expected PROPERTIES WILL_FAIL TRUE)
endfunction()

if (ENABLE_ARRAY)
	build_test(array_at libcxx/array/at.pass.cpp)
	add_test_generic(NAME array_at TRACERS none)

	build_test(array_begin libcxx/array/begin.pass.cpp)
	add_test_generic(NAME array_begin TRACERS none)

	build_test(array_compare libcxx/array/compare.pass.cpp)
	add_test_generic(NAME array_compare TRACERS none)

	build_test(array_contiguous libcxx/array/contiguous.pass.cpp)
	add_test_generic(NAME array_contiguous TRACERS none)

	build_test(array_empty libcxx/array/empty.pass.cpp)
	add_test_generic(NAME array_empty TRACERS none)

	build_test(array_front_back libcxx/array/front_back.pass.cpp)
	add_test_generic(NAME array_front_back TRACERS none)

	build_test(array_indexing libcxx/array/indexing.pass.cpp)
	add_test_generic(NAME array_indexing TRACERS none)

	build_test(array_max_size libcxx/array/max_size.pass.cpp)
	add_test_generic(NAME array_max_size TRACERS none)

	if (MAX_ALIGN_TYPE_EXISTS)
		build_test(array_size_and_alignment libcxx/array/size_and_alignment.pass.cpp)
		add_test_generic(NAME array_size_and_alignment TRACERS none)
	endif()

	build_test(array_types libcxx/array/types.pass.cpp)
	add_test_generic(NAME array_types TRACERS none)

	add_test_expect_failure(array_compare_0 libcxx/array/compare.fail_0.cpp)
	add_test_expect_failure(array_compare_1 libcxx/array/compare.fail_1.cpp)
	add_test_expect_failure(array_compare_2 libcxx/array/compare.fail_2.cpp)

	build_test(array_cons_default libcxx/array/array.cons/default.pass.cpp)
	add_test_generic(NAME array_cons_default TRACERS none)

	add_test_expect_failure(array_copy_assignment libcxx/array/array.cons/copy_assignment.fail.cpp)

	build_test(array_cons_implicit_copy libcxx/array/array.cons/implicit_copy.pass.cpp)
	add_test_generic(NAME array_cons_implicit_copy TRACERS none)

	build_test(array_initializer_list libcxx/array/array.cons/initializer_list.pass.cpp)
	add_test_generic(NAME array_initializer_list TRACERS none)

	if (MAX_ALIGN_TYPE_EXISTS)
		build_test(array_data_const libcxx/array/array.data/data_const.pass.cpp)
		add_test_generic(NAME array_data_const TRACERS none)
	endif()

	if (MAX_ALIGN_TYPE_EXISTS)
		build_test(array_data libcxx/array/array.data/data.pass.cpp)
		add_test_generic(NAME array_data TRACERS none)
	endif()

	add_test_expect_failure(array_fill libcxx/array/array.fill/fill.fail.cpp)

	build_test(array_fill libcxx/array/array.fill/fill.pass.cpp)
	add_test_generic(NAME array_fill TRACERS none)

	build_test(array_size libcxx/array/array.size/size.pass.cpp)
	add_test_generic(NAME array_size TRACERS none)

	build_test(array_swap libcxx/array/array.special/swap.pass.cpp)
	add_test_generic(NAME array_swap TRACERS none)

	add_test_expect_failure(array_swap libcxx/array/array.swap/swap.fail.cpp)

	build_test(array_swap_2 libcxx/array/array.swap/swap.pass.cpp)
	add_test_generic(NAME array_swap_2 TRACERS none)

	build_test(array_get_const_rv libcxx/array/array.tuple/get_const_rv.pass.cpp)
	add_test_generic(NAME array_get_const_rv TRACERS none)

	build_test(array_get_const libcxx/array/array.tuple/get_const.pass.cpp)
	add_test_generic(NAME array_get_const TRACERS none)

	build_test(array_get_rv libcxx/array/array.tuple/get_rv.pass.cpp)
	add_test_generic(NAME array_get_rv TRACERS none)

	build_test(array_get libcxx/array/array.tuple/get.pass.cpp)
	add_test_generic(NAME array_get TRACERS none)

	add_test_expect_failure(array_get libcxx/array/array.tuple/get.fail.cpp)

	build_test(array_iterators libcxx/array/iterators.pass.cpp)
	add_test_generic(NAME array_iterators TRACERS none)
endif()

if (ENABLE_VECTOR)
	build_test_ext(NAME vector_libcxx_assign_copy SRC_FILES libcxx/vector/vector.cons/assign_copy.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_assign_copy TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_assign_initializer_list SRC_FILES libcxx/vector/vector.cons/assign_initializer_list.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_assign_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_assign_iter_iter SRC_FILES libcxx/vector/vector.cons/assign_iter_iter.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_assign_iter_iter TRACERS none pmemcheck memcheck)

	build_test(vector_libcxx_assign_move_pass libcxx/vector/vector.cons/assign_move.pass.cpp)
	add_test_generic(NAME vector_libcxx_assign_move_pass TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_assign_size_value SRC_FILES libcxx/vector/vector.cons/assign_size_value.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_assign_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_ctor_default SRC_FILES libcxx/vector/vector.cons/construct_default.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_ctor_default TRACERS none pmemcheck memcheck)

	build_test(vector_libcxx_ctor_iter_iter libcxx/vector/vector.cons/construct_iter_iter.pass.cpp)
	add_test_generic(NAME vector_libcxx_ctor_iter_iter TRACERS none pmemcheck memcheck)

	build_test(vector_libcxx_ctor_move libcxx/vector/vector.cons/move.pass.cpp)
	add_test_generic(NAME vector_libcxx_ctor_move TRACERS none pmemcheck memcheck)

	build_test(vector_libcxx_ctor_size libcxx/vector/vector.cons/construct_size.pass.cpp)
	add_test_generic(NAME vector_libcxx_ctor_size TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_ctor_size_value SRC_FILES libcxx/vector/vector.cons/construct_size_value.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_ctor_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_copy SRC_FILES libcxx/vector/vector.cons/copy.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_copy TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_dtor_noexcept SRC_FILES libcxx/vector/vector.cons/dtor_noexcept.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_dtor_noexcept TRACERS none)

	build_test(vector_libcxx_contiguous libcxx/vector/contiguous.pass.cpp)
	add_test_generic(NAME vector_libcxx_contiguous TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_types SRC_FILES libcxx/vector/types.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_types TRACERS none)

	build_test_ext(NAME vector_libcxx_iterators SRC_FILES libcxx/vector/iterators.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_iterators TRACERS TRACERS none pmemcheck memcheck)

	build_test(vector_libcxx_data_const libcxx/vector/vector.data/data_const.pass.cpp)
	add_test_generic(NAME vector_libcxx_data_const TRACERS none pmemcheck memcheck)

	build_test(vector_libcxx_data libcxx/vector/vector.data/data.pass.cpp)
	add_test_generic(NAME vector_libcxx_data TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_initializer_list SRC_FILES libcxx/vector/vector.cons/initializer_list.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_initializer_list TRACERS none pmemcheck memcheck)
	
	build_test_ext(NAME vector_libcxx_op_equal_initializer_list SRC_FILES libcxx/vector/vector.cons/op_equal_initializer_list.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_op_equal_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_capacity SRC_FILES libcxx/vector/vector.capacity/capacity.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_capacity TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_empty SRC_FILES libcxx/vector/vector.capacity/empty.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_empty TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_max_size SRC_FILES libcxx/vector/vector.capacity/max_size.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_max_size TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_reserve SRC_FILES libcxx/vector/vector.capacity/reserve.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_reserve TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_resize_size SRC_FILES libcxx/vector/vector.capacity/resize_size.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_resize_size TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_resize_size_value SRC_FILES libcxx/vector/vector.capacity/resize_size_value.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_resize_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_shrink_to_fit SRC_FILES libcxx/vector/vector.capacity/shrink_to_fit.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_shrink_to_fit TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_size SRC_FILES libcxx/vector/vector.capacity/size.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_size TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_swap SRC_FILES libcxx/vector/vector.capacity/swap.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_swap TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_clear SRC_FILES libcxx/vector/vector.modifiers/clear.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_clear TRACERS none pmemcheck memcheck)

	build_test(vector_libcxx_emplace libcxx/vector/vector.modifiers/emplace.pass.cpp)
	add_test_generic(NAME vector_libcxx_emplace TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_emplace_back SRC_FILES libcxx/vector/vector.modifiers/emplace_back.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_emplace_back TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_emplace_extra SRC_FILES libcxx/vector/vector.modifiers/emplace_extra.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_emplace_extra TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_erase_iter SRC_FILES libcxx/vector/vector.modifiers/erase_iter.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_erase_iter TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_erase_iter_iter SRC_FILES libcxx/vector/vector.modifiers/erase_iter_iter.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_erase_iter_iter TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_insert_iter_initializer_list SRC_FILES libcxx/vector/vector.modifiers/insert_iter_initializer_list.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_insert_iter_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_insert_iter_iter_iter SRC_FILES libcxx/vector/vector.modifiers/insert_iter_iter_iter.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_insert_iter_iter_iter TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_insert_iter_rvalue SRC_FILES libcxx/vector/vector.modifiers/insert_iter_rvalue.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_insert_iter_rvalue TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_insert_iter_size_value SRC_FILES libcxx/vector/vector.modifiers/insert_iter_size_value.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_insert_iter_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_insert_iter_value SRC_FILES libcxx/vector/vector.modifiers/insert_iter_value.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_insert_iter_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_pop_back SRC_FILES libcxx/vector/vector.modifiers/pop_back.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_pop_back TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_push_back SRC_FILES libcxx/vector/vector.modifiers/push_back.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_push_back TRACERS none pmemcheck memcheck)

	# XXX: port libcxx test vector.modifiers/push_back_exception_safety.pass

	build_test(vector_libcxx_push_back_rvalue libcxx/vector/vector.modifiers/push_back_rvalue.pass.cpp)
	add_test_generic(NAME vector_libcxx_push_back_rvalue TRACERS none pmemcheck memcheck)

	build_test_ext(NAME vector_libcxx_special_swap SRC_FILES libcxx/vector/vector.special/swap.pass.cpp BUILD_OPTIONS -DVECTOR)
	add_test_generic(NAME vector_libcxx_special_swap TRACERS none pmemcheck memcheck)
endif()

if (ENABLE_STRING)
	build_test(string_libcxx_cons_initializer_list libcxx/basic_string/string.cons/initializer_list.pass.cpp)
	add_test_generic(NAME string_libcxx_cons_initializer_list TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_cons_iter_alloc libcxx/basic_string/string.cons/iter_alloc.pass.cpp)
	add_test_generic(NAME string_libcxx_cons_iter_alloc TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_cons_move libcxx/basic_string/string.cons/move.pass.cpp)
	add_test_generic(NAME string_libcxx_cons_move TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_cons_pointer_alloc libcxx/basic_string/string.cons/pointer_alloc.pass.cpp)
	add_test_generic(NAME string_libcxx_cons_pointer_alloc TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_cons_pointer_size_alloc libcxx/basic_string/string.cons/pointer_size_alloc.pass.cpp)
	add_test_generic(NAME string_libcxx_cons_pointer_size_alloc TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_cons_size_char_alloc libcxx/basic_string/string.cons/size_char_alloc.pass.cpp)
	add_test_generic(NAME string_libcxx_cons_size_char_alloc TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_cons_copy libcxx/basic_string/string.cons/copy.pass.cpp)
	add_test_generic(NAME string_libcxx_cons_copy TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_cons_substr libcxx/basic_string/string.cons/substr.pass.cpp)
	add_test_generic(NAME string_libcxx_cons_substr TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_dtor_noexcept libcxx/basic_string/string.cons/dtor_noexcept.pass.cpp)
	add_test_generic(NAME string_libcxx_dtor_noexcept TRACERS none)

	build_test(string_libcxx_ops_compare_pointer libcxx/basic_string/string.ops/string_compare/pointer.pass.cpp)
	add_test_generic(NAME string_libcxx_ops_compare_pointer TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_ops_compare_size_size_pointer_size libcxx/basic_string/string.ops/string_compare/size_size_pointer_size.pass.cpp)
	add_test_generic(NAME string_libcxx_ops_compare_size_size_pointer_size TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_ops_compare_size_size_pointer libcxx/basic_string/string.ops/string_compare/size_size_pointer.pass.cpp)
	add_test_generic(NAME string_libcxx_ops_compare_size_size_pointer TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_ops_compare_size_size_string_size_size libcxx/basic_string/string.ops/string_compare/size_size_string_size_size.pass.cpp)
	add_test_generic(NAME string_libcxx_ops_compare_size_size_string_size_size TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_ops_compare_size_size_string libcxx/basic_string/string.ops/string_compare/size_size_string.pass.cpp)
	add_test_generic(NAME string_libcxx_ops_compare_size_size_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_ops_compare_string libcxx/basic_string/string.ops/string_compare/string.pass.cpp)
	add_test_generic(NAME string_libcxx_ops_compare_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_nonmembers_opne_pointer_string libcxx/basic_string/string.nonmembers/string_opne/pointer_string.pass.cpp)
	add_test_generic(NAME string_libcxx_nonmembers_opne_pointer_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_nonmembers_opne_string_pointer libcxx/basic_string/string.nonmembers/string_opne/string_pointer.pass.cpp)
	add_test_generic(NAME string_libcxx_nonmembers_opne_string_pointer TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_nonmembers_opne_string_string libcxx/basic_string/string.nonmembers/string_opne/string_string.pass.cpp)
	add_test_generic(NAME string_libcxx_nonmembers_opne_string_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_nonmembers_opeq_pointer_string libcxx/basic_string/string.nonmembers/string_opeq/pointer_string.pass.cpp)
	add_test_generic(NAME string_libcxx_nonmembers_opeq_pointer_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_nonmembers_opeq_string_pointer libcxx/basic_string/string.nonmembers/string_opeq/string_pointer.pass.cpp)
	add_test_generic(NAME string_libcxx_nonmembers_opeq_string_pointer TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_nonmembers_opeq_string_string libcxx/basic_string/string.nonmembers/string_opeq/string_string.pass.cpp)
	add_test_generic(NAME string_libcxx_nonmembers_opeq_string_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_nonmembers_opgt_pointer_string libcxx/basic_string/string.nonmembers/string_opgt/pointer_string.pass.cpp)
	add_test_generic(NAME string_libcxx_nonmembers_opgt_pointer_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_nonmembers_opgt_string_pointer libcxx/basic_string/string.nonmembers/string_opgt/string_pointer.pass.cpp)
	add_test_generic(NAME string_libcxx_nonmembers_opgt_string_pointer TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_nonmembers_opgt_string_string libcxx/basic_string/string.nonmembers/string_opgt/string_string.pass.cpp)
	add_test_generic(NAME string_libcxx_nonmembers_opgt_string_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_nonmembers_opge_pointer_string libcxx/basic_string/string.nonmembers/string_opge/pointer_string.pass.cpp)
	add_test_generic(NAME string_libcxx_nonmembers_opge_pointer_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_nonmembers_opge_string_pointer libcxx/basic_string/string.nonmembers/string_opge/string_pointer.pass.cpp)
	add_test_generic(NAME string_libcxx_nonmembers_opge_string_pointer TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_nonmembers_opge_string_string libcxx/basic_string/string.nonmembers/string_opge/string_string.pass.cpp)
	add_test_generic(NAME string_libcxx_nonmembers_opge_string_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_nonmembers_ople_pointer_string libcxx/basic_string/string.nonmembers/string_ople/pointer_string.pass.cpp)
	add_test_generic(NAME string_libcxx_nonmembers_ople_pointer_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_nonmembers_ople_string_pointer libcxx/basic_string/string.nonmembers/string_ople/string_pointer.pass.cpp)
	add_test_generic(NAME string_libcxx_nonmembers_ople_string_pointer TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_nonmembers_ople_string_string libcxx/basic_string/string.nonmembers/string_ople/string_string.pass.cpp)
	add_test_generic(NAME string_libcxx_nonmembers_ople_string_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_nonmembers_oplt_pointer_string libcxx/basic_string/string.nonmembers/string_oplt/pointer_string.pass.cpp)
	add_test_generic(NAME string_libcxx_nonmembers_oplt_pointer_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_nonmembers_oplt_string_pointer libcxx/basic_string/string.nonmembers/string_oplt/string_pointer.pass.cpp)
	add_test_generic(NAME string_libcxx_nonmembers_oplt_string_pointer TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_nonmembers_oplt_string_string libcxx/basic_string/string.nonmembers/string_oplt/string_string.pass.cpp)
	add_test_generic(NAME string_libcxx_nonmembers_oplt_string_string TRACERS none pmemcheck memcheck)

	# XXX: port libcxx test basic_string/string.starts_with/starts_with.string_view.pass
	# XXX: port libcxx test basic_string/string.starts_with/starts_with.ptr.pass
	# XXX: port libcxx test basic_string/string.starts_with/starts_with.char.pass
	# XXX: port libcxx test basic_string/test_trai
	# XXX: port libcxx test basic_string/string.ends_with/ends_with.ptr.pass
	# XXX: port libcxx test basic_string/string.ends_with/ends_with.string_view.pass
	# XXX: port libcxx test basic_string/string.ends_with/ends_with.char.pass

	build_test(string_libcxx_access_at libcxx/basic_string/string.access/at.pass.cpp)
	add_test_generic(NAME string_libcxx_access_at TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_access_back libcxx/basic_string/string.access/back.pass.cpp)
	add_test_generic(NAME string_libcxx_access_back TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_access_front libcxx/basic_string/string.access/front.pass.cpp)
	add_test_generic(NAME string_libcxx_access_front TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_access_index libcxx/basic_string/string.access/index.pass.cpp)
	add_test_generic(NAME string_libcxx_access_index TRACERS none pmemcheck memcheck)

	# XXX: port libcxx test basic_string/string.nonmembers/string_oplt=/string_string_view.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string_oplt=/string_view_string.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string_operator==/string_string_view.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string_operator==/string_view_string.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string.special/swap.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string.special/swap_noexcept.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string.io/stream_extract.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string.io/stream_insert.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string.io/get_line.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string.io/get_line_delim_rv.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string.io/get_line_delim.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string.io/get_line_rv.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string_op+/pointer_string.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string_op+/string_pointer.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string_op+/char_string.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string_op+/string_char.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string_op+/string_string.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string_oplt/string_string_view.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string_oplt/string_view_string.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string_op!=/string_string_view.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string_op!=/string_view_string.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string_opgt/string_string_view.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string_opgt/string_view_string.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string_opgt=/string_string_view.pass
	# XXX: port libcxx test basic_string/string.nonmembers/string_opgt=/string_view_string.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.last.of/string_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.last.of/string_view_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.last.of/pointer_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.last.of/char_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.last.of/pointer_size_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.last.not.of/string_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.last.not.of/string_view_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.last.not.of/pointer_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.last.not.of/char_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.last.not.of/pointer_size_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_compare/size_size_string_view.pass
	# XXX: port libcxx test basic_string/string.ops/string_compare/size_size_T_size_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_compare/string_view.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.first.not.of/string_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.first.not.of/string_view_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.first.not.of/pointer_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.first.not.of/char_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.first.not.of/pointer_size_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_rfind/string_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_rfind/string_view_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_rfind/pointer_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_rfind/char_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_rfind/pointer_size_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.first.of/string_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.first.of/string_view_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.first.of/pointer_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.first.of/char_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find.first.of/pointer_size_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find/string_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find/string_view_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find/pointer_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find/char_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_find/pointer_size_size.pass
	# XXX: port libcxx test basic_string/string.ops/string_substr/substr.pass

	build_test(string_libcxx_ops_accessors_c_str libcxx/basic_string/string.ops/string.accessors/c_str.pass.cpp)
	add_test_generic(NAME string_libcxx_ops_accessors_c_str TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_ops_accessors_data libcxx/basic_string/string.ops/string.accessors/data.pass.cpp)
	add_test_generic(NAME string_libcxx_ops_accessors_data TRACERS none pmemcheck memcheck)

	# XXX: port libcxx test basic_string/types.pass
	# XXX: port libcxx test basic_string/traits_mismatch.fail
	# XXX: port libcxx test basic_string/string.modifiers/string_swap/swap.pass

	build_test(string_libcxx_modifiers_erase_iter libcxx/basic_string/string.modifiers/string_erase/iter.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_erase_iter TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_erase_iter_iter libcxx/basic_string/string.modifiers/string_erase/iter_iter.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_erase_iter_iter TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_erase_size_size libcxx/basic_string/string.modifiers/string_erase/size_size.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_erase_size_size TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_erase_pop_back libcxx/basic_string/string.modifiers/string_erase/pop_back.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_erase_pop_back TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_replace_iter_iter_initializer_list libcxx/basic_string/string.modifiers/string_replace/iter_iter_initializer_list.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_replace_iter_iter_initializer_list TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_replace_iter_iter_iter_iter libcxx/basic_string/string.modifiers/string_replace/iter_iter_iter_iter.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_replace_iter_iter_iter_iter TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_replace_iter_iter_pointer libcxx/basic_string/string.modifiers/string_replace/iter_iter_pointer.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_replace_iter_iter_pointer TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_replace_iter_iter_pointer_size libcxx/basic_string/string.modifiers/string_replace/iter_iter_pointer_size.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_replace_iter_iter_pointer_size TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_replace_iter_iter_size_char libcxx/basic_string/string.modifiers/string_replace/iter_iter_size_char.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_replace_iter_iter_size_char TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_replace_iter_iter_string libcxx/basic_string/string.modifiers/string_replace/iter_iter_string.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_replace_iter_iter_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_replace_size_size_pointer libcxx/basic_string/string.modifiers/string_replace/size_size_pointer.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_replace_size_size_pointer TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_replace_size_size_pointer_size libcxx/basic_string/string.modifiers/string_replace/size_size_pointer_size.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_replace_size_size_pointer_size TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_replace_size_size_size_char libcxx/basic_string/string.modifiers/string_replace/size_size_size_char.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_replace_size_size_size_char TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_replace_size_size_string libcxx/basic_string/string.modifiers/string_replace/size_size_string.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_replace_size_size_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_replace_size_size_string_size_size libcxx/basic_string/string.modifiers/string_replace/size_size_string_size_size.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_replace_size_size_string_size_size TRACERS none pmemcheck memcheck)

	# XXX: port libcxx test basic_string/string.modifiers/string_replace/size_size_string_view.pass
	# XXX: port libcxx test basic_string/string.modifiers/string_replace/size_size_T_size_size.pass
	# XXX: port libcxx test basic_string/string.modifiers/string_replace/iter_iter_string_view.pass 

	build_test(string_libcxx_modifiers_copy libcxx/basic_string/string.modifiers/string_copy/copy.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_copy TRACERS none pmemcheck memcheck)
		
	build_test(string_libcxx_modifiers_op_plus_equal_char libcxx/basic_string/string.modifiers/string_op_plus_equal/char.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_op_plus_equal_char TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_op_plus_equal_initializer_list libcxx/basic_string/string.modifiers/string_op_plus_equal/initializer_list.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_op_plus_equal_initializer_list TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_op_plus_equal_pointer libcxx/basic_string/string.modifiers/string_op_plus_equal/pointer.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_op_plus_equal_pointer TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_op_plus_equal_string libcxx/basic_string/string.modifiers/string_op_plus_equal/string.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_op_plus_equal_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_append_push_back libcxx/basic_string/string.modifiers/string_append/push_back.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_append_push_back TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_append_iterator libcxx/basic_string/string.modifiers/string_append/iterator.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_append_iterator TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_append_pointer libcxx/basic_string/string.modifiers/string_append/pointer.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_append_pointer TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_append_pointer_size libcxx/basic_string/string.modifiers/string_append/pointer_size.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_append_pointer_size TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_append_size_char libcxx/basic_string/string.modifiers/string_append/size_char.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_append_size_char TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_append_string libcxx/basic_string/string.modifiers/string_append/string.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_append_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_append_initializer_list libcxx/basic_string/string.modifiers/string_append/initializer_list.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_append_initializer_list TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_append_string_size_size libcxx/basic_string/string.modifiers/string_append/string_size_size.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_append_string_size_size TRACERS none pmemcheck memcheck)

	# XXX: port libcxx test basic_string/string.modifiers/string_append/string_view.pass
	# XXX: port libcxx test basic_string/string.modifiers/string_append/T_size_size.pass

	build_test(string_libcxx_modifiers_insert_iter_char libcxx/basic_string/string.modifiers/string_insert/iter_char.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_insert_iter_char TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_insert_iter_initializer_list libcxx/basic_string/string.modifiers/string_insert/iter_initializer_list.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_insert_iter_initializer_list TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_insert_iter_iter_iter libcxx/basic_string/string.modifiers/string_insert/iter_iter_iter.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_insert_iter_iter_iter TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_insert_iter_size_char libcxx/basic_string/string.modifiers/string_insert/iter_size_char.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_insert_iter_size_char TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_insert_size_pointer libcxx/basic_string/string.modifiers/string_insert/size_pointer.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_insert_size_pointer TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_insert_size_pointer_size libcxx/basic_string/string.modifiers/string_insert/size_pointer_size.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_insert_size_pointer_size TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_insert_size_size_char libcxx/basic_string/string.modifiers/string_insert/size_size_char.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_insert_size_size_char TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_insert_size_string libcxx/basic_string/string.modifiers/string_insert/size_string.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_insert_size_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_insert_size_string_size_size libcxx/basic_string/string.modifiers/string_insert/size_string_size_size.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_insert_size_string_size_size TRACERS none pmemcheck memcheck)

	# XXX: port libcxx test basic_string/string.modifiers/string_insert/string_view.pass
	# XXX: port libcxx test basic_string/string.modifiers/string_insert/size_T_size_size.pass

	# XXX: port libcxx test basic_string/string.modifiers/string_assign/T_size_size.pass
	# XXX: port libcxx test basic_string/string.modifiers/string_assign/string_view.pass

	build_test(string_libcxx_modifiers_assign_initializer_list libcxx/basic_string/string.modifiers/string_assign/initializer_list.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_assign_initializer_list TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_assign_iterator libcxx/basic_string/string.modifiers/string_assign/iterator.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_assign_iterator TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_assign_pointer libcxx/basic_string/string.modifiers/string_assign/pointer.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_assign_pointer TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_assign_pointer_size libcxx/basic_string/string.modifiers/string_assign/pointer_size.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_assign_pointer_size TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_assign_rv_string libcxx/basic_string/string.modifiers/string_assign/rv_string.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_assign_rv_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_assign_size_char libcxx/basic_string/string.modifiers/string_assign/size_char.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_assign_size_char TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_assign_string libcxx/basic_string/string.modifiers/string_assign/string.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_assign_string TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_modifiers_assign_string_size_size libcxx/basic_string/string.modifiers/string_assign/string_size_size.pass.cpp)
	add_test_generic(NAME string_libcxx_modifiers_assign_string_size_size TRACERS none pmemcheck memcheck)

	# XXX: port libcxx test basic_string/string.modifiers/nothing_to_do.pass
	# XXX: port libcxx test basic_string/char.bad.fail

	build_test(string_libcxx_iterators_begin libcxx/basic_string/string.iterators/begin.pass.cpp)
	add_test_generic(NAME string_libcxx_iterators_begin TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_iterators_cbegin libcxx/basic_string/string.iterators/cbegin.pass.cpp)
	add_test_generic(NAME string_libcxx_iterators_cbegin TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_iterators_cend libcxx/basic_string/string.iterators/cend.pass.cpp)
	add_test_generic(NAME string_libcxx_iterators_cend TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_iterators_crbegin libcxx/basic_string/string.iterators/crbegin.pass.cpp)
	add_test_generic(NAME string_libcxx_iterators_crbegin TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_iterators_crend libcxx/basic_string/string.iterators/crend.pass.cpp)
	add_test_generic(NAME string_libcxx_iterators_crend TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_iterators_end libcxx/basic_string/string.iterators/end.pass.cpp)
	add_test_generic(NAME string_libcxx_iterators_end TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_iterators_iterators libcxx/basic_string/string.iterators/iterators.pass.cpp)
	add_test_generic(NAME string_libcxx_iterators_iterators TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_iterators_rbegin libcxx/basic_string/string.iterators/rbegin.pass.cpp)
	add_test_generic(NAME string_libcxx_iterators_rbegin TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_iterators_rend libcxx/basic_string/string.iterators/rend.pass.cpp)
	add_test_generic(NAME string_libcxx_iterators_rend TRACERS none pmemcheck memcheck)
		
	# XXX: port libcxx test basic_string/string.require/contiguous.pass

	build_test(string_libcxx_capacity libcxx/basic_string/string.capacity/capacity.pass.cpp)
	add_test_generic(NAME string_libcxx_capacity TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_capacity_clear libcxx/basic_string/string.capacity/clear.pass.cpp)
	add_test_generic(NAME string_libcxx_capacity_clear TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_capacity_resize_size libcxx/basic_string/string.capacity/resize_size.pass.cpp)
	add_test_generic(NAME string_libcxx_capacity_resize_size TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_capacity_resize_size_char libcxx/basic_string/string.capacity/resize_size_char.pass.cpp)
	add_test_generic(NAME string_libcxx_capacity_resize_size_char TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_capacity_size libcxx/basic_string/string.capacity/size.pass.cpp)
	add_test_generic(NAME string_libcxx_capacity_size TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_capacity_empty libcxx/basic_string/string.capacity/empty.pass.cpp)
	add_test_generic(NAME string_libcxx_capacity_empty TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_capacity_length libcxx/basic_string/string.capacity/length.pass.cpp)
	add_test_generic(NAME string_libcxx_capacity_length TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_capacity_over_max_size libcxx/basic_string/string.capacity/over_max_size.pass.cpp)
	add_test_generic(NAME string_libcxx_capacity_over_max_size TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_capacity_reserve libcxx/basic_string/string.capacity/reserve.pass.cpp)
	add_test_generic(NAME string_libcxx_capacity_reserve TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_capacity_shrink_to_fit libcxx/basic_string/string.capacity/shrink_to_fit.pass.cpp)
	add_test_generic(NAME string_libcxx_capacity_shrink_to_fit TRACERS none pmemcheck memcheck)

	# XXX: port libcxx test basic_string/string.capacity/max_size.pass
	# XXX: port libcxx test basic_string/string.cons/string_view_assignment.pass
	# XXX: port libcxx test basic_string/string.cons/T_size_size.pass

	build_test(string_libcxx_brace_assignment libcxx/basic_string/string.cons/brace_assignment.pass.cpp)
	add_test_generic(NAME string_libcxx_brace_assignment TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_char_assignment libcxx/basic_string/string.cons/char_assignment.pass.cpp)
	add_test_generic(NAME string_libcxx_char_assignment TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_copy_assignment libcxx/basic_string/string.cons/copy_assignment.pass.cpp)
	add_test_generic(NAME string_libcxx_copy_assignment TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_initializer_list_assignment libcxx/basic_string/string.cons/initializer_list_assignment.pass.cpp)
	add_test_generic(NAME string_libcxx_initializer_list_assignment TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_move_assignment libcxx/basic_string/string.cons/move_assignment.pass.cpp)
	add_test_generic(NAME string_libcxx_move_assignment TRACERS none pmemcheck memcheck)

	build_test(string_libcxx_pointer_assignment libcxx/basic_string/string.cons/pointer_assignment.pass.cpp)
	add_test_generic(NAME string_libcxx_pointer_assignment TRACERS none pmemcheck memcheck)

	# XXX: port libcxx test basic_string/string.cons/string_view.pass
	# XXX: port libcxx test basic_string/string.cons/initializer_list_assignment.pass
	# XXX: port libcxx test basic_string/string.cons/string_view.fail
endif()

if(ENABLE_SEGMENT_VECTOR_ARRAY_EXPSIZE)
	build_test_ext(NAME segment_vector_array_expsize_libcxx_capacity SRC_FILES libcxx/vector/vector.capacity/capacity.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_capacity TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_empty SRC_FILES libcxx/vector/vector.capacity/empty.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_empty TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_max_size SRC_FILES libcxx/vector/vector.capacity/max_size.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_max_size TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_reserve SRC_FILES libcxx/vector/vector.capacity/reserve.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_reserve TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_resize_size SRC_FILES libcxx/vector/vector.capacity/resize_size.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_resize_size TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_resize_size_value SRC_FILES libcxx/vector/vector.capacity/resize_size_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_resize_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_shrink_to_fit SRC_FILES libcxx/vector/vector.capacity/shrink_to_fit.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_shrink_to_fit TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_size SRC_FILES libcxx/vector/vector.capacity/size.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_size TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_swap SRC_FILES libcxx/vector/vector.capacity/swap.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_swap TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_assign_copy SRC_FILES libcxx/vector/vector.cons/assign_copy.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_assign_copy TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_assign_initializer_list SRC_FILES libcxx/vector/vector.cons/assign_initializer_list.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_assign_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_assign_size_value SRC_FILES libcxx/vector/vector.cons/assign_size_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_assign_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_ctor_default SRC_FILES libcxx/vector/vector.cons/construct_default.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_ctor_default TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_ctor_size_value SRC_FILES libcxx/vector/vector.cons/construct_size_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_ctor_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_copy SRC_FILES libcxx/vector/vector.cons/copy.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_copy TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_dtor_noexcept SRC_FILES libcxx/vector/vector.cons/dtor_noexcept.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_dtor_noexcept TRACERS none)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_initializer_list SRC_FILES libcxx/vector/vector.cons/initializer_list.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_op_equal_initializer_list SRC_FILES libcxx/vector/vector.cons/op_equal_initializer_list.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_op_equal_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_clear SRC_FILES libcxx/vector/vector.modifiers/clear.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_clear TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_emplace_back SRC_FILES libcxx/vector/vector.modifiers/emplace_back.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_emplace_back TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_emplace_extra SRC_FILES libcxx/vector/vector.modifiers/emplace_extra.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_emplace_extra TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_erase_iter_iter SRC_FILES libcxx/vector/vector.modifiers/erase_iter_iter.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_erase_iter_iter TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_erase_iter SRC_FILES libcxx/vector/vector.modifiers/erase_iter.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_erase_iter TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_insert_iter_initializer_list SRC_FILES libcxx/vector/vector.modifiers/insert_iter_initializer_list.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_insert_iter_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_insert_iter_iter_iter SRC_FILES libcxx/vector/vector.modifiers/insert_iter_iter_iter.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_insert_iter_iter_iter TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_insert_iter_rvalue SRC_FILES libcxx/vector/vector.modifiers/insert_iter_rvalue.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_insert_iter_rvalue TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_insert_iter_size_value SRC_FILES libcxx/vector/vector.modifiers/insert_iter_size_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_insert_iter_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_insert_iter_value SRC_FILES libcxx/vector/vector.modifiers/insert_iter_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_insert_iter_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_pop_back SRC_FILES libcxx/vector/vector.modifiers/pop_back.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_pop_back TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_push_back SRC_FILES libcxx/vector/vector.modifiers/push_back.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_push_back TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_special_swap SRC_FILES libcxx/vector/vector.special/swap.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_special_swap TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_iterators SRC_FILES libcxx/vector/iterators.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_iterators TRACERS TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_expsize_libcxx_types SRC_FILES libcxx/vector/types.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_EXPSIZE)
	add_test_generic(NAME segment_vector_array_expsize_libcxx_types TRACERS none)
endif()

if(ENABLE_SEGMENT_VECTOR_VECTOR_EXPSIZE)
	build_test_ext(NAME segment_vector_vector_expsize_libcxx_capacity SRC_FILES libcxx/vector/vector.capacity/capacity.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_capacity TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_empty SRC_FILES libcxx/vector/vector.capacity/empty.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_empty TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_max_size SRC_FILES libcxx/vector/vector.capacity/max_size.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_max_size TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_reserve SRC_FILES libcxx/vector/vector.capacity/reserve.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_reserve TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_resize_size SRC_FILES libcxx/vector/vector.capacity/resize_size.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_resize_size TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_resize_size_value SRC_FILES libcxx/vector/vector.capacity/resize_size_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_resize_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_shrink_to_fit SRC_FILES libcxx/vector/vector.capacity/shrink_to_fit.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_shrink_to_fit TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_size SRC_FILES libcxx/vector/vector.capacity/size.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_size TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_swap SRC_FILES libcxx/vector/vector.capacity/swap.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_swap TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_assign_copy SRC_FILES libcxx/vector/vector.cons/assign_copy.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_assign_copy TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_assign_initializer_list SRC_FILES libcxx/vector/vector.cons/assign_initializer_list.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_assign_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_assign_size_value SRC_FILES libcxx/vector/vector.cons/assign_size_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_assign_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_ctor_default SRC_FILES libcxx/vector/vector.cons/construct_default.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_ctor_default TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_ctor_size_value SRC_FILES libcxx/vector/vector.cons/construct_size_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_ctor_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_copy SRC_FILES libcxx/vector/vector.cons/copy.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_copy TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_dtor_noexcept SRC_FILES libcxx/vector/vector.cons/dtor_noexcept.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_dtor_noexcept TRACERS none)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_initializer_list SRC_FILES libcxx/vector/vector.cons/initializer_list.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_op_equal_initializer_list SRC_FILES libcxx/vector/vector.cons/op_equal_initializer_list.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_op_equal_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_clear SRC_FILES libcxx/vector/vector.modifiers/clear.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_clear TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_emplace_back SRC_FILES libcxx/vector/vector.modifiers/emplace_back.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_emplace_back TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_emplace_extra SRC_FILES libcxx/vector/vector.modifiers/emplace_extra.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_emplace_extra TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_erase_iter_iter SRC_FILES libcxx/vector/vector.modifiers/erase_iter_iter.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_erase_iter_iter TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_erase_iter SRC_FILES libcxx/vector/vector.modifiers/erase_iter.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_erase_iter TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_insert_iter_initializer_list SRC_FILES libcxx/vector/vector.modifiers/insert_iter_initializer_list.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_insert_iter_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_insert_iter_iter_iter SRC_FILES libcxx/vector/vector.modifiers/insert_iter_iter_iter.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_insert_iter_iter_iter TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_insert_iter_rvalue SRC_FILES libcxx/vector/vector.modifiers/insert_iter_rvalue.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_insert_iter_rvalue TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_insert_iter_size_value SRC_FILES libcxx/vector/vector.modifiers/insert_iter_size_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_insert_iter_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_insert_iter_value SRC_FILES libcxx/vector/vector.modifiers/insert_iter_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_insert_iter_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_pop_back SRC_FILES libcxx/vector/vector.modifiers/pop_back.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_pop_back TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_push_back SRC_FILES libcxx/vector/vector.modifiers/push_back.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_push_back TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_special_swap SRC_FILES libcxx/vector/vector.special/swap.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_special_swap TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_iterators SRC_FILES libcxx/vector/iterators.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_iterators TRACERS TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_expsize_libcxx_types SRC_FILES libcxx/vector/types.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_EXPSIZE)
	add_test_generic(NAME segment_vector_vector_expsize_libcxx_types TRACERS none)
endif()

if(ENABLE_SEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_capacity SRC_FILES libcxx/vector/vector.capacity/capacity.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_capacity TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_empty SRC_FILES libcxx/vector/vector.capacity/empty.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_empty TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_max_size SRC_FILES libcxx/vector/vector.capacity/max_size.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_max_size TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_reserve SRC_FILES libcxx/vector/vector.capacity/reserve.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_reserve TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_resize_size SRC_FILES libcxx/vector/vector.capacity/resize_size.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_resize_size TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_resize_size_value SRC_FILES libcxx/vector/vector.capacity/resize_size_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_resize_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_shrink_to_fit SRC_FILES libcxx/vector/vector.capacity/shrink_to_fit.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_shrink_to_fit TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_size SRC_FILES libcxx/vector/vector.capacity/size.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_size TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_swap SRC_FILES libcxx/vector/vector.capacity/swap.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_swap TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_assign_copy SRC_FILES libcxx/vector/vector.cons/assign_copy.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_assign_copy TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_assign_initializer_list SRC_FILES libcxx/vector/vector.cons/assign_initializer_list.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_assign_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_assign_size_value SRC_FILES libcxx/vector/vector.cons/assign_size_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_assign_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_ctor_default SRC_FILES libcxx/vector/vector.cons/construct_default.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_ctor_default TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_ctor_size_value SRC_FILES libcxx/vector/vector.cons/construct_size_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_ctor_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_copy SRC_FILES libcxx/vector/vector.cons/copy.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_copy TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_dtor_noexcept SRC_FILES libcxx/vector/vector.cons/dtor_noexcept.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_dtor_noexcept TRACERS none)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_initializer_list SRC_FILES libcxx/vector/vector.cons/initializer_list.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_op_equal_initializer_list SRC_FILES libcxx/vector/vector.cons/op_equal_initializer_list.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_op_equal_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_clear SRC_FILES libcxx/vector/vector.modifiers/clear.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_clear TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_emplace_back SRC_FILES libcxx/vector/vector.modifiers/emplace_back.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_emplace_back TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_emplace_extra SRC_FILES libcxx/vector/vector.modifiers/emplace_extra.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_emplace_extra TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_erase_iter_iter SRC_FILES libcxx/vector/vector.modifiers/erase_iter_iter.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_erase_iter_iter TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_erase_iter SRC_FILES libcxx/vector/vector.modifiers/erase_iter.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_erase_iter TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_insert_iter_initializer_list SRC_FILES libcxx/vector/vector.modifiers/insert_iter_initializer_list.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_insert_iter_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_insert_iter_iter_iter SRC_FILES libcxx/vector/vector.modifiers/insert_iter_iter_iter.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_insert_iter_iter_iter TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_insert_iter_rvalue SRC_FILES libcxx/vector/vector.modifiers/insert_iter_rvalue.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_insert_iter_rvalue TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_insert_iter_size_value SRC_FILES libcxx/vector/vector.modifiers/insert_iter_size_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_insert_iter_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_insert_iter_value SRC_FILES libcxx/vector/vector.modifiers/insert_iter_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_insert_iter_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_pop_back SRC_FILES libcxx/vector/vector.modifiers/pop_back.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_pop_back TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_push_back SRC_FILES libcxx/vector/vector.modifiers/push_back.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_push_back TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_special_swap SRC_FILES libcxx/vector/vector.special/swap.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_special_swap TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_iterators SRC_FILES libcxx/vector/iterators.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_iterators TRACERS TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_array_fixedsize_libcxx_types SRC_FILES libcxx/vector/types.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_ARRAY_FIXEDSIZE)
	add_test_generic(NAME segment_vector_array_fixedsize_libcxx_types TRACERS none)
endif()

if(ENABLE_SEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_capacity SRC_FILES libcxx/vector/vector.capacity/capacity.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_capacity TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_empty SRC_FILES libcxx/vector/vector.capacity/empty.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_empty TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_max_size SRC_FILES libcxx/vector/vector.capacity/max_size.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_max_size TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_reserve SRC_FILES libcxx/vector/vector.capacity/reserve.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_reserve TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_resize_size SRC_FILES libcxx/vector/vector.capacity/resize_size.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_resize_size TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_resize_size_value SRC_FILES libcxx/vector/vector.capacity/resize_size_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_resize_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_shrink_to_fit SRC_FILES libcxx/vector/vector.capacity/shrink_to_fit.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_shrink_to_fit TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_size SRC_FILES libcxx/vector/vector.capacity/size.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_size TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_swap SRC_FILES libcxx/vector/vector.capacity/swap.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_swap TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_assign_copy SRC_FILES libcxx/vector/vector.cons/assign_copy.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_assign_copy TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_assign_initializer_list SRC_FILES libcxx/vector/vector.cons/assign_initializer_list.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_assign_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_assign_size_value SRC_FILES libcxx/vector/vector.cons/assign_size_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_assign_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_ctor_default SRC_FILES libcxx/vector/vector.cons/construct_default.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_ctor_default TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_ctor_size_value SRC_FILES libcxx/vector/vector.cons/construct_size_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_ctor_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_copy SRC_FILES libcxx/vector/vector.cons/copy.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_copy TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_dtor_noexcept SRC_FILES libcxx/vector/vector.cons/dtor_noexcept.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_dtor_noexcept TRACERS none)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_initializer_list SRC_FILES libcxx/vector/vector.cons/initializer_list.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_op_equal_initializer_list SRC_FILES libcxx/vector/vector.cons/op_equal_initializer_list.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_op_equal_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_clear SRC_FILES libcxx/vector/vector.modifiers/clear.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_clear TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_emplace_back SRC_FILES libcxx/vector/vector.modifiers/emplace_back.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_emplace_back TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_emplace_extra SRC_FILES libcxx/vector/vector.modifiers/emplace_extra.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_emplace_extra TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_erase_iter_iter SRC_FILES libcxx/vector/vector.modifiers/erase_iter_iter.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_erase_iter_iter TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_erase_iter SRC_FILES libcxx/vector/vector.modifiers/erase_iter.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_erase_iter TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_insert_iter_initializer_list SRC_FILES libcxx/vector/vector.modifiers/insert_iter_initializer_list.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_insert_iter_initializer_list TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_insert_iter_iter_iter SRC_FILES libcxx/vector/vector.modifiers/insert_iter_iter_iter.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_insert_iter_iter_iter TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_insert_iter_rvalue SRC_FILES libcxx/vector/vector.modifiers/insert_iter_rvalue.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_insert_iter_rvalue TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_insert_iter_size_value SRC_FILES libcxx/vector/vector.modifiers/insert_iter_size_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_insert_iter_size_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_insert_iter_value SRC_FILES libcxx/vector/vector.modifiers/insert_iter_value.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_insert_iter_value TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_pop_back SRC_FILES libcxx/vector/vector.modifiers/pop_back.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_pop_back TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_push_back SRC_FILES libcxx/vector/vector.modifiers/push_back.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_push_back TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_special_swap SRC_FILES libcxx/vector/vector.special/swap.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_special_swap TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_iterators SRC_FILES libcxx/vector/iterators.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_iterators TRACERS TRACERS none pmemcheck memcheck)

	build_test_ext(NAME segment_vector_vector_fixedsize_libcxx_types SRC_FILES libcxx/vector/types.pass.cpp BUILD_OPTIONS -DSEGMENT_VECTOR_VECTOR_FIXEDSIZE)
	add_test_generic(NAME segment_vector_vector_fixedsize_libcxx_types TRACERS none)
endif()
