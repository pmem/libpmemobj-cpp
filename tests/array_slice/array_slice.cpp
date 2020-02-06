// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/container/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/slice.hpp>
#include <libpmemobj++/transaction.hpp>

static bool Is_pmemcheck_enabled = false;

struct TestSuccess {
	void
	run()
	{
		auto slice = c.range(2, 2);

		UT_ASSERT(slice.size() == 2);
		UT_ASSERT(slice[0] == 3);
		UT_ASSERT(slice[1] == 4);
		UT_ASSERT(slice[0] == slice.at(0));
		UT_ASSERT(slice[1] == slice.at(1));

		UT_ASSERT(slice.begin() == c.begin() + 2);
		UT_ASSERT(slice.end() == c.begin() + 4);

		for (auto &it : slice) {
			it = 0;
		}

		UT_ASSERT(c[2] == 0);
		UT_ASSERT(c[3] == 0);

		auto zero_slice = c.range(0, 0);
		UT_ASSERT(zero_slice.begin() == zero_slice.end());

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

		try {
			/* Out of range */
			c.range(5, 2);
			UT_ASSERT(0);
		} catch (...) {
		}

		try {
			/* Out of range */
			c.crange(5, 2);
			UT_ASSERT(0);
		} catch (...) {
		}

		try {
			/* Out of range */
			c.range(5, 2, 1);
			UT_ASSERT(0);
		} catch (...) {
		}

		try {
			/* Out of range */
			c.range(5, 2, 999);
			UT_ASSERT(0);
		} catch (...) {
		}

		try {
			/* Out of range */
			c.range(5, 2, std::numeric_limits<std::size_t>::max());
			UT_ASSERT(0);
		} catch (...) {
		}

		try {
			/* Out of range */
			static_cast<const C &>(c).range(5, 2);
			UT_ASSERT(0);
		} catch (...) {
		}

		try {
			c.range(4, 2);
		} catch (...) {
			UT_ASSERT(0);
		}

		try {
			c.crange(4, 2);
		} catch (...) {
			UT_ASSERT(0);
		}

		try {
			c.range(4, 2, 1);
		} catch (...) {
			UT_ASSERT(0);
		}

		try {
			c.range(4, 2, 999);
		} catch (...) {
			UT_ASSERT(0);
		}

		try {
			c.range(4, 2, std::numeric_limits<std::size_t>::max());
		} catch (...) {
			UT_ASSERT(0);
		}

		try {
			static_cast<const C &>(c).range(4, 2);
		} catch (...) {
			UT_ASSERT(0);
		}

		char data[10];
		try {
			pmem::obj::slice<char *> good_slice(data, data);
		} catch (...) {
			UT_ASSERT(0);
		}

		try {
			pmem::obj::slice<char *> bad_slice(data + 1, data);
			UT_ASSERT(0);
		} catch (...) {
		}

		{
			auto ptr_s = c.range(0, 0);
			auto it_s = c.range(0, 0, 0);
			UT_ASSERT(ptr_s.size() == 0);
			UT_ASSERT(ptr_s.size() == it_s.size());
			UT_ASSERT(ptr_s.begin() == it_s.begin());
			UT_ASSERT(ptr_s.end() == it_s.end());
		}
		{
			auto ptr_s = c.range(0, 5);
			auto it_s = c.range(0, 5, 1);
			UT_ASSERT(ptr_s.size() == 5);
			UT_ASSERT(ptr_s.size() == it_s.size());
			UT_ASSERT(ptr_s.begin() == it_s.begin());
			UT_ASSERT(ptr_s.end() == it_s.end());
		}
		{
			auto ptr_s = c.range(1, 3);
			auto it_s = c.range(1, 3, 3);
			UT_ASSERT(ptr_s.size() == 3);
			UT_ASSERT(ptr_s.size() == it_s.size());
			UT_ASSERT(ptr_s.begin() == it_s.begin());
			UT_ASSERT(ptr_s.end() == it_s.end());
		}
	}

	void
	run_reverse()
	{
		auto slice = c.range(1, 5, 2);
		UT_ASSERT(slice.size() == 5);

		int i = 0;
		for (auto it = slice.rbegin(); it != slice.rend(); it++, i++)
			*it = i;

		UT_ASSERT(c[5] == 0);
		UT_ASSERT(c[4] == 1);
		UT_ASSERT(c[3] == 2);
		UT_ASSERT(c[2] == 3);
		UT_ASSERT(c[1] == 4);
	}

	using C = pmem::obj::array<double, 6>;
	C c = {{1, 2, 3, 4, 5, 6}};
};

