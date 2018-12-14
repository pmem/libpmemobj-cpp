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

#include <algorithm>
#include <iterator>

#include <libpmemobj++/experimental/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmemobj_exp = pmem::obj::experimental;

struct TestArray;

struct root {
	pmem::obj::persistent_ptr<TestArray> test;
	pmem::obj::persistent_ptr<pmemobj_exp::array<int, 5>> arr;
};

void
assert_array_equal(const pmemobj_exp::array<int, 5> &lhs,
		   const std::array<int, 5> &rhs)
{
	UT_ASSERT(lhs == rhs);
}

void
assert_no_tx()
{
	UT_ASSERT(pmemobj_tx_stage() != TX_STAGE_WORK);
}

void
assert_tx()
{
	UT_ASSERT(pmemobj_tx_stage() == TX_STAGE_WORK);
}

template <typename T>
void
assert_pmem(const T &ptr)
{
	UT_ASSERT(pmemobj_pool_by_ptr(&ptr) != nullptr);
}

template <typename T>
void
assert_no_pmem(const T &ptr)
{
	UT_ASSERT(pmemobj_pool_by_ptr(&ptr) == nullptr);
}

/*
 * this function verifies that f() only succeeds inside transaction and when
 * obj is on pmem.
 */
template <typename T>
void
pmem_tx_only(const T &obj, std::function<void(void)> f)
{
	try {
		f();

		assert_tx();
		assert_pmem(obj);
	} catch (pmem::transaction_error &) {
		assert_no_tx();
	} catch (pmem::pool_error &) {
		assert_no_pmem(obj);
	} catch (...) {
		UT_ASSERT(0);
	}
}

/*
 * this function verifies that f() only succeeds when obj is on pmem.
 */
template <typename T>
void
pmem_only(const T &obj, std::function<void(void)> f)
{
	try {
		f();

		assert_pmem(obj);
	} catch (pmem::pool_error &) {
		assert_no_pmem(obj);
	} catch (...) {
		UT_ASSERT(0);
	}
}

struct TestArray {
	pmemobj_exp::array<int, 5> array;
};

void
test_modifiers(pmem::obj::pool<struct root> &pop,
	       pmemobj_exp::array<int, 5> &array)
{
	auto r = pop.root();

	pmem::obj::transaction::run(pop, [&] {
		r->arr = pmem::obj::make_persistent<
			pmemobj_exp::array<int, 5>>();
		r->arr->fill(1);
	});

	std::array<int, 5> std_array = {2, 2, 2, 2, 2};

	pmem_only(array, [&] {
		array.fill(10);
		assert_array_equal(array, {10, 10, 10, 10, 10});
	});

	pmem_only(array, [&] {
		array.swap(*r->arr);
		assert_array_equal(array, {1, 1, 1, 1, 1});
		assert_array_equal(*r->arr, {10, 10, 10, 10, 10});
	});

	pmem_only(array, [&] {
		array = *(r->arr);
		assert_array_equal(array, {10, 10, 10, 10, 10});
	});

	pmem_only(array, [&] {
		array = std_array;
		assert_array_equal(array, {2, 2, 2, 2, 2});
	});

	pmem_only(array, [&] {
		array = {1, 2, 3, 4, 5};
		assert_array_equal(array, {1, 2, 3, 4, 5});
	});

	pmem_only(array, [&] {
		array = std::move(*(r->arr));
		assert_array_equal(array, {10, 10, 10, 10, 10});
	});

	pmem_only(array, [&] {
		array = std::move(std_array);
		assert_array_equal(array, {2, 2, 2, 2, 2});
	});
}

/*
 * Test if access operators of array and iterators behave correctly
 * if run outside/inside tx and on/off pmem.
 */
void
test_access_operators(pmem::obj::pool<struct root> &pop,
		      pmemobj_exp::array<int, 5> &array)
{
	pmem_tx_only(array, [&] { array[2] = 2; });

	pmem_tx_only(array, [&] { array.at(2) = 2; });

	pmem_tx_only(array, [&] { array.data()[2] = 2; });

	pmem_tx_only(array, [&] { array.front() = 2; });

	pmem_tx_only(array, [&] { array.back() = 2; });

	pmem_tx_only(array, [&] {
		auto slice = array.range(0, array.size());
		(void)slice;
	});

	pmem_tx_only(array, [&] {
		auto it = array.begin();
		*it = 2;
	});

	pmem_tx_only(array, [&] {
		auto it = array.end();
		*it = 2;
	});

	pmem_tx_only(array, [&] {
		auto it = array.rbegin();
		*it = 2;
	});

	pmem_tx_only(array, [&] {
		auto it = array.rend();
		*it = 2;
	});

	try {
		auto it = array.end();
		auto it2 = array.begin();

		it++;
		it2++;

		it += 2;

		it2--;
	} catch (...) {
		UT_ASSERT(0);
	}
}

/* all of functions called here should not throw exception
 * outside tx */
void
test_notx(pmemobj_exp::array<int, 5> &array)
{
	auto const &const_array = array;

	try {
		(void)array.size();
		(void)array.max_size();
		(void)array.empty();
		(void)array.cbegin();
		(void)array.cend();
		(void)array.crbegin();
		(void)array.crend();
		(void)array.unsafe_at(2);
		(void)array.begin().unsafe_at(2);
		(void)array.const_at(2);
		(void)const_array[2];
		(void)const_array.at(2);
		(void)const_array.data();
		(void)const_array.begin();
		(void)const_array.range(0, const_array.size());
		(void)const_array.end();
		(void)const_array.front();
		(void)const_array.back();
		(void)const_array.rbegin();
		(void)const_array.rend();
	} catch (...) {
		UT_ASSERT(0);
	}
}

void
test_transactions(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->test = pmem::obj::make_persistent<TestArray>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {

	} catch (...) {
		UT_ASSERT(0);
	}

	/* on pmem, in transaction */
	pmem::obj::transaction::run(pop, [&] {
		test_access_operators(pop, r->test->array);
		test_modifiers(pop, r->test->array);
		test_notx(r->test->array);
	});

	/* on pmem, outside transaction */
	test_access_operators(pop, r->test->array);
	test_modifiers(pop, r->test->array);
	test_notx(r->test->array);

	pmemobj_exp::array<int, 5> stack_array;

	/* on stack, in transaction */
	pmem::obj::transaction::run(pop, [&] {
		test_access_operators(pop, stack_array);
		test_modifiers(pop, stack_array);
		test_notx(stack_array);
	});

	/* on stack, outside transaction */
	test_access_operators(pop, stack_array);
	test_modifiers(pop, stack_array);
	test_notx(stack_array);
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

	auto pop = pmem::obj::pool<root>::create(
		path, "ArrayTest", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	test_transactions(pop);

	pop.close();

	return 0;
}
