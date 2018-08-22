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
#include <libpmemobj++/experimental/slice.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmemobj_exp = pmem::obj::experimental;

static bool Is_pmemcheck_enabled = false;

struct TestSuccess {
	void
	run()
	{
		auto slice = c.range(2, 2);

		UT_ASSERT(slice.size() == 2);
		UT_ASSERT(slice[0] == 3);
		UT_ASSERT(slice[1] == 4);

		UT_ASSERT(slice.begin() == c.begin() + 2);
		UT_ASSERT(slice.end() == c.begin() + 4);

		for (auto &it : slice) {
			it = 0;
		}

		UT_ASSERT(c[2] == 0);
		UT_ASSERT(c[3] == 0);

		try {
			/* Out of range */
			slice.at(2) = 2;
			UT_ASSERT(0);
		} catch (...) {
		}

		try {
			/* Out of range */
			c.range(100, 2);
			UT_ASSERT(0);
		} catch (...) {
		}
	}

	void
	run_reverse()
	{
		auto slice = c.range(1, 5, 2);

		int i = 0;
		for (auto it = slice.rbegin(); it != slice.rend(); it++, i++)
			*it = i;

		UT_ASSERT(c[5] == 0);
		UT_ASSERT(c[4] == 1);
		UT_ASSERT(c[3] == 2);
		UT_ASSERT(c[2] == 3);
		UT_ASSERT(c[1] == 4);
	}

	using C = pmemobj_exp::array<double, 6>;
	C c = {{1, 2, 3, 4, 5, 6}};
};

struct TestAbort {
	void
	run()
	{
		/* slice from 2 to 12 with snapshot_size = 3
		 * snapshotting ranges are: <2,4>, <5,7>, <8,10>, <11,12> */
		auto slice = c.range(2, 10, 3);

		auto it = slice.begin();

		/* it points to c[2],
		 * <2,4> should be added to a transaction */
		*it = 99;

		it += 10;

		/* it points to c[12],
		 * <11, 12> should be added to a transaction */
		*it = 101;

		it--;
		it--;
		it--;

		/* it points to c[9],
		 * <8,10> should be added to a transaction */
		*it = 100;

		C expected = {
			{1, 2, 99, 4, 5, 6, 7, 8, 9, 100, 11, 12, 101, 14, 15}};
		UT_ASSERT(c == expected);

		if (!Is_pmemcheck_enabled) {
			/* zero <5,7> range not adding it to a transaction */
			c._data[5] = 0;
			c._data[6] = 0;
			c._data[7] = 0;

			C expected = {{1, 2, 99, 4, 5, 0, 0, 0, 9, 100, 11, 12,
				       101, 14, 15}};
			UT_ASSERT(c == expected);
		}
	}

