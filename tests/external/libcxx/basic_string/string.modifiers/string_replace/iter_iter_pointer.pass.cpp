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

#include "../throwing_iterator.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/string.hpp>

namespace nvobj = pmem::obj;

using C = pmem::obj::string;

struct root {
	nvobj::persistent_ptr<C> s, a_copy, s_short, s_long, s_extra_long;
	nvobj::persistent_ptr<C> s_arr[176];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &s1, typename S::size_type pos1,
     typename S::size_type n1, const typename S::value_type *str,
     const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<C>(s1); });

	auto &s = *r->s;

	typename S::size_type old_size = s.size();
	typename S::const_iterator first =
		s.begin() + static_cast<typename S::difference_type>(pos1);
	typename S::const_iterator last =
		s.begin() + static_cast<typename S::difference_type>(pos1 + n1);
	typename S::size_type xlen =
		static_cast<typename S::size_type>(last - first);
	s.replace(first, last, str);
	UT_ASSERT(s == expected);
	typename S::size_type rlen = S::traits_type::length(str);
	UT_ASSERT(s.size() == old_size - xlen + rlen);

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<C>(r->s); });
}

template <class S>
void
test0(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[0], 0, 0, "", *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, "12345", *s_arr[1]);
	test(pop, *s_arr[0], 0, 0, "1234567890", *s_arr[2]);
	test(pop, *s_arr[0], 0, 0, "12345678901234567890", *s_arr[3]);
	test(pop, *s_arr[100], 0, 0, "", *s_arr[100]);
	test(pop, *s_arr[100], 0, 0, "12345", *s_arr[28]);
	test(pop, *s_arr[100], 0, 0, "1234567890", *s_arr[16]);
	test(pop, *s_arr[100], 0, 0, "12345678901234567890", *s_arr[4]);
	test(pop, *s_arr[100], 0, 1, "", *s_arr[167]);
	test(pop, *s_arr[100], 0, 1, "12345", *s_arr[31]);
	test(pop, *s_arr[100], 0, 1, "1234567890", *s_arr[19]);
	test(pop, *s_arr[100], 0, 1, "12345678901234567890", *s_arr[7]);
	test(pop, *s_arr[100], 0, 2, "", *s_arr[170]);
	test(pop, *s_arr[100], 0, 2, "12345", *s_arr[34]);
	test(pop, *s_arr[100], 0, 2, "1234567890", *s_arr[22]);
	test(pop, *s_arr[100], 0, 2, "12345678901234567890", *s_arr[10]);
	test(pop, *s_arr[100], 0, 4, "", *s_arr[171]);
	test(pop, *s_arr[100], 0, 4, "12345", *s_arr[35]);
	test(pop, *s_arr[100], 0, 4, "1234567890", *s_arr[23]);
	test(pop, *s_arr[100], 0, 4, "12345678901234567890", *s_arr[11]);
	test(pop, *s_arr[100], 0, 5, "", *s_arr[0]);
	test(pop, *s_arr[100], 0, 5, "12345", *s_arr[1]);
	test(pop, *s_arr[100], 0, 5, "1234567890", *s_arr[2]);
	test(pop, *s_arr[100], 0, 5, "12345678901234567890", *s_arr[3]);
	test(pop, *s_arr[100], 1, 0, "", *s_arr[100]);
	test(pop, *s_arr[100], 1, 0, "12345", *s_arr[68]);
	test(pop, *s_arr[100], 1, 0, "1234567890", *s_arr[56]);
	test(pop, *s_arr[100], 1, 0, "12345678901234567890", *s_arr[44]);
	test(pop, *s_arr[100], 1, 1, "", *s_arr[158]);
	test(pop, *s_arr[100], 1, 1, "12345", *s_arr[71]);
	test(pop, *s_arr[100], 1, 1, "1234567890", *s_arr[59]);
	test(pop, *s_arr[100], 1, 1, "12345678901234567890", *s_arr[47]);
	test(pop, *s_arr[100], 1, 2, "", *s_arr[161]);
	test(pop, *s_arr[100], 1, 2, "12345", *s_arr[74]);
	test(pop, *s_arr[100], 1, 2, "1234567890", *s_arr[62]);
	test(pop, *s_arr[100], 1, 2, "12345678901234567890", *s_arr[50]);
	test(pop, *s_arr[100], 1, 3, "", *s_arr[162]);
	test(pop, *s_arr[100], 1, 3, "12345", *s_arr[75]);
	test(pop, *s_arr[100], 1, 3, "1234567890", *s_arr[63]);
	test(pop, *s_arr[100], 1, 3, "12345678901234567890", *s_arr[51]);
	test(pop, *s_arr[100], 1, 4, "", *s_arr[40]);
	test(pop, *s_arr[100], 1, 4, "12345", *s_arr[41]);
	test(pop, *s_arr[100], 1, 4, "1234567890", *s_arr[42]);
	test(pop, *s_arr[100], 1, 4, "12345678901234567890", *s_arr[43]);
	test(pop, *s_arr[100], 2, 0, "", *s_arr[100]);
	test(pop, *s_arr[100], 2, 0, "12345", *s_arr[90]);
	test(pop, *s_arr[100], 2, 0, "1234567890", *s_arr[87]);
	test(pop, *s_arr[100], 2, 0, "12345678901234567890", *s_arr[84]);
	test(pop, *s_arr[100], 2, 1, "", *s_arr[156]);
	test(pop, *s_arr[100], 2, 1, "12345", *s_arr[91]);
	test(pop, *s_arr[100], 2, 1, "1234567890", *s_arr[88]);
	test(pop, *s_arr[100], 2, 1, "12345678901234567890", *s_arr[85]);
	test(pop, *s_arr[100], 2, 2, "", *s_arr[157]);
	test(pop, *s_arr[100], 2, 2, "12345", *s_arr[92]);
	test(pop, *s_arr[100], 2, 2, "1234567890", *s_arr[89]);
	test(pop, *s_arr[100], 2, 2, "12345678901234567890", *s_arr[86]);
	test(pop, *s_arr[100], 2, 3, "", *s_arr[80]);
	test(pop, *s_arr[100], 2, 3, "12345", *s_arr[81]);
	test(pop, *s_arr[100], 2, 3, "1234567890", *s_arr[82]);
	test(pop, *s_arr[100], 2, 3, "12345678901234567890", *s_arr[83]);
	test(pop, *s_arr[100], 4, 0, "", *s_arr[100]);
	test(pop, *s_arr[100], 4, 0, "12345", *s_arr[99]);
	test(pop, *s_arr[100], 4, 0, "1234567890", *s_arr[98]);
	test(pop, *s_arr[100], 4, 0, "12345678901234567890", *s_arr[97]);
	test(pop, *s_arr[100], 4, 1, "", *s_arr[93]);
	test(pop, *s_arr[100], 4, 1, "12345", *s_arr[94]);
	test(pop, *s_arr[100], 4, 1, "1234567890", *s_arr[95]);
	test(pop, *s_arr[100], 4, 1, "12345678901234567890", *s_arr[96]);
	test(pop, *s_arr[100], 5, 0, "", *s_arr[100]);
	test(pop, *s_arr[100], 5, 0, "12345", *s_arr[101]);
	test(pop, *s_arr[100], 5, 0, "1234567890", *s_arr[102]);
	test(pop, *s_arr[100], 5, 0, "12345678901234567890", *s_arr[103]);
	test(pop, *s_arr[123], 0, 0, "", *s_arr[123]);
	test(pop, *s_arr[123], 0, 0, "12345", *s_arr[29]);
	test(pop, *s_arr[123], 0, 0, "1234567890", *s_arr[17]);
	test(pop, *s_arr[123], 0, 0, "12345678901234567890", *s_arr[5]);
	test(pop, *s_arr[123], 0, 1, "", *s_arr[168]);
	test(pop, *s_arr[123], 0, 1, "12345", *s_arr[32]);
	test(pop, *s_arr[123], 0, 1, "1234567890", *s_arr[20]);
	test(pop, *s_arr[123], 0, 1, "12345678901234567890", *s_arr[8]);
	test(pop, *s_arr[123], 0, 5, "", *s_arr[172]);
	test(pop, *s_arr[123], 0, 5, "12345", *s_arr[36]);
	test(pop, *s_arr[123], 0, 5, "1234567890", *s_arr[24]);
	test(pop, *s_arr[123], 0, 5, "12345678901234567890", *s_arr[12]);
	test(pop, *s_arr[123], 0, 9, "", *s_arr[173]);
	test(pop, *s_arr[123], 0, 9, "12345", *s_arr[37]);
	test(pop, *s_arr[123], 0, 9, "1234567890", *s_arr[25]);
	test(pop, *s_arr[123], 0, 9, "12345678901234567890", *s_arr[13]);
	test(pop, *s_arr[123], 0, 10, "", *s_arr[0]);
	test(pop, *s_arr[123], 0, 10, "12345", *s_arr[1]);
	test(pop, *s_arr[123], 0, 10, "1234567890", *s_arr[2]);
	test(pop, *s_arr[123], 0, 10, "12345678901234567890", *s_arr[3]);
	test(pop, *s_arr[123], 1, 0, "", *s_arr[123]);
	test(pop, *s_arr[123], 1, 0, "12345", *s_arr[69]);
	test(pop, *s_arr[123], 1, 0, "1234567890", *s_arr[57]);
	test(pop, *s_arr[123], 1, 0, "12345678901234567890", *s_arr[45]);
	test(pop, *s_arr[123], 1, 1, "", *s_arr[159]);
	test(pop, *s_arr[123], 1, 1, "12345", *s_arr[72]);
	test(pop, *s_arr[123], 1, 1, "1234567890", *s_arr[60]);
	test(pop, *s_arr[123], 1, 1, "12345678901234567890", *s_arr[48]);
}