struct TestAbort {
	void
	run()
	{
		/* slice from 2 to 12 with snapshot_size = 3
		 * snapshotting ranges are: <2,4>, <5,7>, <8,10>, <11> */
		auto slice = c.range(2, 10, 3);
		UT_ASSERT(slice.size() == 10);

		auto it = slice.begin();

		/* it points to c[2],
		 * <2,4> should be added to a transaction */
		*it = 99;

		it += 9;

		/* it points to c[11]
		 * <11> should be snapshotted */
		*it = 102;

		it--;
		it--;

		/* it points to c[9],
		 * <8,10> should be added to a transaction */
		*it = 100;

		C expected = {
			{1, 2, 99, 4, 5, 6, 7, 8, 9, 100, 11, 102, 13, 14, 15}};
		UT_ASSERT(c == expected);

		if (!Is_pmemcheck_enabled) {
			it = slice.begin() + 10;
			/* it points to c[12] (outside of range)
			 * no snapshotting */
			*it = 101;

			/* zero <5,7> range not adding it to a transaction */
			c._data[5] = 0;
			c._data[6] = 0;
			c._data[7] = 0;

			C expected = {{1, 2, 99, 4, 5, 0, 0, 0, 9, 100, 11, 102,
				       101, 14, 15}};
			UT_ASSERT(c == expected);

			auto ptr_slice = c2.range(1, 4);
			for (auto &e : ptr_slice)
				e = 1;

			c2._data[0] = 0;
			c2._data[5] = 0;

			C expected2 = {{0, 1, 1, 1, 1, 0, 7, 8, 9, 10, 11, 12,
					13, 14, 15}};
			UT_ASSERT(c2 == expected2);
		}
	}

	void
	run_zero()
	{
		auto slice = c.range(0, c.size(), 0);

		for (auto &e : slice)
			e = 0;
	}

	using C = pmem::obj::array<double, 15>;
	C c = {{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
	C c2 = {{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
};

struct TestRanges {
	template <std::size_t snapshot_size>
	void
	run()
	{
		int ex1[] = {1, 1, 1, 1, 1};
		int ex2[] = {2, 2, 2, 2, 2};

		auto slice = c.range(0, 7, snapshot_size);
		auto cslice = static_cast<const C &>(c).range(0, 7);

		UT_ASSERT(slice.begin() == cslice.begin());
		UT_ASSERT(slice.end() == cslice.end());

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

		auto ptr_slice = c2.range(0, 5);
		for (auto &e : ptr_slice)
			std::fill(e.data, e.data + 5, 1);

		for (auto it = c2.cbegin(); it < c2.cbegin() + 5; it++)
			UT_ASSERT(std::equal(it->data, it->data + 5, ex1));
	}

	struct DataStruct {
		int data[5] = {1, 2, 3, 4, 5};
	};

	using C = pmem::obj::array<DataStruct, 15>;
	C c;
	C c2;
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

		C expected = {{0, 0, 1, 2, 3, 2.5}};
		UT_ASSERT(c == expected);
	}

	using C = pmem::obj::array<double, 6>;
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
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->ptr_s = pmem::obj::make_persistent<TestSuccess>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
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
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->ptr_a = pmem::obj::make_persistent<TestAbort>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	/* Run TestAbort expecting success */
	try {
		pmem::obj::transaction::run(pop, [&] {
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
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->ptr_a = pmem::obj::make_persistent<TestAbort>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	/* Run TestAbort expecting transaction abort */
	try {
		pmem::obj::transaction::run(pop, [&] {
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
						  11, 12, 101, 14, 15}};
			UT_ASSERT(r->ptr_a->c == expected);

			/* Ensure that changes not added to the transaction were
			 * not reverted */
			TestAbort::C expected2 = {{0, 2, 3, 4, 5, 0, 7, 8, 9,
						   10, 11, 12, 13, 14, 15}};
			UT_ASSERT(r->ptr_a->c2 == expected2);
		}
	} catch (...) {
		UT_ASSERT(0);
	}

	if (!Is_pmemcheck_enabled) {
		/* Run TestAbort expecting transaction abort */
		try {
			pmem::obj::transaction::run(pop, [&] {
				r->ptr_a->run_zero();

				pmem::obj::transaction::abort(0);
				UT_ASSERT(0);
			});
		} catch (pmem::manual_tx_abort &) {
			/* Ensure that changes not added to the transaction were
			 * not reverted */
			TestAbort::C expected = {
				{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
			UT_ASSERT(r->ptr_a->c == expected);
		} catch (...) {
			UT_ASSERT(0);
		}
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<TestAbort>(r->ptr_a);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

void
run_test_ranges(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->ptr_r = pmem::obj::make_persistent<TestRanges>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->ptr_r->run<1>();

			pmem::obj::delete_persistent<TestRanges>(r->ptr_r);
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->ptr_r = pmem::obj::make_persistent<TestRanges>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->ptr_r->run<
				std::numeric_limits<std::size_t>::max()>();

			pmem::obj::delete_persistent<TestRanges>(r->ptr_r);
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->ptr_r = pmem::obj::make_persistent<TestRanges>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->ptr_r->run<999>();

			pmem::obj::delete_persistent<TestRanges>(r->ptr_r);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

void
run_test_at(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->ptr_at = pmem::obj::make_persistent<TestAt>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->ptr_at->run();

			pmem::obj::delete_persistent<TestAt>(r->ptr_at);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

static void
test(int argc, char *argv[])
{
	if (argc < 3) {
		UT_FATAL("usage: %s file-name is-pmemcheck-enabled", argv[0]);
	}

	Is_pmemcheck_enabled = (std::stoi(argv[2])) != 0;

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

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
