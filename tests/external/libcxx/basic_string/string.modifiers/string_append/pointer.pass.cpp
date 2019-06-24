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
	nvobj::persistent_ptr<S> s, s_short, s_long, s_extra_long;
	nvobj::persistent_ptr<S> s_arr[9];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &s1,
     const typename S::value_type *str, const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<S>(s1); });

	auto &s = *r->s;

	s.append(str);
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

	auto r = pop.root();
	{
		auto &s_arr = r->s_arr;

		try {
			nvobj::transaction::run(pop, [&] {
				s_arr[0] = nvobj::make_persistent<S>();
				s_arr[1] = nvobj::make_persistent<S>("12345");
				s_arr[2] = nvobj::make_persistent<S>(
					"12345678901234567890");
				s_arr[3] =
					nvobj::make_persistent<S>("1234512345");
				s_arr[4] = nvobj::make_persistent<S>(
					"123451234567890");
				s_arr[5] = nvobj::make_persistent<S>(
					"1234567890123456789012345");
				s_arr[6] = nvobj::make_persistent<S>(
					"1234567890123456789012345678901234567890");
				s_arr[7] = nvobj::make_persistent<S>(
					"12345678901234567890123456789012345678901234567890123456789012345");
				s_arr[8] = nvobj::make_persistent<S>(
					"1234567890123456789012345678901234567890123456789012345678901234567890");
				r->s_short = nvobj::make_persistent<S>("123/");
				r->s_long = nvobj::make_persistent<S>(
					"Lorem ipsum dolor sit amet, consectetur/");
				r->s_extra_long = nvobj::make_persistent<S>(
					"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod/");
			});

			test(pop, *s_arr[0], "", *s_arr[0]);
			test(pop, *s_arr[0], "12345", *s_arr[1]);
			test(pop, *s_arr[0], "12345678901234567890", *s_arr[2]);

			test(pop, *s_arr[1], "", *s_arr[1]);
			test(pop, *s_arr[1], "12345", *s_arr[3]);
			test(pop, *s_arr[1], "1234567890", *s_arr[4]);

			test(pop, *s_arr[2], "", *s_arr[2]);
			test(pop, *s_arr[2], "12345", *s_arr[5]);
			test(pop, *s_arr[2], "12345678901234567890", *s_arr[6]);

			test(pop, *s_arr[7], "", *s_arr[7]);
			test(pop, *s_arr[7], "67890", *s_arr[8]);
			test(pop, *s_arr[6], "1234567890123456789012345",
			     *s_arr[7]);

			// test appending to self
			auto &s_short = *r->s_short;
			auto &s_long = *r->s_long;
			auto &s_extra_long = *r->s_extra_long;

			s_short.append(s_short.c_str());
			UT_ASSERT(s_short == "123/123/");
			s_short.append(s_short.c_str());
			UT_ASSERT(s_short == "123/123/123/123/");
			s_short.append(s_short.c_str());
			UT_ASSERT(s_short ==
				  "123/123/123/123/123/123/123/123/");

			s_long.append(s_long.c_str());
			UT_ASSERT(
				s_long ==
				"Lorem ipsum dolor sit amet, consectetur/Lorem ipsum dolor sit amet, consectetur/");

			s_extra_long.append(s_extra_long.c_str());
			UT_ASSERT(
				s_extra_long ==
				"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod/Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod/");

			nvobj::transaction::run(pop, [&] {
				for (unsigned i = 0; i < 9; ++i) {
					nvobj::delete_persistent<S>(s_arr[i]);
				}
				nvobj::delete_persistent<S>(r->s_short);
				nvobj::delete_persistent<S>(r->s_long);
				nvobj::delete_persistent<S>(r->s_extra_long);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}

	pop.close();

	return 0;
}
