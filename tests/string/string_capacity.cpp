// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using S = pmem::obj::string;

struct root {
	nvobj::persistent_ptr<S> s, s1, s2;
};

/**
 * Check if access method can be called out of transaction scope
 */
void
check_access_out_of_tx(const S &s)
{
	try {
		(void)s.empty();
		(void)s.size();
		(void)s.length();
		(void)s.max_size();
		(void)s.capacity();
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

/**
 * Checks if string's state is reverted when transaction aborts.
 */
void
assert_tx_abort(pmem::obj::pool<struct root> &pop, S &s,
		std::function<void(void)> f)
{
	bool exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			f();
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

void
verify_string(const S &s, const S &expected)
{
	UT_ASSERT(s == expected);
	UT_ASSERT(s.size() == expected.size());
	UT_ASSERT(s.capacity() == expected.capacity());
}

void
check_tx_abort(pmem::obj::pool<struct root> &pop, const S &expected)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->s = nvobj::make_persistent<S>(expected);
		});

		auto &s = *r->s;

		assert_tx_abort(pop, s, [&] { s.resize(30); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.resize(300); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.resize(30, 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.resize(300, 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.reserve(); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.reserve(30); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.reserve(300); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.shrink_to_fit(); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.clear(); });
		verify_string(s, expected);

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<S>(r->s); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
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

	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->s1 = nvobj::make_persistent<S>("0123456789");
		r->s2 = nvobj::make_persistent<S>(
			"0123456789012345678901234567890123456789"
			"0123456789012345678901234567890123456789"
			"0123456789012345678901234567890123456789"
			"0123456789");
	});

	check_access_out_of_tx(*r->s1);
	check_access_out_of_tx(*r->s2);

	check_tx_abort(pop, *r->s1);
	check_tx_abort(pop, *r->s2);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<S>(r->s1);
		nvobj::delete_persistent<S>(r->s2);
	});

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