template <class S>
void
test1(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[123], 1, 4, "", *s_arr[163]);
	test(pop, *s_arr[123], 1, 4, "12345", *s_arr[76]);
	test(pop, *s_arr[123], 1, 4, "1234567890", *s_arr[64]);
	test(pop, *s_arr[123], 1, 4, "12345678901234567890", *s_arr[52]);
	test(pop, *s_arr[123], 1, 8, "", *s_arr[164]);
	test(pop, *s_arr[123], 1, 8, "12345", *s_arr[77]);
	test(pop, *s_arr[123], 1, 8, "1234567890", *s_arr[65]);
	test(pop, *s_arr[123], 1, 8, "12345678901234567890", *s_arr[53]);
	test(pop, *s_arr[123], 1, 9, "", *s_arr[40]);
	test(pop, *s_arr[123], 1, 9, "12345", *s_arr[41]);
	test(pop, *s_arr[123], 1, 9, "1234567890", *s_arr[42]);
	test(pop, *s_arr[123], 1, 9, "12345678901234567890", *s_arr[43]);
	test(pop, *s_arr[123], 5, 0, "", *s_arr[123]);
	test(pop, *s_arr[123], 5, 0, "12345", *s_arr[112]);
	test(pop, *s_arr[123], 5, 0, "1234567890", *s_arr[108]);
	test(pop, *s_arr[123], 5, 0, "12345678901234567890", *s_arr[104]);
	test(pop, *s_arr[123], 5, 1, "", *s_arr[153]);
	test(pop, *s_arr[123], 5, 1, "12345", *s_arr[113]);
	test(pop, *s_arr[123], 5, 1, "1234567890", *s_arr[109]);
	test(pop, *s_arr[123], 5, 1, "12345678901234567890", *s_arr[105]);
	test(pop, *s_arr[123], 5, 2, "", *s_arr[154]);
	test(pop, *s_arr[123], 5, 2, "12345", *s_arr[114]);
	test(pop, *s_arr[123], 5, 2, "1234567890", *s_arr[110]);
	test(pop, *s_arr[123], 5, 2, "12345678901234567890", *s_arr[106]);
	test(pop, *s_arr[123], 5, 4, "", *s_arr[155]);
	test(pop, *s_arr[123], 5, 4, "12345", *s_arr[115]);
	test(pop, *s_arr[123], 5, 4, "1234567890", *s_arr[111]);
	test(pop, *s_arr[123], 5, 4, "12345678901234567890", *s_arr[107]);
	test(pop, *s_arr[123], 5, 5, "", *s_arr[100]);
	test(pop, *s_arr[123], 5, 5, "12345", *s_arr[101]);
	test(pop, *s_arr[123], 5, 5, "1234567890", *s_arr[102]);
	test(pop, *s_arr[123], 5, 5, "12345678901234567890", *s_arr[103]);
	test(pop, *s_arr[123], 9, 0, "", *s_arr[123]);
	test(pop, *s_arr[123], 9, 0, "12345", *s_arr[122]);
	test(pop, *s_arr[123], 9, 0, "1234567890", *s_arr[121]);
	test(pop, *s_arr[123], 9, 0, "12345678901234567890", *s_arr[120]);
	test(pop, *s_arr[123], 9, 1, "", *s_arr[116]);
	test(pop, *s_arr[123], 9, 1, "12345", *s_arr[117]);
	test(pop, *s_arr[123], 9, 1, "1234567890", *s_arr[118]);
	test(pop, *s_arr[123], 9, 1, "12345678901234567890", *s_arr[119]);
	test(pop, *s_arr[123], 10, 0, "", *s_arr[123]);
	test(pop, *s_arr[123], 10, 0, "12345", *s_arr[124]);
	test(pop, *s_arr[123], 10, 0, "1234567890", *s_arr[125]);
	test(pop, *s_arr[123], 10, 0, "12345678901234567890", *s_arr[126]);
	test(pop, *s_arr[146], 0, 0, "", *s_arr[146]);
	test(pop, *s_arr[146], 0, 0, "12345", *s_arr[30]);
	test(pop, *s_arr[146], 0, 0, "1234567890", *s_arr[18]);
	test(pop, *s_arr[146], 0, 0, "12345678901234567890", *s_arr[6]);
	test(pop, *s_arr[146], 0, 1, "", *s_arr[169]);
	test(pop, *s_arr[146], 0, 1, "12345", *s_arr[33]);
	test(pop, *s_arr[146], 0, 1, "1234567890", *s_arr[21]);
	test(pop, *s_arr[146], 0, 1, "12345678901234567890", *s_arr[9]);
	test(pop, *s_arr[146], 0, 10, "", *s_arr[174]);
	test(pop, *s_arr[146], 0, 10, "12345", *s_arr[38]);
	test(pop, *s_arr[146], 0, 10, "1234567890", *s_arr[26]);
	test(pop, *s_arr[146], 0, 10, "12345678901234567890", *s_arr[14]);
	test(pop, *s_arr[146], 0, 19, "", *s_arr[175]);
	test(pop, *s_arr[146], 0, 19, "12345", *s_arr[39]);
	test(pop, *s_arr[146], 0, 19, "1234567890", *s_arr[27]);
	test(pop, *s_arr[146], 0, 19, "12345678901234567890", *s_arr[15]);
	test(pop, *s_arr[146], 0, 20, "", *s_arr[0]);
	test(pop, *s_arr[146], 0, 20, "12345", *s_arr[1]);
	test(pop, *s_arr[146], 0, 20, "1234567890", *s_arr[2]);
	test(pop, *s_arr[146], 0, 20, "12345678901234567890", *s_arr[3]);
	test(pop, *s_arr[146], 1, 0, "", *s_arr[146]);
	test(pop, *s_arr[146], 1, 0, "12345", *s_arr[70]);
	test(pop, *s_arr[146], 1, 0, "1234567890", *s_arr[58]);
	test(pop, *s_arr[146], 1, 0, "12345678901234567890", *s_arr[46]);
	test(pop, *s_arr[146], 1, 1, "", *s_arr[160]);
	test(pop, *s_arr[146], 1, 1, "12345", *s_arr[73]);
	test(pop, *s_arr[146], 1, 1, "1234567890", *s_arr[61]);
	test(pop, *s_arr[146], 1, 1, "12345678901234567890", *s_arr[49]);
	test(pop, *s_arr[146], 1, 9, "", *s_arr[165]);
	test(pop, *s_arr[146], 1, 9, "12345", *s_arr[78]);
	test(pop, *s_arr[146], 1, 9, "1234567890", *s_arr[66]);
	test(pop, *s_arr[146], 1, 9, "12345678901234567890", *s_arr[54]);
	test(pop, *s_arr[146], 1, 18, "", *s_arr[166]);
	test(pop, *s_arr[146], 1, 18, "12345", *s_arr[79]);
	test(pop, *s_arr[146], 1, 18, "1234567890", *s_arr[67]);
	test(pop, *s_arr[146], 1, 18, "12345678901234567890", *s_arr[55]);
	test(pop, *s_arr[146], 1, 19, "", *s_arr[40]);
	test(pop, *s_arr[146], 1, 19, "12345", *s_arr[41]);
	test(pop, *s_arr[146], 1, 19, "1234567890", *s_arr[42]);
	test(pop, *s_arr[146], 1, 19, "12345678901234567890", *s_arr[43]);
	test(pop, *s_arr[146], 10, 0, "", *s_arr[146]);
	test(pop, *s_arr[146], 10, 0, "12345", *s_arr[135]);
	test(pop, *s_arr[146], 10, 0, "1234567890", *s_arr[131]);
	test(pop, *s_arr[146], 10, 0, "12345678901234567890", *s_arr[127]);
	test(pop, *s_arr[146], 10, 1, "", *s_arr[150]);
	test(pop, *s_arr[146], 10, 1, "12345", *s_arr[136]);
	test(pop, *s_arr[146], 10, 1, "1234567890", *s_arr[132]);
	test(pop, *s_arr[146], 10, 1, "12345678901234567890", *s_arr[128]);
	test(pop, *s_arr[146], 10, 5, "", *s_arr[151]);
	test(pop, *s_arr[146], 10, 5, "12345", *s_arr[137]);
	test(pop, *s_arr[146], 10, 5, "1234567890", *s_arr[133]);
	test(pop, *s_arr[146], 10, 5, "12345678901234567890", *s_arr[129]);
	test(pop, *s_arr[146], 10, 9, "", *s_arr[152]);
	test(pop, *s_arr[146], 10, 9, "12345", *s_arr[138]);
	test(pop, *s_arr[146], 10, 9, "1234567890", *s_arr[134]);
	test(pop, *s_arr[146], 10, 9, "12345678901234567890", *s_arr[130]);
}

