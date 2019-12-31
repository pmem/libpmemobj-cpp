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
	nvobj::persistent_ptr<C> str, s_arr[3];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &str1, typename S::value_type *s,
     typename S::size_type n, typename S::size_type pos)
{
	auto r = pop.root();

	nvobj::transaction::run(
		pop, [&] { r->str = nvobj::make_persistent<S>(str1); });

	auto &str = *r->str;

	const S &cs = str;
	if (pos <= cs.size()) {
		typename S::size_type r = cs.copy(s, n, pos);
		typename S::size_type rlen = (std::min)(n, cs.size() - pos);
		UT_ASSERT(r == rlen);
		for (r = 0; r < rlen; ++r)
			UT_ASSERT(S::traits_type::eq(cs[pos + r], s[r]));
	} else {
		try {
			typename S::size_type r = cs.copy(s, n, pos);
			((void)r); // Prevent unused warning
			UT_ASSERT(false);
		} catch (std::out_of_range &) {
			UT_ASSERT(pos > str.size());
		}
	}

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<S>(r->str); });
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

	char s[50];

	auto r = pop.root();
	auto &s_arr = r->s_arr;

	try {
		nvobj::transaction::run(pop, [&] {
			s_arr[0] = nvobj::make_persistent<C>("");
			s_arr[1] = nvobj::make_persistent<C>("abcde");
			s_arr[2] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst");
		});

		test(pop, *s_arr[0], s, 0, 0);
		test(pop, *s_arr[0], s, 0, 1);
		test(pop, *s_arr[0], s, 1, 0);
		test(pop, *s_arr[1], s, 0, 0);
		test(pop, *s_arr[1], s, 0, 1);
		test(pop, *s_arr[1], s, 0, 2);
		test(pop, *s_arr[1], s, 0, 4);
		test(pop, *s_arr[1], s, 0, 5);
		test(pop, *s_arr[1], s, 0, 6);
		test(pop, *s_arr[1], s, 1, 0);
		test(pop, *s_arr[1], s, 1, 1);
		test(pop, *s_arr[1], s, 1, 2);
		test(pop, *s_arr[1], s, 1, 4);
		test(pop, *s_arr[1], s, 1, 5);
		test(pop, *s_arr[1], s, 2, 0);
		test(pop, *s_arr[1], s, 2, 1);
		test(pop, *s_arr[1], s, 2, 2);
		test(pop, *s_arr[1], s, 2, 4);
		test(pop, *s_arr[1], s, 4, 0);
		test(pop, *s_arr[1], s, 4, 1);
		test(pop, *s_arr[1], s, 4, 2);
		test(pop, *s_arr[1], s, 5, 0);
		test(pop, *s_arr[1], s, 5, 1);
		test(pop, *s_arr[1], s, 6, 0);
		test(pop, *s_arr[2], s, 0, 0);
		test(pop, *s_arr[2], s, 0, 1);
		test(pop, *s_arr[2], s, 0, 2);
		test(pop, *s_arr[2], s, 0, 10);
		test(pop, *s_arr[2], s, 0, 19);
		test(pop, *s_arr[2], s, 0, 20);
		test(pop, *s_arr[2], s, 0, 21);
		test(pop, *s_arr[2], s, 1, 0);
		test(pop, *s_arr[2], s, 1, 1);
		test(pop, *s_arr[2], s, 1, 2);
		test(pop, *s_arr[2], s, 1, 9);
		test(pop, *s_arr[2], s, 1, 18);
		test(pop, *s_arr[2], s, 1, 19);
		test(pop, *s_arr[2], s, 1, 20);
		test(pop, *s_arr[2], s, 2, 0);
		test(pop, *s_arr[2], s, 2, 1);
		test(pop, *s_arr[2], s, 2, 2);
		test(pop, *s_arr[2], s, 2, 9);
		test(pop, *s_arr[2], s, 2, 17);
		test(pop, *s_arr[2], s, 2, 18);
		test(pop, *s_arr[2], s, 2, 19);
		test(pop, *s_arr[2], s, 10, 0);
		test(pop, *s_arr[2], s, 10, 1);
		test(pop, *s_arr[2], s, 10, 2);
		test(pop, *s_arr[2], s, 10, 5);
		test(pop, *s_arr[2], s, 10, 9);
		test(pop, *s_arr[2], s, 10, 10);
		test(pop, *s_arr[2], s, 10, 11);
		test(pop, *s_arr[2], s, 19, 0);
		test(pop, *s_arr[2], s, 19, 1);
		test(pop, *s_arr[2], s, 19, 2);
		test(pop, *s_arr[2], s, 20, 0);
		test(pop, *s_arr[2], s, 20, 1);
		test(pop, *s_arr[2], s, 21, 0);

		nvobj::transaction::run(pop, [&] {
			for (unsigned i = 0; i < 3; ++i) {
				nvobj::delete_persistent<C>(s_arr[i]);
			}
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
