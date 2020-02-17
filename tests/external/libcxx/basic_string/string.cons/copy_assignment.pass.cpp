//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019-2020, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <array>
#include <libpmemobj++/container/string.hpp>

namespace nvobj = pmem::obj;

using S = pmem::obj::string;

struct root {
	nvobj::persistent_ptr<S> s1;
	nvobj::persistent_ptr<S> s_arr[7];
};

template <class T>
void
test(nvobj::pool<struct root> &pop, const T &s, const S &s2)
{
	auto r = pop.root();
	nvobj::transaction::run(pop,
				[&] { r->s1 = nvobj::make_persistent<S>(s); });
	auto &s1 = *r->s1;

	s1 = s2;
	UT_ASSERT(s1 == s2);
	UT_ASSERT(s1.capacity() >= s1.size());

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<S>(r->s1); });
}

void
test_self_assignment(nvobj::pool<struct root> &pop, const S &s1)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s1 = nvobj::make_persistent<S>(s1); });

	/* Workaround for -Wself-assign-overloaded compile error */
	auto &s = *r->s1;

	s = *r->s1;
	UT_ASSERT(s == s1);
	UT_ASSERT(s.capacity() >= s1.size());

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<S>(r->s1); });
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "string_test", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto &s_arr = pop.root()->s_arr;

	try {
		nvobj::transaction::run(pop, [&] {
			s_arr[0] = nvobj::make_persistent<S>();
			s_arr[1] = nvobj::make_persistent<S>("1");
			s_arr[2] = nvobj::make_persistent<S>("2");
			s_arr[3] = nvobj::make_persistent<S>("123456789");
			s_arr[4] = nvobj::make_persistent<S>(
				"1234567890123456789012345678901234567890123456789012345678901234567890");
			s_arr[5] = nvobj::make_persistent<S>(
				"1234567890123456789012345678901234567890123456789012345678901234567890"
				"1234567890123456789012345678901234567890123456789012345678901234567890");
			s_arr[6] = nvobj::make_persistent<S>(
				"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
		});

		/* assign from pmem::string */
		test(pop, *s_arr[0], *s_arr[0]);
		test(pop, *s_arr[1], *s_arr[0]);
		test(pop, *s_arr[0], *s_arr[1]);
		test(pop, *s_arr[1], *s_arr[2]);
		test(pop, *s_arr[1], *s_arr[2]);

		test(pop, *s_arr[0], *s_arr[6]);
		test(pop, *s_arr[3], *s_arr[6]);
		test(pop, *s_arr[4], *s_arr[6]);
		test(pop, *s_arr[5], *s_arr[6]);

		test_self_assignment(pop, *s_arr[0]);
		test_self_assignment(pop, *s_arr[3]);

		/* assign from std::string */
		std::array<std::string, 7> std_str_arr = {
			std::string(),
			"1",
			"2",
			"123456789",
			"1234567890123456789012345678901234567890123456789012345678901234567890",
			"12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890",
			"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"};

		test(pop, std_str_arr[0], *s_arr[0]);
		test(pop, std_str_arr[1], *s_arr[0]);
		test(pop, std_str_arr[0], *s_arr[1]);
		test(pop, std_str_arr[1], *s_arr[2]);
		test(pop, std_str_arr[1], *s_arr[2]);

		test(pop, std_str_arr[0], *s_arr[6]);
		test(pop, std_str_arr[3], *s_arr[6]);
		test(pop, std_str_arr[4], *s_arr[6]);
		test(pop, std_str_arr[5], *s_arr[6]);

		nvobj::transaction::run(pop, [&] {
			for (unsigned i = 0; i < 7; ++i) {
				nvobj::delete_persistent<S>(s_arr[i]);
			}
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
