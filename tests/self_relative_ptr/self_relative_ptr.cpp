// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2015-2020, Intel Corporation */

/*
 * obj_cpp_ptr.c -- cpp bindings test
 *
 */

#include "unittest.hpp"

#include <libpmemobj++/experimental/self_relative_ptr.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <sstream>

#define LAYOUT "cpp"

namespace nvobj = pmem::obj;

template <typename T>
using self_relative_ptr = nvobj::experimental::self_relative_ptr<T>;
using self_relative_ptr_base = nvobj::experimental::self_relative_ptr_base;

namespace
{

/*
 * test_null_ptr -- verifies if the pointer correctly behaves like a
 * nullptr-value
 */
void
test_null_ptr(self_relative_ptr<int> &f)
{
	// UT_ASSERT(OID_IS_NULL(f.raw()));
	UT_ASSERT((bool)f == false);
	UT_ASSERT(!f);
	UT_ASSERTeq(f.get(), nullptr);
	UT_ASSERT(f == nullptr);
}

/*
 * get_temp -- returns a temporary persistent_ptr
 */
self_relative_ptr<int>
get_temp()
{
	self_relative_ptr<int> int_null = nullptr;

	return int_null;
}

/*
 * test_ptr_operators_null -- verifies various operations on nullptr pointers
 */
void
test_ptr_operators_null()
{
	self_relative_ptr<int> int_default_null;
	test_null_ptr(int_default_null);

	self_relative_ptr<int> int_explicit_ptr_null = nullptr;
	test_null_ptr(int_explicit_ptr_null);

	self_relative_ptr<int> int_explicit_oid_null = OID_NULL;
	test_null_ptr(int_explicit_oid_null);

	self_relative_ptr<int> int_base = nullptr;
	self_relative_ptr<int> int_same = int_base;
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
	self_relative_ptr<foo> inner;
};

struct root {
	self_relative_ptr<foo> pfoo;
	self_relative_ptr<nvobj::p<int>[TEST_ARR_SIZE]> parr;
	self_relative_ptr_base arr[3];

	/* This variable is unused, but it's here to check if the persistent_ptr
	 * does not violate its own restrictions.
	 */
	self_relative_ptr<nested> outer;
};

/*
 * test_ptr_transactional -- verifies the persistent ptr with the tx C API
 */
void
test_ptr_transactional(nvobj::pool<root> &pop)
{
	auto r = pop.root();
	self_relative_ptr<foo> to_swap;
	try {
		nvobj::transaction::run(pop, [&] {
			// this line commented because pool::create does not
			// call the constructor
			// TODO: uncomment this line
			// UT_ASSERT(r->pfoo == nullptr);

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
			self_relative_ptr<foo> foo_ptr = pfoo;
			self_relative_ptr<foo> swap_ptr = to_swap;

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

			nvobj::delete_persistent<foo>(
				to_swap.to_persitent_ptr());
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
			nvobj::delete_persistent<foo>(
				r->pfoo.to_persitent_ptr());
		});
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
	self_relative_ptr<nvobj::p<int>[]> parr_vsize;

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
			auto distance =
				self_relative_ptr_base::distance_between;

			self_relative_ptr<C> cptr = nvobj::make_persistent<C>();
			self_relative_ptr<B> bptr = cptr;
			std::cout << distance(bptr, cptr) << std::endl;
			UT_ASSERT(abs(distance(bptr, cptr)) == sizeof(A));

			self_relative_ptr<B> bptr2;
			bptr2 = cptr;
			UT_ASSERT(abs(distance(bptr2, cptr)) == sizeof(A));

			self_relative_ptr<B> bptr3 =
				static_cast<self_relative_ptr<B>>(cptr);
			UT_ASSERT(abs(distance(bptr3, cptr)) == sizeof(A));

			nvobj::delete_persistent<C>(cptr.to_persitent_ptr());
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
			r->arr[0] = self_relative_ptr<foo>{
				nvobj::make_persistent<foo>()};
			r->arr[1] = self_relative_ptr<int>{
				nvobj::make_persistent<int>(TEST_INT)};
			r->arr[2] = nullptr;

			UT_ASSERTne(r->arr[0].to_void_pointer(), nullptr);
			UT_ASSERTeq(*static_cast<int *>(
					    r->arr[1].to_void_pointer()),
				    TEST_INT);
			UT_ASSERTeq(r->arr[2].to_void_pointer(), nullptr);

			self_relative_ptr<foo> tmp0 =
				static_cast<foo *>(r->arr[0].to_void_pointer());
			self_relative_ptr<int> tmp1 =
				static_cast<int *>(r->arr[1].to_void_pointer());
			self_relative_ptr<foo> tmp2 =
				static_cast<foo *>(r->arr[2].to_void_pointer());
			nvobj::delete_persistent<foo>(tmp0.to_persitent_ptr());
			nvobj::delete_persistent<int>(tmp1.to_persitent_ptr());
			nvobj::delete_persistent<foo>(tmp2.to_persitent_ptr());
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

} /* namespace */

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
