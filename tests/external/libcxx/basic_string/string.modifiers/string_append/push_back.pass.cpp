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
	nvobj::persistent_ptr<S> s_arr[6];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &s1, typename S::value_type c,
     const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<S>(s1); });

	auto &s = *r->s;

	s.push_back(c);
	UT_ASSERT(s == expected);

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
				s_arr[1] = nvobj::make_persistent<S>(1U, 'a');
				s_arr[2] = nvobj::make_persistent<S>("12345");
				s_arr[3] = nvobj::make_persistent<S>("12345a");
				s_arr[4] = nvobj::make_persistent<S>(
					"12345678901234567890");
				s_arr[5] = nvobj::make_persistent<S>(
					"12345678901234567890a");
			});

			test(pop, *s_arr[0], 'a', *s_arr[1]);
			test(pop, *s_arr[2], 'a', *s_arr[3]);
			test(pop, *s_arr[4], 'a', *s_arr[5]);

			nvobj::transaction::run(pop, [&] {
				for (unsigned i = 0; i < 6; ++i) {
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
