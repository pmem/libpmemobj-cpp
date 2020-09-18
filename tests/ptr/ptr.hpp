// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2015-2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/experimental/self_relative_ptr.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/persistent_ptr_base.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <sstream>

namespace nvobj = pmem::obj;
namespace nvobjexp = nvobj::experimental;

template <template <typename U> class pointer>
void
assert_if_oid_is_null(pointer<int> &f)
{
	UT_ASSERT(OID_IS_NULL(f.raw()));
}

template <>
void
assert_if_oid_is_null(nvobjexp::self_relative_ptr<int> &f)
{
}

/*
 * test_null_ptr -- verifies if the pointer correctly behaves like a
 * nullptr-value
 */
template <template <typename U> class pointer>
void
test_null_ptr(pointer<int> &f)
{
	assert_if_oid_is_null(f);
	UT_ASSERT((bool)f == false);
	UT_ASSERT(!f);
	UT_ASSERTeq(f.get(), nullptr);
	UT_ASSERT(f == nullptr);
}

/*
 * get_temp -- returns a temporary persistent_ptr
 */
template <template <typename U> class pointer>
pointer<int>
get_temp()
{
	pointer<int> int_null = nullptr;

	return int_null;
}

/*
 * test_ptr_operators_null -- verifies various operations on nullptr pointers
 */
template <template <typename U> class pointer>
void
test_ptr_operators_null()
{
	pointer<int> int_default_null;
	test_null_ptr(int_default_null);

	pointer<int> int_explicit_ptr_null = nullptr;
	test_null_ptr(int_explicit_ptr_null);

	pointer<int> int_explicit_oid_null = OID_NULL;
	test_null_ptr(int_explicit_oid_null);

	pointer<int> int_base = nullptr;
	pointer<int> int_same = int_base;
	int_same = int_base;
	test_null_ptr(int_same);

	swap(int_base, int_same);

	auto temp_ptr = get_temp<pointer>();
	test_null_ptr(temp_ptr);
}

constexpr int TEST_INT = 10;
constexpr int TEST_ARR_SIZE = 10;
constexpr char TEST_CHAR = 'a';

struct foo {
	nvobj::p<int> bar;
	nvobj::p<char> arr[TEST_ARR_SIZE];
};

template <template <typename U> class pointer>
struct nested {
	pointer<foo> inner;
};

template <template <typename U> class pointer, class pointer_base>
struct templated_root {
	pointer<foo> pfoo;
	pointer<nvobj::p<int>[(long unsigned) TEST_ARR_SIZE]> parr;
	pointer_base arr[3];

	/* This variable is unused, but it's here to check if the persistent_ptr
	 * does not violate its own restrictions.
	 */
	pointer<nested<pointer>> outer;
};

bool
base_is_null(const nvobjexp::self_relative_ptr_base &ptr)
{
	return ptr.is_null();
}

bool
base_is_null(const nvobj::persistent_ptr_base &ptr)
{
	auto pmemoid = ptr.raw();
	return pmemoid.off == 0 && pmemoid.pool_uuid_lo == 0;
}

/*
 * test_root_pointer -- verifies pointers at the root are nullptr
 */
template <template <typename U> class pointer, class pointer_base>
void
test_root_pointers(templated_root<pointer, pointer_base> &root)
{
	UT_ASSERT(root.pfoo == nullptr);
	UT_ASSERT(root.parr == nullptr);
	for (auto it = std::begin(root.arr); it != std::end(root.arr); ++it) {
		UT_ASSERT(base_is_null(*it));
	}
}

/*
 * test_ptr_array -- verifies the array specialization behavior
 */
template <template <typename U> class pointer, class pointer_base>
void
test_ptr_array(nvobj::pool<templated_root<pointer, pointer_base>> &pop)
{
	pointer<nvobj::p<int>[]> parr_vsize;

	try {
		nvobj::persistent_ptr<nvobj::p<int>[]> local_ptr;
		nvobj::make_persistent_atomic<nvobj::p<int>[]>(pop, local_ptr,
							       TEST_ARR_SIZE);
		parr_vsize = local_ptr;
	} catch (...) {
		UT_ASSERT(0);
	}

	{
		nvobj::transaction::manual tx(pop);

		for (int i = 0; i < TEST_ARR_SIZE; ++i)
			parr_vsize[i] = i;
		nvobj::transaction::commit();
	}

	for (int i = 0; i < TEST_ARR_SIZE; ++i)
		UT_ASSERTeq(parr_vsize[i], i);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->parr = pmemobj_tx_zalloc(sizeof(int) * TEST_ARR_SIZE,
						    0);
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(r->parr != nullptr);

	bool exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			for (int i = 0; i < TEST_ARR_SIZE; ++i)
				r->parr[i] = TEST_INT;

			nvobj::transaction::abort(-1);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(exception_thrown);

	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			for (int i = 0; i < TEST_ARR_SIZE; ++i)
				r->parr[i] = TEST_INT;

			nvobj::transaction::abort(-1);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(exception_thrown);

	for (int i = 0; i < TEST_ARR_SIZE; ++i)
		UT_ASSERTeq(r->parr[i], 0);
}

/*
 * test_ptr_transactional -- verifies the persistent ptr with the tx C API
 */
template <template <typename U> class pointer, class pointer_base>
void
test_ptr_transactional(nvobj::pool<templated_root<pointer, pointer_base>> &pop)
{
	auto r = pop.root();
	pointer<foo> to_swap;
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->pfoo == nullptr);

			r->pfoo = nvobj::make_persistent<foo>();

			/* allocate for future swap test */
			to_swap = nvobj::make_persistent<foo>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	auto pfoo = r->pfoo;

	try {
		nvobj::transaction::run(pop, [&] {
			pfoo->bar = TEST_INT;
			/* raw memory access requires extra care */
			pmem::detail::conditional_add_to_tx(&pfoo->arr);
			memset(&pfoo->arr, TEST_CHAR, sizeof(pfoo->arr));

			/* do the swap test */
			pointer<foo> foo_ptr = pfoo;
			pointer<foo> swap_ptr = to_swap;

			std::stringstream stream;
			stream << "Before swap: " << r->pfoo << " " << to_swap
			       << std::endl;

			to_swap.swap(r->pfoo);

			stream << "After swap: " << r->pfoo << " " << to_swap
			       << std::endl;
			UT_OUT("%s\n", stream.str().c_str());

			UT_ASSERT(to_swap == foo_ptr);
			UT_ASSERT(r->pfoo == swap_ptr);

			swap(r->pfoo, to_swap);
			UT_ASSERT(to_swap == swap_ptr);
			UT_ASSERT(r->pfoo == foo_ptr);

			nvobj::delete_persistent<foo>(to_swap);
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(pfoo->bar, TEST_INT);
	for (auto c : pfoo->arr) {
		UT_ASSERTeq(c, TEST_CHAR);
	}

	bool exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			pfoo->bar = 0;
			nvobj::transaction::abort(-1);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(exception_thrown);
	UT_ASSERTeq(pfoo->bar, TEST_INT);

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<foo>(r->pfoo);
			r->pfoo = nullptr;
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(r->pfoo == nullptr);
	UT_ASSERT(pfoo != nullptr);
}
