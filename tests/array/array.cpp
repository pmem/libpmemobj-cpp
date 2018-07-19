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

#include <libpmemobj++/experimental/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmemobj_exp = pmem::obj::experimental;

struct Test1 {
	void
	run()
	{
		C::reference r1 = c.at(0);
		assert(r1 == 1);
		r1 = 5.5;
		assert(c.front() == 5.5);

		C::reference r2 = c.at(2);
		assert(r2 == 3.5);
		r2 = 7.5;
		assert(c.back() == 7.5);

		try {
			c.at(3);
			assert(false);
		} catch (const std::out_of_range &) {
		}
	}

	using C = pmemobj_exp::array<double, 3>;
	C c = {{1, 2, 3.5}};
};

struct Test2 {
	void
	run()
	{
		C const &cc = c;
		try {
			c.at(0);
			assert(false);
		} catch (const std::out_of_range &) {
		}
		try {
			cc.at(0);
			assert(false);
		} catch (const std::out_of_range &) {
		}
	}

	using C = pmemobj_exp::array<double, 0>;
	C c = {{}};
};

struct Test3 {
	void
	run()
	{
		C::const_reference r1 = c.at(0);
		assert(r1 == 1);

		C::const_reference r2 = c.at(2);
		assert(r2 == 3.5);

		try {
			c.at(3);
			assert(false);
		} catch (const std::out_of_range &) {
		}
	}

	using C = pmemobj_exp::array<double, 3>;
	C c = {{1, 2, 3.5}};
};

struct root {
	pmem::obj::persistent_ptr<Test1> test1;
	pmem::obj::persistent_ptr<Test2> test2;
	pmem::obj::persistent_ptr<Test3> test3;
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

			r->test1->run();

			pmem::obj::delete_persistent<Test1>(r->test1);
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::exec_tx(pop, [&] {
			r->test2 = pmem::obj::make_persistent<Test2>();

			r->test2->run();

			pmem::obj::delete_persistent<Test2>(r->test2);
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::exec_tx(pop, [&] {
			r->test3 = pmem::obj::make_persistent<Test3>();

			r->test3->run();

			pmem::obj::delete_persistent<Test3>(r->test3);
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	pop.close();
}
