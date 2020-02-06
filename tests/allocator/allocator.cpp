// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2020, Intel Corporation */

/*
 * allocator.cpp -- cpp bindings test
 *
 */
#include "unittest.hpp"

#include <libpmemobj++/allocator.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#define LAYOUT "cpp"

namespace nvobj = pmem::obj;

namespace
{

const int TEST_ARR_SIZE = 10;

struct foo {

	/*
	 * Default constructor.
	 */
	foo()
	{
		bar = 1;
		for (int i = 0; i < TEST_ARR_SIZE; ++i)
			arr[i] = static_cast<char>(i);
	}

	/*
	 * Copy constructible.
	 */
	foo(const foo &rhs) = default;

	/*
	 * Check foo values.
	 */
	void
	test_foo()
	{
		UT_ASSERTeq(bar, 1);
		for (int i = 0; i < TEST_ARR_SIZE; ++i)
			UT_ASSERTeq(arr[i], i);
	}

	nvobj::p<int> bar;
	nvobj::p<char> arr[TEST_ARR_SIZE];
};

/*
 * test_alloc_valid -- (internal) test an allocation within a transaction
 */
void
test_alloc_valid(nvobj::pool_base &pop)
{
	nvobj::allocator<foo> al;

	try {
		nvobj::transaction::run(pop, [&] {
			auto fooptr = al.allocate(1);
			UT_ASSERT(pmemobj_alloc_usable_size(fooptr.raw()) >=
				  sizeof(foo));
			al.construct(fooptr, foo());
			fooptr->test_foo();
			al.destroy(fooptr);
			al.deallocate(fooptr);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

/*
 * test_alloc_invalid -- (internal) test an allocation outside of a transaction
 */
void
test_alloc_invalid()
{
	nvobj::allocator<foo> al;
	bool thrown = false;
	try {
		auto fooptr = al.allocate(1);
		al.construct(fooptr, foo());
	} catch (pmem::transaction_scope_error &) {
		thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(thrown);
}

/*
 * test_dealloc_invalid -- (internal) test an deallocation outside of a
 * transaction
 */
void
test_dealloc_invalid(nvobj::pool_base &pop)
{
	nvobj::allocator<foo> al;
	nvobj::persistent_ptr<foo> fooptr;
	bool thrown = false;
	try {
		nvobj::transaction::run(pop, [&] { fooptr = al.allocate(1); });
		al.deallocate(fooptr);
	} catch (pmem::transaction_scope_error &) {
		thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(thrown);

	try {
		nvobj::transaction::run(pop, [&] { al.deallocate(fooptr); });
	} catch (...) {
		UT_ASSERT(0);
	}
}

/*
 * test_alloc_equal -- (internal) test allocator equality/inequality operators
 */
void
test_alloc_equal()
{
	nvobj::allocator<foo> fooal;
	nvobj::allocator<int> intal;
	std::allocator<foo> stdfooal;
	std::allocator<int> stdintal;
	std::allocator<double> stddblal;

	UT_ASSERT(fooal == fooal);
	UT_ASSERT(intal == fooal);
	UT_ASSERT(!(fooal != fooal));
	UT_ASSERT(!(intal != fooal));
	UT_ASSERT(fooal != stdfooal);
	UT_ASSERT(fooal != stdintal);
	UT_ASSERT(fooal != stddblal);
	UT_ASSERT(intal != stdfooal);
	UT_ASSERT(intal != stdintal);
	UT_ASSERT(intal != stddblal);
	UT_ASSERT(!(fooal == stdfooal));
	UT_ASSERT(!(fooal == stdintal));
	UT_ASSERT(!(fooal == stddblal));
	UT_ASSERT(!(intal == stdfooal));
	UT_ASSERT(!(intal == stdintal));
	UT_ASSERT(!(intal == stddblal));
}
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool_base pop;

	try {
		pop = nvobj::pool_base::create(path, LAYOUT, PMEMOBJ_MIN_POOL,
					       S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_alloc_valid(pop);
	test_alloc_invalid();
	test_dealloc_invalid(pop);
	test_alloc_equal();

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
