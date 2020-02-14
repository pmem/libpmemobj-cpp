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
	nvobj::persistent_ptr<S> s;
	nvobj::persistent_ptr<S> s_arr[17];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &s1, typename S::size_type n,
     const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<S>(s1); });
	auto &s = *r->s;

	if (n <= s.max_size()) {
		s.resize(n);
		UT_ASSERT(s == expected);
	} else {
		try {
			s.resize(n);
			UT_ASSERT(false);
		} catch (std::length_error &) {
			UT_ASSERT(n > s.max_size());
		}
	}

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
				s_arr[0] = nvobj::make_persistent<S>();
				s_arr[1] = nvobj::make_persistent<S>("12345");
				s_arr[2] =
					nvobj::make_persistent<S>("1234567890");
				s_arr[3] = nvobj::make_persistent<S>(
					"12345678901234567890123456789012345678901234567890");
				s_arr[4] = nvobj::make_persistent<S>(
					"not going to happen");
				s_arr[5] = nvobj::make_persistent<S>(1U, '\0');
				s_arr[6] = nvobj::make_persistent<S>(10U, '\0');
				s_arr[7] =
					nvobj::make_persistent<S>(100U, '\0');
				s_arr[8] = nvobj::make_persistent<S>("12");
				s_arr[9] = nvobj::make_persistent<S>(
					"12345\0\0\0\0\0\0\0\0\0\0", 15U);
				s_arr[10] = nvobj::make_persistent<S>(
					"12345678901234567890123456789012345678901234567890\0\0\0\0\0\0\0\0\0\0",
					60U);

				s_arr[11] = nvobj::make_persistent<S>(
					"123456789012345678901234567890123456789012345678901234567890123");
				s_arr[12] = nvobj::make_persistent<S>(
					"123456789012345678901234567890123456789012345678901234567890123\0\0\0\0\0\0\0\0\0\0",
					73U);
				s_arr[13] = nvobj::make_persistent<S>(
					"12345678901234567890123456789012345678901234567890123456789012345678901234567890");
				s_arr[14] = nvobj::make_persistent<S>(
					"1234567890123456789012345678901234567890123456789012345678901234");
				s_arr[15] = nvobj::make_persistent<S>(
					"123456789012345678901234567890123456789012345678901234567890123\0",
					64U);
				s_arr[16] = nvobj::make_persistent<S>(
					"1234567890123456789012345678901234567890123456789012345678901234\0\0\0\0\0\0\0\0\0\0",
					74U);
			});

			/* sso to sso */
			test(pop, *s_arr[0], 0, *s_arr[0]);

			test(pop, *s_arr[0], 1, *s_arr[5]);
			test(pop, *s_arr[0], 10, *s_arr[6]);

			test(pop, *s_arr[1], 0, *s_arr[0]);
			test(pop, *s_arr[1], 2, *s_arr[8]);
			test(pop, *s_arr[1], 5, *s_arr[1]);
			test(pop, *s_arr[1], 15, *s_arr[9]);

			test(pop, *s_arr[3], 0, *s_arr[0]);
			test(pop, *s_arr[3], 10, *s_arr[2]);
			test(pop, *s_arr[3], 50, *s_arr[3]);
			test(pop, *s_arr[3], 60, *s_arr[10]);

			test(pop, *s_arr[0], S::npos, *s_arr[4]);

			/* sso to large */
			test(pop, *s_arr[0], 100, *s_arr[7]);
			test(pop, *s_arr[11], 73, *s_arr[12]);
			test(pop, *s_arr[11], 64, *s_arr[15]);

			/* large to sso */
			test(pop, *s_arr[13], 63, *s_arr[11]);
			test(pop, *s_arr[14], 63, *s_arr[11]);
			test(pop, *s_arr[13], 50, *s_arr[3]);
			test(pop, *s_arr[13], 0, *s_arr[0]);

			/* large to large */
			test(pop, *s_arr[13], 64, *s_arr[14]);
			test(pop, *s_arr[14], 74, *s_arr[16]);

			nvobj::transaction::run(pop, [&] {
				for (unsigned i = 0; i < 17; ++i) {
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