	using C = pmemobj_exp::array<double, 15>;
	C c = {{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
};

struct TestRanges {
	void
	run()
	{
		int ex1[] = {1, 1, 1, 1, 1};
		int ex2[] = {2, 2, 2, 2, 2};

		auto slice = c.range(0, 7, 1);

		for (auto &e : slice) {
			std::fill(e.data, e.data + 5, 1);
		}

		for (auto &e : c.range(7, c.size() - 7)) {
			std::fill(e.data, e.data + 5, 2);
		}

		for (auto it = c.cbegin(); it < c.cbegin() + 7; it++) {
			UT_ASSERT(std::equal(it->data, it->data + 5, ex1));
		}

		for (auto it = c.cbegin() + 7; it < c.cend(); it++) {
			UT_ASSERT(std::equal(it->data, it->data + 5, ex2));
		}
	}

	struct DataStruct {
		int data[5] = {1, 2, 3, 4, 5};
	};

	using C = pmemobj_exp::array<DataStruct, 15>;
	C c;
};

struct TestAt {
	void
	run()
	{
		auto slice = c.range(0, c.size(), 1);

		slice[2] = 1;
		slice.begin()[3] = 2;

		auto rit = slice.rbegin();
		*rit = 2.5;

		rit++;
		*rit = 3;

		C excpected = {{0, 0, 1, 2, 3, 2.5}};
		UT_ASSERT(c == excpected);
	}

	using C = pmemobj_exp::array<double, 6>;
	C c = {{0, 0, 0, 0, 0, 0}};
};

struct root {
	pmem::obj::persistent_ptr<TestSuccess> ptr_s;
	pmem::obj::persistent_ptr<TestAbort> ptr_a;
	pmem::obj::persistent_ptr<TestRanges> ptr_r;
	pmem::obj::persistent_ptr<TestAt> ptr_at;
};

void
run_test_success(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.get_root();

	try {
		pmem::obj::transaction::exec_tx(pop, [&] {
			r->ptr_s = pmem::obj::make_persistent<TestSuccess>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::exec_tx(pop, [&] {
			r->ptr_s->run();
			r->ptr_s->run_reverse();

			pmem::obj::delete_persistent<TestSuccess>(r->ptr_s);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

void
run_test_abort(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.get_root();

	try {
		pmem::obj::transaction::exec_tx(pop, [&] {
			r->ptr_a = pmem::obj::make_persistent<TestAbort>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	/* Run TestAbort expecting success */
	try {
		pmem::obj::transaction::exec_tx(pop, [&] {
			r->ptr_a->run();

			pmem::obj::delete_persistent<TestAbort>(r->ptr_a);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

void
run_test_abort_with_revert(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.get_root();

	try {
		pmem::obj::transaction::exec_tx(pop, [&] {
			r->ptr_a = pmem::obj::make_persistent<TestAbort>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	/* Run TestAbort expecting transaction abort */
	try {
		pmem::obj::transaction::exec_tx(pop, [&] {
			r->ptr_a->run();

			pmem::obj::transaction::abort(0);
			UT_ASSERT(0);
		});
	} catch (pmem::manual_tx_abort &) {
		if (Is_pmemcheck_enabled) {
			TestAbort::C expected = {{1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
						  11, 12, 13, 14, 15}};
			UT_ASSERT(r->ptr_a->c == expected);
		} else {
			/* Ensure that changes not added to the transaction were
			 * not reverted */
			TestAbort::C expected = {{1, 2, 3, 4, 5, 0, 0, 0, 9, 10,
						  11, 12, 13, 14, 15}};
			UT_ASSERT(r->ptr_a->c == expected);
		}
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::exec_tx(pop, [&] {
			pmem::obj::delete_persistent<TestAbort>(r->ptr_a);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

void
run_test_ranges(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.get_root();

	try {
		pmem::obj::transaction::exec_tx(pop, [&] {
			r->ptr_r = pmem::obj::make_persistent<TestRanges>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::exec_tx(pop, [&] {
			r->ptr_r->run();

			pmem::obj::delete_persistent<TestRanges>(r->ptr_r);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

void
run_test_at(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.get_root();

	try {
		pmem::obj::transaction::exec_tx(pop, [&] {
			r->ptr_at = pmem::obj::make_persistent<TestAt>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::exec_tx(pop, [&] {
			r->ptr_at->run();

			pmem::obj::delete_persistent<TestAt>(r->ptr_at);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

int
main(int argc, char *argv[])
{
	START();

	if (argc < 3) {
		std::cerr << "usage: " << argv[0] << " file-name "
			  << "is-pmemcheck-enabled " << std::endl;
		return 1;
	}

	Is_pmemcheck_enabled = std::stoi(argv[2]);

	auto path = argv[1];
	auto pop = pmem::obj::pool<root>::create(
		path, "ArrayTest", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	run_test_success(pop);
	run_test_abort(pop);
	run_test_abort_with_revert(pop);
	run_test_ranges(pop);
	run_test_at(pop);

	pop.close();
}
