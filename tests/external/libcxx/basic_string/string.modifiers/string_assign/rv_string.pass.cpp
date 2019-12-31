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
	nvobj::persistent_ptr<S> s, str;
	nvobj::persistent_ptr<S> s_arr[5];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &s1, const S &str1,
     const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->s = nvobj::make_persistent<S>(s1);
		r->str = nvobj::make_persistent<S>(str1);
	});

	auto &s = *r->s;
	auto &str = *r->str;

	s.assign(std::move(str));
	UT_ASSERT(s == expected);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<S>(r->s);
		nvobj::delete_persistent<S>(r->str);
	});
}

void
test_self_assignment(nvobj::pool<struct root> &pop, const S &s1,
		     const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<S>(s1); });

	auto &s = *r->s;

	s.assign(std::move(s));
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
				s_arr[2] =
					nvobj::make_persistent<S>("1234567890");
				s_arr[3] = nvobj::make_persistent<S>(
					"12345678901234567890");
				s_arr[4] = nvobj::make_persistent<S>(
					"12345678901234567890123456789012345678901234567890123456789012345678901234567890");
			});

			test(pop, *s_arr[0], *s_arr[0], *s_arr[0]);
			test(pop, *s_arr[0], *s_arr[1], *s_arr[1]);
			test(pop, *s_arr[0], *s_arr[2], *s_arr[2]);
			test(pop, *s_arr[0], *s_arr[3], *s_arr[3]);
			test(pop, *s_arr[0], *s_arr[4], *s_arr[4]);

			test(pop, *s_arr[1], *s_arr[0], *s_arr[0]);
			test(pop, *s_arr[1], *s_arr[1], *s_arr[1]);
			test(pop, *s_arr[1], *s_arr[2], *s_arr[2]);
			test(pop, *s_arr[1], *s_arr[3], *s_arr[3]);
			test(pop, *s_arr[1], *s_arr[4], *s_arr[4]);

			test(pop, *s_arr[2], *s_arr[0], *s_arr[0]);
			test(pop, *s_arr[2], *s_arr[1], *s_arr[1]);
			test(pop, *s_arr[2], *s_arr[2], *s_arr[2]);
			test(pop, *s_arr[2], *s_arr[3], *s_arr[3]);
			test(pop, *s_arr[2], *s_arr[4], *s_arr[4]);

			test(pop, *s_arr[3], *s_arr[0], *s_arr[0]);
			test(pop, *s_arr[3], *s_arr[1], *s_arr[1]);
			test(pop, *s_arr[3], *s_arr[2], *s_arr[2]);
			test(pop, *s_arr[3], *s_arr[3], *s_arr[3]);
			test(pop, *s_arr[3], *s_arr[4], *s_arr[4]);

			test_self_assignment(pop, *s_arr[3], *s_arr[3]);

			nvobj::transaction::run(pop, [&] {
				for (unsigned i = 0; i < 5; ++i) {
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
