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

#include <iterator>

#include <libpmemobj++/experimental/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmemobj_exp = pmem::obj::experimental;

struct Test1 {
	pmemobj_exp::array<double, 100> c;

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
	pmemobj_exp::array<double, 100> c;

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

struct root {
	pmem::obj::persistent_ptr<Test1> test1;
	pmem::obj::persistent_ptr<Test2> test2;
};

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
		return 1;
	}

	auto path = argv[1];

	auto pop = pmem::obj::pool<root>::create(
		path, "ArrayTest", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.get_root();

	try {
		pmem::obj::transaction::exec_tx(pop, [&] {
			r->test1 = pmem::obj::make_persistent<Test1>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::exec_tx(pop, [&] {
			r->test1->iterator_pass();
			r->test1->check_pass();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::exec_tx(pop, [&] {
			r->test1->iterator_access();

			pmem::obj::delete_persistent<Test1>(r->test1);
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::exec_tx(pop, [&] {
			r->test2 = pmem::obj::make_persistent<Test2>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::exec_tx(pop, [&] {
			r->test2->reverse_iterator_pass();
			r->test2->check_pass();

			pmem::obj::delete_persistent<Test2>(r->test2);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}
