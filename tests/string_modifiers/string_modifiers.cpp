// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj.h>

namespace nvobj = pmem::obj;

using S = pmem::obj::string;

struct root {
	nvobj::persistent_ptr<S> s, s1, str, str2;
};

/**
 * Check if access method can be called out of transaction scope
 */
void
check_access_out_of_tx(nvobj::pool<root> &pop, const char *str)
{
	auto r = pop.root();

	nvobj::transaction::run(
		pop, [&] { r->s1 = nvobj::make_persistent<S>(str); });

	auto &s = *r->s1;

	try {
		char buf[50];
		s.copy(buf, 5, 0);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<S>(r->s1); });
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
check_tx_abort(pmem::obj::pool<struct root> &pop, const char *str,
	       bool truncate = false)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->s = nvobj::make_persistent<S>(str);
			r->s1 = nvobj::make_persistent<S>(str);
		});

		auto &s = *r->s;
		auto &expected = *r->s1;

		if (truncate) {
			s = "01234567890";
			expected = "01234567890";
		}

		assert_tx_abort(pop, s, [&] { s.clear(); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.erase(); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.erase(0); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.erase(1, 5); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.erase(s.begin()); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.erase(s.begin(), s.end()); });
		verify_string(s, expected);

		assert_tx_abort(pop, s,
				[&] { s.erase(s.begin() + 5, s.end()); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.append(5, 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.append(100, 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.append("ABCDEF"); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.append("ABCDEF", 3); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.append({'a', 'b', 'c'}); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.push_back('a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.pop_back(); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s += "12345"; });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s += 'a'; });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s += {'a', 'b', 'c'}; });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.insert(0, 5, 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.insert(5, 5, 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.insert(s.size(), 5, 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.insert(0, "12345"); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.insert(5, "12345"); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.insert(s.size(), "12345"); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.insert(0, "12345", 3); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.insert(s.cbegin(), 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.insert(s.cbegin() + 3, 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.insert(s.cend(), 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.insert(s.cbegin(), 5, 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s,
				[&] { s.insert(s.cbegin() + 3, 5, 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.insert(s.cend(), 5, 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.insert(s.cbegin(), {'a', 'b', 'c'});
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.insert(s.cbegin() + 3, {'a', 'b', 'c'});
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.insert(s.cend(), {'a', 'b', 'c'});
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.replace(s.cbegin(), s.cend(), "12345", 3);
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.replace(s.cbegin(), s.cbegin() + 3, "12345", 3);
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.replace(s.cend(), s.cend(), "12345", 3);
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.replace(s.cbegin(), s.cend(), "12345");
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.replace(s.cbegin(), s.cbegin() + 3, "12345");
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.replace(s.cend(), s.cend(), "12345");
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.replace(0, 3, 5, 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s,
				[&] { s.replace(3, s.size(), 5, 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s,
				[&] { s.replace(s.size(), s.size(), 5, 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.replace(s.cbegin(), s.cend(), 5, 'a');
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.replace(s.cbegin(), s.cbegin() + 3, 5, 'a');
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s,
				[&] { s.replace(s.cend(), s.cend(), 5, 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.replace(0, 3, "12345"); });
		verify_string(s, expected);

		assert_tx_abort(pop, s,
				[&] { s.replace(3, s.size(), "12345"); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.replace(s.size(), s.size(), "12345");
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.replace(0, 3, "12345", 3); });
		verify_string(s, expected);

		assert_tx_abort(pop, s,
				[&] { s.replace(3, s.size(), "12345", 3); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.replace(s.size(), s.size(), "12345", 3);
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.replace(s.cbegin(), s.cend(), {'a', 'b', 'c'});
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.replace(s.cbegin(), s.cbegin() + 3, {'a', 'b', 'c'});
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.replace(s.cend(), s.cend(), {'a', 'b', 'c'});
		});
		verify_string(s, expected);

		nvobj::transaction::run(pop, [&] {
			r->str = nvobj::make_persistent<S>("ABCDEF");
			r->str2 = nvobj::make_persistent<S>("ABCDEF");
		});

		auto &str = *r->str;
		auto &expected_str = *r->str2;

		assert_tx_abort(pop, s, [&] { s.append(str); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.append(str, 1); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.append(str, 1, 2); });
		verify_string(s, expected);

		assert_tx_abort(pop, s,
				[&] { s.append(str.begin(), str.end()); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s += str; });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.insert(0, str); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.insert(5, str); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.insert(s.size(), str); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.insert(0, str, 0); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.insert(5, str, 0); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.insert(s.size(), str, 0); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.insert(s.cbegin(), str.cbegin(), str.cend());
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.insert(s.cbegin() + 3, str.cbegin(), str.cend());
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.insert(s.cend(), str.cbegin(), str.cend());
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.replace(0, 3, str); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.replace(5, 3, str); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.replace(s.size(), 3, str); });
		verify_string(s, expected);

		assert_tx_abort(pop, s,
				[&] { s.replace(s.cbegin(), s.cend(), str); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.replace(s.cbegin(), s.cbegin() + 3, str);
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s,
				[&] { s.replace(s.cend(), s.cend(), str); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.replace(0, 3, str); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.replace(3, 5, str, 0); });
		verify_string(s, expected);

		assert_tx_abort(pop, s,
				[&] { s.replace(5, s.size(), str, 0); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.replace(s.cbegin(), s.cend(), str.cbegin(),
				  str.cend());
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.replace(s.cbegin(), s.cbegin() + 3, str.cbegin(),
				  str.cend());
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.replace(s.cend(), s.cend(), str.cbegin(), str.cend());
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.swap(str); });
		verify_string(s, expected);
		verify_string(str, expected_str);

		assert_tx_abort(pop, s, [&] { pmem::obj::swap(s, str); });
		verify_string(s, expected);
		verify_string(str, expected_str);

		assert_tx_abort(pop, s, [&] {
			s.free_data();
			s = "BEEF";
		});
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] {
			s.free_data();
			s = "BEEFBEEFBEEFBEEFBEEFBEEFBEEFBEEFBEEF"
			    "BEEFBEEFBEEFBEEFBEEFBEEFBEEFBEEFBEEF";
		});
		verify_string(s, expected);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	nvobj::transaction::run(pop, [&] {
		r->s->free_data();
		r->s1->free_data();
		r->str->free_data();
		r->str2->free_data();
		nvobj::delete_persistent<S>(r->s);
		nvobj::delete_persistent<S>(r->s1);
		nvobj::delete_persistent<S>(r->str);
		nvobj::delete_persistent<S>(r->str2);
	});
	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
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

	check_access_out_of_tx(pop, "0123456789");
	check_access_out_of_tx(pop,
			       "0123456789012345678901234567890123456789"
			       "0123456789012345678901234567890123456789"
			       "0123456789012345678901234567890123456789"
			       "0123456789");

	check_tx_abort(pop, "0123456789");
	check_tx_abort(pop,
		       "0123456789012345678901234567890123456789"
		       "0123456789012345678901234567890123456789"
		       "0123456789012345678901234567890123456789"
		       "0123456789");
	check_tx_abort(pop,
		       "0123456789012345678901234567890123456789"
		       "0123456789012345678901234567890123456789"
		       "0123456789012345678901234567890123456789"
		       "0123456789",
		       true);
	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
