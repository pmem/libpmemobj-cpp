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
	nvobj::persistent_ptr<C> s_arr[177];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &s1, typename S::size_type pos,
     typename S::size_type n1, typename S::size_type n2,
     typename S::value_type c, const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<C>(s1); });

	auto &s = *r->s;

	const typename S::size_type old_size = s.size();

	if (pos <= old_size) {
		s.replace(pos, n1, n2, c);
		UT_ASSERT(s == expected);
		typename S::size_type xlen = (std::min)(n1, old_size - pos);
		typename S::size_type rlen = n2;
		UT_ASSERT(s.size() == old_size - xlen + rlen);
	} else {
		try {
			s.replace(pos, n1, n2, c);
			UT_ASSERT(false);
		} catch (std::out_of_range &) {
			UT_ASSERT(pos > old_size);
			UT_ASSERT(s == s1);
		}
	}

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<C>(r->s); });
}

template <class S>
void
test0(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[0], 0, 0, 0, '2', *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, 5, '2', *s_arr[1]);
	test(pop, *s_arr[0], 0, 0, 10, '2', *s_arr[2]);
	test(pop, *s_arr[0], 0, 0, 20, '2', *s_arr[3]);
	test(pop, *s_arr[0], 0, 1, 0, '2', *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, 5, '2', *s_arr[1]);
	test(pop, *s_arr[0], 0, 1, 10, '2', *s_arr[2]);
	test(pop, *s_arr[0], 0, 1, 20, '2', *s_arr[3]);
	test(pop, *s_arr[0], 1, 0, 0, '2', *s_arr[170]);
	test(pop, *s_arr[0], 1, 0, 5, '2', *s_arr[170]);
	test(pop, *s_arr[0], 1, 0, 10, '2', *s_arr[170]);
	test(pop, *s_arr[0], 1, 0, 20, '2', *s_arr[170]);
	test(pop, *s_arr[100], 0, 0, 0, '2', *s_arr[100]);
	test(pop, *s_arr[100], 0, 0, 5, '2', *s_arr[28]);
	test(pop, *s_arr[100], 0, 0, 10, '2', *s_arr[16]);
	test(pop, *s_arr[100], 0, 0, 20, '2', *s_arr[4]);
	test(pop, *s_arr[100], 0, 1, 0, '2', *s_arr[167]);
	test(pop, *s_arr[100], 0, 1, 5, '2', *s_arr[31]);
	test(pop, *s_arr[100], 0, 1, 10, '2', *s_arr[19]);
	test(pop, *s_arr[100], 0, 1, 20, '2', *s_arr[7]);
	test(pop, *s_arr[100], 0, 2, 0, '2', *s_arr[171]);
	test(pop, *s_arr[100], 0, 2, 5, '2', *s_arr[34]);
	test(pop, *s_arr[100], 0, 2, 10, '2', *s_arr[22]);
	test(pop, *s_arr[100], 0, 2, 20, '2', *s_arr[10]);
	test(pop, *s_arr[100], 0, 4, 0, '2', *s_arr[172]);
	test(pop, *s_arr[100], 0, 4, 5, '2', *s_arr[35]);
	test(pop, *s_arr[100], 0, 4, 10, '2', *s_arr[23]);
	test(pop, *s_arr[100], 0, 4, 20, '2', *s_arr[11]);
	test(pop, *s_arr[100], 0, 5, 0, '2', *s_arr[0]);
	test(pop, *s_arr[100], 0, 5, 5, '2', *s_arr[1]);
	test(pop, *s_arr[100], 0, 5, 10, '2', *s_arr[2]);
	test(pop, *s_arr[100], 0, 5, 20, '2', *s_arr[3]);
	test(pop, *s_arr[100], 0, 6, 0, '2', *s_arr[0]);
	test(pop, *s_arr[100], 0, 6, 5, '2', *s_arr[1]);
	test(pop, *s_arr[100], 0, 6, 10, '2', *s_arr[2]);
	test(pop, *s_arr[100], 0, 6, 20, '2', *s_arr[3]);
	test(pop, *s_arr[100], 1, 0, 0, '2', *s_arr[100]);
	test(pop, *s_arr[100], 1, 0, 5, '2', *s_arr[68]);
	test(pop, *s_arr[100], 1, 0, 10, '2', *s_arr[56]);
	test(pop, *s_arr[100], 1, 0, 20, '2', *s_arr[44]);
	test(pop, *s_arr[100], 1, 1, 0, '2', *s_arr[158]);
	test(pop, *s_arr[100], 1, 1, 5, '2', *s_arr[71]);
	test(pop, *s_arr[100], 1, 1, 10, '2', *s_arr[59]);
	test(pop, *s_arr[100], 1, 1, 20, '2', *s_arr[47]);
	test(pop, *s_arr[100], 1, 2, 0, '2', *s_arr[161]);
	test(pop, *s_arr[100], 1, 2, 5, '2', *s_arr[74]);
	test(pop, *s_arr[100], 1, 2, 10, '2', *s_arr[62]);
	test(pop, *s_arr[100], 1, 2, 20, '2', *s_arr[50]);
	test(pop, *s_arr[100], 1, 3, 0, '2', *s_arr[162]);
	test(pop, *s_arr[100], 1, 3, 5, '2', *s_arr[75]);
	test(pop, *s_arr[100], 1, 3, 10, '2', *s_arr[63]);
	test(pop, *s_arr[100], 1, 3, 20, '2', *s_arr[51]);
	test(pop, *s_arr[100], 1, 4, 0, '2', *s_arr[40]);
	test(pop, *s_arr[100], 1, 4, 5, '2', *s_arr[41]);
	test(pop, *s_arr[100], 1, 4, 10, '2', *s_arr[42]);
	test(pop, *s_arr[100], 1, 4, 20, '2', *s_arr[43]);
	test(pop, *s_arr[100], 1, 5, 0, '2', *s_arr[40]);
	test(pop, *s_arr[100], 1, 5, 5, '2', *s_arr[41]);
	test(pop, *s_arr[100], 1, 5, 10, '2', *s_arr[42]);
	test(pop, *s_arr[100], 1, 5, 20, '2', *s_arr[43]);
	test(pop, *s_arr[100], 2, 0, 0, '2', *s_arr[100]);
	test(pop, *s_arr[100], 2, 0, 5, '2', *s_arr[90]);
	test(pop, *s_arr[100], 2, 0, 10, '2', *s_arr[87]);
	test(pop, *s_arr[100], 2, 0, 20, '2', *s_arr[84]);
	test(pop, *s_arr[100], 2, 1, 0, '2', *s_arr[156]);
	test(pop, *s_arr[100], 2, 1, 5, '2', *s_arr[91]);
	test(pop, *s_arr[100], 2, 1, 10, '2', *s_arr[88]);
	test(pop, *s_arr[100], 2, 1, 20, '2', *s_arr[85]);
	test(pop, *s_arr[100], 2, 2, 0, '2', *s_arr[157]);
	test(pop, *s_arr[100], 2, 2, 5, '2', *s_arr[92]);
	test(pop, *s_arr[100], 2, 2, 10, '2', *s_arr[89]);
	test(pop, *s_arr[100], 2, 2, 20, '2', *s_arr[86]);
	test(pop, *s_arr[100], 2, 3, 0, '2', *s_arr[80]);
	test(pop, *s_arr[100], 2, 3, 5, '2', *s_arr[81]);
	test(pop, *s_arr[100], 2, 3, 10, '2', *s_arr[82]);
	test(pop, *s_arr[100], 2, 3, 20, '2', *s_arr[83]);
	test(pop, *s_arr[100], 2, 4, 0, '2', *s_arr[80]);
	test(pop, *s_arr[100], 2, 4, 5, '2', *s_arr[81]);
	test(pop, *s_arr[100], 2, 4, 10, '2', *s_arr[82]);
	test(pop, *s_arr[100], 2, 4, 20, '2', *s_arr[83]);
	test(pop, *s_arr[100], 4, 0, 0, '2', *s_arr[100]);
	test(pop, *s_arr[100], 4, 0, 5, '2', *s_arr[99]);
	test(pop, *s_arr[100], 4, 0, 10, '2', *s_arr[98]);
	test(pop, *s_arr[100], 4, 0, 20, '2', *s_arr[97]);
	test(pop, *s_arr[100], 4, 1, 0, '2', *s_arr[93]);
	test(pop, *s_arr[100], 4, 1, 5, '2', *s_arr[94]);
	test(pop, *s_arr[100], 4, 1, 10, '2', *s_arr[95]);
	test(pop, *s_arr[100], 4, 1, 20, '2', *s_arr[96]);
	test(pop, *s_arr[100], 4, 2, 0, '2', *s_arr[93]);
	test(pop, *s_arr[100], 4, 2, 5, '2', *s_arr[94]);
	test(pop, *s_arr[100], 4, 2, 10, '2', *s_arr[95]);
	test(pop, *s_arr[100], 4, 2, 20, '2', *s_arr[96]);
	test(pop, *s_arr[100], 5, 0, 0, '2', *s_arr[100]);
	test(pop, *s_arr[100], 5, 0, 5, '2', *s_arr[101]);
	test(pop, *s_arr[100], 5, 0, 10, '2', *s_arr[102]);
	test(pop, *s_arr[100], 5, 0, 20, '2', *s_arr[103]);
	test(pop, *s_arr[100], 5, 1, 0, '2', *s_arr[100]);
	test(pop, *s_arr[100], 5, 1, 5, '2', *s_arr[101]);
	test(pop, *s_arr[100], 5, 1, 10, '2', *s_arr[102]);
	test(pop, *s_arr[100], 5, 1, 20, '2', *s_arr[103]);
}

