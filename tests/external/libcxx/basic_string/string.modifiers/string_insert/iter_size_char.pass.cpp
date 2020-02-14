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
	nvobj::persistent_ptr<C> s_arr[76];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &s1,
     typename S::difference_type pos, typename S::size_type n,
     typename S::value_type c, const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<C>(s1); });

	auto &s = *r->s;

	typename S::const_iterator p = s.cbegin() + pos;
	typename S::iterator i = s.insert(p, n, c);
	UT_ASSERT(i - s.begin() == pos);
	UT_ASSERT(s == expected);

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
		path, "string_test", 2 * PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	auto &s_arr = r->s_arr;

	try {
		nvobj::transaction::run(pop,
					[&] {
						s_arr[0] =
							nvobj::make_persistent<
								C>("");
						s_arr[1] =
							nvobj::make_persistent<
								C>("11111");
						s_arr[2] =
							nvobj::make_persistent<
								C>(
								"1111111111");
						s_arr[3] = nvobj::make_persistent<
							C>(
							"11111111111111111111");
						s_arr[4] = nvobj::make_persistent<
							C>(
							"11111111111111111111abcde");
						s_arr[5] = nvobj::make_persistent<
							C>(
							"11111111111111111111abcdefghij");
						s_arr[6] = nvobj::make_persistent<
							C>(
							"11111111111111111111abcdefghijklmnopqrst");
						s_arr[7] = nvobj::make_persistent<
							C>("1111111111abcde");
						s_arr[8] = nvobj::make_persistent<
							C>(
							"1111111111abcdefghij");
						s_arr[9] = nvobj::make_persistent<
							C>(
							"1111111111abcdefghijklmnopqrst");
						s_arr[10] =
							nvobj::make_persistent<
								C>(
								"11111abcde");
						s_arr[11] = nvobj::make_persistent<
							C>("11111abcdefghij");
						s_arr[12] = nvobj::make_persistent<
							C>(
							"11111abcdefghijklmnopqrst");
						s_arr[13] =
							nvobj::make_persistent<
								C>("123");
						s_arr[14] = nvobj::make_persistent<
							C>(
							"a11111111111111111111bcde");
						s_arr[15] = nvobj::make_persistent<
							C>(
							"a11111111111111111111bcdefghij");
						s_arr[16] = nvobj::make_persistent<
							C>(
							"a11111111111111111111bcdefghijklmnopqrst");
						s_arr[17] = nvobj::make_persistent<
							C>("a1111111111bcde");
						s_arr[18] = nvobj::make_persistent<
							C>(
							"a1111111111bcdefghij");
						s_arr[19] = nvobj::make_persistent<
							C>(
							"a1111111111bcdefghijklmnopqrst");
						s_arr[20] =
							nvobj::make_persistent<
								C>(
								"a11111bcde");
						s_arr[21] = nvobj::make_persistent<
							C>("a11111bcdefghij");
						s_arr[22] = nvobj::make_persistent<
							C>(
							"a11111bcdefghijklmnopqrst");
						s_arr[23] = nvobj::make_persistent<
							C>(
							"ab11111111111111111111cde");
						s_arr[24] = nvobj::make_persistent<
							C>("ab1111111111cde");
						s_arr[25] =
							nvobj::make_persistent<
								C>(
								"ab11111cde");
						s_arr[26] =
							nvobj::make_persistent<
								C>("abc");
						s_arr[27] = nvobj::make_persistent<
							C>(
							"abcd11111111111111111111e");
						s_arr[28] = nvobj::make_persistent<
							C>("abcd1111111111e");
						s_arr[29] =
							nvobj::make_persistent<
								C>(
								"abcd11111e");
						s_arr[30] =
							nvobj::make_persistent<
								C>("abcde");
						s_arr[31] =
							nvobj::make_persistent<
								C>(
								"abcde11111");
						s_arr[32] = nvobj::make_persistent<
							C>("abcde1111111111");
						s_arr[33] = nvobj::make_persistent<
							C>(
							"abcde11111111111111111111");
						s_arr[34] = nvobj::make_persistent<
							C>(
							"abcde11111111111111111111fghij");
						s_arr[35] = nvobj::make_persistent<
							C>(
							"abcde1111111111fghij");
						s_arr[36] = nvobj::make_persistent<
							C>("abcde11111fghij");
						s_arr[37] = nvobj::make_persistent<
							C>(
							"abcdefghi11111111111111111111j");
						s_arr[38] = nvobj::make_persistent<
							C>(
							"abcdefghi1111111111j");
						s_arr[39] = nvobj::make_persistent<
							C>("abcdefghi11111j");
						s_arr[40] =
							nvobj::make_persistent<
								C>(
								"abcdefghij");
						s_arr[41] = nvobj::make_persistent<
							C>("abcdefghij11111");
						s_arr[42] = nvobj::make_persistent<
							C>(
							"abcdefghij1111111111");
						s_arr[43] = nvobj::make_persistent<
							C>(
							"abcdefghij11111111111111111111");
						s_arr[44] = nvobj::make_persistent<
							C>(
							"abcdefghij11111111111111111111klmnopqrst");
						s_arr[45] = nvobj::make_persistent<
							C>(
							"abcdefghij1111111111klmnopqrst");
						s_arr[46] = nvobj::make_persistent<
							C>(
							"abcdefghij11111klmnopqrst");
						s_arr[47] = nvobj::make_persistent<
							C>(
							"abcdefghijklmnopqrs11111111111111111111t");
						s_arr[48] = nvobj::make_persistent<
							C>(
							"abcdefghijklmnopqrs1111111111t");
						s_arr[49] = nvobj::make_persistent<
							C>(
							"abcdefghijklmnopqrs11111t");
						s_arr[50] = nvobj::make_persistent<
							C>(
							"abcdefghijklmnopqrst");
						s_arr[51] = nvobj::make_persistent<
							C>(
							"abcdefghijklmnopqrst11111");
						s_arr[52] = nvobj::make_persistent<
							C>(
							"abcdefghijklmnopqrst1111111111");
						s_arr[53] = nvobj::make_persistent<
							C>(
							"abcdefghijklmnopqrst11111111111111111111");
						s_arr[54] = nvobj::make_persistent<
							C>(
							"ABCDEFGHIJKLMNOPQRSTabcdefghijklmnopqrst12345678901234567890");
						s_arr[55] = nvobj::make_persistent<
							C>(
							"11111ABCDEFGHIJKLMNOPQRSTabcdefghijklmnopqrst12345678901234567890");
						s_arr[56] = nvobj::make_persistent<
							C>(
							"11111111111111111111ABCDEFGHIJKLMNOPQRSTabcdefghijklmnopqrst12345678901234567890");
						s_arr[57] = nvobj::make_persistent<
							C>(
							"A11111BCDEFGHIJKLMNOPQRSTabcdefghijklmnopqrst12345678901234567890");
						s_arr[58] = nvobj::make_persistent<
							C>(
							"A11111111111111111111BCDEFGHIJKLMNOPQRSTabcdefghijklmnopqrst12345678901234567890");
						s_arr[59] = nvobj::make_persistent<
							C>(
							"ABCDEFGHIJ11111KLMNOPQRSTabcdefghijklmnopqrst12345678901234567890");
						s_arr[60] = nvobj::make_persistent<
							C>(
							"ABCDEFGHIJ11111111111111111111KLMNOPQRSTabcdefghijklmnopqrst12345678901234567890");
						s_arr[61] = nvobj::make_persistent<
							C>(
							"ABCDEFGHIJKLMNOPQRSTabcdefghijklmnopqrst1234567890123456789111110");
						s_arr[62] = nvobj::make_persistent<
							C>(
							"ABCDEFGHIJKLMNOPQRSTabcdefghijklmnopqrst1234567890123456789111111111111111111110");
						s_arr[63] = nvobj::make_persistent<
							C>(
							"ABCDEFGHIJKLMNOPQRSTabcdefghijklmnopqrst1234567890123456789011111");
						s_arr[64] = nvobj::make_persistent<
							C>(
							"ABCDEFGHIJKLMNOPQRSTabcdefghijklmnopqrst1234567890123456789011111111111111111111");
						s_arr[65] = nvobj::make_persistent<
							C>(
							"ABCDEFGHIJKLMNOPQRSTABCDEFGHIJabcdefghijklmnopqrst12345678901234567890");
						s_arr[66] = nvobj::make_persistent<
							C>(
							"11111ABCDEFGHIJKLMNOPQRSTABCDEFGHIJabcdefghijklmnopqrst12345678901234567890");
						s_arr[67] = nvobj::make_persistent<
							C>(
							"11111111111111111111ABCDEFGHIJKLMNOPQRSTABCDEFGHIJabcdefghijklmnopqrst12345678901234567890");
						s_arr[68] = nvobj::make_persistent<
							C>(
							"A11111BCDEFGHIJKLMNOPQRSTABCDEFGHIJabcdefghijklmnopqrst12345678901234567890");
						s_arr[69] = nvobj::make_persistent<
							C>(
							"A11111111111111111111BCDEFGHIJKLMNOPQRSTABCDEFGHIJabcdefghijklmnopqrst12345678901234567890");
						s_arr[70] = nvobj::make_persistent<
							C>(
							"ABCDEFGHIJ11111KLMNOPQRSTABCDEFGHIJabcdefghijklmnopqrst12345678901234567890");
						s_arr[71] = nvobj::make_persistent<
							C>(
							"ABCDEFGHIJ11111111111111111111KLMNOPQRSTABCDEFGHIJabcdefghijklmnopqrst12345678901234567890");
						s_arr[72] = nvobj::make_persistent<
							C>(
							"ABCDEFGHIJKLMNOPQRSTABCDEFGHIJabcdefghijklmnopqrst1234567890123456789111110");
						s_arr[73] = nvobj::make_persistent<
							C>(
							"ABCDEFGHIJKLMNOPQRSTABCDEFGHIJabcdefghijklmnopqrst1234567890123456789111111111111111111110");
						s_arr[74] = nvobj::make_persistent<
							C>(
							"ABCDEFGHIJKLMNOPQRSTABCDEFGHIJabcdefghijklmnopqrst1234567890123456789011111");
						s_arr[75] = nvobj::make_persistent<
							C>(
							"ABCDEFGHIJKLMNOPQRSTABCDEFGHIJabcdefghijklmnopqrst1234567890123456789011111111111111111111");
					});

		test(pop, *s_arr[0], 0, 0, '1', *s_arr[0]);
		test(pop, *s_arr[0], 0, 5, '1', *s_arr[1]);
		test(pop, *s_arr[0], 0, 10, '1', *s_arr[2]);
		test(pop, *s_arr[0], 0, 20, '1', *s_arr[3]);
		test(pop, *s_arr[30], 0, 0, '1', *s_arr[30]);
		test(pop, *s_arr[30], 0, 5, '1', *s_arr[10]);
		test(pop, *s_arr[30], 0, 10, '1', *s_arr[7]);
		test(pop, *s_arr[30], 0, 20, '1', *s_arr[4]);
		test(pop, *s_arr[30], 1, 0, '1', *s_arr[30]);
		test(pop, *s_arr[30], 1, 5, '1', *s_arr[20]);
		test(pop, *s_arr[30], 1, 10, '1', *s_arr[17]);
		test(pop, *s_arr[30], 1, 20, '1', *s_arr[14]);
		test(pop, *s_arr[30], 2, 0, '1', *s_arr[30]);
		test(pop, *s_arr[30], 2, 5, '1', *s_arr[25]);
		test(pop, *s_arr[30], 2, 10, '1', *s_arr[24]);
		test(pop, *s_arr[30], 2, 20, '1', *s_arr[23]);
		test(pop, *s_arr[30], 4, 0, '1', *s_arr[30]);
		test(pop, *s_arr[30], 4, 5, '1', *s_arr[29]);
		test(pop, *s_arr[30], 4, 10, '1', *s_arr[28]);
		test(pop, *s_arr[30], 4, 20, '1', *s_arr[27]);
		test(pop, *s_arr[30], 5, 0, '1', *s_arr[30]);
		test(pop, *s_arr[30], 5, 5, '1', *s_arr[31]);
		test(pop, *s_arr[30], 5, 10, '1', *s_arr[32]);
		test(pop, *s_arr[30], 5, 20, '1', *s_arr[33]);
		test(pop, *s_arr[40], 0, 0, '1', *s_arr[40]);
		test(pop, *s_arr[40], 0, 5, '1', *s_arr[11]);
		test(pop, *s_arr[40], 0, 10, '1', *s_arr[8]);
		test(pop, *s_arr[40], 0, 20, '1', *s_arr[5]);
		test(pop, *s_arr[40], 1, 0, '1', *s_arr[40]);
		test(pop, *s_arr[40], 1, 5, '1', *s_arr[21]);
		test(pop, *s_arr[40], 1, 10, '1', *s_arr[18]);
		test(pop, *s_arr[40], 1, 20, '1', *s_arr[15]);
		test(pop, *s_arr[40], 5, 0, '1', *s_arr[40]);
		test(pop, *s_arr[40], 5, 5, '1', *s_arr[36]);
		test(pop, *s_arr[40], 5, 10, '1', *s_arr[35]);
		test(pop, *s_arr[40], 5, 20, '1', *s_arr[34]);
		test(pop, *s_arr[40], 9, 0, '1', *s_arr[40]);
		test(pop, *s_arr[40], 9, 5, '1', *s_arr[39]);
		test(pop, *s_arr[40], 9, 10, '1', *s_arr[38]);
		test(pop, *s_arr[40], 9, 20, '1', *s_arr[37]);
		test(pop, *s_arr[40], 10, 0, '1', *s_arr[40]);
		test(pop, *s_arr[40], 10, 5, '1', *s_arr[41]);
		test(pop, *s_arr[40], 10, 10, '1', *s_arr[42]);
		test(pop, *s_arr[40], 10, 20, '1', *s_arr[43]);
		test(pop, *s_arr[50], 0, 0, '1', *s_arr[50]);
		test(pop, *s_arr[50], 0, 5, '1', *s_arr[12]);
		test(pop, *s_arr[50], 0, 10, '1', *s_arr[9]);
		test(pop, *s_arr[50], 0, 20, '1', *s_arr[6]);
		test(pop, *s_arr[50], 1, 0, '1', *s_arr[50]);
		test(pop, *s_arr[50], 1, 5, '1', *s_arr[22]);
		test(pop, *s_arr[50], 1, 10, '1', *s_arr[19]);
		test(pop, *s_arr[50], 1, 20, '1', *s_arr[16]);
		test(pop, *s_arr[50], 10, 0, '1', *s_arr[50]);
		test(pop, *s_arr[50], 10, 5, '1', *s_arr[46]);
		test(pop, *s_arr[50], 10, 10, '1', *s_arr[45]);
		test(pop, *s_arr[50], 10, 20, '1', *s_arr[44]);
		test(pop, *s_arr[50], 19, 0, '1', *s_arr[50]);
		test(pop, *s_arr[50], 19, 5, '1', *s_arr[49]);
		test(pop, *s_arr[50], 19, 10, '1', *s_arr[48]);
		test(pop, *s_arr[50], 19, 20, '1', *s_arr[47]);
		test(pop, *s_arr[50], 20, 0, '1', *s_arr[50]);
		test(pop, *s_arr[50], 20, 5, '1', *s_arr[51]);
		test(pop, *s_arr[50], 20, 10, '1', *s_arr[52]);
		test(pop, *s_arr[50], 20, 20, '1', *s_arr[53]);
		test(pop, *s_arr[54], 0, 5, '1', *s_arr[55]);
		test(pop, *s_arr[54], 0, 20, '1', *s_arr[56]);
		test(pop, *s_arr[54], 1, 5, '1', *s_arr[57]);
		test(pop, *s_arr[54], 1, 20, '1', *s_arr[58]);
		test(pop, *s_arr[54], 10, 5, '1', *s_arr[59]);
		test(pop, *s_arr[54], 10, 20, '1', *s_arr[60]);
		test(pop, *s_arr[54], 59, 5, '1', *s_arr[61]);
		test(pop, *s_arr[54], 59, 20, '1', *s_arr[62]);
		test(pop, *s_arr[54], 60, 5, '1', *s_arr[63]);
		test(pop, *s_arr[54], 60, 20, '1', *s_arr[64]);
		test(pop, *s_arr[65], 0, 5, '1', *s_arr[66]);
		test(pop, *s_arr[65], 0, 20, '1', *s_arr[67]);
		test(pop, *s_arr[65], 1, 5, '1', *s_arr[68]);
		test(pop, *s_arr[65], 1, 20, '1', *s_arr[69]);
		test(pop, *s_arr[65], 10, 5, '1', *s_arr[70]);
		test(pop, *s_arr[65], 10, 20, '1', *s_arr[71]);
		test(pop, *s_arr[65], 69, 5, '1', *s_arr[72]);
		test(pop, *s_arr[65], 69, 20, '1', *s_arr[73]);
		test(pop, *s_arr[65], 70, 5, '1', *s_arr[74]);
		test(pop, *s_arr[65], 70, 20, '1', *s_arr[75]);

		nvobj::transaction::run(pop, [&] {
			for (unsigned i = 0; i < 76; ++i) {
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
