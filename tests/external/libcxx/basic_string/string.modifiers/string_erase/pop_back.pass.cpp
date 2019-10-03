//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/container/string.hpp>

namespace nvobj = pmem::obj;

using C = pmem::obj::string;

struct root {
	nvobj::persistent_ptr<C> s, s_arr[6];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &s1, const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<C>(s1); });

	auto &s = *r->s;

	s.pop_back();
	UT_ASSERT(s[s.size()] == typename S::value_type());
	UT_ASSERT(s == expected);

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<C>(r->s); });
}

int
main(int argc, char *argv[])
{
	START();

	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
		return 1;
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "string_test", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();
	{
		auto &s_arr = r->s_arr;
		try {
			nvobj::transaction::run(pop, [&] {
				s_arr[0] = nvobj::make_persistent<C>("abcde");
				s_arr[1] = nvobj::make_persistent<C>("abcd");
				s_arr[2] =
					nvobj::make_persistent<C>("abcdefghij");
				s_arr[3] =
					nvobj::make_persistent<C>("abcdefghi");
				s_arr[4] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst");
				s_arr[5] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs");
			});

			test(pop, *s_arr[0], *s_arr[1]);
			test(pop, *s_arr[2], *s_arr[3]);
			test(pop, *s_arr[4], *s_arr[5]);

			nvobj::transaction::run(pop, [&] {
				for (unsigned i = 0; i < 6; ++i) {
					nvobj::delete_persistent<C>(s_arr[i]);
				}
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}

	pop.close();

	return 0;
}