template <class S>
void
test1(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[100], 6, 0, 0, '2', *s_arr[170]);
	test(pop, *s_arr[100], 6, 0, 5, '2', *s_arr[170]);
	test(pop, *s_arr[100], 6, 0, 10, '2', *s_arr[170]);
	test(pop, *s_arr[100], 6, 0, 20, '2', *s_arr[170]);
	test(pop, *s_arr[123], 0, 0, 0, '2', *s_arr[123]);
	test(pop, *s_arr[123], 0, 0, 5, '2', *s_arr[29]);
	test(pop, *s_arr[123], 0, 0, 10, '2', *s_arr[17]);
	test(pop, *s_arr[123], 0, 0, 20, '2', *s_arr[5]);
	test(pop, *s_arr[123], 0, 1, 0, '2', *s_arr[168]);
	test(pop, *s_arr[123], 0, 1, 5, '2', *s_arr[32]);
	test(pop, *s_arr[123], 0, 1, 10, '2', *s_arr[20]);
	test(pop, *s_arr[123], 0, 1, 20, '2', *s_arr[8]);
	test(pop, *s_arr[123], 0, 5, 0, '2', *s_arr[173]);
	test(pop, *s_arr[123], 0, 5, 5, '2', *s_arr[36]);
	test(pop, *s_arr[123], 0, 5, 10, '2', *s_arr[24]);
	test(pop, *s_arr[123], 0, 5, 20, '2', *s_arr[12]);
	test(pop, *s_arr[123], 0, 9, 0, '2', *s_arr[174]);
	test(pop, *s_arr[123], 0, 9, 5, '2', *s_arr[37]);
	test(pop, *s_arr[123], 0, 9, 10, '2', *s_arr[25]);
	test(pop, *s_arr[123], 0, 9, 20, '2', *s_arr[13]);
	test(pop, *s_arr[123], 0, 10, 0, '2', *s_arr[0]);
	test(pop, *s_arr[123], 0, 10, 5, '2', *s_arr[1]);
	test(pop, *s_arr[123], 0, 10, 10, '2', *s_arr[2]);
	test(pop, *s_arr[123], 0, 10, 20, '2', *s_arr[3]);
	test(pop, *s_arr[123], 0, 11, 0, '2', *s_arr[0]);
	test(pop, *s_arr[123], 0, 11, 5, '2', *s_arr[1]);
	test(pop, *s_arr[123], 0, 11, 10, '2', *s_arr[2]);
	test(pop, *s_arr[123], 0, 11, 20, '2', *s_arr[3]);
	test(pop, *s_arr[123], 1, 0, 0, '2', *s_arr[123]);
	test(pop, *s_arr[123], 1, 0, 5, '2', *s_arr[69]);
	test(pop, *s_arr[123], 1, 0, 10, '2', *s_arr[57]);
	test(pop, *s_arr[123], 1, 0, 20, '2', *s_arr[45]);
	test(pop, *s_arr[123], 1, 1, 0, '2', *s_arr[159]);
	test(pop, *s_arr[123], 1, 1, 5, '2', *s_arr[72]);
	test(pop, *s_arr[123], 1, 1, 10, '2', *s_arr[60]);
	test(pop, *s_arr[123], 1, 1, 20, '2', *s_arr[48]);
	test(pop, *s_arr[123], 1, 4, 0, '2', *s_arr[163]);
	test(pop, *s_arr[123], 1, 4, 5, '2', *s_arr[76]);
	test(pop, *s_arr[123], 1, 4, 10, '2', *s_arr[64]);
	test(pop, *s_arr[123], 1, 4, 20, '2', *s_arr[52]);
	test(pop, *s_arr[123], 1, 8, 0, '2', *s_arr[164]);
	test(pop, *s_arr[123], 1, 8, 5, '2', *s_arr[77]);
	test(pop, *s_arr[123], 1, 8, 10, '2', *s_arr[65]);
	test(pop, *s_arr[123], 1, 8, 20, '2', *s_arr[53]);
	test(pop, *s_arr[123], 1, 9, 0, '2', *s_arr[40]);
	test(pop, *s_arr[123], 1, 9, 5, '2', *s_arr[41]);
	test(pop, *s_arr[123], 1, 9, 10, '2', *s_arr[42]);
	test(pop, *s_arr[123], 1, 9, 20, '2', *s_arr[43]);
	test(pop, *s_arr[123], 1, 10, 0, '2', *s_arr[40]);
	test(pop, *s_arr[123], 1, 10, 5, '2', *s_arr[41]);
	test(pop, *s_arr[123], 1, 10, 10, '2', *s_arr[42]);
	test(pop, *s_arr[123], 1, 10, 20, '2', *s_arr[43]);
	test(pop, *s_arr[123], 5, 0, 0, '2', *s_arr[123]);
	test(pop, *s_arr[123], 5, 0, 5, '2', *s_arr[112]);
	test(pop, *s_arr[123], 5, 0, 10, '2', *s_arr[108]);
	test(pop, *s_arr[123], 5, 0, 20, '2', *s_arr[104]);
	test(pop, *s_arr[123], 5, 1, 0, '2', *s_arr[153]);
	test(pop, *s_arr[123], 5, 1, 5, '2', *s_arr[113]);
	test(pop, *s_arr[123], 5, 1, 10, '2', *s_arr[109]);
	test(pop, *s_arr[123], 5, 1, 20, '2', *s_arr[105]);
	test(pop, *s_arr[123], 5, 2, 0, '2', *s_arr[154]);
	test(pop, *s_arr[123], 5, 2, 5, '2', *s_arr[114]);
	test(pop, *s_arr[123], 5, 2, 10, '2', *s_arr[110]);
	test(pop, *s_arr[123], 5, 2, 20, '2', *s_arr[106]);
	test(pop, *s_arr[123], 5, 4, 0, '2', *s_arr[155]);
	test(pop, *s_arr[123], 5, 4, 5, '2', *s_arr[115]);
	test(pop, *s_arr[123], 5, 4, 10, '2', *s_arr[111]);
	test(pop, *s_arr[123], 5, 4, 20, '2', *s_arr[107]);
	test(pop, *s_arr[123], 5, 5, 0, '2', *s_arr[100]);
	test(pop, *s_arr[123], 5, 5, 5, '2', *s_arr[101]);
	test(pop, *s_arr[123], 5, 5, 10, '2', *s_arr[102]);
	test(pop, *s_arr[123], 5, 5, 20, '2', *s_arr[103]);
	test(pop, *s_arr[123], 5, 6, 0, '2', *s_arr[100]);
	test(pop, *s_arr[123], 5, 6, 5, '2', *s_arr[101]);
	test(pop, *s_arr[123], 5, 6, 10, '2', *s_arr[102]);
	test(pop, *s_arr[123], 5, 6, 20, '2', *s_arr[103]);
	test(pop, *s_arr[123], 9, 0, 0, '2', *s_arr[123]);
	test(pop, *s_arr[123], 9, 0, 5, '2', *s_arr[122]);
	test(pop, *s_arr[123], 9, 0, 10, '2', *s_arr[121]);
	test(pop, *s_arr[123], 9, 0, 20, '2', *s_arr[120]);
	test(pop, *s_arr[123], 9, 1, 0, '2', *s_arr[116]);
	test(pop, *s_arr[123], 9, 1, 5, '2', *s_arr[117]);
	test(pop, *s_arr[123], 9, 1, 10, '2', *s_arr[118]);
	test(pop, *s_arr[123], 9, 1, 20, '2', *s_arr[119]);
	test(pop, *s_arr[123], 9, 2, 0, '2', *s_arr[116]);
	test(pop, *s_arr[123], 9, 2, 5, '2', *s_arr[117]);
	test(pop, *s_arr[123], 9, 2, 10, '2', *s_arr[118]);
	test(pop, *s_arr[123], 9, 2, 20, '2', *s_arr[119]);
	test(pop, *s_arr[123], 10, 0, 0, '2', *s_arr[123]);
	test(pop, *s_arr[123], 10, 0, 5, '2', *s_arr[124]);
	test(pop, *s_arr[123], 10, 0, 10, '2', *s_arr[125]);
	test(pop, *s_arr[123], 10, 0, 20, '2', *s_arr[126]);
	test(pop, *s_arr[123], 10, 1, 0, '2', *s_arr[123]);
	test(pop, *s_arr[123], 10, 1, 5, '2', *s_arr[124]);
	test(pop, *s_arr[123], 10, 1, 10, '2', *s_arr[125]);
	test(pop, *s_arr[123], 10, 1, 20, '2', *s_arr[126]);
	test(pop, *s_arr[123], 11, 0, 0, '2', *s_arr[170]);
	test(pop, *s_arr[123], 11, 0, 5, '2', *s_arr[170]);
	test(pop, *s_arr[123], 11, 0, 10, '2', *s_arr[170]);
	test(pop, *s_arr[123], 11, 0, 20, '2', *s_arr[170]);
}

