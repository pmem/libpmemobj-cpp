// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "../common/transaction_helpers.hpp"
#include "../common/unittest.hpp"

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/slice.hpp>
#include <libpmemobj++/transaction.hpp>

#include <algorithm>

namespace nvobj = pmem::obj;

using S = pmem::obj::string;

struct root {
	nvobj::persistent_ptr<S> s;
};

void
test_out_of_range_exception(pmem::obj::pool<root> &pop, bool use_sso_only)
{
	auto r = pop.root();

	size_t bonus_size = use_sso_only ? 0 : 20;

	try {
		nvobj::transaction::run(pop, [&] {
			r->s = nvobj::make_persistent<S>(10U + bonus_size, 'a');
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	S &str = *(r->s);
	const S &const_str = *(r->s);

	try {
		nvobj::transaction::run(pop, [&] {
			auto slice = str.range(
				0, 10 + bonus_size); /* should not throw */
			(void)slice;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		nvobj::transaction::run(pop, [&] {
			auto slice = str.crange(
				0, 10 + bonus_size); /* should not throw */
			(void)slice;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		nvobj::transaction::run(pop, [&] {
			auto slice = const_str.range(
				0, 10 + bonus_size); /* should not throw */
			(void)slice;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		nvobj::transaction::run(pop, [&] {
			auto slice = str.range(0, 10 + bonus_size,
					       3); /* should not throw */
			(void)slice;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	bool exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			auto slice = str.range(
				0, 11 + bonus_size); /* should throw */
			(void)slice;
		});
		UT_ASSERT(0);
	} catch (std::out_of_range &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);

	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			auto slice = str.range(0, 11 + bonus_size,
					       3); /* should throw */
			(void)slice;
		});
		UT_ASSERT(0);
	} catch (std::out_of_range &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);

	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			auto slice = const_str.range(
				0, 11 + bonus_size); /* should throw */
			(void)slice;
		});
		UT_ASSERT(0);
	} catch (std::out_of_range &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);

	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			auto slice = str.crange(
				0, 11 + bonus_size); /* should throw */
			(void)slice;
		});
		UT_ASSERT(0);
	} catch (std::out_of_range &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<S>(r->s); });
}

void
test_returned_values(pmem::obj::pool<root> &pop, bool use_sso_only)
{
	auto r = pop.root();

	size_t bonus_size = use_sso_only ? 0 : 20;

	try {
		nvobj::transaction::run(pop, [&] {
			r->s = nvobj::make_persistent<S>(10U + bonus_size, 'a');
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	S &str = *(r->s);
	const S &const_str = *(r->s);

	assert_tx_abort(pop, [&] {
		nvobj::slice<S::pointer> slice = str.range(0, 3);
		UT_ASSERTeq(&str.front(), slice.begin());
		UT_ASSERTeq(&str.front() + 3, slice.end());

		size_t cnt = 0;
		for (auto &c : slice) {
			UT_ASSERTeq(c, 'a');
			c = 'b';
			UT_ASSERTeq(str[cnt++], 'b');
			UT_ASSERTeq(std::count(slice.begin(), slice.end(), 'b'),
				    static_cast<long>(cnt));
			UT_ASSERTeq(str.size(), 10U + bonus_size);
		}
	});
	UT_ASSERTeq(str.find('b'), str.npos);

	assert_tx_abort(pop, [&] {
		nvobj::slice<S::range_snapshotting_iterator> slice =
			str.range(2, 8 + bonus_size, 1);
		UT_ASSERTeq(&str.front() + 2, &*slice.begin());
		UT_ASSERTeq(&str.front() + 10 + bonus_size, &*slice.end());

		size_t cnt = 2;
		for (auto &c : slice) {
			UT_ASSERTeq(c, 'a');
			c = 'b';
			UT_ASSERTeq(str[cnt++], 'b');
			UT_ASSERTeq(std::count(slice.begin(), slice.end(), 'b'),
				    static_cast<long>(cnt - 2));
			UT_ASSERTeq(str.size(), 10U + bonus_size);
		}
	});
	UT_ASSERTeq(str.find('b'), str.npos);

	assert_tx_abort(pop, [&] {
		nvobj::slice<S::range_snapshotting_iterator> slice =
			str.range(0, 10 + bonus_size, 11 + bonus_size);
		UT_ASSERTeq(&str.front(), &*slice.begin());
		UT_ASSERTeq(&str.front() + 10 + bonus_size, &*slice.end());

		size_t cnt = 0;
		for (auto &c : slice) {
			UT_ASSERTeq(c, 'a');
			c = 'b';
			UT_ASSERTeq(str[cnt++], 'b');
			UT_ASSERTeq(std::count(slice.begin(), slice.end(), 'b'),
				    static_cast<long>(cnt));
			UT_ASSERTeq(str.size(), 10U + bonus_size);
		}
	});
	UT_ASSERTeq(str.find('b'), str.npos);

	nvobj::transaction::run(pop, [&] {
		nvobj::slice<S::pointer> slice = str.range(0, 10 + bonus_size);
		for (auto &c : slice) {
			c = 'b';
		}
	});
	UT_ASSERTeq(str.size(), 10U + bonus_size);
	UT_ASSERTeq(std::count(str.begin(), str.end(), 'b'),
		    static_cast<long>(10 + bonus_size));

	nvobj::slice<S::const_iterator> slice1 = const_str.range(0, 3);
	UT_ASSERTeq(&const_str.front(), slice1.begin());
	UT_ASSERTeq(&const_str.front() + 3, slice1.end());

	nvobj::slice<S::const_iterator> slice2 = str.crange(0, 3);
	UT_ASSERTeq(&str.front(), slice2.begin());
	UT_ASSERTeq(&str.front() + 3, slice2.end());

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<S>(r->s); });
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "StringTest", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	test_out_of_range_exception(pop, true);
	test_out_of_range_exception(pop, false);
	test_returned_values(pop, true);
	test_returned_values(pop, false);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
