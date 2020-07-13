// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2015-2020, Intel Corporation */

/*
 * obj_cpp_ptr.c -- cpp bindings test
 *
 */

#include "ptr.hpp"
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

namespace
{

using root = templated_root<nvobj::persistent_ptr, nvobj::persistent_ptr_base>;

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
		pop = nvobj::pool<root>::create(path, LAYOUT, PMEMOBJ_MIN_POOL,
						S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_root_pointers<nvobj::persistent_ptr, nvobj::persistent_ptr_base>(
		*pop.root());
	test_ptr_operators_null<nvobj::persistent_ptr>();
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
