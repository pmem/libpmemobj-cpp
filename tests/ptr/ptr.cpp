// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2015-2020, Intel Corporation */

/*
 * obj_cpp_ptr.c -- cpp bindings test
 *
 */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/persistent_ptr_base.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#define LAYOUT "cpp"

namespace nvobj = pmem::obj;

namespace
{

/*
 * test_null_ptr -- verifies if the pointer correctly behaves like a
 * nullptr-value
 */
void
test_null_ptr(nvobj::persistent_ptr<int> &f)
{
	UT_ASSERT(OID_IS_NULL(f.raw()));
	UT_ASSERT((bool)f == false);
	UT_ASSERT(!f);
	UT_ASSERTeq(f.get(), nullptr);
	UT_ASSERT(f == nullptr);
}

/*
 * get_temp -- returns a temporary persistent_ptr
 */
nvobj::persistent_ptr<int>
get_temp()
{
	nvobj::persistent_ptr<int> int_null = nullptr;

	return int_null;
}

/*
 * test_ptr_operators_null -- verifies various operations on nullptr pointers
 */
void
test_ptr_operators_null()
{
	nvobj::persistent_ptr<int> int_default_null;
	test_null_ptr(int_default_null);

	nvobj::persistent_ptr<int> int_explicit_ptr_null = nullptr;
	test_null_ptr(int_explicit_ptr_null);

	nvobj::persistent_ptr<int> int_explicit_oid_null = OID_NULL;
	test_null_ptr(int_explicit_oid_null);

	nvobj::persistent_ptr<int> int_base = nullptr;
	nvobj::persistent_ptr<int> int_same = int_base;
	int_same = int_base;
	test_null_ptr(int_same);

	swap(int_base, int_same);

	auto temp_ptr = get_temp();
	test_null_ptr(temp_ptr);
}

const int TEST_INT = 10;
const int TEST_ARR_SIZE = 10;
const char TEST_CHAR = 'a';

struct foo {
	nvobj::p<int> bar;
	nvobj::p<char> arr[TEST_ARR_SIZE];
};

struct nested {
	nvobj::persistent_ptr<foo> inner;
};

struct root {
	nvobj::persistent_ptr<foo> pfoo;
	nvobj::persistent_ptr<nvobj::p<int>[TEST_ARR_SIZE]> parr;
	nvobj::persistent_ptr_base arr[3];

	/* This variable is unused, but it's here to check if the persistent_ptr
	 * does not violate its own restrictions.
	 */
	nvobj::persistent_ptr<nested> outer;
};

/*
 * test_ptr_atomic -- verifies the persistent ptr with the atomic C API
 */
void
test_ptr_atomic(nvobj::pool<root> &pop)
{
	nvobj::persistent_ptr<foo> pfoo;

	try {
		nvobj::make_persistent_atomic<foo>(pop, pfoo);
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTne(pfoo.get(), nullptr);

	(*pfoo).bar = TEST_INT;
	pop.persist(&pfoo->bar, sizeof(pfoo->bar));
	pop.memset_persist(pfoo->arr, TEST_CHAR, sizeof(pfoo->arr));

	for (auto c : pfoo->arr) {
		UT_ASSERTeq(c, TEST_CHAR);
	}

	try {
		nvobj::delete_persistent_atomic<foo>(pfoo);
		pfoo = nullptr;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(pfoo.get(), nullptr);
}

/*
 * test_ptr_transactional -- verifies the persistent ptr with the tx C API
 */
void
test_ptr_transactional(nvobj::pool<root> &pop)
{
	auto r = pop.root();
	nvobj::persistent_ptr<foo> to_swap;
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
			nvobj::persistent_ptr<foo> foo_ptr{r->pfoo};
			nvobj::persistent_ptr<foo> swap_ptr{to_swap};
			to_swap.swap(r->pfoo);
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
		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<foo>(r->pfoo); });
		r->pfoo = nullptr;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(r->pfoo == nullptr);
	UT_ASSERT(pfoo != nullptr);
}

/*
 * test_ptr_array -- verifies the array specialization behavior
 */
void
test_ptr_array(nvobj::pool<root> &pop)
{
	nvobj::persistent_ptr<nvobj::p<int>[]> parr_vsize;

	try {
		nvobj::make_persistent_atomic<nvobj::p<int>[]>(pop, parr_vsize,
							       TEST_ARR_SIZE);
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
 * test_offset -- test offset calculation within a hierarchy
 */
void
test_offset(nvobj::pool<root> &pop)
{
	struct A {
		uint64_t a;
	};

	struct B {
		uint64_t b;
	};

	struct C : public A, public B {
		uint64_t c;
	};

	try {
		nvobj::transaction::run(pop, [] {
			auto cptr = nvobj::make_persistent<C>();
			nvobj::persistent_ptr<B> bptr = cptr;
			UT_ASSERT((bptr.raw().off - cptr.raw().off) ==
				  sizeof(A));

			nvobj::persistent_ptr<B> bptr2;
			bptr2 = cptr;
			UT_ASSERT((bptr2.raw().off - cptr.raw().off) ==
				  sizeof(A));

			nvobj::persistent_ptr<B> bptr3 =
				static_cast<nvobj::persistent_ptr<B>>(cptr);
			UT_ASSERT((bptr3.raw().off - cptr.raw().off) ==
				  sizeof(A));

			nvobj::delete_persistent<C>(cptr);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

void
test_base_ptr_casting(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->arr[0] = nvobj::make_persistent<foo>();
			r->arr[1] = nvobj::make_persistent<int>(TEST_INT);
			r->arr[2] = nullptr;

			UT_ASSERT(!OID_IS_NULL(r->arr[0].raw()));
			UT_ASSERTeq(*(int *)pmemobj_direct(r->arr[1].raw()),
				    TEST_INT);
			UT_ASSERT(OID_IS_NULL(r->arr[2].raw()));

			nvobj::persistent_ptr<foo> tmp0 = r->arr[0].raw();
			nvobj::persistent_ptr<int> tmp1 = r->arr[1].raw();
			nvobj::persistent_ptr<foo> tmp2 = r->arr[2].raw();
			nvobj::delete_persistent<foo>(tmp0);
			nvobj::delete_persistent<int>(tmp1);
			nvobj::delete_persistent<foo>(tmp2);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<struct root>::create(
			path, LAYOUT, PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_ptr_operators_null();
	test_ptr_atomic(pop);
	test_ptr_transactional(pop);
	test_ptr_array(pop);
	test_offset(pop);
	test_base_ptr_casting(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
