// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

#include "unittest.hpp"

#include <iterator>

#include <libpmemobj++/container/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

struct Test1 {
	pmem::obj::array<double, 100> c;

	void
	iterator_pass()
	{
		for (auto it = c.begin(); it != c.end(); it++)
			*it = 1;
	}

	void
	check_pass()
	{
		for (auto &e : c)
			UT_ASSERT(e == 1);
	}

	void
	iterator_access()
	{
		auto it = c.begin();
		auto it2 = it + 20;

		swap(it, it2);

		it2[static_cast<std::ptrdiff_t>(c.size() - 1)] = 10;
		it[20] = 20;

		UT_ASSERT(c[c.size() - 1] == 10);
		UT_ASSERT(c[20 + 20] == 20);
	}
};

struct Test2 {
	pmem::obj::array<double, 100> c;

	void
	reverse_iterator_pass()
	{
		for (auto it = c.rbegin(); it != c.rend(); it++)
			*it = 1;
	}

	void
	check_pass()
	{
		for (auto &e : c)
			UT_ASSERT(e == 1);
	}
};

struct Test3 {
	using C = pmem::obj::array<double, 100>;
	C c;

	void
	iterator_operators()
	{
		auto slice = c.range(0, c.size());
		auto sub_slice = c.range(1, c.size() - 2);
		auto cslice = c.crange(0, c.size());

		UT_ASSERT(c.begin() == c.cbegin());
		UT_ASSERT(c.begin() == slice.begin());
		UT_ASSERT(c.begin() == sub_slice.begin() - 1);
		UT_ASSERT(c.begin() == cslice.begin());

		UT_ASSERT(c.cbegin() == slice.begin());
		UT_ASSERT(c.cbegin() == sub_slice.begin() - 1);
		UT_ASSERT(c.cbegin() == cslice.begin());

		UT_ASSERT(sub_slice.begin() - 1 == slice.begin());
		UT_ASSERT(sub_slice.begin() - 1 == cslice.begin());
		UT_ASSERT(slice.begin() == cslice.begin());
		UT_ASSERT(cslice.begin() == slice.begin());

		UT_ASSERT(c.end() == c.cend());
		UT_ASSERT(c.end() == slice.end());
		UT_ASSERT(c.end() == sub_slice.end() + 1);
		UT_ASSERT(c.end() == cslice.end());

		UT_ASSERT(c.cend() == slice.end());
		UT_ASSERT(c.cend() == sub_slice.end() + 1);
		UT_ASSERT(c.cend() == cslice.end());

		UT_ASSERT(sub_slice.end() + 1 == slice.end());
		UT_ASSERT(sub_slice.end() + 1 == cslice.end());
		UT_ASSERT(slice.end() == cslice.end());
		UT_ASSERT(cslice.end() == slice.end());

		UT_ASSERT(c.end() > c.begin());
		UT_ASSERT(c.end() > slice.begin());
		UT_ASSERT(c.end() > sub_slice.begin() + 1);
		UT_ASSERT(c.end() > cslice.begin());

		UT_ASSERT(slice.begin() < c.cend());
		UT_ASSERT(sub_slice.begin() + 1 < c.cend());
		UT_ASSERT(cslice.begin() < c.cend());

		UT_ASSERT(sub_slice.end() + 1 != slice.begin());
		UT_ASSERT(sub_slice.end() + 1 != cslice.begin());
		UT_ASSERT(slice.end() != cslice.begin());
		UT_ASSERT(cslice.end() != slice.begin());

		UT_ASSERT(C::size_type(c.end() - c.cbegin()) == c.size());
	}
};

struct root {
	pmem::obj::persistent_ptr<Test1> test1;
	pmem::obj::persistent_ptr<Test2> test2;
	pmem::obj::persistent_ptr<Test3> test3;
};

void
run_test1(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->test1 = pmem::obj::make_persistent<Test1>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->test1->iterator_pass();
			r->test1->check_pass();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->test1->iterator_access();

			pmem::obj::delete_persistent<Test1>(r->test1);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

void
run_test2(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->test2 = pmem::obj::make_persistent<Test2>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->test2->reverse_iterator_pass();
			r->test2->check_pass();

			pmem::obj::delete_persistent<Test2>(r->test2);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

void
run_test3(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->test3 = pmem::obj::make_persistent<Test3>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->test3->iterator_operators();

			pmem::obj::delete_persistent<Test3>(r->test3);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];

	auto pop = pmem::obj::pool<root>::create(
		path, "ArrayTest", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	run_test1(pop);
	run_test2(pop);
	run_test3(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
