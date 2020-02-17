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
	nvobj::persistent_ptr<C> s, s_short, s_long, s_extra_long;
	nvobj::persistent_ptr<C> s_arr[54];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &s1, typename S::size_type pos,
     const typename S::value_type *str, const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<C>(s1); });

	auto &s = *r->s;

	const typename S::size_type old_size = s.size();

	if (pos <= old_size) {
		s.insert(pos, str);
		UT_ASSERT(s == expected);
	} else {
		try {
			s.insert(pos, str);
			UT_ASSERT(false);
		} catch (std::out_of_range &) {
			UT_ASSERT(pos > old_size);
			UT_ASSERT(s == s1);
		}
	}

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<C>(r->s); });
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
				s_arr[0] = nvobj::make_persistent<C>("");
				s_arr[1] = nvobj::make_persistent<C>("12345");
				s_arr[2] =
					nvobj::make_persistent<C>("1234567890");
				s_arr[3] = nvobj::make_persistent<C>(
					"12345678901234567890");
				s_arr[4] = nvobj::make_persistent<C>(
					"12345678901234567890abcde");
				s_arr[5] = nvobj::make_persistent<C>(
					"12345678901234567890abcdefghij");
				s_arr[6] = nvobj::make_persistent<C>(
					"12345678901234567890abcdefghijklmnopqrst");
				s_arr[7] = nvobj::make_persistent<C>(
					"1234567890abcde");
				s_arr[8] = nvobj::make_persistent<C>(
					"1234567890abcdefghij");
				s_arr[9] = nvobj::make_persistent<C>(
					"1234567890abcdefghijklmnopqrst");
				s_arr[10] =
					nvobj::make_persistent<C>("12345abcde");
				s_arr[11] = nvobj::make_persistent<C>(
					"12345abcdefghij");
				s_arr[12] = nvobj::make_persistent<C>(
					"12345abcdefghijklmnopqrst");
				s_arr[13] = nvobj::make_persistent<C>(
					"a12345678901234567890bcde");
				s_arr[14] = nvobj::make_persistent<C>(
					"a12345678901234567890bcdefghij");
				s_arr[15] = nvobj::make_persistent<C>(
					"a12345678901234567890bcdefghijklmnopqrst");
				s_arr[16] = nvobj::make_persistent<C>(
					"a1234567890bcde");
				s_arr[17] = nvobj::make_persistent<C>(
					"a1234567890bcdefghij");
				s_arr[18] = nvobj::make_persistent<C>(
					"a1234567890bcdefghijklmnopqrst");
				s_arr[19] =
					nvobj::make_persistent<C>("a12345bcde");
				s_arr[20] = nvobj::make_persistent<C>(
					"a12345bcdefghij");
				s_arr[21] = nvobj::make_persistent<C>(
					"a12345bcdefghijklmnopqrst");
				s_arr[22] = nvobj::make_persistent<C>(
					"ab12345678901234567890cde");
				s_arr[23] = nvobj::make_persistent<C>(
					"ab1234567890cde");
				s_arr[24] =
					nvobj::make_persistent<C>("ab12345cde");
				s_arr[25] = nvobj::make_persistent<C>(
					"abcd12345678901234567890e");
				s_arr[26] = nvobj::make_persistent<C>(
					"abcd1234567890e");
				s_arr[27] =
					nvobj::make_persistent<C>("abcd12345e");
				s_arr[28] = nvobj::make_persistent<C>("abcde");
				s_arr[29] =
					nvobj::make_persistent<C>("abcde12345");
				s_arr[30] = nvobj::make_persistent<C>(
					"abcde1234567890");
				s_arr[31] = nvobj::make_persistent<C>(
					"abcde12345678901234567890");
				s_arr[32] = nvobj::make_persistent<C>(
					"abcde12345678901234567890fghij");
				s_arr[33] = nvobj::make_persistent<C>(
					"abcde1234567890fghij");
				s_arr[34] = nvobj::make_persistent<C>(
					"abcde12345fghij");
				s_arr[35] = nvobj::make_persistent<C>(
					"abcdefghi12345678901234567890j");
				s_arr[36] = nvobj::make_persistent<C>(
					"abcdefghi1234567890j");
				s_arr[37] = nvobj::make_persistent<C>(
					"abcdefghi12345j");
				s_arr[38] =
					nvobj::make_persistent<C>("abcdefghij");
				s_arr[39] = nvobj::make_persistent<C>(
					"abcdefghij12345");
				s_arr[40] = nvobj::make_persistent<C>(
					"abcdefghij1234567890");
				s_arr[41] = nvobj::make_persistent<C>(
					"abcdefghij12345678901234567890");
				s_arr[42] = nvobj::make_persistent<C>(
					"abcdefghij12345678901234567890klmnopqrst");
				s_arr[43] = nvobj::make_persistent<C>(
					"abcdefghij1234567890klmnopqrst");
				s_arr[44] = nvobj::make_persistent<C>(
					"abcdefghij12345klmnopqrst");
				s_arr[45] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs12345678901234567890t");
				s_arr[46] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs1234567890t");
				s_arr[47] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs12345t");
				s_arr[48] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst");
				s_arr[49] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst12345");
				s_arr[50] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst1234567890");
				s_arr[51] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst12345678901234567890");
				s_arr[52] = nvobj::make_persistent<C>(
					"can't happen");
				s_arr[53] = nvobj::make_persistent<C>("abcde");
			});

			test(pop, *s_arr[0], 0, "", *s_arr[0]);
			test(pop, *s_arr[0], 0, "12345", *s_arr[1]);
			test(pop, *s_arr[0], 0, "1234567890", *s_arr[2]);
			test(pop, *s_arr[0], 0, "12345678901234567890",
			     *s_arr[3]);
			test(pop, *s_arr[0], 1, "", *s_arr[52]);
			test(pop, *s_arr[0], 1, "12345", *s_arr[52]);
			test(pop, *s_arr[0], 1, "1234567890", *s_arr[52]);
			test(pop, *s_arr[0], 1, "12345678901234567890",
			     *s_arr[52]);
			test(pop, *s_arr[28], 0, "", *s_arr[53]);
			test(pop, *s_arr[28], 0, "12345", *s_arr[10]);
			test(pop, *s_arr[28], 0, "1234567890", *s_arr[7]);
			test(pop, *s_arr[28], 0, "12345678901234567890",
			     *s_arr[4]);
			test(pop, *s_arr[28], 1, "", *s_arr[53]);
			test(pop, *s_arr[28], 1, "12345", *s_arr[19]);
			test(pop, *s_arr[28], 1, "1234567890", *s_arr[16]);
			test(pop, *s_arr[28], 1, "12345678901234567890",
			     *s_arr[13]);
			test(pop, *s_arr[28], 2, "", *s_arr[53]);
			test(pop, *s_arr[28], 2, "12345", *s_arr[24]);
			test(pop, *s_arr[28], 2, "1234567890", *s_arr[23]);
			test(pop, *s_arr[28], 2, "12345678901234567890",
			     *s_arr[22]);
			test(pop, *s_arr[28], 4, "", *s_arr[53]);
			test(pop, *s_arr[28], 4, "12345", *s_arr[27]);
			test(pop, *s_arr[28], 4, "1234567890", *s_arr[26]);
			test(pop, *s_arr[28], 4, "12345678901234567890",
			     *s_arr[25]);
			test(pop, *s_arr[28], 5, "", *s_arr[53]);
			test(pop, *s_arr[28], 5, "12345", *s_arr[29]);
			test(pop, *s_arr[28], 5, "1234567890", *s_arr[30]);
			test(pop, *s_arr[28], 5, "12345678901234567890",
			     *s_arr[31]);
			test(pop, *s_arr[28], 6, "", *s_arr[52]);
			test(pop, *s_arr[28], 6, "12345", *s_arr[52]);
			test(pop, *s_arr[28], 6, "1234567890", *s_arr[52]);
			test(pop, *s_arr[28], 6, "12345678901234567890",
			     *s_arr[52]);
			test(pop, *s_arr[38], 0, "", *s_arr[38]);
			test(pop, *s_arr[38], 0, "12345", *s_arr[11]);
			test(pop, *s_arr[38], 0, "1234567890", *s_arr[8]);
			test(pop, *s_arr[38], 0, "12345678901234567890",
			     *s_arr[5]);
			test(pop, *s_arr[38], 1, "", *s_arr[38]);
			test(pop, *s_arr[38], 1, "12345", *s_arr[20]);
			test(pop, *s_arr[38], 1, "1234567890", *s_arr[17]);
			test(pop, *s_arr[38], 1, "12345678901234567890",
			     *s_arr[14]);
			test(pop, *s_arr[38], 5, "", *s_arr[38]);
			test(pop, *s_arr[38], 5, "12345", *s_arr[34]);
			test(pop, *s_arr[38], 5, "1234567890", *s_arr[33]);
			test(pop, *s_arr[38], 5, "12345678901234567890",
			     *s_arr[32]);
			test(pop, *s_arr[38], 9, "", *s_arr[38]);
			test(pop, *s_arr[38], 9, "12345", *s_arr[37]);
			test(pop, *s_arr[38], 9, "1234567890", *s_arr[36]);
			test(pop, *s_arr[38], 9, "12345678901234567890",
			     *s_arr[35]);
			test(pop, *s_arr[38], 10, "", *s_arr[38]);
			test(pop, *s_arr[38], 10, "12345", *s_arr[39]);
			test(pop, *s_arr[38], 10, "1234567890", *s_arr[40]);
			test(pop, *s_arr[38], 10, "12345678901234567890",
			     *s_arr[41]);
			test(pop, *s_arr[38], 11, "", *s_arr[52]);
			test(pop, *s_arr[38], 11, "12345", *s_arr[52]);
			test(pop, *s_arr[38], 11, "1234567890", *s_arr[52]);
			test(pop, *s_arr[38], 11, "12345678901234567890",
			     *s_arr[52]);
			test(pop, *s_arr[48], 0, "", *s_arr[48]);
			test(pop, *s_arr[48], 0, "12345", *s_arr[12]);
			test(pop, *s_arr[48], 0, "1234567890", *s_arr[9]);
			test(pop, *s_arr[48], 0, "12345678901234567890",
			     *s_arr[6]);
			test(pop, *s_arr[48], 1, "", *s_arr[48]);
			test(pop, *s_arr[48], 1, "12345", *s_arr[21]);
			test(pop, *s_arr[48], 1, "1234567890", *s_arr[18]);
			test(pop, *s_arr[48], 1, "12345678901234567890",
			     *s_arr[15]);
			test(pop, *s_arr[48], 10, "", *s_arr[48]);
			test(pop, *s_arr[48], 10, "12345", *s_arr[44]);
			test(pop, *s_arr[48], 10, "1234567890", *s_arr[43]);
			test(pop, *s_arr[48], 10, "12345678901234567890",
			     *s_arr[42]);
			test(pop, *s_arr[48], 19, "", *s_arr[48]);
			test(pop, *s_arr[48], 19, "12345", *s_arr[47]);
			test(pop, *s_arr[48], 19, "1234567890", *s_arr[46]);
			test(pop, *s_arr[48], 19, "12345678901234567890",
			     *s_arr[45]);
			test(pop, *s_arr[48], 20, "", *s_arr[48]);
			test(pop, *s_arr[48], 20, "12345", *s_arr[49]);
			test(pop, *s_arr[48], 20, "1234567890", *s_arr[50]);
			test(pop, *s_arr[48], 20, "12345678901234567890",
			     *s_arr[51]);
			test(pop, *s_arr[48], 21, "", *s_arr[52]);
			test(pop, *s_arr[48], 21, "12345", *s_arr[52]);
			test(pop, *s_arr[48], 21, "1234567890", *s_arr[52]);
			test(pop, *s_arr[48], 21, "12345678901234567890",
			     *s_arr[52]);

			nvobj::transaction::run(pop, [&] {
				for (unsigned i = 0; i < 54; ++i) {
					nvobj::delete_persistent<C>(s_arr[i]);
				}
			});

			nvobj::transaction::run(pop, [&] {
				r->s_short = nvobj::make_persistent<C>("123/");
				r->s_long = nvobj::make_persistent<C>(
					"Lorem ipsum dolor sit amet, consectetur/");
				r->s_extra_long = nvobj::make_persistent<C>(
					"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod/");
			});

			auto &s_short = *r->s_short;
			auto &s_long = *r->s_long;
			auto &s_extra_long = *r->s_extra_long;

			s_short.insert(0, s_short.c_str());
			UT_ASSERT(s_short == "123/123/");
			s_short.insert(0, s_short.c_str());
			UT_ASSERT(s_short == "123/123/123/123/");
			s_short.insert(0, s_short.c_str());
			UT_ASSERT(s_short ==
				  "123/123/123/123/123/123/123/123/");

			s_long.insert(0, s_long.c_str());
			UT_ASSERT(
				s_long ==
				"Lorem ipsum dolor sit amet, consectetur/Lorem ipsum dolor sit amet, consectetur/");

			s_extra_long.insert(0, s_extra_long.c_str());
			UT_ASSERT(
				s_extra_long ==
				"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod/Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod/");

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C>(r->s_short);
				nvobj::delete_persistent<C>(r->s_long);
				nvobj::delete_persistent<C>(r->s_extra_long);
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
