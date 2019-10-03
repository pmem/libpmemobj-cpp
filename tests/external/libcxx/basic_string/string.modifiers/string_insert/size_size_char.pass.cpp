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
	nvobj::persistent_ptr<C> s;
	nvobj::persistent_ptr<C> s_arr[53];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &s1, typename S::size_type pos,
     typename S::size_type n, typename S::value_type str, const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<C>(s1); });

	auto &s = *r->s;

	const typename S::size_type old_size = s.size();

	if (pos <= old_size) {
		s.insert(pos, n, str);
		UT_ASSERT(s == expected);
	} else {
		try {
			s.insert(pos, n, str);
			UT_ASSERT(false);
		} catch (std::out_of_range &) {
			UT_ASSERT(pos > old_size);
			UT_ASSERT(s == s1);
		}
	}

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
	auto &s_arr = r->s_arr;

	try {
		nvobj::transaction::run(pop, [&] {
			s_arr[0] = nvobj::make_persistent<C>("");
			s_arr[1] = nvobj::make_persistent<C>("11111");
			s_arr[2] = nvobj::make_persistent<C>("1111111111");
			s_arr[3] = nvobj::make_persistent<C>(
				"11111111111111111111");
			s_arr[4] = nvobj::make_persistent<C>(
				"11111111111111111111abcde");
			s_arr[5] = nvobj::make_persistent<C>(
				"11111111111111111111abcdefghij");
			s_arr[6] = nvobj::make_persistent<C>(
				"11111111111111111111abcdefghijklmnopqrst");
			s_arr[7] = nvobj::make_persistent<C>("1111111111abcde");
			s_arr[8] = nvobj::make_persistent<C>(
				"1111111111abcdefghij");
			s_arr[9] = nvobj::make_persistent<C>(
				"1111111111abcdefghijklmnopqrst");
			s_arr[10] = nvobj::make_persistent<C>("11111abcde");
			s_arr[11] =
				nvobj::make_persistent<C>("11111abcdefghij");
			s_arr[12] = nvobj::make_persistent<C>(
				"11111abcdefghijklmnopqrst");
			s_arr[13] = nvobj::make_persistent<C>(
				"a11111111111111111111bcde");
			s_arr[14] = nvobj::make_persistent<C>(
				"a11111111111111111111bcdefghij");
			s_arr[15] = nvobj::make_persistent<C>(
				"a11111111111111111111bcdefghijklmnopqrst");
			s_arr[16] =
				nvobj::make_persistent<C>("a1111111111bcde");
			s_arr[17] = nvobj::make_persistent<C>(
				"a1111111111bcdefghij");
			s_arr[18] = nvobj::make_persistent<C>(
				"a1111111111bcdefghijklmnopqrst");
			s_arr[19] = nvobj::make_persistent<C>("a11111bcde");
			s_arr[20] =
				nvobj::make_persistent<C>("a11111bcdefghij");
			s_arr[21] = nvobj::make_persistent<C>(
				"a11111bcdefghijklmnopqrst");
			s_arr[22] = nvobj::make_persistent<C>(
				"ab11111111111111111111cde");
			s_arr[23] =
				nvobj::make_persistent<C>("ab1111111111cde");
			s_arr[24] = nvobj::make_persistent<C>("ab11111cde");
			s_arr[25] = nvobj::make_persistent<C>(
				"abcd11111111111111111111e");
			s_arr[26] =
				nvobj::make_persistent<C>("abcd1111111111e");
			s_arr[27] = nvobj::make_persistent<C>("abcd11111e");
			s_arr[28] = nvobj::make_persistent<C>("abcde");
			s_arr[29] = nvobj::make_persistent<C>("abcde11111");
			s_arr[30] =
				nvobj::make_persistent<C>("abcde1111111111");
			s_arr[31] = nvobj::make_persistent<C>(
				"abcde11111111111111111111");
			s_arr[32] = nvobj::make_persistent<C>(
				"abcde11111111111111111111fghij");
			s_arr[33] = nvobj::make_persistent<C>(
				"abcde1111111111fghij");
			s_arr[34] =
				nvobj::make_persistent<C>("abcde11111fghij");
			s_arr[35] = nvobj::make_persistent<C>(
				"abcdefghi11111111111111111111j");
			s_arr[36] = nvobj::make_persistent<C>(
				"abcdefghi1111111111j");
			s_arr[37] =
				nvobj::make_persistent<C>("abcdefghi11111j");
			s_arr[38] = nvobj::make_persistent<C>("abcdefghij");
			s_arr[39] =
				nvobj::make_persistent<C>("abcdefghij11111");
			s_arr[40] = nvobj::make_persistent<C>(
				"abcdefghij1111111111");
			s_arr[41] = nvobj::make_persistent<C>(
				"abcdefghij11111111111111111111");
			s_arr[42] = nvobj::make_persistent<C>(
				"abcdefghij11111111111111111111klmnopqrst");
			s_arr[43] = nvobj::make_persistent<C>(
				"abcdefghij1111111111klmnopqrst");
			s_arr[44] = nvobj::make_persistent<C>(
				"abcdefghij11111klmnopqrst");
			s_arr[45] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs11111111111111111111t");
			s_arr[46] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs1111111111t");
			s_arr[47] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs11111t");
			s_arr[48] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst");
			s_arr[49] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst11111");
			s_arr[50] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst1111111111");
			s_arr[51] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst11111111111111111111");
			s_arr[52] = nvobj::make_persistent<C>("can't happen");
		});

		test(pop, *s_arr[0], 0, 0, '1', *s_arr[0]);
		test(pop, *s_arr[0], 0, 5, '1', *s_arr[1]);
		test(pop, *s_arr[0], 0, 10, '1', *s_arr[2]);
		test(pop, *s_arr[0], 0, 20, '1', *s_arr[3]);
		test(pop, *s_arr[0], 1, 0, '1', *s_arr[52]);
		test(pop, *s_arr[0], 1, 5, '1', *s_arr[52]);
		test(pop, *s_arr[0], 1, 10, '1', *s_arr[52]);
		test(pop, *s_arr[0], 1, 20, '1', *s_arr[52]);
		test(pop, *s_arr[28], 0, 0, '1', *s_arr[28]);
		test(pop, *s_arr[28], 0, 5, '1', *s_arr[10]);
		test(pop, *s_arr[28], 0, 10, '1', *s_arr[7]);
		test(pop, *s_arr[28], 0, 20, '1', *s_arr[4]);
		test(pop, *s_arr[28], 1, 0, '1', *s_arr[28]);
		test(pop, *s_arr[28], 1, 5, '1', *s_arr[19]);
		test(pop, *s_arr[28], 1, 10, '1', *s_arr[16]);
		test(pop, *s_arr[28], 1, 20, '1', *s_arr[13]);
		test(pop, *s_arr[28], 2, 0, '1', *s_arr[28]);
		test(pop, *s_arr[28], 2, 5, '1', *s_arr[24]);
		test(pop, *s_arr[28], 2, 10, '1', *s_arr[23]);
		test(pop, *s_arr[28], 2, 20, '1', *s_arr[22]);
		test(pop, *s_arr[28], 4, 0, '1', *s_arr[28]);
		test(pop, *s_arr[28], 4, 5, '1', *s_arr[27]);
		test(pop, *s_arr[28], 4, 10, '1', *s_arr[26]);
		test(pop, *s_arr[28], 4, 20, '1', *s_arr[25]);
		test(pop, *s_arr[28], 5, 0, '1', *s_arr[28]);
		test(pop, *s_arr[28], 5, 5, '1', *s_arr[29]);
		test(pop, *s_arr[28], 5, 10, '1', *s_arr[30]);
		test(pop, *s_arr[28], 5, 20, '1', *s_arr[31]);
		test(pop, *s_arr[28], 6, 0, '1', *s_arr[52]);
		test(pop, *s_arr[28], 6, 5, '1', *s_arr[52]);
		test(pop, *s_arr[28], 6, 10, '1', *s_arr[52]);
		test(pop, *s_arr[28], 6, 20, '1', *s_arr[52]);
		test(pop, *s_arr[38], 0, 0, '1', *s_arr[38]);
		test(pop, *s_arr[38], 0, 5, '1', *s_arr[11]);
		test(pop, *s_arr[38], 0, 10, '1', *s_arr[8]);
		test(pop, *s_arr[38], 0, 20, '1', *s_arr[5]);
		test(pop, *s_arr[38], 1, 0, '1', *s_arr[38]);
		test(pop, *s_arr[38], 1, 5, '1', *s_arr[20]);
		test(pop, *s_arr[38], 1, 10, '1', *s_arr[17]);
		test(pop, *s_arr[38], 1, 20, '1', *s_arr[14]);
		test(pop, *s_arr[38], 5, 0, '1', *s_arr[38]);
		test(pop, *s_arr[38], 5, 5, '1', *s_arr[34]);
		test(pop, *s_arr[38], 5, 10, '1', *s_arr[33]);
		test(pop, *s_arr[38], 5, 20, '1', *s_arr[32]);
		test(pop, *s_arr[38], 9, 0, '1', *s_arr[38]);
		test(pop, *s_arr[38], 9, 5, '1', *s_arr[37]);
		test(pop, *s_arr[38], 9, 10, '1', *s_arr[36]);
		test(pop, *s_arr[38], 9, 20, '1', *s_arr[35]);
		test(pop, *s_arr[38], 10, 0, '1', *s_arr[38]);
		test(pop, *s_arr[38], 10, 5, '1', *s_arr[39]);
		test(pop, *s_arr[38], 10, 10, '1', *s_arr[40]);
		test(pop, *s_arr[38], 10, 20, '1', *s_arr[41]);
		test(pop, *s_arr[38], 11, 0, '1', *s_arr[52]);
		test(pop, *s_arr[38], 11, 5, '1', *s_arr[52]);
		test(pop, *s_arr[38], 11, 10, '1', *s_arr[52]);
		test(pop, *s_arr[38], 11, 20, '1', *s_arr[52]);
		test(pop, *s_arr[48], 0, 0, '1', *s_arr[48]);
		test(pop, *s_arr[48], 0, 5, '1', *s_arr[12]);
		test(pop, *s_arr[48], 0, 10, '1', *s_arr[9]);
		test(pop, *s_arr[48], 0, 20, '1', *s_arr[6]);
		test(pop, *s_arr[48], 1, 0, '1', *s_arr[48]);
		test(pop, *s_arr[48], 1, 5, '1', *s_arr[21]);
		test(pop, *s_arr[48], 1, 10, '1', *s_arr[18]);
		test(pop, *s_arr[48], 1, 20, '1', *s_arr[15]);
		test(pop, *s_arr[48], 10, 0, '1', *s_arr[48]);
		test(pop, *s_arr[48], 10, 5, '1', *s_arr[44]);
		test(pop, *s_arr[48], 10, 10, '1', *s_arr[43]);
		test(pop, *s_arr[48], 10, 20, '1', *s_arr[42]);
		test(pop, *s_arr[48], 19, 0, '1', *s_arr[48]);
		test(pop, *s_arr[48], 19, 5, '1', *s_arr[47]);
		test(pop, *s_arr[48], 19, 10, '1', *s_arr[46]);
		test(pop, *s_arr[48], 19, 20, '1', *s_arr[45]);
		test(pop, *s_arr[48], 20, 0, '1', *s_arr[48]);
		test(pop, *s_arr[48], 20, 5, '1', *s_arr[49]);
		test(pop, *s_arr[48], 20, 10, '1', *s_arr[50]);
		test(pop, *s_arr[48], 20, 20, '1', *s_arr[51]);
		test(pop, *s_arr[48], 21, 0, '1', *s_arr[52]);
		test(pop, *s_arr[48], 21, 5, '1', *s_arr[52]);
		test(pop, *s_arr[48], 21, 10, '1', *s_arr[52]);
		test(pop, *s_arr[48], 21, 20, '1', *s_arr[52]);

		nvobj::transaction::run(pop, [&] {
			for (unsigned i = 0; i < 53; ++i) {
				nvobj::delete_persistent<C>(s_arr[i]);
			}
		});

	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	pop.close();

	return 0;
}
