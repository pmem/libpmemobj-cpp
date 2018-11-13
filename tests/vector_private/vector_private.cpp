/*
 * Copyright 2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "unittest.hpp"

#include "libpmemobj++/detail/common.hpp"
#include "libpmemobj++/make_persistent.hpp"
#include "libpmemobj++/make_persistent_atomic.hpp"
#include "libpmemobj++/pool.hpp"
#include "libpmemobj++/transaction.hpp"
#include <cstring>
#include <vector>

/**
 * Ugly way to test private methods, but safe since vector is header-only
 * container
 */
#define private public
#include <libpmemobj++/experimental/vector.hpp>

namespace nvobj = pmem::obj;
namespace pmem_exp = pmem::obj::experimental;
namespace test = test_common;

#define TEST_VAL_1 666
#define TEST_VAL_2 66
#define TEST_VAL_3 6
#define TEST_VAL_4 33
#define TEST_VAL_5 3

struct root {
	nvobj::persistent_ptr<pmem_exp::vector<int>> v_pptr;
};

/**
 * Test _alloc pmem::obj::experimental::vector private function.
 *
 * First case: allocate memory for TEST_VAL_1 elements of given type (int),
 * check if _capacity had changed. Expect no expception was thrown.
 *
 * Second case: allocate memory for more than max_size() elements of given type
 * (int), expect std::length_error exception is thrown.
 */
void
test_vector_private_alloc(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();
	/* first case */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v_pptr =
				nvobj::make_persistent<pmem_exp::vector<int>>();
			r->v_pptr->_alloc(TEST_VAL_1);
		});
		UT_ASSERT(r->v_pptr->_capacity == TEST_VAL_1);
		nvobj::delete_persistent_atomic<pmem_exp::vector<int>>(
			r->v_pptr);
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl
			  << std::strerror(nvobj::transaction::error())
			  << std::endl;
		UT_ASSERT(0);
	}

	/* second case */
	bool exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			r->v_pptr =
				nvobj::make_persistent<pmem_exp::vector<int>>();
			r->v_pptr->_alloc(r->v_pptr->max_size() + 1);
		});
	} catch (std::length_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}
	UT_ASSERT(exception_thrown);
}

/**
 * Test _dealloc pmem::obj::experimental::vector private function.
 *
 * First case: allocate memory for TEST_VAL_1 elements of given type (int) and
 * call _dealloc. Expect _capacity chagned to 0 and no expception was thrown.
 */
void
test_vector_private_dealloc(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();
	/* first case */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v_pptr =
				nvobj::make_persistent<pmem_exp::vector<int>>();
			r->v_pptr->_alloc(TEST_VAL_1);
			UT_ASSERT(r->v_pptr->_data != nullptr);
			r->v_pptr->_dealloc();
		});
		UT_ASSERT(r->v_pptr->_capacity == 0);
		nvobj::delete_persistent_atomic<pmem_exp::vector<int>>(
			r->v_pptr);
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl
			  << std::strerror(nvobj::transaction::error())
			  << std::endl;
		UT_ASSERT(0);
	}
}

/**
 * Test _construct_at_end pmem::obj::experimental::vector private function.
 *
 * First case: allocate memory for TEST_VAL_1 elements of given type (int) and
 * call _construct_at_end overload for count-value arguments. Check if first
 * TEST_VAL_2 elements in underlying array was constructed with TEST_VAL_3
 * value.
 *
 * Second case: using allocated memory in previous testcase, construct
 * additional TEST_VAL_4 at the end of underlying array. Note that
 * _construct_at_end function requires that memory for elements to be created
 * must be snapshotted. Comapre values in underlying array with expected values.
 *
 * Third case: using allocated memory in previous testcase call
 * _construct_at_end overload for InputIterator-InputIterator arguments. Check
 * if first TEST_VAL_2 elements in underlying array was constructed with values
 * pointed by argument's iterators.
 *
 * Fourth case: using allocated memory in previous testcase, construct
 * additional TEST_VAL_4 at the end of underlying array by calling
 * _construct_at_end overload for InputIterator-InputIterator arguments. Note
 * that _construct_at_end function requires that memory for elements to be
 * created must be snapshotted. Comapre values in underlying array with expected
 * values.
 */