template <class S>
void
test2(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[146], 0, 0, 0, '2', *s_arr[146]);
	test(pop, *s_arr[146], 0, 0, 5, '2', *s_arr[30]);
	test(pop, *s_arr[146], 0, 0, 10, '2', *s_arr[18]);
	test(pop, *s_arr[146], 0, 0, 20, '2', *s_arr[6]);
	test(pop, *s_arr[146], 0, 1, 0, '2', *s_arr[169]);
	test(pop, *s_arr[146], 0, 1, 5, '2', *s_arr[33]);
	test(pop, *s_arr[146], 0, 1, 10, '2', *s_arr[21]);
	test(pop, *s_arr[146], 0, 1, 20, '2', *s_arr[9]);
	test(pop, *s_arr[146], 0, 10, 0, '2', *s_arr[175]);
	test(pop, *s_arr[146], 0, 10, 5, '2', *s_arr[38]);
	test(pop, *s_arr[146], 0, 10, 10, '2', *s_arr[26]);
	test(pop, *s_arr[146], 0, 10, 20, '2', *s_arr[14]);
	test(pop, *s_arr[146], 0, 19, 0, '2', *s_arr[176]);
	test(pop, *s_arr[146], 0, 19, 5, '2', *s_arr[39]);
	test(pop, *s_arr[146], 0, 19, 10, '2', *s_arr[27]);
	test(pop, *s_arr[146], 0, 19, 20, '2', *s_arr[15]);
	test(pop, *s_arr[146], 0, 20, 0, '2', *s_arr[0]);
	test(pop, *s_arr[146], 0, 20, 5, '2', *s_arr[1]);
	test(pop, *s_arr[146], 0, 20, 10, '2', *s_arr[2]);
	test(pop, *s_arr[146], 0, 20, 20, '2', *s_arr[3]);
	test(pop, *s_arr[146], 0, 21, 0, '2', *s_arr[0]);
	test(pop, *s_arr[146], 0, 21, 5, '2', *s_arr[1]);
	test(pop, *s_arr[146], 0, 21, 10, '2', *s_arr[2]);
	test(pop, *s_arr[146], 0, 21, 20, '2', *s_arr[3]);
	test(pop, *s_arr[146], 1, 0, 0, '2', *s_arr[146]);
	test(pop, *s_arr[146], 1, 0, 5, '2', *s_arr[70]);
	test(pop, *s_arr[146], 1, 0, 10, '2', *s_arr[58]);
	test(pop, *s_arr[146], 1, 0, 20, '2', *s_arr[46]);
	test(pop, *s_arr[146], 1, 1, 0, '2', *s_arr[160]);
	test(pop, *s_arr[146], 1, 1, 5, '2', *s_arr[73]);
	test(pop, *s_arr[146], 1, 1, 10, '2', *s_arr[61]);
	test(pop, *s_arr[146], 1, 1, 20, '2', *s_arr[49]);
	test(pop, *s_arr[146], 1, 9, 0, '2', *s_arr[165]);
	test(pop, *s_arr[146], 1, 9, 5, '2', *s_arr[78]);
	test(pop, *s_arr[146], 1, 9, 10, '2', *s_arr[66]);
	test(pop, *s_arr[146], 1, 9, 20, '2', *s_arr[54]);
	test(pop, *s_arr[146], 1, 18, 0, '2', *s_arr[166]);
	test(pop, *s_arr[146], 1, 18, 5, '2', *s_arr[79]);
	test(pop, *s_arr[146], 1, 18, 10, '2', *s_arr[67]);
	test(pop, *s_arr[146], 1, 18, 20, '2', *s_arr[55]);
	test(pop, *s_arr[146], 1, 19, 0, '2', *s_arr[40]);
	test(pop, *s_arr[146], 1, 19, 5, '2', *s_arr[41]);
	test(pop, *s_arr[146], 1, 19, 10, '2', *s_arr[42]);
	test(pop, *s_arr[146], 1, 19, 20, '2', *s_arr[43]);
	test(pop, *s_arr[146], 1, 20, 0, '2', *s_arr[40]);
	test(pop, *s_arr[146], 1, 20, 5, '2', *s_arr[41]);
	test(pop, *s_arr[146], 1, 20, 10, '2', *s_arr[42]);
	test(pop, *s_arr[146], 1, 20, 20, '2', *s_arr[43]);
	test(pop, *s_arr[146], 10, 0, 0, '2', *s_arr[146]);
	test(pop, *s_arr[146], 10, 0, 5, '2', *s_arr[135]);
	test(pop, *s_arr[146], 10, 0, 10, '2', *s_arr[131]);
	test(pop, *s_arr[146], 10, 0, 20, '2', *s_arr[127]);
	test(pop, *s_arr[146], 10, 1, 0, '2', *s_arr[150]);
	test(pop, *s_arr[146], 10, 1, 5, '2', *s_arr[136]);
	test(pop, *s_arr[146], 10, 1, 10, '2', *s_arr[132]);
	test(pop, *s_arr[146], 10, 1, 20, '2', *s_arr[128]);
	test(pop, *s_arr[146], 10, 5, 0, '2', *s_arr[151]);
	test(pop, *s_arr[146], 10, 5, 5, '2', *s_arr[137]);
	test(pop, *s_arr[146], 10, 5, 10, '2', *s_arr[133]);
	test(pop, *s_arr[146], 10, 5, 20, '2', *s_arr[129]);
	test(pop, *s_arr[146], 10, 9, 0, '2', *s_arr[152]);
	test(pop, *s_arr[146], 10, 9, 5, '2', *s_arr[138]);
	test(pop, *s_arr[146], 10, 9, 10, '2', *s_arr[134]);
	test(pop, *s_arr[146], 10, 9, 20, '2', *s_arr[130]);
	test(pop, *s_arr[146], 10, 10, 0, '2', *s_arr[123]);
	test(pop, *s_arr[146], 10, 10, 5, '2', *s_arr[124]);
	test(pop, *s_arr[146], 10, 10, 10, '2', *s_arr[125]);
	test(pop, *s_arr[146], 10, 10, 20, '2', *s_arr[126]);
	test(pop, *s_arr[146], 10, 11, 0, '2', *s_arr[123]);
	test(pop, *s_arr[146], 10, 11, 5, '2', *s_arr[124]);
	test(pop, *s_arr[146], 10, 11, 10, '2', *s_arr[125]);
	test(pop, *s_arr[146], 10, 11, 20, '2', *s_arr[126]);
	test(pop, *s_arr[146], 19, 0, 0, '2', *s_arr[146]);
	test(pop, *s_arr[146], 19, 0, 5, '2', *s_arr[145]);
	test(pop, *s_arr[146], 19, 0, 10, '2', *s_arr[144]);
	test(pop, *s_arr[146], 19, 0, 20, '2', *s_arr[143]);
	test(pop, *s_arr[146], 19, 1, 0, '2', *s_arr[139]);
	test(pop, *s_arr[146], 19, 1, 5, '2', *s_arr[140]);
	test(pop, *s_arr[146], 19, 1, 10, '2', *s_arr[141]);
	test(pop, *s_arr[146], 19, 1, 20, '2', *s_arr[142]);
	test(pop, *s_arr[146], 19, 2, 0, '2', *s_arr[139]);
	test(pop, *s_arr[146], 19, 2, 5, '2', *s_arr[140]);
	test(pop, *s_arr[146], 19, 2, 10, '2', *s_arr[141]);
	test(pop, *s_arr[146], 19, 2, 20, '2', *s_arr[142]);
	test(pop, *s_arr[146], 20, 0, 0, '2', *s_arr[146]);
	test(pop, *s_arr[146], 20, 0, 5, '2', *s_arr[147]);
	test(pop, *s_arr[146], 20, 0, 10, '2', *s_arr[148]);
	test(pop, *s_arr[146], 20, 0, 20, '2', *s_arr[149]);
	test(pop, *s_arr[146], 20, 1, 0, '2', *s_arr[146]);
	test(pop, *s_arr[146], 20, 1, 5, '2', *s_arr[147]);
	test(pop, *s_arr[146], 20, 1, 10, '2', *s_arr[148]);
	test(pop, *s_arr[146], 20, 1, 20, '2', *s_arr[149]);
	test(pop, *s_arr[146], 21, 0, 0, '2', *s_arr[170]);
	test(pop, *s_arr[146], 21, 0, 5, '2', *s_arr[170]);
	test(pop, *s_arr[146], 21, 0, 10, '2', *s_arr[170]);
	test(pop, *s_arr[146], 21, 0, 20, '2', *s_arr[170]);
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
		path, "string_test", 2 * PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();
	{
		auto &s_arr = r->s_arr;

		try {
			nvobj::transaction::run(pop, [&] {
				s_arr[0] = nvobj::make_persistent<C>("");
				s_arr[1] = nvobj::make_persistent<C>("22222");
				s_arr[2] =
					nvobj::make_persistent<C>("2222222222");
				s_arr[3] = nvobj::make_persistent<C>(
					"22222222222222222222");
				s_arr[4] = nvobj::make_persistent<C>(
					"22222222222222222222abcde");
				s_arr[5] = nvobj::make_persistent<C>(
					"22222222222222222222abcdefghij");
				s_arr[6] = nvobj::make_persistent<C>(
					"22222222222222222222abcdefghijklmnopqrst");
				s_arr[7] = nvobj::make_persistent<C>(
					"22222222222222222222bcde");
				s_arr[8] = nvobj::make_persistent<C>(
					"22222222222222222222bcdefghij");
				s_arr[9] = nvobj::make_persistent<C>(
					"22222222222222222222bcdefghijklmnopqrst");
				s_arr[10] = nvobj::make_persistent<C>(
					"22222222222222222222cde");
				s_arr[11] = nvobj::make_persistent<C>(
					"22222222222222222222e");
				s_arr[12] = nvobj::make_persistent<C>(
					"22222222222222222222fghij");
				s_arr[13] = nvobj::make_persistent<C>(
					"22222222222222222222j");
				s_arr[14] = nvobj::make_persistent<C>(
					"22222222222222222222klmnopqrst");
				s_arr[15] = nvobj::make_persistent<C>(
					"22222222222222222222t");
				s_arr[16] = nvobj::make_persistent<C>(
					"2222222222abcde");
				s_arr[17] = nvobj::make_persistent<C>(
					"2222222222abcdefghij");
				s_arr[18] = nvobj::make_persistent<C>(
					"2222222222abcdefghijklmnopqrst");
				s_arr[19] = nvobj::make_persistent<C>(
					"2222222222bcde");
				s_arr[20] = nvobj::make_persistent<C>(
					"2222222222bcdefghij");
				s_arr[21] = nvobj::make_persistent<C>(
					"2222222222bcdefghijklmnopqrst");
				s_arr[22] = nvobj::make_persistent<C>(
					"2222222222cde");
				s_arr[23] = nvobj::make_persistent<C>(
					"2222222222e");
				s_arr[24] = nvobj::make_persistent<C>(
					"2222222222fghij");
				s_arr[25] = nvobj::make_persistent<C>(
					"2222222222j");
				s_arr[26] = nvobj::make_persistent<C>(
					"2222222222klmnopqrst");
				s_arr[27] = nvobj::make_persistent<C>(
					"2222222222t");
				s_arr[28] =
					nvobj::make_persistent<C>("22222abcde");
				s_arr[29] = nvobj::make_persistent<C>(
					"22222abcdefghij");
				s_arr[30] = nvobj::make_persistent<C>(
					"22222abcdefghijklmnopqrst");
				s_arr[31] =
					nvobj::make_persistent<C>("22222bcde");
				s_arr[32] = nvobj::make_persistent<C>(
					"22222bcdefghij");
				s_arr[33] = nvobj::make_persistent<C>(
					"22222bcdefghijklmnopqrst");
				s_arr[34] =
					nvobj::make_persistent<C>("22222cde");
				s_arr[35] = nvobj::make_persistent<C>("22222e");
				s_arr[36] =
					nvobj::make_persistent<C>("22222fghij");
				s_arr[37] = nvobj::make_persistent<C>("22222j");
				s_arr[38] = nvobj::make_persistent<C>(
					"22222klmnopqrst");
				s_arr[39] = nvobj::make_persistent<C>("22222t");
				s_arr[40] = nvobj::make_persistent<C>("a");
				s_arr[41] = nvobj::make_persistent<C>("a22222");
				s_arr[42] = nvobj::make_persistent<C>(
					"a2222222222");
				s_arr[43] = nvobj::make_persistent<C>(
					"a22222222222222222222");
				s_arr[44] = nvobj::make_persistent<C>(
					"a22222222222222222222bcde");
				s_arr[45] = nvobj::make_persistent<C>(
					"a22222222222222222222bcdefghij");
				s_arr[46] = nvobj::make_persistent<C>(
					"a22222222222222222222bcdefghijklmnopqrst");
				s_arr[47] = nvobj::make_persistent<C>(
					"a22222222222222222222cde");
				s_arr[48] = nvobj::make_persistent<C>(
					"a22222222222222222222cdefghij");
				s_arr[49] = nvobj::make_persistent<C>(
					"a22222222222222222222cdefghijklmnopqrst");
				s_arr[50] = nvobj::make_persistent<C>(
					"a22222222222222222222de");
				s_arr[51] = nvobj::make_persistent<C>(
					"a22222222222222222222e");
				s_arr[52] = nvobj::make_persistent<C>(
					"a22222222222222222222fghij");
				s_arr[53] = nvobj::make_persistent<C>(
					"a22222222222222222222j");
				s_arr[54] = nvobj::make_persistent<C>(
					"a22222222222222222222klmnopqrst");
				s_arr[55] = nvobj::make_persistent<C>(
					"a22222222222222222222t");
				s_arr[56] = nvobj::make_persistent<C>(
					"a2222222222bcde");
				s_arr[57] = nvobj::make_persistent<C>(
					"a2222222222bcdefghij");
				s_arr[58] = nvobj::make_persistent<C>(
					"a2222222222bcdefghijklmnopqrst");
				s_arr[59] = nvobj::make_persistent<C>(
					"a2222222222cde");
				s_arr[60] = nvobj::make_persistent<C>(
					"a2222222222cdefghij");
				s_arr[61] = nvobj::make_persistent<C>(
					"a2222222222cdefghijklmnopqrst");
				s_arr[62] = nvobj::make_persistent<C>(
					"a2222222222de");
				s_arr[63] = nvobj::make_persistent<C>(
					"a2222222222e");
				s_arr[64] = nvobj::make_persistent<C>(
					"a2222222222fghij");
				s_arr[65] = nvobj::make_persistent<C>(
					"a2222222222j");
				s_arr[66] = nvobj::make_persistent<C>(
					"a2222222222klmnopqrst");
				s_arr[67] = nvobj::make_persistent<C>(
					"a2222222222t");
				s_arr[68] =
					nvobj::make_persistent<C>("a22222bcde");
				s_arr[69] = nvobj::make_persistent<C>(
					"a22222bcdefghij");
				s_arr[70] = nvobj::make_persistent<C>(
					"a22222bcdefghijklmnopqrst");
				s_arr[71] =
					nvobj::make_persistent<C>("a22222cde");
				s_arr[72] = nvobj::make_persistent<C>(
					"a22222cdefghij");
				s_arr[73] = nvobj::make_persistent<C>(
					"a22222cdefghijklmnopqrst");
				s_arr[74] =
					nvobj::make_persistent<C>("a22222de");
				s_arr[75] =
					nvobj::make_persistent<C>("a22222e");
				s_arr[76] = nvobj::make_persistent<C>(
					"a22222fghij");
				s_arr[77] =
					nvobj::make_persistent<C>("a22222j");
				s_arr[78] = nvobj::make_persistent<C>(
					"a22222klmnopqrst");
				s_arr[79] =
					nvobj::make_persistent<C>("a22222t");
				s_arr[80] = nvobj::make_persistent<C>("ab");
				s_arr[81] =
					nvobj::make_persistent<C>("ab22222");
				s_arr[82] = nvobj::make_persistent<C>(
					"ab2222222222");
				s_arr[83] = nvobj::make_persistent<C>(
					"ab22222222222222222222");
				s_arr[84] = nvobj::make_persistent<C>(
					"ab22222222222222222222cde");
				s_arr[85] = nvobj::make_persistent<C>(
					"ab22222222222222222222de");
				s_arr[86] = nvobj::make_persistent<C>(
					"ab22222222222222222222e");
				s_arr[87] = nvobj::make_persistent<C>(
					"ab2222222222cde");
				s_arr[88] = nvobj::make_persistent<C>(
					"ab2222222222de");
				s_arr[89] = nvobj::make_persistent<C>(
					"ab2222222222e");
				s_arr[90] =
					nvobj::make_persistent<C>("ab22222cde");
				s_arr[91] =
					nvobj::make_persistent<C>("ab22222de");
				s_arr[92] =
					nvobj::make_persistent<C>("ab22222e");
				s_arr[93] = nvobj::make_persistent<C>("abcd");
				s_arr[94] =
					nvobj::make_persistent<C>("abcd22222");
				s_arr[95] = nvobj::make_persistent<C>(
					"abcd2222222222");
				s_arr[96] = nvobj::make_persistent<C>(
					"abcd22222222222222222222");
				s_arr[97] = nvobj::make_persistent<C>(
					"abcd22222222222222222222e");
				s_arr[98] = nvobj::make_persistent<C>(
					"abcd2222222222e");
				s_arr[99] =
					nvobj::make_persistent<C>("abcd22222e");
				s_arr[100] = nvobj::make_persistent<C>("abcde");
				s_arr[101] =
					nvobj::make_persistent<C>("abcde22222");
				s_arr[102] = nvobj::make_persistent<C>(
					"abcde2222222222");
				s_arr[103] = nvobj::make_persistent<C>(
					"abcde22222222222222222222");
				s_arr[104] = nvobj::make_persistent<C>(
					"abcde22222222222222222222fghij");
				s_arr[105] = nvobj::make_persistent<C>(
					"abcde22222222222222222222ghij");
				s_arr[106] = nvobj::make_persistent<C>(
					"abcde22222222222222222222hij");
				s_arr[107] = nvobj::make_persistent<C>(
					"abcde22222222222222222222j");
				s_arr[108] = nvobj::make_persistent<C>(
					"abcde2222222222fghij");
				s_arr[109] = nvobj::make_persistent<C>(
					"abcde2222222222ghij");
				s_arr[110] = nvobj::make_persistent<C>(
					"abcde2222222222hij");
				s_arr[111] = nvobj::make_persistent<C>(
					"abcde2222222222j");
				s_arr[112] = nvobj::make_persistent<C>(
					"abcde22222fghij");
				s_arr[113] = nvobj::make_persistent<C>(
					"abcde22222ghij");
				s_arr[114] = nvobj::make_persistent<C>(
					"abcde22222hij");
				s_arr[115] = nvobj::make_persistent<C>(
					"abcde22222j");
				s_arr[116] =
					nvobj::make_persistent<C>("abcdefghi");
				s_arr[117] = nvobj::make_persistent<C>(
					"abcdefghi22222");
				s_arr[118] = nvobj::make_persistent<C>(
					"abcdefghi2222222222");
				s_arr[119] = nvobj::make_persistent<C>(
					"abcdefghi22222222222222222222");
				s_arr[120] = nvobj::make_persistent<C>(
					"abcdefghi22222222222222222222j");
				s_arr[121] = nvobj::make_persistent<C>(
					"abcdefghi2222222222j");
				s_arr[122] = nvobj::make_persistent<C>(
					"abcdefghi22222j");
				s_arr[123] =
					nvobj::make_persistent<C>("abcdefghij");
				s_arr[124] = nvobj::make_persistent<C>(
					"abcdefghij22222");
				s_arr[125] = nvobj::make_persistent<C>(
					"abcdefghij2222222222");
				s_arr[126] = nvobj::make_persistent<C>(
					"abcdefghij22222222222222222222");
				s_arr[127] = nvobj::make_persistent<C>(
					"abcdefghij22222222222222222222klmnopqrst");
				s_arr[128] = nvobj::make_persistent<C>(
					"abcdefghij22222222222222222222lmnopqrst");
				s_arr[129] = nvobj::make_persistent<C>(
					"abcdefghij22222222222222222222pqrst");
				s_arr[130] = nvobj::make_persistent<C>(
					"abcdefghij22222222222222222222t");
				s_arr[131] = nvobj::make_persistent<C>(
					"abcdefghij2222222222klmnopqrst");
				s_arr[132] = nvobj::make_persistent<C>(
					"abcdefghij2222222222lmnopqrst");
				s_arr[133] = nvobj::make_persistent<C>(
					"abcdefghij2222222222pqrst");
				s_arr[134] = nvobj::make_persistent<C>(
					"abcdefghij2222222222t");
				s_arr[135] = nvobj::make_persistent<C>(
					"abcdefghij22222klmnopqrst");
				s_arr[136] = nvobj::make_persistent<C>(
					"abcdefghij22222lmnopqrst");
				s_arr[137] = nvobj::make_persistent<C>(
					"abcdefghij22222pqrst");
				s_arr[138] = nvobj::make_persistent<C>(
					"abcdefghij22222t");
				s_arr[139] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs");
				s_arr[140] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs22222");
				s_arr[141] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs2222222222");
				s_arr[142] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs22222222222222222222");
				s_arr[143] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs22222222222222222222t");
				s_arr[144] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs2222222222t");
				s_arr[145] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs22222t");
				s_arr[146] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst");
				s_arr[147] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst22222");
				s_arr[148] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst2222222222");
				s_arr[149] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst22222222222222222222");
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
				s_arr[170] = nvobj::make_persistent<C>(
					"can't happen");
				s_arr[171] = nvobj::make_persistent<C>("cde");
				s_arr[172] = nvobj::make_persistent<C>("e");
				s_arr[173] = nvobj::make_persistent<C>("fghij");
				s_arr[174] = nvobj::make_persistent<C>("j");
				s_arr[175] =
					nvobj::make_persistent<C>("klmnopqrst");
				s_arr[176] = nvobj::make_persistent<C>("t");
			});

			test0<C>(pop);
			test1<C>(pop);
			test2<C>(pop);

			nvobj::transaction::run(pop, [&] {
				for (unsigned i = 0; i < 177; ++i) {
					nvobj::delete_persistent<C>(s_arr[i]);
				}
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}

	pop.close();

	return 0;
}
