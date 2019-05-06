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

#include <libpmemobj++/experimental/string.hpp>

namespace nvobj = pmem::obj;
namespace pmem_exp = pmem::obj::experimental;
using S = pmem_exp::string;

struct root {
	nvobj::persistent_ptr<S> s;
	nvobj::persistent_ptr<S> s_arr[13];
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

	if (n <= s.max_size()) {
		s.resize(n, c);
		UT_ASSERT(s == expected);
	} else {
		try {
			s.resize(n, c);
			UT_ASSERT(false);
		} catch (std::length_error &) {
			UT_ASSERT(n > s.max_size());
		}
	}

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

	auto r = pop.root();
	{
		auto &s_arr = r->s_arr;
		try {
			nvobj::transaction::run(pop, [&] {
				s_arr[0] = nvobj::make_persistent<S>();
				s_arr[1] = nvobj::make_persistent<S>("a");
				s_arr[2] = nvobj::make_persistent<S>(10U, 'a');
				s_arr[3] = nvobj::make_persistent<S>(100U, 'a');
				s_arr[4] = nvobj::make_persistent<S>("12345");
				s_arr[5] = nvobj::make_persistent<S>("12");
				s_arr[6] = nvobj::make_persistent<S>(
					"12345aaaaaaaaaa");
				s_arr[7] = nvobj::make_persistent<S>(
					"12345678901234567890123456789012345678901234567890");
				s_arr[8] =
					nvobj::make_persistent<S>("1234567890");
				s_arr[9] = nvobj::make_persistent<S>(
					"12345678901234567890123456789012345678901234567890aaaaaaaaaa");
				s_arr[10] = nvobj::make_persistent<S>(
					"not going to happen");
				s_arr[11] = nvobj::make_persistent<S>(
					"12345678901234567890123456789012345678901234567890aaaaaaaaaaaaaaa");
				s_arr[12] = nvobj::make_persistent<S>(
					"12345678901234567890123456789012345678901234567890aaaaaaaaaaaaaaaaaaaa");
			});

			/* sso to sso */
			test(pop, *s_arr[0], 0, 'a', *s_arr[0]);
			test(pop, *s_arr[0], 1, 'a', *s_arr[1]);
			test(pop, *s_arr[0], 10, 'a', *s_arr[2]);
			test(pop, *s_arr[4], 0, 'a', *s_arr[0]);
			test(pop, *s_arr[4], 2, 'a', *s_arr[5]);
			test(pop, *s_arr[4], 5, 'a', *s_arr[4]);
			test(pop, *s_arr[4], 15, 'a', *s_arr[6]);
			test(pop, *s_arr[7], 0, 'a', *s_arr[0]);
			test(pop, *s_arr[7], 10, 'a', *s_arr[8]);
			test(pop, *s_arr[7], 50, 'a', *s_arr[7]);
			test(pop, *s_arr[7], 60, 'a', *s_arr[9]);

			/* sso to large */
			test(pop, *s_arr[0], 100, 'a', *s_arr[3]);
			test(pop, *s_arr[7], 65, 'a', *s_arr[11]);
			test(pop, *s_arr[7], 70, 'a', *s_arr[12]);
			test(pop, *s_arr[0], S::npos, 'a', *s_arr[10]);

			/* large to sso */
			test(pop, *s_arr[12], 0, 'a', *s_arr[0]);
			test(pop, *s_arr[12], 2, 'a', *s_arr[5]);
			test(pop, *s_arr[12], 50, 'a', *s_arr[7]);

			/* large to large */
			test(pop, *s_arr[11], 70, 'a', *s_arr[12]);

			nvobj::transaction::run(pop, [&] {
				for (unsigned i = 0; i < 13; ++i) {
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
