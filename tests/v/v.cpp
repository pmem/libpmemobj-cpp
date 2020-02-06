// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

/*
 * obj_cpp_v.c -- cpp bindings test
 *
 */

#include "unittest.hpp"

#include <atomic>
#include <libpmemobj++/experimental/v.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#define LAYOUT "cpp"

namespace nvobj = pmem::obj;
namespace nvobj_exp = pmem::obj::experimental;

namespace
{

static const int TEST_VALUE = 10;

struct emplace_constructible {
	emplace_constructible() : a(), b(), c()
	{
	}

	emplace_constructible(int &a, int &b, int &&c)
	    : a(a), b(b), c(std::move(c))
	{
	}

	int a;
	int b;
	int c;
};

struct work_in_destructor {
	static int destructor_called;

	work_in_destructor()
	{
		a = TEST_VALUE;
	}

	~work_in_destructor()
	{
		destructor_called = 1;
	}

	int a;
};

int work_in_destructor::destructor_called = 0;

struct foo {
	foo() : counter(TEST_VALUE){};
	int counter;
};

struct bar {
	nvobj_exp::v<foo> vfoo;

	nvobj_exp::v<int> vi;
	nvobj_exp::v<int> vi2;
	nvobj_exp::v<char> vc;
	nvobj_exp::v<emplace_constructible> ndc;

	bar()
	{
	}
};

struct root {
	nvobj_exp::v<foo> f;
	nvobj::persistent_ptr<bar> bar_ptr;
	nvobj::persistent_ptr<nvobj_exp::v<work_in_destructor>> work_ptr;
};

/*
 * test_init -- test volatile value initialization
 */
void
test_init(nvobj::pool<root> &pop)
{
	UT_ASSERTeq(pop.root()->f.get().counter, TEST_VALUE);
	UT_ASSERTeq(pop.root()->bar_ptr->vfoo.get().counter, TEST_VALUE);
}

/*
 * test_conversion -- test v conversion operator
 */
void
test_conversion(nvobj::pool<root> &pop)
{
	auto r = pop.root()->bar_ptr;

	r->vi = 2;
	r->vc = 2;

	UT_ASSERT(r->vi == r->vc);
	UT_ASSERT(r->vi == 2);
	UT_ASSERT(2 == r->vi);
	UT_ASSERT(r->vi - 2 == 0);

	int &i1 = r->vi;
	char &i2 = r->vc;

	UT_ASSERT(i1 == i2);
	i1 = 1;

	UT_ASSERT(r->vi == i1);
}

/*
 * test_operators -- test v assignment operators
 */
void
test_operators(nvobj::pool<root> &pop)
{
	auto r = pop.root()->bar_ptr;

	r->vi = 2;
	r->vc = 3;

	UT_ASSERT(r->vi != r->vc);
	r->vi = r->vc;
	UT_ASSERT(r->vi == r->vc);

	r->vi = 2;
	r->vi2 = 3;
	std::swap(r->vi, r->vi2);
	UT_ASSERT(r->vi == 3);
	UT_ASSERT(r->vi2 == 2);

	r->vi2 = 2;
	r->vi = r->vi2;
	UT_ASSERT(r->vi == 2);
}

/*
 * test_variadic_get -- test v get with arguments
 */
void
test_variadic_get(nvobj::pool<root> &pop)
{
	auto r = pop.root()->bar_ptr;

	int a = 1, b = 2;
	auto &ref = r->ndc.get(a, b, 3);
	UT_ASSERT(ref.a == 1);
	UT_ASSERT(ref.b == 2);
	UT_ASSERT(ref.c == 3);

	auto &ref2 = r->ndc.unsafe_get();
	UT_ASSERT(&ref == &ref2);
	UT_ASSERT(ref2.a == 1);
	UT_ASSERT(ref2.b == 2);
	UT_ASSERT(ref2.c == 3);
}

/*
 * test_destructor -- test v destructor
 */
void
test_destructor(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->work_ptr = nvobj::make_persistent<
				nvobj_exp::v<work_in_destructor>>();
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(r->work_ptr->get().a == TEST_VALUE);
	UT_ASSERT(work_in_destructor::destructor_called == 0);

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<
				nvobj_exp::v<work_in_destructor>>(r->work_ptr);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	/* destructor should not be called */
	UT_ASSERT(work_in_destructor::destructor_called == 0);
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

	nvobj::make_persistent_atomic<bar>(pop, pop.root()->bar_ptr);

	test_init(pop);

	pop.root()->f.get().counter = 20;
	UT_ASSERTeq(pop.root()->f.get().counter, 20);

	pop.close();

	pop = nvobj::pool<struct root>::open(path, LAYOUT);

	test_init(pop);
	test_conversion(pop);
	test_operators(pop);
	test_variadic_get(pop);
	test_destructor(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
