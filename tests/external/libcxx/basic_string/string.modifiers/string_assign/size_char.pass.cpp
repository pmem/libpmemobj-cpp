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

using S = pmem::obj::string;

struct root {
	nvobj::persistent_ptr<S> s;
	nvobj::persistent_ptr<S> s_arr[6];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &s1, typename S::size_type n,
     typename S::value_type c, const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<S>(s1); });

	auto &s = *r->s;

	s.assign(n, c);
	UT_ASSERT(s == expected);

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<S>(r->s); });
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

	auto &s_arr = pop.root()->s_arr;
	{
		try {
			nvobj::transaction::run(pop, [&] {
				s_arr[0] = nvobj::make_persistent<S>();
				s_arr[1] = nvobj::make_persistent<S>("12345");
				s_arr[2] = nvobj::make_persistent<S>(
					"12345678901234567890");
				s_arr[3] = nvobj::make_persistent<S>(1U, 'a');
				s_arr[4] = nvobj::make_persistent<S>(10U, 'a');
				s_arr[5] = nvobj::make_persistent<S>(100U, 'a');
			});

			test(pop, *s_arr[0], 0, 'a', *s_arr[0]);
			test(pop, *s_arr[0], 1, 'a', *s_arr[3]);
			test(pop, *s_arr[0], 10, 'a', *s_arr[4]);
			test(pop, *s_arr[0], 100, 'a', *s_arr[5]);

			test(pop, *s_arr[1], 0, 'a', *s_arr[0]);
			test(pop, *s_arr[1], 1, 'a', *s_arr[3]);
			test(pop, *s_arr[1], 10, 'a', *s_arr[4]);

			test(pop, *s_arr[2], 0, 'a', *s_arr[0]);
			test(pop, *s_arr[2], 1, 'a', *s_arr[3]);
			test(pop, *s_arr[2], 10, 'a', *s_arr[4]);

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

	return 0;
}
