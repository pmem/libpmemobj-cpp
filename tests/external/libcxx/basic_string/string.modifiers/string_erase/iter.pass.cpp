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

#include <libpmemobj++/container/string.hpp>

namespace nvobj = pmem::obj;

using S = pmem::obj::string;

struct root {
	nvobj::persistent_ptr<S> s, s_arr[22];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &s1,
     typename S::difference_type pos, const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<S>(s1); });

	auto &s = *r->s;

	typename S::const_iterator p = s.begin() + pos;
	typename S::iterator i = s.erase(p);

	UT_ASSERT(s[s.size()] == typename S::value_type());
	UT_ASSERT(s == expected);
	UT_ASSERT(i - s.begin() == pos);

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
		path, "string_test", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();
	{
		auto &s_arr = r->s_arr;
		try {
			nvobj::transaction::run(pop, [&] {
				s_arr[0] = nvobj::make_persistent<S>("abcde");
				s_arr[1] = nvobj::make_persistent<S>("bcde");
				s_arr[2] = nvobj::make_persistent<S>("acde");
				s_arr[3] = nvobj::make_persistent<S>("abde");
				s_arr[4] = nvobj::make_persistent<S>("abcd");

				s_arr[5] =
					nvobj::make_persistent<S>("abcdefghij");
				s_arr[6] =
					nvobj::make_persistent<S>("bcdefghij");
				s_arr[7] =
					nvobj::make_persistent<S>("acdefghij");
				s_arr[8] =
					nvobj::make_persistent<S>("abcdeghij");
				s_arr[9] =
					nvobj::make_persistent<S>("abcdefghi");

				s_arr[10] = nvobj::make_persistent<S>(
					"abcdefghijklmnopqrst");
				s_arr[11] = nvobj::make_persistent<S>(
					"bcdefghijklmnopqrst");
				s_arr[12] = nvobj::make_persistent<S>(
					"acdefghijklmnopqrst");
				s_arr[13] = nvobj::make_persistent<S>(
					"abcdefghijlmnopqrst");
				s_arr[14] = nvobj::make_persistent<S>(
					"abcdefghijklmnopqrs");

				s_arr[15] = nvobj::make_persistent<S>(
					"0123456789012345678901234567890123456789012345678901234567890123456789");
				s_arr[16] = nvobj::make_persistent<S>(
					"123456789012345678901234567890123456789012345678901234567890123456789");
				s_arr[17] = nvobj::make_persistent<S>(
					"023456789012345678901234567890123456789012345678901234567890123456789");
				s_arr[18] = nvobj::make_persistent<S>(
					"012345678901234567891234567890123456789012345678901234567890123456789");
				s_arr[19] = nvobj::make_persistent<S>(
					"012345678901234567890123456789012345678901234567890123456789012345678");

				s_arr[20] = nvobj::make_persistent<S>(
					"0123456789012345678901234567890123456789012345678901234567890123");
				s_arr[21] = nvobj::make_persistent<S>(
					"123456789012345678901234567890123456789012345678901234567890123");
			});

			test(pop, *s_arr[0], 0, *s_arr[1]);
			test(pop, *s_arr[0], 1, *s_arr[2]);
			test(pop, *s_arr[0], 2, *s_arr[3]);
			test(pop, *s_arr[0], 4, *s_arr[4]);

			test(pop, *s_arr[5], 0, *s_arr[6]);
			test(pop, *s_arr[5], 1, *s_arr[7]);
			test(pop, *s_arr[5], 5, *s_arr[8]);
			test(pop, *s_arr[5], 9, *s_arr[9]);

			test(pop, *s_arr[10], 0, *s_arr[11]);
			test(pop, *s_arr[10], 1, *s_arr[12]);
			test(pop, *s_arr[10], 10, *s_arr[13]);
			test(pop, *s_arr[10], 19, *s_arr[14]);

			test(pop, *s_arr[15], 0, *s_arr[16]);
			test(pop, *s_arr[15], 1, *s_arr[17]);
			test(pop, *s_arr[15], 20, *s_arr[18]);
			test(pop, *s_arr[15], 69, *s_arr[19]);

			test(pop, *s_arr[20], 0, *s_arr[21]);

			nvobj::transaction::run(pop, [&] {
				for (unsigned i = 0; i < 22; ++i) {
					nvobj::delete_persistent<S>(s_arr[i]);
				}
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