void
test_vector_construct_at_end(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();
	try {
		/* first case */
		nvobj::transaction::run(pop, [&] {
			r->v_pptr =
				nvobj::make_persistent<pmem_exp::vector<int>>();
			r->v_pptr->_alloc(TEST_VAL_1);
			UT_ASSERT(r->v_pptr->_data != nullptr);
			UT_ASSERT(r->v_pptr->_capacity == TEST_VAL_1);
			r->v_pptr->_construct_at_end(TEST_VAL_2, TEST_VAL_3);
		});
		UT_ASSERT(r->v_pptr->_size == TEST_VAL_2);
		auto ptr = r->v_pptr->_data.get();
		for (unsigned i = 0; i < TEST_VAL_2; ++i)
			UT_ASSERT(*ptr++ == TEST_VAL_3);

		/* second case */
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->v_pptr->_capacity >=
				  r->v_pptr->_size + TEST_VAL_4);
			pmem::detail::conditional_add_to_tx(
				r->v_pptr->_data.get() +
					static_cast<std::ptrdiff_t>(
						r->v_pptr->_size),
				TEST_VAL_4);
			r->v_pptr->_construct_at_end(TEST_VAL_4, TEST_VAL_5);
		});
		UT_ASSERT(r->v_pptr->_size == TEST_VAL_2 + TEST_VAL_4);

		ptr = r->v_pptr->_data.get();
		unsigned j = 0;
		for (; j < TEST_VAL_2; ++j)
			UT_ASSERT(*ptr++ == TEST_VAL_3);
		for (; j < TEST_VAL_4; ++j)
			UT_ASSERT(*ptr++ == TEST_VAL_5);

		nvobj::transaction::run(pop, [&] {
			r->v_pptr->_dealloc();
			nvobj::delete_persistent<pmem_exp::vector<int>>(
				r->v_pptr);
		});

		/* third case */
		std::vector<int> v(TEST_VAL_2, TEST_VAL_3);
		v.insert(v.end(), TEST_VAL_4, TEST_VAL_5);
		auto first = test::input_it<int>(&*v.begin());
		auto middle = test::input_it<int>(&*v.begin() + TEST_VAL_2);
		auto last = test::input_it<int>(&*v.end());
		nvobj::transaction::run(pop, [&] {
			r->v_pptr =
				nvobj::make_persistent<pmem_exp::vector<int>>();
			r->v_pptr->_alloc(TEST_VAL_1);
			UT_ASSERT(r->v_pptr->_data != nullptr);
			UT_ASSERT(r->v_pptr->_capacity == TEST_VAL_1);
			r->v_pptr->_construct_at_end(first, middle);
		});
		UT_ASSERT(r->v_pptr->_size == TEST_VAL_2);
		ptr = r->v_pptr->_data.get();
		for (unsigned i = 0; i < TEST_VAL_2; ++i)
			UT_ASSERT(*ptr++ == v[i]);

		/* fourth case */
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->v_pptr->_capacity >=
				  r->v_pptr->_size + TEST_VAL_4);
			pmem::detail::conditional_add_to_tx(
				r->v_pptr->_data.get() +
					static_cast<std::ptrdiff_t>(
						r->v_pptr->_size),
				TEST_VAL_4);
			r->v_pptr->_construct_at_end(middle, last);
		});
		UT_ASSERT(r->v_pptr->_size == TEST_VAL_2 + TEST_VAL_4);

		ptr = r->v_pptr->_data.get();
		j = 0;
		for (; j < TEST_VAL_2; ++j)
			UT_ASSERT(*ptr++ == v[j]);
		for (; j < TEST_VAL_4; ++j)
			UT_ASSERT(*ptr++ == v[j]);

		nvobj::transaction::run(pop, [&] {
			r->v_pptr->_dealloc();
			nvobj::delete_persistent<pmem_exp::vector<int>>(
				r->v_pptr);
		});

	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl
			  << std::strerror(nvobj::transaction::error())
			  << std::endl;
		UT_ASSERT(0);
	}
}

/**
 * Test _destruct_at_end pmem::obj::experimental::vector private function.
 *
 * First case: allocate memory for TEST_VAL_1 elements of given type (int) and
 * fill it with non-zero value. Call _destruct_at_end with TEST_VAL_2 argument
 * and compare values in undderlying array with expected results.
 */
void
test_vector_destruct_at_end(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();
	/* first case */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v_pptr =
				nvobj::make_persistent<pmem_exp::vector<int>>();
			r->v_pptr->_alloc(TEST_VAL_1);
			UT_ASSERT(r->v_pptr->_data != nullptr);
			r->v_pptr->_construct_at_end(TEST_VAL_1, TEST_VAL_3);
			r->v_pptr->_destruct_at_end(TEST_VAL_2);
		});
		UT_ASSERT(r->v_pptr->_size == TEST_VAL_2);
		auto ptr = r->v_pptr->_data.get();
		unsigned j = 0;
		for (; j < TEST_VAL_2; ++j)
			UT_ASSERT(*ptr++ == TEST_VAL_3);
		for (; j < TEST_VAL_3; ++j)
			UT_ASSERT(*ptr++ == 0);

		nvobj::transaction::run(pop, [&] {
			r->v_pptr->_dealloc();
			nvobj::delete_persistent<pmem_exp::vector<int>>(
				r->v_pptr);
		});

	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl
			  << std::strerror(nvobj::transaction::error())
			  << std::endl;
		UT_ASSERT(0);
	}
}

int
main(int argc, char *argv[])
{
	START();
	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
		return 1;
	}
	auto path = argv[1];
	auto pop =
		nvobj::pool<root>::create(path, "VectorTest: vector_private",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	test_vector_private_alloc(pop);
	test_vector_private_dealloc(pop);
	test_vector_construct_at_end(pop);
	test_vector_destruct_at_end(pop);

	pop.close();

	return 0;
}
