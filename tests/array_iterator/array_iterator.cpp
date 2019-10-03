/*
 * Copyright 2018-2019, Intel Corporation
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

		it2[c.size() - 1] = 10;
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

	run_test1(pop);
	run_test2(pop);
	run_test3(pop);

	pop.close();

	return 0;
}
