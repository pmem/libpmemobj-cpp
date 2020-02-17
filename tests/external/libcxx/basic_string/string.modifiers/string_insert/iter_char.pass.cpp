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

using C = pmem::obj::string;

struct root {
	nvobj::persistent_ptr<C> s;
	nvobj::persistent_ptr<C> s_arr[14];
};

template <class S>
void
test(S &s, typename S::const_iterator p, typename S::value_type c,
     const S &expected)
{
	bool sufficient_cap = s.size() < s.capacity();
	typename S::difference_type pos = p - s.begin();
	typename S::iterator i = s.insert(p, c);
	UT_ASSERT(s == expected);
	UT_ASSERT(i - s.begin() == pos);
	UT_ASSERT(*i == c);
	if (sufficient_cap)
		UT_ASSERT(i == p);
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

	auto &s_arr = r->s_arr;

	try {
		nvobj::transaction::run(pop, [&] {
			r->s = nvobj::make_persistent<C>();
			s_arr[0] = nvobj::make_persistent<C>("1");
			s_arr[1] = nvobj::make_persistent<C>("a1");
			s_arr[2] = nvobj::make_persistent<C>("a1b");
			s_arr[3] = nvobj::make_persistent<C>("a1cb");
			s_arr[4] = nvobj::make_persistent<C>("a1dcb");
			s_arr[5] = nvobj::make_persistent<C>("a12dcb");
			s_arr[6] = nvobj::make_persistent<C>("a132dcb");
			s_arr[7] = nvobj::make_persistent<C>("a1432dcb");
			s_arr[8] = nvobj::make_persistent<C>("a51432dcb");
			s_arr[9] = nvobj::make_persistent<C>("a561432dcb");
			s_arr[10] = nvobj::make_persistent<C>("a5671432dcb");
			s_arr[11] = nvobj::make_persistent<C>("a567A1432dcb");
			s_arr[12] = nvobj::make_persistent<C>("a567AB1432dcb");
			s_arr[13] = nvobj::make_persistent<C>("a567ABC1432dcb");
		});

		auto &s = *r->s;

		test(s, s.begin(), '1', *s_arr[0]);
		test(s, s.begin(), 'a', *s_arr[1]);
		test(s, s.end(), 'b', *s_arr[2]);
		test(s, s.end() - 1, 'c', *s_arr[3]);
		test(s, s.end() - 2, 'd', *s_arr[4]);
		test(s, s.end() - 3, '2', *s_arr[5]);
		test(s, s.end() - 4, '3', *s_arr[6]);
		test(s, s.end() - 5, '4', *s_arr[7]);
		test(s, s.begin() + 1, '5', *s_arr[8]);
		test(s, s.begin() + 2, '6', *s_arr[9]);
		test(s, s.begin() + 3, '7', *s_arr[10]);
		test(s, s.begin() + 4, 'A', *s_arr[11]);
		test(s, s.begin() + 5, 'B', *s_arr[12]);
		test(s, s.begin() + 6, 'C', *s_arr[13]);

		nvobj::transaction::run(pop, [&] {
			for (unsigned i = 0; i < 14; ++i) {
				nvobj::delete_persistent<C>(s_arr[i]);
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
