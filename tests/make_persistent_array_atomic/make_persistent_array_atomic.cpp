// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2020, Intel Corporation */

/*
 * obj_cpp_make_persistent_array_atomic.cpp -- cpp make_persistent test for
 * arrays
 */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj/ctl.h>

#define LAYOUT "cpp"

namespace nvobj = pmem::obj;

namespace
{

const int TEST_ARR_SIZE = 10;

class foo {
public:
	foo() : bar(1)
	{
		for (int i = 0; i < TEST_ARR_SIZE; ++i)
			this->arr[i] = 1;
	}

	/*
	 * Assert values of foo.
	 */
	void
	check_foo()
	{
		UT_ASSERTeq(1, this->bar);
		for (int i = 0; i < TEST_ARR_SIZE; ++i)
			UT_ASSERTeq(1, this->arr[i]);
	}

	~foo() = default;

	nvobj::p<int> bar;
	nvobj::p<char> arr[TEST_ARR_SIZE];
};

struct root {
	nvobj::persistent_ptr<foo[]> pfoo;
};

class bar {
public:
	bar()
	{
		/* throw any exception */
		throw 1;
	}
};

/*
 * test_make_one_d -- (internal) test make_persistent of a 1d array
 */
void
test_make_one_d(nvobj::pool_base &pop)
{
	nvobj::persistent_ptr<foo[]> pfoo;
	nvobj::make_persistent_atomic<foo[]>(pop, pfoo, 5);
	for (int i = 0; i < 5; ++i)
		pfoo[i].check_foo();

	nvobj::delete_persistent_atomic<foo[]>(pfoo, 5);

	nvobj::make_persistent_atomic<foo[]>(pop, pfoo, 6);
	for (int i = 0; i < 6; ++i)
		pfoo[i].check_foo();

	nvobj::delete_persistent_atomic<foo[]>(pfoo, 6);

	nvobj::persistent_ptr<foo[5]> pfooN;
	nvobj::make_persistent_atomic<foo[5]>(pop, pfooN);
	for (int i = 0; i < 5; ++i)
		pfooN[i].check_foo();

	nvobj::delete_persistent_atomic<foo[5]>(pfooN);
}

/*
 * test_make_N_d -- (internal) test make_persistent of 2d and 3d arrays
 */
void
test_make_N_d(nvobj::pool_base &pop)
{
	nvobj::persistent_ptr<foo[][2]> pfoo;
	nvobj::make_persistent_atomic<foo[][2]>(pop, pfoo, 5);
	for (int i = 0; i < 5; ++i)
		for (int j = 0; j < 2; j++)
			pfoo[i][j].check_foo();

	nvobj::delete_persistent_atomic<foo[][2]>(pfoo, 5);

	nvobj::persistent_ptr<foo[][3]> pfoo2;
	nvobj::make_persistent_atomic<foo[][3]>(pop, pfoo2, 6);
	for (int i = 0; i < 6; ++i)
		for (int j = 0; j < 3; j++)
			pfoo2[i][j].check_foo();

	nvobj::delete_persistent_atomic<foo[][3]>(pfoo2, 6);

	nvobj::persistent_ptr<foo[5][2]> pfooN;
	nvobj::make_persistent_atomic<foo[5][2]>(pop, pfooN);
	for (int i = 0; i < 5; ++i)
		for (int j = 0; j < 2; j++)
			pfooN[i][j].check_foo();

	nvobj::delete_persistent_atomic<foo[5][2]>(pfooN);

	nvobj::persistent_ptr<foo[][2][3]> pfoo3;
	nvobj::make_persistent_atomic<foo[][2][3]>(pop, pfoo3, 5);
	for (int i = 0; i < 5; ++i)
		for (int j = 0; j < 2; j++)
			for (int k = 0; k < 3; k++)
				pfoo3[i][j][k].check_foo();

	nvobj::delete_persistent_atomic<foo[][2][3]>(pfoo3, 5);

	nvobj::persistent_ptr<foo[5][2][3]> pfoo3N;
	nvobj::make_persistent_atomic<foo[5][2][3]>(pop, pfoo3N);
	for (int i = 0; i < 5; ++i)
		for (int j = 0; j < 2; j++)
			for (int k = 0; k < 3; k++)
				pfoo3N[i][j][k].check_foo();

	nvobj::delete_persistent_atomic<foo[5][2][3]>(pfoo3N);
}

/*
 * test_constructor_exception -- (internal) test exceptions thrown in
 * constructors
 */
void
test_constructor_exception(nvobj::pool_base &pop)
{
	nvobj::persistent_ptr<bar[]> pfoo;
	bool except = false;
	try {
		nvobj::make_persistent_atomic<bar[]>(pop, pfoo, 5);
	} catch (std::bad_alloc &) {
		except = true;
	}

	UT_ASSERT(except);
}

/*
 * test_delete_null -- (internal) test atomic delete nullptr
 */
void
test_delete_null()
{
	nvobj::persistent_ptr<foo[]> pfoo;
	nvobj::persistent_ptr<bar[3]> pbar;

	UT_ASSERT(pfoo == nullptr);
	UT_ASSERT(pbar == nullptr);

	try {
		nvobj::delete_persistent_atomic<foo[]>(pfoo, 2);
		nvobj::delete_persistent_atomic<bar[3]>(pbar);
	} catch (...) {
		UT_ASSERT(0);
	}
}

/*
 * test_flags -- (internal) test proper handling of flags
 */
void
test_flags(nvobj::pool<struct root> &pop)
{
	nvobj::persistent_ptr<foo[]> pfoo = nullptr;
	nvobj::persistent_ptr<foo[10]> pfoo_sized = nullptr;

	auto alloc_class = pop.ctl_set<struct pobj_alloc_class_desc>(
		"heap.alloc_class.new.desc",
		{sizeof(foo), 0, 200, POBJ_HEADER_COMPACT, 0});

	try {
		nvobj::make_persistent_atomic<foo[10]>(
			pop, pfoo_sized,
			nvobj::allocation_flag_atomic::class_id(
				alloc_class.class_id));

		nvobj::make_persistent_atomic<foo[]>(
			pop, pfoo, 10,
			nvobj::allocation_flag_atomic::class_id(
				alloc_class.class_id));
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(pmemobj_alloc_usable_size(pfoo.raw()), sizeof(foo) * 10);
	UT_ASSERTeq(pmemobj_alloc_usable_size(pfoo_sized.raw()),
		    sizeof(foo) * 10);

	try {
		nvobj::delete_persistent_atomic<foo[]>(pfoo, 10);
		nvobj::delete_persistent_atomic<foo[10]>(pfoo_sized);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<struct root> pop;

	try {
		pop = nvobj::pool<struct root>::create(
			path, LAYOUT, PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_make_one_d(pop);
	test_make_N_d(pop);
	test_constructor_exception(pop);
	test_delete_null();
	test_flags(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