template <class S>
void
test2(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[146], 10, 10, "", *s_arr[123]);
	test(pop, *s_arr[146], 10, 10, "12345", *s_arr[124]);
	test(pop, *s_arr[146], 10, 10, "1234567890", *s_arr[125]);
	test(pop, *s_arr[146], 10, 10, "12345678901234567890", *s_arr[126]);
	test(pop, *s_arr[146], 19, 0, "", *s_arr[146]);
	test(pop, *s_arr[146], 19, 0, "12345", *s_arr[145]);
	test(pop, *s_arr[146], 19, 0, "1234567890", *s_arr[144]);
	test(pop, *s_arr[146], 19, 0, "12345678901234567890", *s_arr[143]);
	test(pop, *s_arr[146], 19, 1, "", *s_arr[139]);
	test(pop, *s_arr[146], 19, 1, "12345", *s_arr[140]);
	test(pop, *s_arr[146], 19, 1, "1234567890", *s_arr[141]);
	test(pop, *s_arr[146], 19, 1, "12345678901234567890", *s_arr[142]);
	test(pop, *s_arr[146], 20, 0, "", *s_arr[146]);
	test(pop, *s_arr[146], 20, 0, "12345", *s_arr[147]);
	test(pop, *s_arr[146], 20, 0, "1234567890", *s_arr[148]);
	test(pop, *s_arr[146], 20, 0, "12345678901234567890", *s_arr[149]);
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
					"12345678901234567890bcde");
				s_arr[8] = nvobj::make_persistent<C>(
					"12345678901234567890bcdefghij");
				s_arr[9] = nvobj::make_persistent<C>(
					"12345678901234567890bcdefghijklmnopqrst");
				s_arr[10] = nvobj::make_persistent<C>(
					"12345678901234567890cde");
				s_arr[11] = nvobj::make_persistent<C>(
					"12345678901234567890e");
				s_arr[12] = nvobj::make_persistent<C>(
					"12345678901234567890fghij");
				s_arr[13] = nvobj::make_persistent<C>(
					"12345678901234567890j");
				s_arr[14] = nvobj::make_persistent<C>(
					"12345678901234567890klmnopqrst");
				s_arr[15] = nvobj::make_persistent<C>(
					"12345678901234567890t");
				s_arr[16] = nvobj::make_persistent<C>(
					"1234567890abcde");
				s_arr[17] = nvobj::make_persistent<C>(
					"1234567890abcdefghij");
				s_arr[18] = nvobj::make_persistent<C>(
					"1234567890abcdefghijklmnopqrst");
				s_arr[19] = nvobj::make_persistent<C>(
					"1234567890bcde");
				s_arr[20] = nvobj::make_persistent<C>(
					"1234567890bcdefghij");
				s_arr[21] = nvobj::make_persistent<C>(
					"1234567890bcdefghijklmnopqrst");
				s_arr[22] = nvobj::make_persistent<C>(
					"1234567890cde");
				s_arr[23] = nvobj::make_persistent<C>(
					"1234567890e");
				s_arr[24] = nvobj::make_persistent<C>(
					"1234567890fghij");
				s_arr[25] = nvobj::make_persistent<C>(
					"1234567890j");
				s_arr[26] = nvobj::make_persistent<C>(
					"1234567890klmnopqrst");
				s_arr[27] = nvobj::make_persistent<C>(
					"1234567890t");
				s_arr[28] =
					nvobj::make_persistent<C>("12345abcde");
				s_arr[29] = nvobj::make_persistent<C>(
					"12345abcdefghij");
				s_arr[30] = nvobj::make_persistent<C>(
					"12345abcdefghijklmnopqrst");
				s_arr[31] =
					nvobj::make_persistent<C>("12345bcde");
				s_arr[32] = nvobj::make_persistent<C>(
					"12345bcdefghij");
				s_arr[33] = nvobj::make_persistent<C>(
					"12345bcdefghijklmnopqrst");
				s_arr[34] =
					nvobj::make_persistent<C>("12345cde");
				s_arr[35] = nvobj::make_persistent<C>("12345e");
				s_arr[36] =
					nvobj::make_persistent<C>("12345fghij");
				s_arr[37] = nvobj::make_persistent<C>("12345j");
				s_arr[38] = nvobj::make_persistent<C>(
					"12345klmnopqrst");
				s_arr[39] = nvobj::make_persistent<C>("12345t");
				s_arr[40] = nvobj::make_persistent<C>("a");
				s_arr[41] = nvobj::make_persistent<C>("a12345");
				s_arr[42] = nvobj::make_persistent<C>(
					"a1234567890");
				s_arr[43] = nvobj::make_persistent<C>(
					"a12345678901234567890");
				s_arr[44] = nvobj::make_persistent<C>(
					"a12345678901234567890bcde");
				s_arr[45] = nvobj::make_persistent<C>(
					"a12345678901234567890bcdefghij");
				s_arr[46] = nvobj::make_persistent<C>(
					"a12345678901234567890bcdefghijklmnopqrst");
				s_arr[47] = nvobj::make_persistent<C>(
					"a12345678901234567890cde");
				s_arr[48] = nvobj::make_persistent<C>(
					"a12345678901234567890cdefghij");
				s_arr[49] = nvobj::make_persistent<C>(
					"a12345678901234567890cdefghijklmnopqrst");
				s_arr[50] = nvobj::make_persistent<C>(
					"a12345678901234567890de");
				s_arr[51] = nvobj::make_persistent<C>(
					"a12345678901234567890e");
				s_arr[52] = nvobj::make_persistent<C>(
					"a12345678901234567890fghij");
				s_arr[53] = nvobj::make_persistent<C>(
					"a12345678901234567890j");
				s_arr[54] = nvobj::make_persistent<C>(
					"a12345678901234567890klmnopqrst");
				s_arr[55] = nvobj::make_persistent<C>(
					"a12345678901234567890t");
				s_arr[56] = nvobj::make_persistent<C>(
					"a1234567890bcde");
				s_arr[57] = nvobj::make_persistent<C>(
					"a1234567890bcdefghij");
				s_arr[58] = nvobj::make_persistent<C>(
					"a1234567890bcdefghijklmnopqrst");
				s_arr[59] = nvobj::make_persistent<C>(
					"a1234567890cde");
				s_arr[60] = nvobj::make_persistent<C>(
					"a1234567890cdefghij");
				s_arr[61] = nvobj::make_persistent<C>(
					"a1234567890cdefghijklmnopqrst");
				s_arr[62] = nvobj::make_persistent<C>(
					"a1234567890de");
				s_arr[63] = nvobj::make_persistent<C>(
					"a1234567890e");
				s_arr[64] = nvobj::make_persistent<C>(
					"a1234567890fghij");
				s_arr[65] = nvobj::make_persistent<C>(
					"a1234567890j");
				s_arr[66] = nvobj::make_persistent<C>(
					"a1234567890klmnopqrst");
				s_arr[67] = nvobj::make_persistent<C>(
					"a1234567890t");
				s_arr[68] =
					nvobj::make_persistent<C>("a12345bcde");
				s_arr[69] = nvobj::make_persistent<C>(
					"a12345bcdefghij");
				s_arr[70] = nvobj::make_persistent<C>(
					"a12345bcdefghijklmnopqrst");
				s_arr[71] =
					nvobj::make_persistent<C>("a12345cde");
				s_arr[72] = nvobj::make_persistent<C>(
					"a12345cdefghij");
				s_arr[73] = nvobj::make_persistent<C>(
					"a12345cdefghijklmnopqrst");
				s_arr[74] =
					nvobj::make_persistent<C>("a12345de");
				s_arr[75] =
					nvobj::make_persistent<C>("a12345e");
				s_arr[76] = nvobj::make_persistent<C>(
					"a12345fghij");
				s_arr[77] =
					nvobj::make_persistent<C>("a12345j");
				s_arr[78] = nvobj::make_persistent<C>(
					"a12345klmnopqrst");
				s_arr[79] =
					nvobj::make_persistent<C>("a12345t");
				s_arr[80] = nvobj::make_persistent<C>("ab");
				s_arr[81] =
					nvobj::make_persistent<C>("ab12345");
				s_arr[82] = nvobj::make_persistent<C>(
					"ab1234567890");
				s_arr[83] = nvobj::make_persistent<C>(
					"ab12345678901234567890");
				s_arr[84] = nvobj::make_persistent<C>(
					"ab12345678901234567890cde");
				s_arr[85] = nvobj::make_persistent<C>(
					"ab12345678901234567890de");
				s_arr[86] = nvobj::make_persistent<C>(
					"ab12345678901234567890e");
				s_arr[87] = nvobj::make_persistent<C>(
					"ab1234567890cde");
				s_arr[88] = nvobj::make_persistent<C>(
					"ab1234567890de");
				s_arr[89] = nvobj::make_persistent<C>(
					"ab1234567890e");
				s_arr[90] =
					nvobj::make_persistent<C>("ab12345cde");
				s_arr[91] =
					nvobj::make_persistent<C>("ab12345de");
				s_arr[92] =
					nvobj::make_persistent<C>("ab12345e");
				s_arr[93] = nvobj::make_persistent<C>("abcd");
				s_arr[94] =
					nvobj::make_persistent<C>("abcd12345");
				s_arr[95] = nvobj::make_persistent<C>(
					"abcd1234567890");
				s_arr[96] = nvobj::make_persistent<C>(
					"abcd12345678901234567890");
				s_arr[97] = nvobj::make_persistent<C>(
					"abcd12345678901234567890e");
				s_arr[98] = nvobj::make_persistent<C>(
					"abcd1234567890e");
				s_arr[99] =
					nvobj::make_persistent<C>("abcd12345e");
				s_arr[100] = nvobj::make_persistent<C>("abcde");
				s_arr[101] =
					nvobj::make_persistent<C>("abcde12345");
				s_arr[102] = nvobj::make_persistent<C>(
					"abcde1234567890");
				s_arr[103] = nvobj::make_persistent<C>(
					"abcde12345678901234567890");
				s_arr[104] = nvobj::make_persistent<C>(
					"abcde12345678901234567890fghij");
				s_arr[105] = nvobj::make_persistent<C>(
					"abcde12345678901234567890ghij");
				s_arr[106] = nvobj::make_persistent<C>(
					"abcde12345678901234567890hij");
				s_arr[107] = nvobj::make_persistent<C>(
					"abcde12345678901234567890j");
				s_arr[108] = nvobj::make_persistent<C>(
					"abcde1234567890fghij");
				s_arr[109] = nvobj::make_persistent<C>(
					"abcde1234567890ghij");
				s_arr[110] = nvobj::make_persistent<C>(
					"abcde1234567890hij");
				s_arr[111] = nvobj::make_persistent<C>(
					"abcde1234567890j");
				s_arr[112] = nvobj::make_persistent<C>(
					"abcde12345fghij");
				s_arr[113] = nvobj::make_persistent<C>(
					"abcde12345ghij");
				s_arr[114] = nvobj::make_persistent<C>(
					"abcde12345hij");
				s_arr[115] = nvobj::make_persistent<C>(
					"abcde12345j");
				s_arr[116] =
					nvobj::make_persistent<C>("abcdefghi");
				s_arr[117] = nvobj::make_persistent<C>(
					"abcdefghi12345");
				s_arr[118] = nvobj::make_persistent<C>(
					"abcdefghi1234567890");
				s_arr[119] = nvobj::make_persistent<C>(
					"abcdefghi12345678901234567890");
				s_arr[120] = nvobj::make_persistent<C>(
					"abcdefghi12345678901234567890j");
				s_arr[121] = nvobj::make_persistent<C>(
					"abcdefghi1234567890j");
				s_arr[122] = nvobj::make_persistent<C>(
					"abcdefghi12345j");
				s_arr[123] =
					nvobj::make_persistent<C>("abcdefghij");
				s_arr[124] = nvobj::make_persistent<C>(
					"abcdefghij12345");
				s_arr[125] = nvobj::make_persistent<C>(
					"abcdefghij1234567890");
				s_arr[126] = nvobj::make_persistent<C>(
					"abcdefghij12345678901234567890");
				s_arr[127] = nvobj::make_persistent<C>(
					"abcdefghij12345678901234567890klmnopqrst");
				s_arr[128] = nvobj::make_persistent<C>(
					"abcdefghij12345678901234567890lmnopqrst");
				s_arr[129] = nvobj::make_persistent<C>(
					"abcdefghij12345678901234567890pqrst");
				s_arr[130] = nvobj::make_persistent<C>(
					"abcdefghij12345678901234567890t");
				s_arr[131] = nvobj::make_persistent<C>(
					"abcdefghij1234567890klmnopqrst");
				s_arr[132] = nvobj::make_persistent<C>(
					"abcdefghij1234567890lmnopqrst");
				s_arr[133] = nvobj::make_persistent<C>(
					"abcdefghij1234567890pqrst");
				s_arr[134] = nvobj::make_persistent<C>(
					"abcdefghij1234567890t");
				s_arr[135] = nvobj::make_persistent<C>(
					"abcdefghij12345klmnopqrst");
				s_arr[136] = nvobj::make_persistent<C>(
					"abcdefghij12345lmnopqrst");
				s_arr[137] = nvobj::make_persistent<C>(
					"abcdefghij12345pqrst");
				s_arr[138] = nvobj::make_persistent<C>(
					"abcdefghij12345t");
				s_arr[139] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs");
				s_arr[140] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs12345");
				s_arr[141] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs1234567890");
				s_arr[142] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs12345678901234567890");
				s_arr[143] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs12345678901234567890t");
				s_arr[144] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs1234567890t");
				s_arr[145] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs12345t");
				s_arr[146] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst");
				s_arr[147] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst12345");
				s_arr[148] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst1234567890");
				s_arr[149] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst12345678901234567890");
				s_arr[150] = nvobj::make_persistent<C>(
					"abcdefghijlmnopqrst");
				s_arr[151] = nvobj::make_persistent<C>(
					"abcdefghijpqrst");
				s_arr[152] = nvobj::make_persistent<C>(
					"abcdefghijt");
				s_arr[153] =
					nvobj::make_persistent<C>("abcdeghij");
				s_arr[154] =
					nvobj::make_persistent<C>("abcdehij");
				s_arr[155] =
					nvobj::make_persistent<C>("abcdej");
				s_arr[156] = nvobj::make_persistent<C>("abde");
				s_arr[157] = nvobj::make_persistent<C>("abe");
				s_arr[158] = nvobj::make_persistent<C>("acde");
				s_arr[159] =
					nvobj::make_persistent<C>("acdefghij");
				s_arr[160] = nvobj::make_persistent<C>(
					"acdefghijklmnopqrst");
				s_arr[161] = nvobj::make_persistent<C>("ade");
				s_arr[162] = nvobj::make_persistent<C>("ae");
				s_arr[163] =
					nvobj::make_persistent<C>("afghij");
				s_arr[164] = nvobj::make_persistent<C>("aj");
				s_arr[165] = nvobj::make_persistent<C>(
					"aklmnopqrst");
				s_arr[166] = nvobj::make_persistent<C>("at");
				s_arr[167] = nvobj::make_persistent<C>("bcde");
				s_arr[168] =
					nvobj::make_persistent<C>("bcdefghij");
				s_arr[169] = nvobj::make_persistent<C>(
					"bcdefghijklmnopqrst");
				s_arr[170] = nvobj::make_persistent<C>("cde");
				s_arr[171] = nvobj::make_persistent<C>("e");
				s_arr[172] = nvobj::make_persistent<C>("fghij");
				s_arr[173] = nvobj::make_persistent<C>("j");
				s_arr[174] =
					nvobj::make_persistent<C>("klmnopqrst");
				s_arr[175] = nvobj::make_persistent<C>("t");
			});

			test0<C>(pop);
			test1<C>(pop);
			test2<C>(pop);

			nvobj::transaction::run(pop, [&] {
				for (unsigned i = 0; i < 176; ++i) {
					nvobj::delete_persistent<C>(s_arr[i]);
				}
			});

			// test replacing into self
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

			s_short.replace(s_short.begin(), s_short.begin(),
					s_short.c_str());
			UT_ASSERT(s_short == "123/123/");
			s_short.replace(s_short.begin(), s_short.begin(),
					s_short.c_str());
			UT_ASSERT(s_short == "123/123/123/123/");
			s_short.replace(s_short.begin(), s_short.begin(),
					s_short.c_str());
			UT_ASSERT(s_short ==
				  "123/123/123/123/123/123/123/123/");

			s_long.replace(s_long.begin(), s_long.begin(),
				       s_long.c_str());
			UT_ASSERT(
				s_long ==
				"Lorem ipsum dolor sit amet, consectetur/Lorem ipsum dolor sit amet, consectetur/");

			s_extra_long.replace(s_extra_long.begin(),
					     s_extra_long.begin(),
					     s_extra_long.c_str());

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
