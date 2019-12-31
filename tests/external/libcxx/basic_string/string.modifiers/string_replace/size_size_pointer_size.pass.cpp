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
	nvobj::persistent_ptr<C> s_arr[412];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &s1, typename S::size_type pos,
     typename S::size_type n1, const typename S::value_type *str,
     typename S::size_type n2, const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<C>(s1); });

	auto &s = *r->s;

	const typename S::size_type old_size = s.size();

	if (pos <= old_size) {
		s.replace(pos, n1, str, n2);
		UT_ASSERT(s == expected);
		typename S::size_type xlen = (std::min)(n1, old_size - pos);
		typename S::size_type rlen = n2;
		UT_ASSERT(s.size() == old_size - xlen + rlen);
	} else {
		try {
			s.replace(pos, n1, str, n2);
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

	test(pop, *s_arr[0], 0, 0, "", 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, "12345", 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, "12345", 1, *s_arr[1]);
	test(pop, *s_arr[0], 0, 0, "12345", 2, *s_arr[2]);
	test(pop, *s_arr[0], 0, 0, "12345", 4, *s_arr[3]);
	test(pop, *s_arr[0], 0, 0, "12345", 5, *s_arr[4]);
	test(pop, *s_arr[0], 0, 0, "1234567890", 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, "1234567890", 1, *s_arr[1]);
	test(pop, *s_arr[0], 0, 0, "1234567890", 5, *s_arr[4]);
	test(pop, *s_arr[0], 0, 0, "1234567890", 9, *s_arr[5]);
	test(pop, *s_arr[0], 0, 0, "1234567890", 10, *s_arr[6]);
	test(pop, *s_arr[0], 0, 0, "12345678901234567890", 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, "12345678901234567890", 1, *s_arr[1]);
	test(pop, *s_arr[0], 0, 0, "12345678901234567890", 10, *s_arr[6]);
	test(pop, *s_arr[0], 0, 0, "12345678901234567890", 19, *s_arr[7]);
	test(pop, *s_arr[0], 0, 0, "12345678901234567890", 20, *s_arr[8]);
	test(pop, *s_arr[0], 0, 1, "", 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, "12345", 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, "12345", 1, *s_arr[1]);
	test(pop, *s_arr[0], 0, 1, "12345", 2, *s_arr[2]);
	test(pop, *s_arr[0], 0, 1, "12345", 4, *s_arr[3]);
	test(pop, *s_arr[0], 0, 1, "12345", 5, *s_arr[4]);
	test(pop, *s_arr[0], 0, 1, "1234567890", 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, "1234567890", 1, *s_arr[1]);
	test(pop, *s_arr[0], 0, 1, "1234567890", 5, *s_arr[4]);
	test(pop, *s_arr[0], 0, 1, "1234567890", 9, *s_arr[5]);
	test(pop, *s_arr[0], 0, 1, "1234567890", 10, *s_arr[6]);
	test(pop, *s_arr[0], 0, 1, "12345678901234567890", 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, "12345678901234567890", 1, *s_arr[1]);
	test(pop, *s_arr[0], 0, 1, "12345678901234567890", 10, *s_arr[6]);
	test(pop, *s_arr[0], 0, 1, "12345678901234567890", 19, *s_arr[7]);
	test(pop, *s_arr[0], 0, 1, "12345678901234567890", 20, *s_arr[8]);
	test(pop, *s_arr[0], 1, 0, "", 0, *s_arr[405]);
	test(pop, *s_arr[0], 1, 0, "12345", 0, *s_arr[405]);
	test(pop, *s_arr[0], 1, 0, "12345", 1, *s_arr[405]);
	test(pop, *s_arr[0], 1, 0, "12345", 2, *s_arr[405]);
	test(pop, *s_arr[0], 1, 0, "12345", 4, *s_arr[405]);
	test(pop, *s_arr[0], 1, 0, "12345", 5, *s_arr[405]);
	test(pop, *s_arr[0], 1, 0, "1234567890", 0, *s_arr[405]);
	test(pop, *s_arr[0], 1, 0, "1234567890", 1, *s_arr[405]);
	test(pop, *s_arr[0], 1, 0, "1234567890", 5, *s_arr[405]);
	test(pop, *s_arr[0], 1, 0, "1234567890", 9, *s_arr[405]);
	test(pop, *s_arr[0], 1, 0, "1234567890", 10, *s_arr[405]);
	test(pop, *s_arr[0], 1, 0, "12345678901234567890", 0, *s_arr[405]);
	test(pop, *s_arr[0], 1, 0, "12345678901234567890", 1, *s_arr[405]);
	test(pop, *s_arr[0], 1, 0, "12345678901234567890", 10, *s_arr[405]);
	test(pop, *s_arr[0], 1, 0, "12345678901234567890", 19, *s_arr[405]);
	test(pop, *s_arr[0], 1, 0, "12345678901234567890", 20, *s_arr[405]);
	test(pop, *s_arr[260], 0, 0, "", 0, *s_arr[260]);
	test(pop, *s_arr[260], 0, 0, "12345", 0, *s_arr[260]);
	test(pop, *s_arr[260], 0, 0, "12345", 1, *s_arr[93]);
	test(pop, *s_arr[260], 0, 0, "12345", 2, *s_arr[81]);
	test(pop, *s_arr[260], 0, 0, "12345", 4, *s_arr[69]);
	test(pop, *s_arr[260], 0, 0, "12345", 5, *s_arr[57]);
	test(pop, *s_arr[260], 0, 0, "1234567890", 0, *s_arr[260]);
	test(pop, *s_arr[260], 0, 0, "1234567890", 1, *s_arr[93]);
	test(pop, *s_arr[260], 0, 0, "1234567890", 5, *s_arr[57]);
	test(pop, *s_arr[260], 0, 0, "1234567890", 9, *s_arr[45]);
	test(pop, *s_arr[260], 0, 0, "1234567890", 10, *s_arr[33]);
	test(pop, *s_arr[260], 0, 0, "12345678901234567890", 0, *s_arr[260]);
	test(pop, *s_arr[260], 0, 0, "12345678901234567890", 1, *s_arr[93]);
	test(pop, *s_arr[260], 0, 0, "12345678901234567890", 10, *s_arr[33]);
	test(pop, *s_arr[260], 0, 0, "12345678901234567890", 19, *s_arr[21]);
	test(pop, *s_arr[260], 0, 0, "12345678901234567890", 20, *s_arr[9]);
	test(pop, *s_arr[260], 0, 1, "", 0, *s_arr[402]);
	test(pop, *s_arr[260], 0, 1, "12345", 0, *s_arr[402]);
	test(pop, *s_arr[260], 0, 1, "12345", 1, *s_arr[96]);
	test(pop, *s_arr[260], 0, 1, "12345", 2, *s_arr[84]);
	test(pop, *s_arr[260], 0, 1, "12345", 4, *s_arr[72]);
	test(pop, *s_arr[260], 0, 1, "12345", 5, *s_arr[60]);
	test(pop, *s_arr[260], 0, 1, "1234567890", 0, *s_arr[402]);
	test(pop, *s_arr[260], 0, 1, "1234567890", 1, *s_arr[96]);
	test(pop, *s_arr[260], 0, 1, "1234567890", 5, *s_arr[60]);
	test(pop, *s_arr[260], 0, 1, "1234567890", 9, *s_arr[48]);
	test(pop, *s_arr[260], 0, 1, "1234567890", 10, *s_arr[36]);
	test(pop, *s_arr[260], 0, 1, "12345678901234567890", 0, *s_arr[402]);
	test(pop, *s_arr[260], 0, 1, "12345678901234567890", 1, *s_arr[96]);
	test(pop, *s_arr[260], 0, 1, "12345678901234567890", 10, *s_arr[36]);
	test(pop, *s_arr[260], 0, 1, "12345678901234567890", 19, *s_arr[24]);
	test(pop, *s_arr[260], 0, 1, "12345678901234567890", 20, *s_arr[12]);
	test(pop, *s_arr[260], 0, 2, "", 0, *s_arr[406]);
	test(pop, *s_arr[260], 0, 2, "12345", 0, *s_arr[406]);
	test(pop, *s_arr[260], 0, 2, "12345", 1, *s_arr[99]);
	test(pop, *s_arr[260], 0, 2, "12345", 2, *s_arr[87]);
	test(pop, *s_arr[260], 0, 2, "12345", 4, *s_arr[75]);
	test(pop, *s_arr[260], 0, 2, "12345", 5, *s_arr[63]);
	test(pop, *s_arr[260], 0, 2, "1234567890", 0, *s_arr[406]);
	test(pop, *s_arr[260], 0, 2, "1234567890", 1, *s_arr[99]);
	test(pop, *s_arr[260], 0, 2, "1234567890", 5, *s_arr[63]);
	test(pop, *s_arr[260], 0, 2, "1234567890", 9, *s_arr[51]);
	test(pop, *s_arr[260], 0, 2, "1234567890", 10, *s_arr[39]);
	test(pop, *s_arr[260], 0, 2, "12345678901234567890", 0, *s_arr[406]);
	test(pop, *s_arr[260], 0, 2, "12345678901234567890", 1, *s_arr[99]);
	test(pop, *s_arr[260], 0, 2, "12345678901234567890", 10, *s_arr[39]);
	test(pop, *s_arr[260], 0, 2, "12345678901234567890", 19, *s_arr[27]);
	test(pop, *s_arr[260], 0, 2, "12345678901234567890", 20, *s_arr[15]);
	test(pop, *s_arr[260], 0, 4, "", 0, *s_arr[407]);
	test(pop, *s_arr[260], 0, 4, "12345", 0, *s_arr[407]);
	test(pop, *s_arr[260], 0, 4, "12345", 1, *s_arr[100]);
	test(pop, *s_arr[260], 0, 4, "12345", 2, *s_arr[88]);
}

template <class S>
void
test1(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[260], 0, 4, "12345", 4, *s_arr[76]);
	test(pop, *s_arr[260], 0, 4, "12345", 5, *s_arr[64]);
	test(pop, *s_arr[260], 0, 4, "1234567890", 0, *s_arr[407]);
	test(pop, *s_arr[260], 0, 4, "1234567890", 1, *s_arr[100]);
	test(pop, *s_arr[260], 0, 4, "1234567890", 5, *s_arr[64]);
	test(pop, *s_arr[260], 0, 4, "1234567890", 9, *s_arr[52]);
	test(pop, *s_arr[260], 0, 4, "1234567890", 10, *s_arr[40]);
	test(pop, *s_arr[260], 0, 4, "12345678901234567890", 0, *s_arr[407]);
	test(pop, *s_arr[260], 0, 4, "12345678901234567890", 1, *s_arr[100]);
	test(pop, *s_arr[260], 0, 4, "12345678901234567890", 10, *s_arr[40]);
	test(pop, *s_arr[260], 0, 4, "12345678901234567890", 19, *s_arr[28]);
	test(pop, *s_arr[260], 0, 4, "12345678901234567890", 20, *s_arr[16]);
	test(pop, *s_arr[260], 0, 5, "", 0, *s_arr[0]);
	test(pop, *s_arr[260], 0, 5, "12345", 0, *s_arr[0]);
	test(pop, *s_arr[260], 0, 5, "12345", 1, *s_arr[1]);
	test(pop, *s_arr[260], 0, 5, "12345", 2, *s_arr[2]);
	test(pop, *s_arr[260], 0, 5, "12345", 4, *s_arr[3]);
	test(pop, *s_arr[260], 0, 5, "12345", 5, *s_arr[4]);
	test(pop, *s_arr[260], 0, 5, "1234567890", 0, *s_arr[0]);
	test(pop, *s_arr[260], 0, 5, "1234567890", 1, *s_arr[1]);
	test(pop, *s_arr[260], 0, 5, "1234567890", 5, *s_arr[4]);
	test(pop, *s_arr[260], 0, 5, "1234567890", 9, *s_arr[5]);
	test(pop, *s_arr[260], 0, 5, "1234567890", 10, *s_arr[6]);
	test(pop, *s_arr[260], 0, 5, "12345678901234567890", 0, *s_arr[0]);
	test(pop, *s_arr[260], 0, 5, "12345678901234567890", 1, *s_arr[1]);
	test(pop, *s_arr[260], 0, 5, "12345678901234567890", 10, *s_arr[6]);
	test(pop, *s_arr[260], 0, 5, "12345678901234567890", 19, *s_arr[7]);
	test(pop, *s_arr[260], 0, 5, "12345678901234567890", 20, *s_arr[8]);
	test(pop, *s_arr[260], 0, 6, "", 0, *s_arr[0]);
	test(pop, *s_arr[260], 0, 6, "12345", 0, *s_arr[0]);
	test(pop, *s_arr[260], 0, 6, "12345", 1, *s_arr[1]);
	test(pop, *s_arr[260], 0, 6, "12345", 2, *s_arr[2]);
	test(pop, *s_arr[260], 0, 6, "12345", 4, *s_arr[3]);
	test(pop, *s_arr[260], 0, 6, "12345", 5, *s_arr[4]);
	test(pop, *s_arr[260], 0, 6, "1234567890", 0, *s_arr[0]);
	test(pop, *s_arr[260], 0, 6, "1234567890", 1, *s_arr[1]);
	test(pop, *s_arr[260], 0, 6, "1234567890", 5, *s_arr[4]);
	test(pop, *s_arr[260], 0, 6, "1234567890", 9, *s_arr[5]);
	test(pop, *s_arr[260], 0, 6, "1234567890", 10, *s_arr[6]);
	test(pop, *s_arr[260], 0, 6, "12345678901234567890", 0, *s_arr[0]);
	test(pop, *s_arr[260], 0, 6, "12345678901234567890", 1, *s_arr[1]);
	test(pop, *s_arr[260], 0, 6, "12345678901234567890", 10, *s_arr[6]);
	test(pop, *s_arr[260], 0, 6, "12345678901234567890", 19, *s_arr[7]);
	test(pop, *s_arr[260], 0, 6, "12345678901234567890", 20, *s_arr[8]);
	test(pop, *s_arr[260], 1, 0, "", 0, *s_arr[260]);
	test(pop, *s_arr[260], 1, 0, "12345", 0, *s_arr[260]);
	test(pop, *s_arr[260], 1, 0, "12345", 1, *s_arr[198]);
	test(pop, *s_arr[260], 1, 0, "12345", 2, *s_arr[186]);
	test(pop, *s_arr[260], 1, 0, "12345", 4, *s_arr[174]);
	test(pop, *s_arr[260], 1, 0, "12345", 5, *s_arr[162]);
	test(pop, *s_arr[260], 1, 0, "1234567890", 0, *s_arr[260]);
	test(pop, *s_arr[260], 1, 0, "1234567890", 1, *s_arr[198]);
	test(pop, *s_arr[260], 1, 0, "1234567890", 5, *s_arr[162]);
	test(pop, *s_arr[260], 1, 0, "1234567890", 9, *s_arr[150]);
	test(pop, *s_arr[260], 1, 0, "1234567890", 10, *s_arr[138]);
	test(pop, *s_arr[260], 1, 0, "12345678901234567890", 0, *s_arr[260]);
	test(pop, *s_arr[260], 1, 0, "12345678901234567890", 1, *s_arr[198]);
	test(pop, *s_arr[260], 1, 0, "12345678901234567890", 10, *s_arr[138]);
	test(pop, *s_arr[260], 1, 0, "12345678901234567890", 19, *s_arr[126]);
	test(pop, *s_arr[260], 1, 0, "12345678901234567890", 20, *s_arr[114]);
	test(pop, *s_arr[260], 1, 1, "", 0, *s_arr[393]);
	test(pop, *s_arr[260], 1, 1, "12345", 0, *s_arr[393]);
	test(pop, *s_arr[260], 1, 1, "12345", 1, *s_arr[201]);
	test(pop, *s_arr[260], 1, 1, "12345", 2, *s_arr[189]);
	test(pop, *s_arr[260], 1, 1, "12345", 4, *s_arr[177]);
	test(pop, *s_arr[260], 1, 1, "12345", 5, *s_arr[165]);
	test(pop, *s_arr[260], 1, 1, "1234567890", 0, *s_arr[393]);
	test(pop, *s_arr[260], 1, 1, "1234567890", 1, *s_arr[201]);
	test(pop, *s_arr[260], 1, 1, "1234567890", 5, *s_arr[165]);
	test(pop, *s_arr[260], 1, 1, "1234567890", 9, *s_arr[153]);
	test(pop, *s_arr[260], 1, 1, "1234567890", 10, *s_arr[141]);
	test(pop, *s_arr[260], 1, 1, "12345678901234567890", 0, *s_arr[393]);
	test(pop, *s_arr[260], 1, 1, "12345678901234567890", 1, *s_arr[201]);
	test(pop, *s_arr[260], 1, 1, "12345678901234567890", 10, *s_arr[141]);
	test(pop, *s_arr[260], 1, 1, "12345678901234567890", 19, *s_arr[129]);
	test(pop, *s_arr[260], 1, 1, "12345678901234567890", 20, *s_arr[117]);
	test(pop, *s_arr[260], 1, 2, "", 0, *s_arr[396]);
	test(pop, *s_arr[260], 1, 2, "12345", 0, *s_arr[396]);
	test(pop, *s_arr[260], 1, 2, "12345", 1, *s_arr[204]);
	test(pop, *s_arr[260], 1, 2, "12345", 2, *s_arr[192]);
	test(pop, *s_arr[260], 1, 2, "12345", 4, *s_arr[180]);
	test(pop, *s_arr[260], 1, 2, "12345", 5, *s_arr[168]);
	test(pop, *s_arr[260], 1, 2, "1234567890", 0, *s_arr[396]);
	test(pop, *s_arr[260], 1, 2, "1234567890", 1, *s_arr[204]);
	test(pop, *s_arr[260], 1, 2, "1234567890", 5, *s_arr[168]);
	test(pop, *s_arr[260], 1, 2, "1234567890", 9, *s_arr[156]);
	test(pop, *s_arr[260], 1, 2, "1234567890", 10, *s_arr[144]);
	test(pop, *s_arr[260], 1, 2, "12345678901234567890", 0, *s_arr[396]);
	test(pop, *s_arr[260], 1, 2, "12345678901234567890", 1, *s_arr[204]);
	test(pop, *s_arr[260], 1, 2, "12345678901234567890", 10, *s_arr[144]);
	test(pop, *s_arr[260], 1, 2, "12345678901234567890", 19, *s_arr[132]);
	test(pop, *s_arr[260], 1, 2, "12345678901234567890", 20, *s_arr[120]);
	test(pop, *s_arr[260], 1, 3, "", 0, *s_arr[397]);
	test(pop, *s_arr[260], 1, 3, "12345", 0, *s_arr[397]);
	test(pop, *s_arr[260], 1, 3, "12345", 1, *s_arr[205]);
	test(pop, *s_arr[260], 1, 3, "12345", 2, *s_arr[193]);
	test(pop, *s_arr[260], 1, 3, "12345", 4, *s_arr[181]);
	test(pop, *s_arr[260], 1, 3, "12345", 5, *s_arr[169]);
	test(pop, *s_arr[260], 1, 3, "1234567890", 0, *s_arr[397]);
	test(pop, *s_arr[260], 1, 3, "1234567890", 1, *s_arr[205]);
}

template <class S>
void
test2(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[260], 1, 3, "1234567890", 5, *s_arr[169]);
	test(pop, *s_arr[260], 1, 3, "1234567890", 9, *s_arr[157]);
	test(pop, *s_arr[260], 1, 3, "1234567890", 10, *s_arr[145]);
	test(pop, *s_arr[260], 1, 3, "12345678901234567890", 0, *s_arr[397]);
	test(pop, *s_arr[260], 1, 3, "12345678901234567890", 1, *s_arr[205]);
	test(pop, *s_arr[260], 1, 3, "12345678901234567890", 10, *s_arr[145]);
	test(pop, *s_arr[260], 1, 3, "12345678901234567890", 19, *s_arr[133]);
	test(pop, *s_arr[260], 1, 3, "12345678901234567890", 20, *s_arr[121]);
	test(pop, *s_arr[260], 1, 4, "", 0, *s_arr[105]);
	test(pop, *s_arr[260], 1, 4, "12345", 0, *s_arr[105]);
	test(pop, *s_arr[260], 1, 4, "12345", 1, *s_arr[106]);
	test(pop, *s_arr[260], 1, 4, "12345", 2, *s_arr[107]);
	test(pop, *s_arr[260], 1, 4, "12345", 4, *s_arr[108]);
	test(pop, *s_arr[260], 1, 4, "12345", 5, *s_arr[109]);
	test(pop, *s_arr[260], 1, 4, "1234567890", 0, *s_arr[105]);
	test(pop, *s_arr[260], 1, 4, "1234567890", 1, *s_arr[106]);
	test(pop, *s_arr[260], 1, 4, "1234567890", 5, *s_arr[109]);
	test(pop, *s_arr[260], 1, 4, "1234567890", 9, *s_arr[110]);
	test(pop, *s_arr[260], 1, 4, "1234567890", 10, *s_arr[111]);
	test(pop, *s_arr[260], 1, 4, "12345678901234567890", 0, *s_arr[105]);
	test(pop, *s_arr[260], 1, 4, "12345678901234567890", 1, *s_arr[106]);
	test(pop, *s_arr[260], 1, 4, "12345678901234567890", 10, *s_arr[111]);
	test(pop, *s_arr[260], 1, 4, "12345678901234567890", 19, *s_arr[112]);
	test(pop, *s_arr[260], 1, 4, "12345678901234567890", 20, *s_arr[113]);
	test(pop, *s_arr[260], 1, 5, "", 0, *s_arr[105]);
	test(pop, *s_arr[260], 1, 5, "12345", 0, *s_arr[105]);
	test(pop, *s_arr[260], 1, 5, "12345", 1, *s_arr[106]);
	test(pop, *s_arr[260], 1, 5, "12345", 2, *s_arr[107]);
	test(pop, *s_arr[260], 1, 5, "12345", 4, *s_arr[108]);
	test(pop, *s_arr[260], 1, 5, "12345", 5, *s_arr[109]);
	test(pop, *s_arr[260], 1, 5, "1234567890", 0, *s_arr[105]);
	test(pop, *s_arr[260], 1, 5, "1234567890", 1, *s_arr[106]);
	test(pop, *s_arr[260], 1, 5, "1234567890", 5, *s_arr[109]);
	test(pop, *s_arr[260], 1, 5, "1234567890", 9, *s_arr[110]);
	test(pop, *s_arr[260], 1, 5, "1234567890", 10, *s_arr[111]);
	test(pop, *s_arr[260], 1, 5, "12345678901234567890", 0, *s_arr[105]);
	test(pop, *s_arr[260], 1, 5, "12345678901234567890", 1, *s_arr[106]);
	test(pop, *s_arr[260], 1, 5, "12345678901234567890", 10, *s_arr[111]);
	test(pop, *s_arr[260], 1, 5, "12345678901234567890", 19, *s_arr[112]);
	test(pop, *s_arr[260], 1, 5, "12345678901234567890", 20, *s_arr[113]);
	test(pop, *s_arr[260], 2, 0, "", 0, *s_arr[260]);
	test(pop, *s_arr[260], 2, 0, "12345", 0, *s_arr[260]);
	test(pop, *s_arr[260], 2, 0, "12345", 1, *s_arr[240]);
	test(pop, *s_arr[260], 2, 0, "12345", 2, *s_arr[237]);
	test(pop, *s_arr[260], 2, 0, "12345", 4, *s_arr[234]);
	test(pop, *s_arr[260], 2, 0, "12345", 5, *s_arr[231]);
	test(pop, *s_arr[260], 2, 0, "1234567890", 0, *s_arr[260]);
	test(pop, *s_arr[260], 2, 0, "1234567890", 1, *s_arr[240]);
	test(pop, *s_arr[260], 2, 0, "1234567890", 5, *s_arr[231]);
	test(pop, *s_arr[260], 2, 0, "1234567890", 9, *s_arr[228]);
	test(pop, *s_arr[260], 2, 0, "1234567890", 10, *s_arr[225]);
	test(pop, *s_arr[260], 2, 0, "12345678901234567890", 0, *s_arr[260]);
	test(pop, *s_arr[260], 2, 0, "12345678901234567890", 1, *s_arr[240]);
	test(pop, *s_arr[260], 2, 0, "12345678901234567890", 10, *s_arr[225]);
	test(pop, *s_arr[260], 2, 0, "12345678901234567890", 19, *s_arr[222]);
	test(pop, *s_arr[260], 2, 0, "12345678901234567890", 20, *s_arr[219]);
	test(pop, *s_arr[260], 2, 1, "", 0, *s_arr[391]);
	test(pop, *s_arr[260], 2, 1, "12345", 0, *s_arr[391]);
	test(pop, *s_arr[260], 2, 1, "12345", 1, *s_arr[241]);
	test(pop, *s_arr[260], 2, 1, "12345", 2, *s_arr[238]);
	test(pop, *s_arr[260], 2, 1, "12345", 4, *s_arr[235]);
	test(pop, *s_arr[260], 2, 1, "12345", 5, *s_arr[232]);
	test(pop, *s_arr[260], 2, 1, "1234567890", 0, *s_arr[391]);
	test(pop, *s_arr[260], 2, 1, "1234567890", 1, *s_arr[241]);
	test(pop, *s_arr[260], 2, 1, "1234567890", 5, *s_arr[232]);
	test(pop, *s_arr[260], 2, 1, "1234567890", 9, *s_arr[229]);
	test(pop, *s_arr[260], 2, 1, "1234567890", 10, *s_arr[226]);
	test(pop, *s_arr[260], 2, 1, "12345678901234567890", 0, *s_arr[391]);
	test(pop, *s_arr[260], 2, 1, "12345678901234567890", 1, *s_arr[241]);
	test(pop, *s_arr[260], 2, 1, "12345678901234567890", 10, *s_arr[226]);
	test(pop, *s_arr[260], 2, 1, "12345678901234567890", 19, *s_arr[223]);
	test(pop, *s_arr[260], 2, 1, "12345678901234567890", 20, *s_arr[220]);
	test(pop, *s_arr[260], 2, 2, "", 0, *s_arr[392]);
	test(pop, *s_arr[260], 2, 2, "12345", 0, *s_arr[392]);
	test(pop, *s_arr[260], 2, 2, "12345", 1, *s_arr[242]);
	test(pop, *s_arr[260], 2, 2, "12345", 2, *s_arr[239]);
	test(pop, *s_arr[260], 2, 2, "12345", 4, *s_arr[236]);
	test(pop, *s_arr[260], 2, 2, "12345", 5, *s_arr[233]);
	test(pop, *s_arr[260], 2, 2, "1234567890", 0, *s_arr[392]);
	test(pop, *s_arr[260], 2, 2, "1234567890", 1, *s_arr[242]);
	test(pop, *s_arr[260], 2, 2, "1234567890", 5, *s_arr[233]);
	test(pop, *s_arr[260], 2, 2, "1234567890", 9, *s_arr[230]);
	test(pop, *s_arr[260], 2, 2, "1234567890", 10, *s_arr[227]);
	test(pop, *s_arr[260], 2, 2, "12345678901234567890", 0, *s_arr[392]);
	test(pop, *s_arr[260], 2, 2, "12345678901234567890", 1, *s_arr[242]);
	test(pop, *s_arr[260], 2, 2, "12345678901234567890", 10, *s_arr[227]);
	test(pop, *s_arr[260], 2, 2, "12345678901234567890", 19, *s_arr[224]);
	test(pop, *s_arr[260], 2, 2, "12345678901234567890", 20, *s_arr[221]);
	test(pop, *s_arr[260], 2, 3, "", 0, *s_arr[210]);
	test(pop, *s_arr[260], 2, 3, "12345", 0, *s_arr[210]);
	test(pop, *s_arr[260], 2, 3, "12345", 1, *s_arr[211]);
	test(pop, *s_arr[260], 2, 3, "12345", 2, *s_arr[212]);
	test(pop, *s_arr[260], 2, 3, "12345", 4, *s_arr[213]);
	test(pop, *s_arr[260], 2, 3, "12345", 5, *s_arr[214]);
	test(pop, *s_arr[260], 2, 3, "1234567890", 0, *s_arr[210]);
	test(pop, *s_arr[260], 2, 3, "1234567890", 1, *s_arr[211]);
	test(pop, *s_arr[260], 2, 3, "1234567890", 5, *s_arr[214]);
	test(pop, *s_arr[260], 2, 3, "1234567890", 9, *s_arr[215]);
	test(pop, *s_arr[260], 2, 3, "1234567890", 10, *s_arr[216]);
	test(pop, *s_arr[260], 2, 3, "12345678901234567890", 0, *s_arr[210]);
}

template <class S>
void
test3(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[260], 2, 3, "12345678901234567890", 1, *s_arr[211]);
	test(pop, *s_arr[260], 2, 3, "12345678901234567890", 10, *s_arr[216]);
	test(pop, *s_arr[260], 2, 3, "12345678901234567890", 19, *s_arr[217]);
	test(pop, *s_arr[260], 2, 3, "12345678901234567890", 20, *s_arr[218]);
	test(pop, *s_arr[260], 2, 4, "", 0, *s_arr[210]);
	test(pop, *s_arr[260], 2, 4, "12345", 0, *s_arr[210]);
	test(pop, *s_arr[260], 2, 4, "12345", 1, *s_arr[211]);
	test(pop, *s_arr[260], 2, 4, "12345", 2, *s_arr[212]);
	test(pop, *s_arr[260], 2, 4, "12345", 4, *s_arr[213]);
	test(pop, *s_arr[260], 2, 4, "12345", 5, *s_arr[214]);
	test(pop, *s_arr[260], 2, 4, "1234567890", 0, *s_arr[210]);
	test(pop, *s_arr[260], 2, 4, "1234567890", 1, *s_arr[211]);
	test(pop, *s_arr[260], 2, 4, "1234567890", 5, *s_arr[214]);
	test(pop, *s_arr[260], 2, 4, "1234567890", 9, *s_arr[215]);
	test(pop, *s_arr[260], 2, 4, "1234567890", 10, *s_arr[216]);
	test(pop, *s_arr[260], 2, 4, "12345678901234567890", 0, *s_arr[210]);
	test(pop, *s_arr[260], 2, 4, "12345678901234567890", 1, *s_arr[211]);
	test(pop, *s_arr[260], 2, 4, "12345678901234567890", 10, *s_arr[216]);
	test(pop, *s_arr[260], 2, 4, "12345678901234567890", 19, *s_arr[217]);
	test(pop, *s_arr[260], 2, 4, "12345678901234567890", 20, *s_arr[218]);
	test(pop, *s_arr[260], 4, 0, "", 0, *s_arr[260]);
	test(pop, *s_arr[260], 4, 0, "12345", 0, *s_arr[260]);
	test(pop, *s_arr[260], 4, 0, "12345", 1, *s_arr[259]);
	test(pop, *s_arr[260], 4, 0, "12345", 2, *s_arr[258]);
	test(pop, *s_arr[260], 4, 0, "12345", 4, *s_arr[257]);
	test(pop, *s_arr[260], 4, 0, "12345", 5, *s_arr[256]);
	test(pop, *s_arr[260], 4, 0, "1234567890", 0, *s_arr[260]);
	test(pop, *s_arr[260], 4, 0, "1234567890", 1, *s_arr[259]);
	test(pop, *s_arr[260], 4, 0, "1234567890", 5, *s_arr[256]);
	test(pop, *s_arr[260], 4, 0, "1234567890", 9, *s_arr[255]);
	test(pop, *s_arr[260], 4, 0, "1234567890", 10, *s_arr[254]);
	test(pop, *s_arr[260], 4, 0, "12345678901234567890", 0, *s_arr[260]);
	test(pop, *s_arr[260], 4, 0, "12345678901234567890", 1, *s_arr[259]);
	test(pop, *s_arr[260], 4, 0, "12345678901234567890", 10, *s_arr[254]);
	test(pop, *s_arr[260], 4, 0, "12345678901234567890", 19, *s_arr[253]);
	test(pop, *s_arr[260], 4, 0, "12345678901234567890", 20, *s_arr[252]);
	test(pop, *s_arr[260], 4, 1, "", 0, *s_arr[243]);
	test(pop, *s_arr[260], 4, 1, "12345", 0, *s_arr[243]);
	test(pop, *s_arr[260], 4, 1, "12345", 1, *s_arr[244]);
	test(pop, *s_arr[260], 4, 1, "12345", 2, *s_arr[245]);
	test(pop, *s_arr[260], 4, 1, "12345", 4, *s_arr[246]);
	test(pop, *s_arr[260], 4, 1, "12345", 5, *s_arr[247]);
	test(pop, *s_arr[260], 4, 1, "1234567890", 0, *s_arr[243]);
	test(pop, *s_arr[260], 4, 1, "1234567890", 1, *s_arr[244]);
	test(pop, *s_arr[260], 4, 1, "1234567890", 5, *s_arr[247]);
	test(pop, *s_arr[260], 4, 1, "1234567890", 9, *s_arr[248]);
	test(pop, *s_arr[260], 4, 1, "1234567890", 10, *s_arr[249]);
	test(pop, *s_arr[260], 4, 1, "12345678901234567890", 0, *s_arr[243]);
	test(pop, *s_arr[260], 4, 1, "12345678901234567890", 1, *s_arr[244]);
	test(pop, *s_arr[260], 4, 1, "12345678901234567890", 10, *s_arr[249]);
	test(pop, *s_arr[260], 4, 1, "12345678901234567890", 19, *s_arr[250]);
	test(pop, *s_arr[260], 4, 1, "12345678901234567890", 20, *s_arr[251]);
	test(pop, *s_arr[260], 4, 2, "", 0, *s_arr[243]);
	test(pop, *s_arr[260], 4, 2, "12345", 0, *s_arr[243]);
	test(pop, *s_arr[260], 4, 2, "12345", 1, *s_arr[244]);
	test(pop, *s_arr[260], 4, 2, "12345", 2, *s_arr[245]);
	test(pop, *s_arr[260], 4, 2, "12345", 4, *s_arr[246]);
	test(pop, *s_arr[260], 4, 2, "12345", 5, *s_arr[247]);
	test(pop, *s_arr[260], 4, 2, "1234567890", 0, *s_arr[243]);
	test(pop, *s_arr[260], 4, 2, "1234567890", 1, *s_arr[244]);
	test(pop, *s_arr[260], 4, 2, "1234567890", 5, *s_arr[247]);
	test(pop, *s_arr[260], 4, 2, "1234567890", 9, *s_arr[248]);
	test(pop, *s_arr[260], 4, 2, "1234567890", 10, *s_arr[249]);
	test(pop, *s_arr[260], 4, 2, "12345678901234567890", 0, *s_arr[243]);
	test(pop, *s_arr[260], 4, 2, "12345678901234567890", 1, *s_arr[244]);
	test(pop, *s_arr[260], 4, 2, "12345678901234567890", 10, *s_arr[249]);
	test(pop, *s_arr[260], 4, 2, "12345678901234567890", 19, *s_arr[250]);
	test(pop, *s_arr[260], 4, 2, "12345678901234567890", 20, *s_arr[251]);
	test(pop, *s_arr[260], 5, 0, "", 0, *s_arr[260]);
	test(pop, *s_arr[260], 5, 0, "12345", 0, *s_arr[260]);
	test(pop, *s_arr[260], 5, 0, "12345", 1, *s_arr[261]);
	test(pop, *s_arr[260], 5, 0, "12345", 2, *s_arr[262]);
	test(pop, *s_arr[260], 5, 0, "12345", 4, *s_arr[263]);
	test(pop, *s_arr[260], 5, 0, "12345", 5, *s_arr[264]);
	test(pop, *s_arr[260], 5, 0, "1234567890", 0, *s_arr[260]);
	test(pop, *s_arr[260], 5, 0, "1234567890", 1, *s_arr[261]);
	test(pop, *s_arr[260], 5, 0, "1234567890", 5, *s_arr[264]);
	test(pop, *s_arr[260], 5, 0, "1234567890", 9, *s_arr[265]);
	test(pop, *s_arr[260], 5, 0, "1234567890", 10, *s_arr[266]);
	test(pop, *s_arr[260], 5, 0, "12345678901234567890", 0, *s_arr[260]);
	test(pop, *s_arr[260], 5, 0, "12345678901234567890", 1, *s_arr[261]);
	test(pop, *s_arr[260], 5, 0, "12345678901234567890", 10, *s_arr[266]);
	test(pop, *s_arr[260], 5, 0, "12345678901234567890", 19, *s_arr[267]);
	test(pop, *s_arr[260], 5, 0, "12345678901234567890", 20, *s_arr[268]);
	test(pop, *s_arr[260], 5, 1, "", 0, *s_arr[260]);
	test(pop, *s_arr[260], 5, 1, "12345", 0, *s_arr[260]);
	test(pop, *s_arr[260], 5, 1, "12345", 1, *s_arr[261]);
	test(pop, *s_arr[260], 5, 1, "12345", 2, *s_arr[262]);
	test(pop, *s_arr[260], 5, 1, "12345", 4, *s_arr[263]);
	test(pop, *s_arr[260], 5, 1, "12345", 5, *s_arr[264]);
	test(pop, *s_arr[260], 5, 1, "1234567890", 0, *s_arr[260]);
	test(pop, *s_arr[260], 5, 1, "1234567890", 1, *s_arr[261]);
	test(pop, *s_arr[260], 5, 1, "1234567890", 5, *s_arr[264]);
	test(pop, *s_arr[260], 5, 1, "1234567890", 9, *s_arr[265]);
	test(pop, *s_arr[260], 5, 1, "1234567890", 10, *s_arr[266]);
	test(pop, *s_arr[260], 5, 1, "12345678901234567890", 0, *s_arr[260]);
	test(pop, *s_arr[260], 5, 1, "12345678901234567890", 1, *s_arr[261]);
	test(pop, *s_arr[260], 5, 1, "12345678901234567890", 10, *s_arr[266]);
	test(pop, *s_arr[260], 5, 1, "12345678901234567890", 19, *s_arr[267]);
	test(pop, *s_arr[260], 5, 1, "12345678901234567890", 20, *s_arr[268]);
}

template <class S>
void
test4(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[260], 6, 0, "", 0, *s_arr[405]);
	test(pop, *s_arr[260], 6, 0, "12345", 0, *s_arr[405]);
	test(pop, *s_arr[260], 6, 0, "12345", 1, *s_arr[405]);
	test(pop, *s_arr[260], 6, 0, "12345", 2, *s_arr[405]);
	test(pop, *s_arr[260], 6, 0, "12345", 4, *s_arr[405]);
	test(pop, *s_arr[260], 6, 0, "12345", 5, *s_arr[405]);
	test(pop, *s_arr[260], 6, 0, "1234567890", 0, *s_arr[405]);
	test(pop, *s_arr[260], 6, 0, "1234567890", 1, *s_arr[405]);
	test(pop, *s_arr[260], 6, 0, "1234567890", 5, *s_arr[405]);
	test(pop, *s_arr[260], 6, 0, "1234567890", 9, *s_arr[405]);
	test(pop, *s_arr[260], 6, 0, "1234567890", 10, *s_arr[405]);
	test(pop, *s_arr[260], 6, 0, "12345678901234567890", 0, *s_arr[405]);
	test(pop, *s_arr[260], 6, 0, "12345678901234567890", 1, *s_arr[405]);
	test(pop, *s_arr[260], 6, 0, "12345678901234567890", 10, *s_arr[405]);
	test(pop, *s_arr[260], 6, 0, "12345678901234567890", 19, *s_arr[405]);
	test(pop, *s_arr[260], 6, 0, "12345678901234567890", 20, *s_arr[405]);
	test(pop, *s_arr[318], 0, 0, "", 0, *s_arr[318]);
	test(pop, *s_arr[318], 0, 0, "12345", 0, *s_arr[318]);
	test(pop, *s_arr[318], 0, 0, "12345", 1, *s_arr[94]);
	test(pop, *s_arr[318], 0, 0, "12345", 2, *s_arr[82]);
	test(pop, *s_arr[318], 0, 0, "12345", 4, *s_arr[70]);
	test(pop, *s_arr[318], 0, 0, "12345", 5, *s_arr[58]);
	test(pop, *s_arr[318], 0, 0, "1234567890", 0, *s_arr[318]);
	test(pop, *s_arr[318], 0, 0, "1234567890", 1, *s_arr[94]);
	test(pop, *s_arr[318], 0, 0, "1234567890", 5, *s_arr[58]);
	test(pop, *s_arr[318], 0, 0, "1234567890", 9, *s_arr[46]);
	test(pop, *s_arr[318], 0, 0, "1234567890", 10, *s_arr[34]);
	test(pop, *s_arr[318], 0, 0, "12345678901234567890", 0, *s_arr[318]);
	test(pop, *s_arr[318], 0, 0, "12345678901234567890", 1, *s_arr[94]);
	test(pop, *s_arr[318], 0, 0, "12345678901234567890", 10, *s_arr[34]);
	test(pop, *s_arr[318], 0, 0, "12345678901234567890", 19, *s_arr[22]);
	test(pop, *s_arr[318], 0, 0, "12345678901234567890", 20, *s_arr[10]);
	test(pop, *s_arr[318], 0, 1, "", 0, *s_arr[403]);
	test(pop, *s_arr[318], 0, 1, "12345", 0, *s_arr[403]);
	test(pop, *s_arr[318], 0, 1, "12345", 1, *s_arr[97]);
	test(pop, *s_arr[318], 0, 1, "12345", 2, *s_arr[85]);
	test(pop, *s_arr[318], 0, 1, "12345", 4, *s_arr[73]);
	test(pop, *s_arr[318], 0, 1, "12345", 5, *s_arr[61]);
	test(pop, *s_arr[318], 0, 1, "1234567890", 0, *s_arr[403]);
	test(pop, *s_arr[318], 0, 1, "1234567890", 1, *s_arr[97]);
	test(pop, *s_arr[318], 0, 1, "1234567890", 5, *s_arr[61]);
	test(pop, *s_arr[318], 0, 1, "1234567890", 9, *s_arr[49]);
	test(pop, *s_arr[318], 0, 1, "1234567890", 10, *s_arr[37]);
	test(pop, *s_arr[318], 0, 1, "12345678901234567890", 0, *s_arr[403]);
	test(pop, *s_arr[318], 0, 1, "12345678901234567890", 1, *s_arr[97]);
	test(pop, *s_arr[318], 0, 1, "12345678901234567890", 10, *s_arr[37]);
	test(pop, *s_arr[318], 0, 1, "12345678901234567890", 19, *s_arr[25]);
	test(pop, *s_arr[318], 0, 1, "12345678901234567890", 20, *s_arr[13]);
	test(pop, *s_arr[318], 0, 5, "", 0, *s_arr[408]);
	test(pop, *s_arr[318], 0, 5, "12345", 0, *s_arr[408]);
	test(pop, *s_arr[318], 0, 5, "12345", 1, *s_arr[101]);
	test(pop, *s_arr[318], 0, 5, "12345", 2, *s_arr[89]);
	test(pop, *s_arr[318], 0, 5, "12345", 4, *s_arr[77]);
	test(pop, *s_arr[318], 0, 5, "12345", 5, *s_arr[65]);
	test(pop, *s_arr[318], 0, 5, "1234567890", 0, *s_arr[408]);
	test(pop, *s_arr[318], 0, 5, "1234567890", 1, *s_arr[101]);
	test(pop, *s_arr[318], 0, 5, "1234567890", 5, *s_arr[65]);
	test(pop, *s_arr[318], 0, 5, "1234567890", 9, *s_arr[53]);
	test(pop, *s_arr[318], 0, 5, "1234567890", 10, *s_arr[41]);
	test(pop, *s_arr[318], 0, 5, "12345678901234567890", 0, *s_arr[408]);
	test(pop, *s_arr[318], 0, 5, "12345678901234567890", 1, *s_arr[101]);
	test(pop, *s_arr[318], 0, 5, "12345678901234567890", 10, *s_arr[41]);
	test(pop, *s_arr[318], 0, 5, "12345678901234567890", 19, *s_arr[29]);
	test(pop, *s_arr[318], 0, 5, "12345678901234567890", 20, *s_arr[17]);
	test(pop, *s_arr[318], 0, 9, "", 0, *s_arr[409]);
	test(pop, *s_arr[318], 0, 9, "12345", 0, *s_arr[409]);
	test(pop, *s_arr[318], 0, 9, "12345", 1, *s_arr[102]);
	test(pop, *s_arr[318], 0, 9, "12345", 2, *s_arr[90]);
	test(pop, *s_arr[318], 0, 9, "12345", 4, *s_arr[78]);
	test(pop, *s_arr[318], 0, 9, "12345", 5, *s_arr[66]);
	test(pop, *s_arr[318], 0, 9, "1234567890", 0, *s_arr[409]);
	test(pop, *s_arr[318], 0, 9, "1234567890", 1, *s_arr[102]);
	test(pop, *s_arr[318], 0, 9, "1234567890", 5, *s_arr[66]);
	test(pop, *s_arr[318], 0, 9, "1234567890", 9, *s_arr[54]);
	test(pop, *s_arr[318], 0, 9, "1234567890", 10, *s_arr[42]);
	test(pop, *s_arr[318], 0, 9, "12345678901234567890", 0, *s_arr[409]);
	test(pop, *s_arr[318], 0, 9, "12345678901234567890", 1, *s_arr[102]);
	test(pop, *s_arr[318], 0, 9, "12345678901234567890", 10, *s_arr[42]);
	test(pop, *s_arr[318], 0, 9, "12345678901234567890", 19, *s_arr[30]);
	test(pop, *s_arr[318], 0, 9, "12345678901234567890", 20, *s_arr[18]);
	test(pop, *s_arr[318], 0, 10, "", 0, *s_arr[0]);
	test(pop, *s_arr[318], 0, 10, "12345", 0, *s_arr[0]);
	test(pop, *s_arr[318], 0, 10, "12345", 1, *s_arr[1]);
	test(pop, *s_arr[318], 0, 10, "12345", 2, *s_arr[2]);
	test(pop, *s_arr[318], 0, 10, "12345", 4, *s_arr[3]);
	test(pop, *s_arr[318], 0, 10, "12345", 5, *s_arr[4]);
	test(pop, *s_arr[318], 0, 10, "1234567890", 0, *s_arr[0]);
	test(pop, *s_arr[318], 0, 10, "1234567890", 1, *s_arr[1]);
	test(pop, *s_arr[318], 0, 10, "1234567890", 5, *s_arr[4]);
	test(pop, *s_arr[318], 0, 10, "1234567890", 9, *s_arr[5]);
	test(pop, *s_arr[318], 0, 10, "1234567890", 10, *s_arr[6]);
	test(pop, *s_arr[318], 0, 10, "12345678901234567890", 0, *s_arr[0]);
	test(pop, *s_arr[318], 0, 10, "12345678901234567890", 1, *s_arr[1]);
	test(pop, *s_arr[318], 0, 10, "12345678901234567890", 10, *s_arr[6]);
	test(pop, *s_arr[318], 0, 10, "12345678901234567890", 19, *s_arr[7]);
	test(pop, *s_arr[318], 0, 10, "12345678901234567890", 20, *s_arr[8]);
	test(pop, *s_arr[318], 0, 11, "", 0, *s_arr[0]);
	test(pop, *s_arr[318], 0, 11, "12345", 0, *s_arr[0]);
	test(pop, *s_arr[318], 0, 11, "12345", 1, *s_arr[1]);
	test(pop, *s_arr[318], 0, 11, "12345", 2, *s_arr[2]);
}

template <class S>
void
test5(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[318], 0, 11, "12345", 4, *s_arr[3]);
	test(pop, *s_arr[318], 0, 11, "12345", 5, *s_arr[4]);
	test(pop, *s_arr[318], 0, 11, "1234567890", 0, *s_arr[0]);
	test(pop, *s_arr[318], 0, 11, "1234567890", 1, *s_arr[1]);
	test(pop, *s_arr[318], 0, 11, "1234567890", 5, *s_arr[4]);
	test(pop, *s_arr[318], 0, 11, "1234567890", 9, *s_arr[5]);
	test(pop, *s_arr[318], 0, 11, "1234567890", 10, *s_arr[6]);
	test(pop, *s_arr[318], 0, 11, "12345678901234567890", 0, *s_arr[0]);
	test(pop, *s_arr[318], 0, 11, "12345678901234567890", 1, *s_arr[1]);
	test(pop, *s_arr[318], 0, 11, "12345678901234567890", 10, *s_arr[6]);
	test(pop, *s_arr[318], 0, 11, "12345678901234567890", 19, *s_arr[7]);
	test(pop, *s_arr[318], 0, 11, "12345678901234567890", 20, *s_arr[8]);
	test(pop, *s_arr[318], 1, 0, "", 0, *s_arr[318]);
	test(pop, *s_arr[318], 1, 0, "12345", 0, *s_arr[318]);
	test(pop, *s_arr[318], 1, 0, "12345", 1, *s_arr[199]);
	test(pop, *s_arr[318], 1, 0, "12345", 2, *s_arr[187]);
	test(pop, *s_arr[318], 1, 0, "12345", 4, *s_arr[175]);
	test(pop, *s_arr[318], 1, 0, "12345", 5, *s_arr[163]);
	test(pop, *s_arr[318], 1, 0, "1234567890", 0, *s_arr[318]);
	test(pop, *s_arr[318], 1, 0, "1234567890", 1, *s_arr[199]);
	test(pop, *s_arr[318], 1, 0, "1234567890", 5, *s_arr[163]);
	test(pop, *s_arr[318], 1, 0, "1234567890", 9, *s_arr[151]);
	test(pop, *s_arr[318], 1, 0, "1234567890", 10, *s_arr[139]);
	test(pop, *s_arr[318], 1, 0, "12345678901234567890", 0, *s_arr[318]);
	test(pop, *s_arr[318], 1, 0, "12345678901234567890", 1, *s_arr[199]);
	test(pop, *s_arr[318], 1, 0, "12345678901234567890", 10, *s_arr[139]);
	test(pop, *s_arr[318], 1, 0, "12345678901234567890", 19, *s_arr[127]);
	test(pop, *s_arr[318], 1, 0, "12345678901234567890", 20, *s_arr[115]);
	test(pop, *s_arr[318], 1, 1, "", 0, *s_arr[394]);
	test(pop, *s_arr[318], 1, 1, "12345", 0, *s_arr[394]);
	test(pop, *s_arr[318], 1, 1, "12345", 1, *s_arr[202]);
	test(pop, *s_arr[318], 1, 1, "12345", 2, *s_arr[190]);
	test(pop, *s_arr[318], 1, 1, "12345", 4, *s_arr[178]);
	test(pop, *s_arr[318], 1, 1, "12345", 5, *s_arr[166]);
	test(pop, *s_arr[318], 1, 1, "1234567890", 0, *s_arr[394]);
	test(pop, *s_arr[318], 1, 1, "1234567890", 1, *s_arr[202]);
	test(pop, *s_arr[318], 1, 1, "1234567890", 5, *s_arr[166]);
	test(pop, *s_arr[318], 1, 1, "1234567890", 9, *s_arr[154]);
	test(pop, *s_arr[318], 1, 1, "1234567890", 10, *s_arr[142]);
	test(pop, *s_arr[318], 1, 1, "12345678901234567890", 0, *s_arr[394]);
	test(pop, *s_arr[318], 1, 1, "12345678901234567890", 1, *s_arr[202]);
	test(pop, *s_arr[318], 1, 1, "12345678901234567890", 10, *s_arr[142]);
	test(pop, *s_arr[318], 1, 1, "12345678901234567890", 19, *s_arr[130]);
	test(pop, *s_arr[318], 1, 1, "12345678901234567890", 20, *s_arr[118]);
	test(pop, *s_arr[318], 1, 4, "", 0, *s_arr[398]);
	test(pop, *s_arr[318], 1, 4, "12345", 0, *s_arr[398]);
	test(pop, *s_arr[318], 1, 4, "12345", 1, *s_arr[206]);
	test(pop, *s_arr[318], 1, 4, "12345", 2, *s_arr[194]);
	test(pop, *s_arr[318], 1, 4, "12345", 4, *s_arr[182]);
	test(pop, *s_arr[318], 1, 4, "12345", 5, *s_arr[170]);
	test(pop, *s_arr[318], 1, 4, "1234567890", 0, *s_arr[398]);
	test(pop, *s_arr[318], 1, 4, "1234567890", 1, *s_arr[206]);
	test(pop, *s_arr[318], 1, 4, "1234567890", 5, *s_arr[170]);
	test(pop, *s_arr[318], 1, 4, "1234567890", 9, *s_arr[158]);
	test(pop, *s_arr[318], 1, 4, "1234567890", 10, *s_arr[146]);
	test(pop, *s_arr[318], 1, 4, "12345678901234567890", 0, *s_arr[398]);
	test(pop, *s_arr[318], 1, 4, "12345678901234567890", 1, *s_arr[206]);
	test(pop, *s_arr[318], 1, 4, "12345678901234567890", 10, *s_arr[146]);
	test(pop, *s_arr[318], 1, 4, "12345678901234567890", 19, *s_arr[134]);
	test(pop, *s_arr[318], 1, 4, "12345678901234567890", 20, *s_arr[122]);
	test(pop, *s_arr[318], 1, 8, "", 0, *s_arr[399]);
	test(pop, *s_arr[318], 1, 8, "12345", 0, *s_arr[399]);
	test(pop, *s_arr[318], 1, 8, "12345", 1, *s_arr[207]);
	test(pop, *s_arr[318], 1, 8, "12345", 2, *s_arr[195]);
	test(pop, *s_arr[318], 1, 8, "12345", 4, *s_arr[183]);
	test(pop, *s_arr[318], 1, 8, "12345", 5, *s_arr[171]);
	test(pop, *s_arr[318], 1, 8, "1234567890", 0, *s_arr[399]);
	test(pop, *s_arr[318], 1, 8, "1234567890", 1, *s_arr[207]);
	test(pop, *s_arr[318], 1, 8, "1234567890", 5, *s_arr[171]);
	test(pop, *s_arr[318], 1, 8, "1234567890", 9, *s_arr[159]);
	test(pop, *s_arr[318], 1, 8, "1234567890", 10, *s_arr[147]);
	test(pop, *s_arr[318], 1, 8, "12345678901234567890", 0, *s_arr[399]);
	test(pop, *s_arr[318], 1, 8, "12345678901234567890", 1, *s_arr[207]);
	test(pop, *s_arr[318], 1, 8, "12345678901234567890", 10, *s_arr[147]);
	test(pop, *s_arr[318], 1, 8, "12345678901234567890", 19, *s_arr[135]);
	test(pop, *s_arr[318], 1, 8, "12345678901234567890", 20, *s_arr[123]);
	test(pop, *s_arr[318], 1, 9, "", 0, *s_arr[105]);
	test(pop, *s_arr[318], 1, 9, "12345", 0, *s_arr[105]);
	test(pop, *s_arr[318], 1, 9, "12345", 1, *s_arr[106]);
	test(pop, *s_arr[318], 1, 9, "12345", 2, *s_arr[107]);
	test(pop, *s_arr[318], 1, 9, "12345", 4, *s_arr[108]);
	test(pop, *s_arr[318], 1, 9, "12345", 5, *s_arr[109]);
	test(pop, *s_arr[318], 1, 9, "1234567890", 0, *s_arr[105]);
	test(pop, *s_arr[318], 1, 9, "1234567890", 1, *s_arr[106]);
	test(pop, *s_arr[318], 1, 9, "1234567890", 5, *s_arr[109]);
	test(pop, *s_arr[318], 1, 9, "1234567890", 9, *s_arr[110]);
	test(pop, *s_arr[318], 1, 9, "1234567890", 10, *s_arr[111]);
	test(pop, *s_arr[318], 1, 9, "12345678901234567890", 0, *s_arr[105]);
	test(pop, *s_arr[318], 1, 9, "12345678901234567890", 1, *s_arr[106]);
	test(pop, *s_arr[318], 1, 9, "12345678901234567890", 10, *s_arr[111]);
	test(pop, *s_arr[318], 1, 9, "12345678901234567890", 19, *s_arr[112]);
	test(pop, *s_arr[318], 1, 9, "12345678901234567890", 20, *s_arr[113]);
	test(pop, *s_arr[318], 1, 10, "", 0, *s_arr[105]);
	test(pop, *s_arr[318], 1, 10, "12345", 0, *s_arr[105]);
	test(pop, *s_arr[318], 1, 10, "12345", 1, *s_arr[106]);
	test(pop, *s_arr[318], 1, 10, "12345", 2, *s_arr[107]);
	test(pop, *s_arr[318], 1, 10, "12345", 4, *s_arr[108]);
	test(pop, *s_arr[318], 1, 10, "12345", 5, *s_arr[109]);
	test(pop, *s_arr[318], 1, 10, "1234567890", 0, *s_arr[105]);
	test(pop, *s_arr[318], 1, 10, "1234567890", 1, *s_arr[106]);
}

template <class S>
void
test6(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[318], 1, 10, "1234567890", 5, *s_arr[109]);
	test(pop, *s_arr[318], 1, 10, "1234567890", 9, *s_arr[110]);
	test(pop, *s_arr[318], 1, 10, "1234567890", 10, *s_arr[111]);
	test(pop, *s_arr[318], 1, 10, "12345678901234567890", 0, *s_arr[105]);
	test(pop, *s_arr[318], 1, 10, "12345678901234567890", 1, *s_arr[106]);
	test(pop, *s_arr[318], 1, 10, "12345678901234567890", 10, *s_arr[111]);
	test(pop, *s_arr[318], 1, 10, "12345678901234567890", 19, *s_arr[112]);
	test(pop, *s_arr[318], 1, 10, "12345678901234567890", 20, *s_arr[113]);
	test(pop, *s_arr[318], 5, 0, "", 0, *s_arr[318]);
	test(pop, *s_arr[318], 5, 0, "12345", 0, *s_arr[318]);
	test(pop, *s_arr[318], 5, 0, "12345", 1, *s_arr[297]);
	test(pop, *s_arr[318], 5, 0, "12345", 2, *s_arr[293]);
	test(pop, *s_arr[318], 5, 0, "12345", 4, *s_arr[289]);
	test(pop, *s_arr[318], 5, 0, "12345", 5, *s_arr[285]);
	test(pop, *s_arr[318], 5, 0, "1234567890", 0, *s_arr[318]);
	test(pop, *s_arr[318], 5, 0, "1234567890", 1, *s_arr[297]);
	test(pop, *s_arr[318], 5, 0, "1234567890", 5, *s_arr[285]);
	test(pop, *s_arr[318], 5, 0, "1234567890", 9, *s_arr[281]);
	test(pop, *s_arr[318], 5, 0, "1234567890", 10, *s_arr[277]);
	test(pop, *s_arr[318], 5, 0, "12345678901234567890", 0, *s_arr[318]);
	test(pop, *s_arr[318], 5, 0, "12345678901234567890", 1, *s_arr[297]);
	test(pop, *s_arr[318], 5, 0, "12345678901234567890", 10, *s_arr[277]);
	test(pop, *s_arr[318], 5, 0, "12345678901234567890", 19, *s_arr[273]);
	test(pop, *s_arr[318], 5, 0, "12345678901234567890", 20, *s_arr[269]);
	test(pop, *s_arr[318], 5, 1, "", 0, *s_arr[388]);
	test(pop, *s_arr[318], 5, 1, "12345", 0, *s_arr[388]);
	test(pop, *s_arr[318], 5, 1, "12345", 1, *s_arr[298]);
	test(pop, *s_arr[318], 5, 1, "12345", 2, *s_arr[294]);
	test(pop, *s_arr[318], 5, 1, "12345", 4, *s_arr[290]);
	test(pop, *s_arr[318], 5, 1, "12345", 5, *s_arr[286]);
	test(pop, *s_arr[318], 5, 1, "1234567890", 0, *s_arr[388]);
	test(pop, *s_arr[318], 5, 1, "1234567890", 1, *s_arr[298]);
	test(pop, *s_arr[318], 5, 1, "1234567890", 5, *s_arr[286]);
	test(pop, *s_arr[318], 5, 1, "1234567890", 9, *s_arr[282]);
	test(pop, *s_arr[318], 5, 1, "1234567890", 10, *s_arr[278]);
	test(pop, *s_arr[318], 5, 1, "12345678901234567890", 0, *s_arr[388]);
	test(pop, *s_arr[318], 5, 1, "12345678901234567890", 1, *s_arr[298]);
	test(pop, *s_arr[318], 5, 1, "12345678901234567890", 10, *s_arr[278]);
	test(pop, *s_arr[318], 5, 1, "12345678901234567890", 19, *s_arr[274]);
	test(pop, *s_arr[318], 5, 1, "12345678901234567890", 20, *s_arr[270]);
	test(pop, *s_arr[318], 5, 2, "", 0, *s_arr[389]);
	test(pop, *s_arr[318], 5, 2, "12345", 0, *s_arr[389]);
	test(pop, *s_arr[318], 5, 2, "12345", 1, *s_arr[299]);
	test(pop, *s_arr[318], 5, 2, "12345", 2, *s_arr[295]);
	test(pop, *s_arr[318], 5, 2, "12345", 4, *s_arr[291]);
	test(pop, *s_arr[318], 5, 2, "12345", 5, *s_arr[287]);
	test(pop, *s_arr[318], 5, 2, "1234567890", 0, *s_arr[389]);
	test(pop, *s_arr[318], 5, 2, "1234567890", 1, *s_arr[299]);
	test(pop, *s_arr[318], 5, 2, "1234567890", 5, *s_arr[287]);
	test(pop, *s_arr[318], 5, 2, "1234567890", 9, *s_arr[283]);
	test(pop, *s_arr[318], 5, 2, "1234567890", 10, *s_arr[279]);
	test(pop, *s_arr[318], 5, 2, "12345678901234567890", 0, *s_arr[389]);
	test(pop, *s_arr[318], 5, 2, "12345678901234567890", 1, *s_arr[299]);
	test(pop, *s_arr[318], 5, 2, "12345678901234567890", 10, *s_arr[279]);
	test(pop, *s_arr[318], 5, 2, "12345678901234567890", 19, *s_arr[275]);
	test(pop, *s_arr[318], 5, 2, "12345678901234567890", 20, *s_arr[271]);
	test(pop, *s_arr[318], 5, 4, "", 0, *s_arr[390]);
	test(pop, *s_arr[318], 5, 4, "12345", 0, *s_arr[390]);
	test(pop, *s_arr[318], 5, 4, "12345", 1, *s_arr[300]);
	test(pop, *s_arr[318], 5, 4, "12345", 2, *s_arr[296]);
	test(pop, *s_arr[318], 5, 4, "12345", 4, *s_arr[292]);
	test(pop, *s_arr[318], 5, 4, "12345", 5, *s_arr[288]);
	test(pop, *s_arr[318], 5, 4, "1234567890", 0, *s_arr[390]);
	test(pop, *s_arr[318], 5, 4, "1234567890", 1, *s_arr[300]);
	test(pop, *s_arr[318], 5, 4, "1234567890", 5, *s_arr[288]);
	test(pop, *s_arr[318], 5, 4, "1234567890", 9, *s_arr[284]);
	test(pop, *s_arr[318], 5, 4, "1234567890", 10, *s_arr[280]);
	test(pop, *s_arr[318], 5, 4, "12345678901234567890", 0, *s_arr[390]);
	test(pop, *s_arr[318], 5, 4, "12345678901234567890", 1, *s_arr[300]);
	test(pop, *s_arr[318], 5, 4, "12345678901234567890", 10, *s_arr[280]);
	test(pop, *s_arr[318], 5, 4, "12345678901234567890", 19, *s_arr[276]);
	test(pop, *s_arr[318], 5, 4, "12345678901234567890", 20, *s_arr[272]);
	test(pop, *s_arr[318], 5, 5, "", 0, *s_arr[260]);
	test(pop, *s_arr[318], 5, 5, "12345", 0, *s_arr[260]);
	test(pop, *s_arr[318], 5, 5, "12345", 1, *s_arr[261]);
	test(pop, *s_arr[318], 5, 5, "12345", 2, *s_arr[262]);
	test(pop, *s_arr[318], 5, 5, "12345", 4, *s_arr[263]);
	test(pop, *s_arr[318], 5, 5, "12345", 5, *s_arr[264]);
	test(pop, *s_arr[318], 5, 5, "1234567890", 0, *s_arr[260]);
	test(pop, *s_arr[318], 5, 5, "1234567890", 1, *s_arr[261]);
	test(pop, *s_arr[318], 5, 5, "1234567890", 5, *s_arr[264]);
	test(pop, *s_arr[318], 5, 5, "1234567890", 9, *s_arr[265]);
	test(pop, *s_arr[318], 5, 5, "1234567890", 10, *s_arr[266]);
	test(pop, *s_arr[318], 5, 5, "12345678901234567890", 0, *s_arr[260]);
	test(pop, *s_arr[318], 5, 5, "12345678901234567890", 1, *s_arr[261]);
	test(pop, *s_arr[318], 5, 5, "12345678901234567890", 10, *s_arr[266]);
	test(pop, *s_arr[318], 5, 5, "12345678901234567890", 19, *s_arr[267]);
	test(pop, *s_arr[318], 5, 5, "12345678901234567890", 20, *s_arr[268]);
	test(pop, *s_arr[318], 5, 6, "", 0, *s_arr[260]);
	test(pop, *s_arr[318], 5, 6, "12345", 0, *s_arr[260]);
	test(pop, *s_arr[318], 5, 6, "12345", 1, *s_arr[261]);
	test(pop, *s_arr[318], 5, 6, "12345", 2, *s_arr[262]);
	test(pop, *s_arr[318], 5, 6, "12345", 4, *s_arr[263]);
	test(pop, *s_arr[318], 5, 6, "12345", 5, *s_arr[264]);
	test(pop, *s_arr[318], 5, 6, "1234567890", 0, *s_arr[260]);
	test(pop, *s_arr[318], 5, 6, "1234567890", 1, *s_arr[261]);
	test(pop, *s_arr[318], 5, 6, "1234567890", 5, *s_arr[264]);
	test(pop, *s_arr[318], 5, 6, "1234567890", 9, *s_arr[265]);
	test(pop, *s_arr[318], 5, 6, "1234567890", 10, *s_arr[266]);
	test(pop, *s_arr[318], 5, 6, "12345678901234567890", 0, *s_arr[260]);
}

template <class S>
void
test7(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[318], 5, 6, "12345678901234567890", 1, *s_arr[261]);
	test(pop, *s_arr[318], 5, 6, "12345678901234567890", 10, *s_arr[266]);
	test(pop, *s_arr[318], 5, 6, "12345678901234567890", 19, *s_arr[267]);
	test(pop, *s_arr[318], 5, 6, "12345678901234567890", 20, *s_arr[268]);
	test(pop, *s_arr[318], 9, 0, "", 0, *s_arr[318]);
	test(pop, *s_arr[318], 9, 0, "12345", 0, *s_arr[318]);
	test(pop, *s_arr[318], 9, 0, "12345", 1, *s_arr[317]);
	test(pop, *s_arr[318], 9, 0, "12345", 2, *s_arr[316]);
	test(pop, *s_arr[318], 9, 0, "12345", 4, *s_arr[315]);
	test(pop, *s_arr[318], 9, 0, "12345", 5, *s_arr[314]);
	test(pop, *s_arr[318], 9, 0, "1234567890", 0, *s_arr[318]);
	test(pop, *s_arr[318], 9, 0, "1234567890", 1, *s_arr[317]);
	test(pop, *s_arr[318], 9, 0, "1234567890", 5, *s_arr[314]);
	test(pop, *s_arr[318], 9, 0, "1234567890", 9, *s_arr[313]);
	test(pop, *s_arr[318], 9, 0, "1234567890", 10, *s_arr[312]);
	test(pop, *s_arr[318], 9, 0, "12345678901234567890", 0, *s_arr[318]);
	test(pop, *s_arr[318], 9, 0, "12345678901234567890", 1, *s_arr[317]);
	test(pop, *s_arr[318], 9, 0, "12345678901234567890", 10, *s_arr[312]);
	test(pop, *s_arr[318], 9, 0, "12345678901234567890", 19, *s_arr[311]);
	test(pop, *s_arr[318], 9, 0, "12345678901234567890", 20, *s_arr[310]);
	test(pop, *s_arr[318], 9, 1, "", 0, *s_arr[301]);
	test(pop, *s_arr[318], 9, 1, "12345", 0, *s_arr[301]);
	test(pop, *s_arr[318], 9, 1, "12345", 1, *s_arr[302]);
	test(pop, *s_arr[318], 9, 1, "12345", 2, *s_arr[303]);
	test(pop, *s_arr[318], 9, 1, "12345", 4, *s_arr[304]);
	test(pop, *s_arr[318], 9, 1, "12345", 5, *s_arr[305]);
	test(pop, *s_arr[318], 9, 1, "1234567890", 0, *s_arr[301]);
	test(pop, *s_arr[318], 9, 1, "1234567890", 1, *s_arr[302]);
	test(pop, *s_arr[318], 9, 1, "1234567890", 5, *s_arr[305]);
	test(pop, *s_arr[318], 9, 1, "1234567890", 9, *s_arr[306]);
	test(pop, *s_arr[318], 9, 1, "1234567890", 10, *s_arr[307]);
	test(pop, *s_arr[318], 9, 1, "12345678901234567890", 0, *s_arr[301]);
	test(pop, *s_arr[318], 9, 1, "12345678901234567890", 1, *s_arr[302]);
	test(pop, *s_arr[318], 9, 1, "12345678901234567890", 10, *s_arr[307]);
	test(pop, *s_arr[318], 9, 1, "12345678901234567890", 19, *s_arr[308]);
	test(pop, *s_arr[318], 9, 1, "12345678901234567890", 20, *s_arr[309]);
	test(pop, *s_arr[318], 9, 2, "", 0, *s_arr[301]);
	test(pop, *s_arr[318], 9, 2, "12345", 0, *s_arr[301]);
	test(pop, *s_arr[318], 9, 2, "12345", 1, *s_arr[302]);
	test(pop, *s_arr[318], 9, 2, "12345", 2, *s_arr[303]);
	test(pop, *s_arr[318], 9, 2, "12345", 4, *s_arr[304]);
	test(pop, *s_arr[318], 9, 2, "12345", 5, *s_arr[305]);
	test(pop, *s_arr[318], 9, 2, "1234567890", 0, *s_arr[301]);
	test(pop, *s_arr[318], 9, 2, "1234567890", 1, *s_arr[302]);
	test(pop, *s_arr[318], 9, 2, "1234567890", 5, *s_arr[305]);
	test(pop, *s_arr[318], 9, 2, "1234567890", 9, *s_arr[306]);
	test(pop, *s_arr[318], 9, 2, "1234567890", 10, *s_arr[307]);
	test(pop, *s_arr[318], 9, 2, "12345678901234567890", 0, *s_arr[301]);
	test(pop, *s_arr[318], 9, 2, "12345678901234567890", 1, *s_arr[302]);
	test(pop, *s_arr[318], 9, 2, "12345678901234567890", 10, *s_arr[307]);
	test(pop, *s_arr[318], 9, 2, "12345678901234567890", 19, *s_arr[308]);
	test(pop, *s_arr[318], 9, 2, "12345678901234567890", 20, *s_arr[309]);
	test(pop, *s_arr[318], 10, 0, "", 0, *s_arr[318]);
	test(pop, *s_arr[318], 10, 0, "12345", 0, *s_arr[318]);
	test(pop, *s_arr[318], 10, 0, "12345", 1, *s_arr[319]);
	test(pop, *s_arr[318], 10, 0, "12345", 2, *s_arr[320]);
	test(pop, *s_arr[318], 10, 0, "12345", 4, *s_arr[321]);
	test(pop, *s_arr[318], 10, 0, "12345", 5, *s_arr[322]);
	test(pop, *s_arr[318], 10, 0, "1234567890", 0, *s_arr[318]);
	test(pop, *s_arr[318], 10, 0, "1234567890", 1, *s_arr[319]);
	test(pop, *s_arr[318], 10, 0, "1234567890", 5, *s_arr[322]);
	test(pop, *s_arr[318], 10, 0, "1234567890", 9, *s_arr[323]);
	test(pop, *s_arr[318], 10, 0, "1234567890", 10, *s_arr[324]);
	test(pop, *s_arr[318], 10, 0, "12345678901234567890", 0, *s_arr[318]);
	test(pop, *s_arr[318], 10, 0, "12345678901234567890", 1, *s_arr[319]);
	test(pop, *s_arr[318], 10, 0, "12345678901234567890", 10, *s_arr[324]);
	test(pop, *s_arr[318], 10, 0, "12345678901234567890", 19, *s_arr[325]);
	test(pop, *s_arr[318], 10, 0, "12345678901234567890", 20, *s_arr[326]);
	test(pop, *s_arr[318], 10, 1, "", 0, *s_arr[318]);
	test(pop, *s_arr[318], 10, 1, "12345", 0, *s_arr[318]);
	test(pop, *s_arr[318], 10, 1, "12345", 1, *s_arr[319]);
	test(pop, *s_arr[318], 10, 1, "12345", 2, *s_arr[320]);
	test(pop, *s_arr[318], 10, 1, "12345", 4, *s_arr[321]);
	test(pop, *s_arr[318], 10, 1, "12345", 5, *s_arr[322]);
	test(pop, *s_arr[318], 10, 1, "1234567890", 0, *s_arr[318]);
	test(pop, *s_arr[318], 10, 1, "1234567890", 1, *s_arr[319]);
	test(pop, *s_arr[318], 10, 1, "1234567890", 5, *s_arr[322]);
	test(pop, *s_arr[318], 10, 1, "1234567890", 9, *s_arr[323]);
	test(pop, *s_arr[318], 10, 1, "1234567890", 10, *s_arr[324]);
	test(pop, *s_arr[318], 10, 1, "12345678901234567890", 0, *s_arr[318]);
	test(pop, *s_arr[318], 10, 1, "12345678901234567890", 1, *s_arr[319]);
	test(pop, *s_arr[318], 10, 1, "12345678901234567890", 10, *s_arr[324]);
	test(pop, *s_arr[318], 10, 1, "12345678901234567890", 19, *s_arr[325]);
	test(pop, *s_arr[318], 10, 1, "12345678901234567890", 20, *s_arr[326]);
	test(pop, *s_arr[318], 11, 0, "", 0, *s_arr[405]);
	test(pop, *s_arr[318], 11, 0, "12345", 0, *s_arr[405]);
	test(pop, *s_arr[318], 11, 0, "12345", 1, *s_arr[405]);
	test(pop, *s_arr[318], 11, 0, "12345", 2, *s_arr[405]);
	test(pop, *s_arr[318], 11, 0, "12345", 4, *s_arr[405]);
	test(pop, *s_arr[318], 11, 0, "12345", 5, *s_arr[405]);
	test(pop, *s_arr[318], 11, 0, "1234567890", 0, *s_arr[405]);
	test(pop, *s_arr[318], 11, 0, "1234567890", 1, *s_arr[405]);
	test(pop, *s_arr[318], 11, 0, "1234567890", 5, *s_arr[405]);
	test(pop, *s_arr[318], 11, 0, "1234567890", 9, *s_arr[405]);
	test(pop, *s_arr[318], 11, 0, "1234567890", 10, *s_arr[405]);
	test(pop, *s_arr[318], 11, 0, "12345678901234567890", 0, *s_arr[405]);
	test(pop, *s_arr[318], 11, 0, "12345678901234567890", 1, *s_arr[405]);
	test(pop, *s_arr[318], 11, 0, "12345678901234567890", 10, *s_arr[405]);
	test(pop, *s_arr[318], 11, 0, "12345678901234567890", 19, *s_arr[405]);
	test(pop, *s_arr[318], 11, 0, "12345678901234567890", 20, *s_arr[405]);
}

template <class S>
void
test8(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[376], 0, 0, "", 0, *s_arr[376]);
	test(pop, *s_arr[376], 0, 0, "12345", 0, *s_arr[376]);
	test(pop, *s_arr[376], 0, 0, "12345", 1, *s_arr[95]);
	test(pop, *s_arr[376], 0, 0, "12345", 2, *s_arr[83]);
	test(pop, *s_arr[376], 0, 0, "12345", 4, *s_arr[71]);
	test(pop, *s_arr[376], 0, 0, "12345", 5, *s_arr[59]);
	test(pop, *s_arr[376], 0, 0, "1234567890", 0, *s_arr[376]);
	test(pop, *s_arr[376], 0, 0, "1234567890", 1, *s_arr[95]);
	test(pop, *s_arr[376], 0, 0, "1234567890", 5, *s_arr[59]);
	test(pop, *s_arr[376], 0, 0, "1234567890", 9, *s_arr[47]);
	test(pop, *s_arr[376], 0, 0, "1234567890", 10, *s_arr[35]);
	test(pop, *s_arr[376], 0, 0, "12345678901234567890", 0, *s_arr[376]);
	test(pop, *s_arr[376], 0, 0, "12345678901234567890", 1, *s_arr[95]);
	test(pop, *s_arr[376], 0, 0, "12345678901234567890", 10, *s_arr[35]);
	test(pop, *s_arr[376], 0, 0, "12345678901234567890", 19, *s_arr[23]);
	test(pop, *s_arr[376], 0, 0, "12345678901234567890", 20, *s_arr[11]);
	test(pop, *s_arr[376], 0, 1, "", 0, *s_arr[404]);
	test(pop, *s_arr[376], 0, 1, "12345", 0, *s_arr[404]);
	test(pop, *s_arr[376], 0, 1, "12345", 1, *s_arr[98]);
	test(pop, *s_arr[376], 0, 1, "12345", 2, *s_arr[86]);
	test(pop, *s_arr[376], 0, 1, "12345", 4, *s_arr[74]);
	test(pop, *s_arr[376], 0, 1, "12345", 5, *s_arr[62]);
	test(pop, *s_arr[376], 0, 1, "1234567890", 0, *s_arr[404]);
	test(pop, *s_arr[376], 0, 1, "1234567890", 1, *s_arr[98]);
	test(pop, *s_arr[376], 0, 1, "1234567890", 5, *s_arr[62]);
	test(pop, *s_arr[376], 0, 1, "1234567890", 9, *s_arr[50]);
	test(pop, *s_arr[376], 0, 1, "1234567890", 10, *s_arr[38]);
	test(pop, *s_arr[376], 0, 1, "12345678901234567890", 0, *s_arr[404]);
	test(pop, *s_arr[376], 0, 1, "12345678901234567890", 1, *s_arr[98]);
	test(pop, *s_arr[376], 0, 1, "12345678901234567890", 10, *s_arr[38]);
	test(pop, *s_arr[376], 0, 1, "12345678901234567890", 19, *s_arr[26]);
	test(pop, *s_arr[376], 0, 1, "12345678901234567890", 20, *s_arr[14]);
	test(pop, *s_arr[376], 0, 10, "", 0, *s_arr[410]);
	test(pop, *s_arr[376], 0, 10, "12345", 0, *s_arr[410]);
	test(pop, *s_arr[376], 0, 10, "12345", 1, *s_arr[103]);
	test(pop, *s_arr[376], 0, 10, "12345", 2, *s_arr[91]);
	test(pop, *s_arr[376], 0, 10, "12345", 4, *s_arr[79]);
	test(pop, *s_arr[376], 0, 10, "12345", 5, *s_arr[67]);
	test(pop, *s_arr[376], 0, 10, "1234567890", 0, *s_arr[410]);
	test(pop, *s_arr[376], 0, 10, "1234567890", 1, *s_arr[103]);
	test(pop, *s_arr[376], 0, 10, "1234567890", 5, *s_arr[67]);
	test(pop, *s_arr[376], 0, 10, "1234567890", 9, *s_arr[55]);
	test(pop, *s_arr[376], 0, 10, "1234567890", 10, *s_arr[43]);
	test(pop, *s_arr[376], 0, 10, "12345678901234567890", 0, *s_arr[410]);
	test(pop, *s_arr[376], 0, 10, "12345678901234567890", 1, *s_arr[103]);
	test(pop, *s_arr[376], 0, 10, "12345678901234567890", 10, *s_arr[43]);
	test(pop, *s_arr[376], 0, 10, "12345678901234567890", 19, *s_arr[31]);
	test(pop, *s_arr[376], 0, 10, "12345678901234567890", 20, *s_arr[19]);
	test(pop, *s_arr[376], 0, 19, "", 0, *s_arr[411]);
	test(pop, *s_arr[376], 0, 19, "12345", 0, *s_arr[411]);
	test(pop, *s_arr[376], 0, 19, "12345", 1, *s_arr[104]);
	test(pop, *s_arr[376], 0, 19, "12345", 2, *s_arr[92]);
	test(pop, *s_arr[376], 0, 19, "12345", 4, *s_arr[80]);
	test(pop, *s_arr[376], 0, 19, "12345", 5, *s_arr[68]);
	test(pop, *s_arr[376], 0, 19, "1234567890", 0, *s_arr[411]);
	test(pop, *s_arr[376], 0, 19, "1234567890", 1, *s_arr[104]);
	test(pop, *s_arr[376], 0, 19, "1234567890", 5, *s_arr[68]);
	test(pop, *s_arr[376], 0, 19, "1234567890", 9, *s_arr[56]);
	test(pop, *s_arr[376], 0, 19, "1234567890", 10, *s_arr[44]);
	test(pop, *s_arr[376], 0, 19, "12345678901234567890", 0, *s_arr[411]);
	test(pop, *s_arr[376], 0, 19, "12345678901234567890", 1, *s_arr[104]);
	test(pop, *s_arr[376], 0, 19, "12345678901234567890", 10, *s_arr[44]);
	test(pop, *s_arr[376], 0, 19, "12345678901234567890", 19, *s_arr[32]);
	test(pop, *s_arr[376], 0, 19, "12345678901234567890", 20, *s_arr[20]);
	test(pop, *s_arr[376], 0, 20, "", 0, *s_arr[0]);
	test(pop, *s_arr[376], 0, 20, "12345", 0, *s_arr[0]);
	test(pop, *s_arr[376], 0, 20, "12345", 1, *s_arr[1]);
	test(pop, *s_arr[376], 0, 20, "12345", 2, *s_arr[2]);
	test(pop, *s_arr[376], 0, 20, "12345", 4, *s_arr[3]);
	test(pop, *s_arr[376], 0, 20, "12345", 5, *s_arr[4]);
	test(pop, *s_arr[376], 0, 20, "1234567890", 0, *s_arr[0]);
	test(pop, *s_arr[376], 0, 20, "1234567890", 1, *s_arr[1]);
	test(pop, *s_arr[376], 0, 20, "1234567890", 5, *s_arr[4]);
	test(pop, *s_arr[376], 0, 20, "1234567890", 9, *s_arr[5]);
	test(pop, *s_arr[376], 0, 20, "1234567890", 10, *s_arr[6]);
	test(pop, *s_arr[376], 0, 20, "12345678901234567890", 0, *s_arr[0]);
	test(pop, *s_arr[376], 0, 20, "12345678901234567890", 1, *s_arr[1]);
	test(pop, *s_arr[376], 0, 20, "12345678901234567890", 10, *s_arr[6]);
	test(pop, *s_arr[376], 0, 20, "12345678901234567890", 19, *s_arr[7]);
	test(pop, *s_arr[376], 0, 20, "12345678901234567890", 20, *s_arr[8]);
	test(pop, *s_arr[376], 0, 21, "", 0, *s_arr[0]);
	test(pop, *s_arr[376], 0, 21, "12345", 0, *s_arr[0]);
	test(pop, *s_arr[376], 0, 21, "12345", 1, *s_arr[1]);
	test(pop, *s_arr[376], 0, 21, "12345", 2, *s_arr[2]);
	test(pop, *s_arr[376], 0, 21, "12345", 4, *s_arr[3]);
	test(pop, *s_arr[376], 0, 21, "12345", 5, *s_arr[4]);
	test(pop, *s_arr[376], 0, 21, "1234567890", 0, *s_arr[0]);
	test(pop, *s_arr[376], 0, 21, "1234567890", 1, *s_arr[1]);
	test(pop, *s_arr[376], 0, 21, "1234567890", 5, *s_arr[4]);
	test(pop, *s_arr[376], 0, 21, "1234567890", 9, *s_arr[5]);
	test(pop, *s_arr[376], 0, 21, "1234567890", 10, *s_arr[6]);
	test(pop, *s_arr[376], 0, 21, "12345678901234567890", 0, *s_arr[0]);
	test(pop, *s_arr[376], 0, 21, "12345678901234567890", 1, *s_arr[1]);
	test(pop, *s_arr[376], 0, 21, "12345678901234567890", 10, *s_arr[6]);
	test(pop, *s_arr[376], 0, 21, "12345678901234567890", 19, *s_arr[7]);
	test(pop, *s_arr[376], 0, 21, "12345678901234567890", 20, *s_arr[8]);
	test(pop, *s_arr[376], 1, 0, "", 0, *s_arr[376]);
	test(pop, *s_arr[376], 1, 0, "12345", 0, *s_arr[376]);
	test(pop, *s_arr[376], 1, 0, "12345", 1, *s_arr[200]);
	test(pop, *s_arr[376], 1, 0, "12345", 2, *s_arr[188]);
}

template <class S>
void
test9(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[376], 1, 0, "12345", 4, *s_arr[176]);
	test(pop, *s_arr[376], 1, 0, "12345", 5, *s_arr[164]);
	test(pop, *s_arr[376], 1, 0, "1234567890", 0, *s_arr[376]);
	test(pop, *s_arr[376], 1, 0, "1234567890", 1, *s_arr[200]);
	test(pop, *s_arr[376], 1, 0, "1234567890", 5, *s_arr[164]);
	test(pop, *s_arr[376], 1, 0, "1234567890", 9, *s_arr[152]);
	test(pop, *s_arr[376], 1, 0, "1234567890", 10, *s_arr[140]);
	test(pop, *s_arr[376], 1, 0, "12345678901234567890", 0, *s_arr[376]);
	test(pop, *s_arr[376], 1, 0, "12345678901234567890", 1, *s_arr[200]);
	test(pop, *s_arr[376], 1, 0, "12345678901234567890", 10, *s_arr[140]);
	test(pop, *s_arr[376], 1, 0, "12345678901234567890", 19, *s_arr[128]);
	test(pop, *s_arr[376], 1, 0, "12345678901234567890", 20, *s_arr[116]);
	test(pop, *s_arr[376], 1, 1, "", 0, *s_arr[395]);
	test(pop, *s_arr[376], 1, 1, "12345", 0, *s_arr[395]);
	test(pop, *s_arr[376], 1, 1, "12345", 1, *s_arr[203]);
	test(pop, *s_arr[376], 1, 1, "12345", 2, *s_arr[191]);
	test(pop, *s_arr[376], 1, 1, "12345", 4, *s_arr[179]);
	test(pop, *s_arr[376], 1, 1, "12345", 5, *s_arr[167]);
	test(pop, *s_arr[376], 1, 1, "1234567890", 0, *s_arr[395]);
	test(pop, *s_arr[376], 1, 1, "1234567890", 1, *s_arr[203]);
	test(pop, *s_arr[376], 1, 1, "1234567890", 5, *s_arr[167]);
	test(pop, *s_arr[376], 1, 1, "1234567890", 9, *s_arr[155]);
	test(pop, *s_arr[376], 1, 1, "1234567890", 10, *s_arr[143]);
	test(pop, *s_arr[376], 1, 1, "12345678901234567890", 0, *s_arr[395]);
	test(pop, *s_arr[376], 1, 1, "12345678901234567890", 1, *s_arr[203]);
	test(pop, *s_arr[376], 1, 1, "12345678901234567890", 10, *s_arr[143]);
	test(pop, *s_arr[376], 1, 1, "12345678901234567890", 19, *s_arr[131]);
	test(pop, *s_arr[376], 1, 1, "12345678901234567890", 20, *s_arr[119]);
	test(pop, *s_arr[376], 1, 9, "", 0, *s_arr[400]);
	test(pop, *s_arr[376], 1, 9, "12345", 0, *s_arr[400]);
	test(pop, *s_arr[376], 1, 9, "12345", 1, *s_arr[208]);
	test(pop, *s_arr[376], 1, 9, "12345", 2, *s_arr[196]);
	test(pop, *s_arr[376], 1, 9, "12345", 4, *s_arr[184]);
	test(pop, *s_arr[376], 1, 9, "12345", 5, *s_arr[172]);
	test(pop, *s_arr[376], 1, 9, "1234567890", 0, *s_arr[400]);
	test(pop, *s_arr[376], 1, 9, "1234567890", 1, *s_arr[208]);
	test(pop, *s_arr[376], 1, 9, "1234567890", 5, *s_arr[172]);
	test(pop, *s_arr[376], 1, 9, "1234567890", 9, *s_arr[160]);
	test(pop, *s_arr[376], 1, 9, "1234567890", 10, *s_arr[148]);
	test(pop, *s_arr[376], 1, 9, "12345678901234567890", 0, *s_arr[400]);
	test(pop, *s_arr[376], 1, 9, "12345678901234567890", 1, *s_arr[208]);
	test(pop, *s_arr[376], 1, 9, "12345678901234567890", 10, *s_arr[148]);
	test(pop, *s_arr[376], 1, 9, "12345678901234567890", 19, *s_arr[136]);
	test(pop, *s_arr[376], 1, 9, "12345678901234567890", 20, *s_arr[124]);
	test(pop, *s_arr[376], 1, 18, "", 0, *s_arr[401]);
	test(pop, *s_arr[376], 1, 18, "12345", 0, *s_arr[401]);
	test(pop, *s_arr[376], 1, 18, "12345", 1, *s_arr[209]);
	test(pop, *s_arr[376], 1, 18, "12345", 2, *s_arr[197]);
	test(pop, *s_arr[376], 1, 18, "12345", 4, *s_arr[185]);
	test(pop, *s_arr[376], 1, 18, "12345", 5, *s_arr[173]);
	test(pop, *s_arr[376], 1, 18, "1234567890", 0, *s_arr[401]);
	test(pop, *s_arr[376], 1, 18, "1234567890", 1, *s_arr[209]);
	test(pop, *s_arr[376], 1, 18, "1234567890", 5, *s_arr[173]);
	test(pop, *s_arr[376], 1, 18, "1234567890", 9, *s_arr[161]);
	test(pop, *s_arr[376], 1, 18, "1234567890", 10, *s_arr[149]);
	test(pop, *s_arr[376], 1, 18, "12345678901234567890", 0, *s_arr[401]);
	test(pop, *s_arr[376], 1, 18, "12345678901234567890", 1, *s_arr[209]);
	test(pop, *s_arr[376], 1, 18, "12345678901234567890", 10, *s_arr[149]);
	test(pop, *s_arr[376], 1, 18, "12345678901234567890", 19, *s_arr[137]);
	test(pop, *s_arr[376], 1, 18, "12345678901234567890", 20, *s_arr[125]);
	test(pop, *s_arr[376], 1, 19, "", 0, *s_arr[105]);
	test(pop, *s_arr[376], 1, 19, "12345", 0, *s_arr[105]);
	test(pop, *s_arr[376], 1, 19, "12345", 1, *s_arr[106]);
	test(pop, *s_arr[376], 1, 19, "12345", 2, *s_arr[107]);
	test(pop, *s_arr[376], 1, 19, "12345", 4, *s_arr[108]);
	test(pop, *s_arr[376], 1, 19, "12345", 5, *s_arr[109]);
	test(pop, *s_arr[376], 1, 19, "1234567890", 0, *s_arr[105]);
	test(pop, *s_arr[376], 1, 19, "1234567890", 1, *s_arr[106]);
	test(pop, *s_arr[376], 1, 19, "1234567890", 5, *s_arr[109]);
	test(pop, *s_arr[376], 1, 19, "1234567890", 9, *s_arr[110]);
	test(pop, *s_arr[376], 1, 19, "1234567890", 10, *s_arr[111]);
	test(pop, *s_arr[376], 1, 19, "12345678901234567890", 0, *s_arr[105]);
	test(pop, *s_arr[376], 1, 19, "12345678901234567890", 1, *s_arr[106]);
	test(pop, *s_arr[376], 1, 19, "12345678901234567890", 10, *s_arr[111]);
	test(pop, *s_arr[376], 1, 19, "12345678901234567890", 19, *s_arr[112]);
	test(pop, *s_arr[376], 1, 19, "12345678901234567890", 20, *s_arr[113]);
	test(pop, *s_arr[376], 1, 20, "", 0, *s_arr[105]);
	test(pop, *s_arr[376], 1, 20, "12345", 0, *s_arr[105]);
	test(pop, *s_arr[376], 1, 20, "12345", 1, *s_arr[106]);
	test(pop, *s_arr[376], 1, 20, "12345", 2, *s_arr[107]);
	test(pop, *s_arr[376], 1, 20, "12345", 4, *s_arr[108]);
	test(pop, *s_arr[376], 1, 20, "12345", 5, *s_arr[109]);
	test(pop, *s_arr[376], 1, 20, "1234567890", 0, *s_arr[105]);
	test(pop, *s_arr[376], 1, 20, "1234567890", 1, *s_arr[106]);
	test(pop, *s_arr[376], 1, 20, "1234567890", 5, *s_arr[109]);
	test(pop, *s_arr[376], 1, 20, "1234567890", 9, *s_arr[110]);
	test(pop, *s_arr[376], 1, 20, "1234567890", 10, *s_arr[111]);
	test(pop, *s_arr[376], 1, 20, "12345678901234567890", 0, *s_arr[105]);
	test(pop, *s_arr[376], 1, 20, "12345678901234567890", 1, *s_arr[106]);
	test(pop, *s_arr[376], 1, 20, "12345678901234567890", 10, *s_arr[111]);
	test(pop, *s_arr[376], 1, 20, "12345678901234567890", 19, *s_arr[112]);
	test(pop, *s_arr[376], 1, 20, "12345678901234567890", 20, *s_arr[113]);
	test(pop, *s_arr[376], 10, 0, "", 0, *s_arr[376]);
	test(pop, *s_arr[376], 10, 0, "12345", 0, *s_arr[376]);
	test(pop, *s_arr[376], 10, 0, "12345", 1, *s_arr[355]);
	test(pop, *s_arr[376], 10, 0, "12345", 2, *s_arr[351]);
	test(pop, *s_arr[376], 10, 0, "12345", 4, *s_arr[347]);
	test(pop, *s_arr[376], 10, 0, "12345", 5, *s_arr[343]);
	test(pop, *s_arr[376], 10, 0, "1234567890", 0, *s_arr[376]);
	test(pop, *s_arr[376], 10, 0, "1234567890", 1, *s_arr[355]);
}

template <class S>
void
test10(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[376], 10, 0, "1234567890", 5, *s_arr[343]);
	test(pop, *s_arr[376], 10, 0, "1234567890", 9, *s_arr[339]);
	test(pop, *s_arr[376], 10, 0, "1234567890", 10, *s_arr[335]);
	test(pop, *s_arr[376], 10, 0, "12345678901234567890", 0, *s_arr[376]);
	test(pop, *s_arr[376], 10, 0, "12345678901234567890", 1, *s_arr[355]);
	test(pop, *s_arr[376], 10, 0, "12345678901234567890", 10, *s_arr[335]);
	test(pop, *s_arr[376], 10, 0, "12345678901234567890", 19, *s_arr[331]);
	test(pop, *s_arr[376], 10, 0, "12345678901234567890", 20, *s_arr[327]);
	test(pop, *s_arr[376], 10, 1, "", 0, *s_arr[385]);
	test(pop, *s_arr[376], 10, 1, "12345", 0, *s_arr[385]);
	test(pop, *s_arr[376], 10, 1, "12345", 1, *s_arr[356]);
	test(pop, *s_arr[376], 10, 1, "12345", 2, *s_arr[352]);
	test(pop, *s_arr[376], 10, 1, "12345", 4, *s_arr[348]);
	test(pop, *s_arr[376], 10, 1, "12345", 5, *s_arr[344]);
	test(pop, *s_arr[376], 10, 1, "1234567890", 0, *s_arr[385]);
	test(pop, *s_arr[376], 10, 1, "1234567890", 1, *s_arr[356]);
	test(pop, *s_arr[376], 10, 1, "1234567890", 5, *s_arr[344]);
	test(pop, *s_arr[376], 10, 1, "1234567890", 9, *s_arr[340]);
	test(pop, *s_arr[376], 10, 1, "1234567890", 10, *s_arr[336]);
	test(pop, *s_arr[376], 10, 1, "12345678901234567890", 0, *s_arr[385]);
	test(pop, *s_arr[376], 10, 1, "12345678901234567890", 1, *s_arr[356]);
	test(pop, *s_arr[376], 10, 1, "12345678901234567890", 10, *s_arr[336]);
	test(pop, *s_arr[376], 10, 1, "12345678901234567890", 19, *s_arr[332]);
	test(pop, *s_arr[376], 10, 1, "12345678901234567890", 20, *s_arr[328]);
	test(pop, *s_arr[376], 10, 5, "", 0, *s_arr[386]);
	test(pop, *s_arr[376], 10, 5, "12345", 0, *s_arr[386]);
	test(pop, *s_arr[376], 10, 5, "12345", 1, *s_arr[357]);
	test(pop, *s_arr[376], 10, 5, "12345", 2, *s_arr[353]);
	test(pop, *s_arr[376], 10, 5, "12345", 4, *s_arr[349]);
	test(pop, *s_arr[376], 10, 5, "12345", 5, *s_arr[345]);
	test(pop, *s_arr[376], 10, 5, "1234567890", 0, *s_arr[386]);
	test(pop, *s_arr[376], 10, 5, "1234567890", 1, *s_arr[357]);
	test(pop, *s_arr[376], 10, 5, "1234567890", 5, *s_arr[345]);
	test(pop, *s_arr[376], 10, 5, "1234567890", 9, *s_arr[341]);
	test(pop, *s_arr[376], 10, 5, "1234567890", 10, *s_arr[337]);
	test(pop, *s_arr[376], 10, 5, "12345678901234567890", 0, *s_arr[386]);
	test(pop, *s_arr[376], 10, 5, "12345678901234567890", 1, *s_arr[357]);
	test(pop, *s_arr[376], 10, 5, "12345678901234567890", 10, *s_arr[337]);
	test(pop, *s_arr[376], 10, 5, "12345678901234567890", 19, *s_arr[333]);
	test(pop, *s_arr[376], 10, 5, "12345678901234567890", 20, *s_arr[329]);
	test(pop, *s_arr[376], 10, 9, "", 0, *s_arr[387]);
	test(pop, *s_arr[376], 10, 9, "12345", 0, *s_arr[387]);
	test(pop, *s_arr[376], 10, 9, "12345", 1, *s_arr[358]);
	test(pop, *s_arr[376], 10, 9, "12345", 2, *s_arr[354]);
	test(pop, *s_arr[376], 10, 9, "12345", 4, *s_arr[350]);
	test(pop, *s_arr[376], 10, 9, "12345", 5, *s_arr[346]);
	test(pop, *s_arr[376], 10, 9, "1234567890", 0, *s_arr[387]);
	test(pop, *s_arr[376], 10, 9, "1234567890", 1, *s_arr[358]);
	test(pop, *s_arr[376], 10, 9, "1234567890", 5, *s_arr[346]);
	test(pop, *s_arr[376], 10, 9, "1234567890", 9, *s_arr[342]);
	test(pop, *s_arr[376], 10, 9, "1234567890", 10, *s_arr[338]);
	test(pop, *s_arr[376], 10, 9, "12345678901234567890", 0, *s_arr[387]);
	test(pop, *s_arr[376], 10, 9, "12345678901234567890", 1, *s_arr[358]);
	test(pop, *s_arr[376], 10, 9, "12345678901234567890", 10, *s_arr[338]);
	test(pop, *s_arr[376], 10, 9, "12345678901234567890", 19, *s_arr[334]);
	test(pop, *s_arr[376], 10, 9, "12345678901234567890", 20, *s_arr[330]);
	test(pop, *s_arr[376], 10, 10, "", 0, *s_arr[318]);
	test(pop, *s_arr[376], 10, 10, "12345", 0, *s_arr[318]);
	test(pop, *s_arr[376], 10, 10, "12345", 1, *s_arr[319]);
	test(pop, *s_arr[376], 10, 10, "12345", 2, *s_arr[320]);
	test(pop, *s_arr[376], 10, 10, "12345", 4, *s_arr[321]);
	test(pop, *s_arr[376], 10, 10, "12345", 5, *s_arr[322]);
	test(pop, *s_arr[376], 10, 10, "1234567890", 0, *s_arr[318]);
	test(pop, *s_arr[376], 10, 10, "1234567890", 1, *s_arr[319]);
	test(pop, *s_arr[376], 10, 10, "1234567890", 5, *s_arr[322]);
	test(pop, *s_arr[376], 10, 10, "1234567890", 9, *s_arr[323]);
	test(pop, *s_arr[376], 10, 10, "1234567890", 10, *s_arr[324]);
	test(pop, *s_arr[376], 10, 10, "12345678901234567890", 0, *s_arr[318]);
	test(pop, *s_arr[376], 10, 10, "12345678901234567890", 1, *s_arr[319]);
	test(pop, *s_arr[376], 10, 10, "12345678901234567890", 10, *s_arr[324]);
	test(pop, *s_arr[376], 10, 10, "12345678901234567890", 19, *s_arr[325]);
	test(pop, *s_arr[376], 10, 10, "12345678901234567890", 20, *s_arr[326]);
	test(pop, *s_arr[376], 10, 11, "", 0, *s_arr[318]);
	test(pop, *s_arr[376], 10, 11, "12345", 0, *s_arr[318]);
	test(pop, *s_arr[376], 10, 11, "12345", 1, *s_arr[319]);
	test(pop, *s_arr[376], 10, 11, "12345", 2, *s_arr[320]);
	test(pop, *s_arr[376], 10, 11, "12345", 4, *s_arr[321]);
	test(pop, *s_arr[376], 10, 11, "12345", 5, *s_arr[322]);
	test(pop, *s_arr[376], 10, 11, "1234567890", 0, *s_arr[318]);
	test(pop, *s_arr[376], 10, 11, "1234567890", 1, *s_arr[319]);
	test(pop, *s_arr[376], 10, 11, "1234567890", 5, *s_arr[322]);
	test(pop, *s_arr[376], 10, 11, "1234567890", 9, *s_arr[323]);
	test(pop, *s_arr[376], 10, 11, "1234567890", 10, *s_arr[324]);
	test(pop, *s_arr[376], 10, 11, "12345678901234567890", 0, *s_arr[318]);
	test(pop, *s_arr[376], 10, 11, "12345678901234567890", 1, *s_arr[319]);
	test(pop, *s_arr[376], 10, 11, "12345678901234567890", 10, *s_arr[324]);
	test(pop, *s_arr[376], 10, 11, "12345678901234567890", 19, *s_arr[325]);
	test(pop, *s_arr[376], 10, 11, "12345678901234567890", 20, *s_arr[326]);
	test(pop, *s_arr[376], 19, 0, "", 0, *s_arr[376]);
	test(pop, *s_arr[376], 19, 0, "12345", 0, *s_arr[376]);
	test(pop, *s_arr[376], 19, 0, "12345", 1, *s_arr[375]);
	test(pop, *s_arr[376], 19, 0, "12345", 2, *s_arr[374]);
	test(pop, *s_arr[376], 19, 0, "12345", 4, *s_arr[373]);
	test(pop, *s_arr[376], 19, 0, "12345", 5, *s_arr[372]);
	test(pop, *s_arr[376], 19, 0, "1234567890", 0, *s_arr[376]);
	test(pop, *s_arr[376], 19, 0, "1234567890", 1, *s_arr[375]);
	test(pop, *s_arr[376], 19, 0, "1234567890", 5, *s_arr[372]);
	test(pop, *s_arr[376], 19, 0, "1234567890", 9, *s_arr[371]);
	test(pop, *s_arr[376], 19, 0, "1234567890", 10, *s_arr[370]);
	test(pop, *s_arr[376], 19, 0, "12345678901234567890", 0, *s_arr[376]);
}

template <class S>
void
test11(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[376], 19, 0, "12345678901234567890", 1, *s_arr[375]);
	test(pop, *s_arr[376], 19, 0, "12345678901234567890", 10, *s_arr[370]);
	test(pop, *s_arr[376], 19, 0, "12345678901234567890", 19, *s_arr[369]);
	test(pop, *s_arr[376], 19, 0, "12345678901234567890", 20, *s_arr[368]);
	test(pop, *s_arr[376], 19, 1, "", 0, *s_arr[359]);
	test(pop, *s_arr[376], 19, 1, "12345", 0, *s_arr[359]);
	test(pop, *s_arr[376], 19, 1, "12345", 1, *s_arr[360]);
	test(pop, *s_arr[376], 19, 1, "12345", 2, *s_arr[361]);
	test(pop, *s_arr[376], 19, 1, "12345", 4, *s_arr[362]);
	test(pop, *s_arr[376], 19, 1, "12345", 5, *s_arr[363]);
	test(pop, *s_arr[376], 19, 1, "1234567890", 0, *s_arr[359]);
	test(pop, *s_arr[376], 19, 1, "1234567890", 1, *s_arr[360]);
	test(pop, *s_arr[376], 19, 1, "1234567890", 5, *s_arr[363]);
	test(pop, *s_arr[376], 19, 1, "1234567890", 9, *s_arr[364]);
	test(pop, *s_arr[376], 19, 1, "1234567890", 10, *s_arr[365]);
	test(pop, *s_arr[376], 19, 1, "12345678901234567890", 0, *s_arr[359]);
	test(pop, *s_arr[376], 19, 1, "12345678901234567890", 1, *s_arr[360]);
	test(pop, *s_arr[376], 19, 1, "12345678901234567890", 10, *s_arr[365]);
	test(pop, *s_arr[376], 19, 1, "12345678901234567890", 19, *s_arr[366]);
	test(pop, *s_arr[376], 19, 1, "12345678901234567890", 20, *s_arr[367]);
	test(pop, *s_arr[376], 19, 2, "", 0, *s_arr[359]);
	test(pop, *s_arr[376], 19, 2, "12345", 0, *s_arr[359]);
	test(pop, *s_arr[376], 19, 2, "12345", 1, *s_arr[360]);
	test(pop, *s_arr[376], 19, 2, "12345", 2, *s_arr[361]);
	test(pop, *s_arr[376], 19, 2, "12345", 4, *s_arr[362]);
	test(pop, *s_arr[376], 19, 2, "12345", 5, *s_arr[363]);
	test(pop, *s_arr[376], 19, 2, "1234567890", 0, *s_arr[359]);
	test(pop, *s_arr[376], 19, 2, "1234567890", 1, *s_arr[360]);
	test(pop, *s_arr[376], 19, 2, "1234567890", 5, *s_arr[363]);
	test(pop, *s_arr[376], 19, 2, "1234567890", 9, *s_arr[364]);
	test(pop, *s_arr[376], 19, 2, "1234567890", 10, *s_arr[365]);
	test(pop, *s_arr[376], 19, 2, "12345678901234567890", 0, *s_arr[359]);
	test(pop, *s_arr[376], 19, 2, "12345678901234567890", 1, *s_arr[360]);
	test(pop, *s_arr[376], 19, 2, "12345678901234567890", 10, *s_arr[365]);
	test(pop, *s_arr[376], 19, 2, "12345678901234567890", 19, *s_arr[366]);
	test(pop, *s_arr[376], 19, 2, "12345678901234567890", 20, *s_arr[367]);
	test(pop, *s_arr[376], 20, 0, "", 0, *s_arr[376]);
	test(pop, *s_arr[376], 20, 0, "12345", 0, *s_arr[376]);
	test(pop, *s_arr[376], 20, 0, "12345", 1, *s_arr[377]);
	test(pop, *s_arr[376], 20, 0, "12345", 2, *s_arr[378]);
	test(pop, *s_arr[376], 20, 0, "12345", 4, *s_arr[379]);
	test(pop, *s_arr[376], 20, 0, "12345", 5, *s_arr[380]);
	test(pop, *s_arr[376], 20, 0, "1234567890", 0, *s_arr[376]);
	test(pop, *s_arr[376], 20, 0, "1234567890", 1, *s_arr[377]);
	test(pop, *s_arr[376], 20, 0, "1234567890", 5, *s_arr[380]);
	test(pop, *s_arr[376], 20, 0, "1234567890", 9, *s_arr[381]);
	test(pop, *s_arr[376], 20, 0, "1234567890", 10, *s_arr[382]);
	test(pop, *s_arr[376], 20, 0, "12345678901234567890", 0, *s_arr[376]);
	test(pop, *s_arr[376], 20, 0, "12345678901234567890", 1, *s_arr[377]);
	test(pop, *s_arr[376], 20, 0, "12345678901234567890", 10, *s_arr[382]);
	test(pop, *s_arr[376], 20, 0, "12345678901234567890", 19, *s_arr[383]);
	test(pop, *s_arr[376], 20, 0, "12345678901234567890", 20, *s_arr[384]);
	test(pop, *s_arr[376], 20, 1, "", 0, *s_arr[376]);
	test(pop, *s_arr[376], 20, 1, "12345", 0, *s_arr[376]);
	test(pop, *s_arr[376], 20, 1, "12345", 1, *s_arr[377]);
	test(pop, *s_arr[376], 20, 1, "12345", 2, *s_arr[378]);
	test(pop, *s_arr[376], 20, 1, "12345", 4, *s_arr[379]);
	test(pop, *s_arr[376], 20, 1, "12345", 5, *s_arr[380]);
	test(pop, *s_arr[376], 20, 1, "1234567890", 0, *s_arr[376]);
	test(pop, *s_arr[376], 20, 1, "1234567890", 1, *s_arr[377]);
	test(pop, *s_arr[376], 20, 1, "1234567890", 5, *s_arr[380]);
	test(pop, *s_arr[376], 20, 1, "1234567890", 9, *s_arr[381]);
	test(pop, *s_arr[376], 20, 1, "1234567890", 10, *s_arr[382]);
	test(pop, *s_arr[376], 20, 1, "12345678901234567890", 0, *s_arr[376]);
	test(pop, *s_arr[376], 20, 1, "12345678901234567890", 1, *s_arr[377]);
	test(pop, *s_arr[376], 20, 1, "12345678901234567890", 10, *s_arr[382]);
	test(pop, *s_arr[376], 20, 1, "12345678901234567890", 19, *s_arr[383]);
	test(pop, *s_arr[376], 20, 1, "12345678901234567890", 20, *s_arr[384]);
	test(pop, *s_arr[376], 21, 0, "", 0, *s_arr[405]);
	test(pop, *s_arr[376], 21, 0, "12345", 0, *s_arr[405]);
	test(pop, *s_arr[376], 21, 0, "12345", 1, *s_arr[405]);
	test(pop, *s_arr[376], 21, 0, "12345", 2, *s_arr[405]);
	test(pop, *s_arr[376], 21, 0, "12345", 4, *s_arr[405]);
	test(pop, *s_arr[376], 21, 0, "12345", 5, *s_arr[405]);
	test(pop, *s_arr[376], 21, 0, "1234567890", 0, *s_arr[405]);
	test(pop, *s_arr[376], 21, 0, "1234567890", 1, *s_arr[405]);
	test(pop, *s_arr[376], 21, 0, "1234567890", 5, *s_arr[405]);
	test(pop, *s_arr[376], 21, 0, "1234567890", 9, *s_arr[405]);
	test(pop, *s_arr[376], 21, 0, "1234567890", 10, *s_arr[405]);
	test(pop, *s_arr[376], 21, 0, "12345678901234567890", 0, *s_arr[405]);
	test(pop, *s_arr[376], 21, 0, "12345678901234567890", 1, *s_arr[405]);
	test(pop, *s_arr[376], 21, 0, "12345678901234567890", 10, *s_arr[405]);
	test(pop, *s_arr[376], 21, 0, "12345678901234567890", 19, *s_arr[405]);
	test(pop, *s_arr[376], 21, 0, "12345678901234567890", 20, *s_arr[405]);
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
				s_arr[1] = nvobj::make_persistent<C>("1");
				s_arr[2] = nvobj::make_persistent<C>("12");
				s_arr[3] = nvobj::make_persistent<C>("1234");
				s_arr[4] = nvobj::make_persistent<C>("12345");
				s_arr[5] =
					nvobj::make_persistent<C>("123456789");
				s_arr[6] =
					nvobj::make_persistent<C>("1234567890");
				s_arr[7] = nvobj::make_persistent<C>(
					"1234567890123456789");
				s_arr[8] = nvobj::make_persistent<C>(
					"12345678901234567890");
				s_arr[9] = nvobj::make_persistent<C>(
					"12345678901234567890abcde");
				s_arr[10] = nvobj::make_persistent<C>(
					"12345678901234567890abcdefghij");
				s_arr[11] = nvobj::make_persistent<C>(
					"12345678901234567890abcdefghijklmnopqrst");
				s_arr[12] = nvobj::make_persistent<C>(
					"12345678901234567890bcde");
				s_arr[13] = nvobj::make_persistent<C>(
					"12345678901234567890bcdefghij");
				s_arr[14] = nvobj::make_persistent<C>(
					"12345678901234567890bcdefghijklmnopqrst");
				s_arr[15] = nvobj::make_persistent<C>(
					"12345678901234567890cde");
				s_arr[16] = nvobj::make_persistent<C>(
					"12345678901234567890e");
				s_arr[17] = nvobj::make_persistent<C>(
					"12345678901234567890fghij");
				s_arr[18] = nvobj::make_persistent<C>(
					"12345678901234567890j");
				s_arr[19] = nvobj::make_persistent<C>(
					"12345678901234567890klmnopqrst");
				s_arr[20] = nvobj::make_persistent<C>(
					"12345678901234567890t");
				s_arr[21] = nvobj::make_persistent<C>(
					"1234567890123456789abcde");
				s_arr[22] = nvobj::make_persistent<C>(
					"1234567890123456789abcdefghij");
				s_arr[23] = nvobj::make_persistent<C>(
					"1234567890123456789abcdefghijklmnopqrst");
				s_arr[24] = nvobj::make_persistent<C>(
					"1234567890123456789bcde");
				s_arr[25] = nvobj::make_persistent<C>(
					"1234567890123456789bcdefghij");
				s_arr[26] = nvobj::make_persistent<C>(
					"1234567890123456789bcdefghijklmnopqrst");
				s_arr[27] = nvobj::make_persistent<C>(
					"1234567890123456789cde");
				s_arr[28] = nvobj::make_persistent<C>(
					"1234567890123456789e");
				s_arr[29] = nvobj::make_persistent<C>(
					"1234567890123456789fghij");
				s_arr[30] = nvobj::make_persistent<C>(
					"1234567890123456789j");
				s_arr[31] = nvobj::make_persistent<C>(
					"1234567890123456789klmnopqrst");
				s_arr[32] = nvobj::make_persistent<C>(
					"1234567890123456789t");
				s_arr[33] = nvobj::make_persistent<C>(
					"1234567890abcde");
				s_arr[34] = nvobj::make_persistent<C>(
					"1234567890abcdefghij");
				s_arr[35] = nvobj::make_persistent<C>(
					"1234567890abcdefghijklmnopqrst");
				s_arr[36] = nvobj::make_persistent<C>(
					"1234567890bcde");
				s_arr[37] = nvobj::make_persistent<C>(
					"1234567890bcdefghij");
				s_arr[38] = nvobj::make_persistent<C>(
					"1234567890bcdefghijklmnopqrst");
				s_arr[39] = nvobj::make_persistent<C>(
					"1234567890cde");
				s_arr[40] = nvobj::make_persistent<C>(
					"1234567890e");
				s_arr[41] = nvobj::make_persistent<C>(
					"1234567890fghij");
				s_arr[42] = nvobj::make_persistent<C>(
					"1234567890j");
				s_arr[43] = nvobj::make_persistent<C>(
					"1234567890klmnopqrst");
				s_arr[44] = nvobj::make_persistent<C>(
					"1234567890t");
				s_arr[45] = nvobj::make_persistent<C>(
					"123456789abcde");
				s_arr[46] = nvobj::make_persistent<C>(
					"123456789abcdefghij");
				s_arr[47] = nvobj::make_persistent<C>(
					"123456789abcdefghijklmnopqrst");
				s_arr[48] = nvobj::make_persistent<C>(
					"123456789bcde");
				s_arr[49] = nvobj::make_persistent<C>(
					"123456789bcdefghij");
				s_arr[50] = nvobj::make_persistent<C>(
					"123456789bcdefghijklmnopqrst");
				s_arr[51] = nvobj::make_persistent<C>(
					"123456789cde");
				s_arr[52] =
					nvobj::make_persistent<C>("123456789e");
				s_arr[53] = nvobj::make_persistent<C>(
					"123456789fghij");
				s_arr[54] =
					nvobj::make_persistent<C>("123456789j");
				s_arr[55] = nvobj::make_persistent<C>(
					"123456789klmnopqrst");
				s_arr[56] =
					nvobj::make_persistent<C>("123456789t");
				s_arr[57] =
					nvobj::make_persistent<C>("12345abcde");
				s_arr[58] = nvobj::make_persistent<C>(
					"12345abcdefghij");
				s_arr[59] = nvobj::make_persistent<C>(
					"12345abcdefghijklmnopqrst");
				s_arr[60] =
					nvobj::make_persistent<C>("12345bcde");
				s_arr[61] = nvobj::make_persistent<C>(
					"12345bcdefghij");
				s_arr[62] = nvobj::make_persistent<C>(
					"12345bcdefghijklmnopqrst");
				s_arr[63] =
					nvobj::make_persistent<C>("12345cde");
				s_arr[64] = nvobj::make_persistent<C>("12345e");
				s_arr[65] =
					nvobj::make_persistent<C>("12345fghij");
				s_arr[66] = nvobj::make_persistent<C>("12345j");
				s_arr[67] = nvobj::make_persistent<C>(
					"12345klmnopqrst");
				s_arr[68] = nvobj::make_persistent<C>("12345t");
				s_arr[69] =
					nvobj::make_persistent<C>("1234abcde");
				s_arr[70] = nvobj::make_persistent<C>(
					"1234abcdefghij");
				s_arr[71] = nvobj::make_persistent<C>(
					"1234abcdefghijklmnopqrst");
				s_arr[72] =
					nvobj::make_persistent<C>("1234bcde");
				s_arr[73] = nvobj::make_persistent<C>(
					"1234bcdefghij");
				s_arr[74] = nvobj::make_persistent<C>(
					"1234bcdefghijklmnopqrst");
				s_arr[75] =
					nvobj::make_persistent<C>("1234cde");
				s_arr[76] = nvobj::make_persistent<C>("1234e");
				s_arr[77] =
					nvobj::make_persistent<C>("1234fghij");
				s_arr[78] = nvobj::make_persistent<C>("1234j");
				s_arr[79] = nvobj::make_persistent<C>(
					"1234klmnopqrst");
				s_arr[80] = nvobj::make_persistent<C>("1234t");
				s_arr[81] =
					nvobj::make_persistent<C>("12abcde");
				s_arr[82] = nvobj::make_persistent<C>(
					"12abcdefghij");
				s_arr[83] = nvobj::make_persistent<C>(
					"12abcdefghijklmnopqrst");
				s_arr[84] = nvobj::make_persistent<C>("12bcde");
				s_arr[85] = nvobj::make_persistent<C>(
					"12bcdefghij");
				s_arr[86] = nvobj::make_persistent<C>(
					"12bcdefghijklmnopqrst");
				s_arr[87] = nvobj::make_persistent<C>("12cde");
				s_arr[88] = nvobj::make_persistent<C>("12e");
				s_arr[89] =
					nvobj::make_persistent<C>("12fghij");
				s_arr[90] = nvobj::make_persistent<C>("12j");
				s_arr[91] = nvobj::make_persistent<C>(
					"12klmnopqrst");
				s_arr[92] = nvobj::make_persistent<C>("12t");
				s_arr[93] = nvobj::make_persistent<C>("1abcde");
				s_arr[94] = nvobj::make_persistent<C>(
					"1abcdefghij");
				s_arr[95] = nvobj::make_persistent<C>(
					"1abcdefghijklmnopqrst");
				s_arr[96] = nvobj::make_persistent<C>("1bcde");
				s_arr[97] =
					nvobj::make_persistent<C>("1bcdefghij");
				s_arr[98] = nvobj::make_persistent<C>(
					"1bcdefghijklmnopqrst");
				s_arr[99] = nvobj::make_persistent<C>("1cde");
				s_arr[100] = nvobj::make_persistent<C>("1e");
				s_arr[101] =
					nvobj::make_persistent<C>("1fghij");
				s_arr[102] = nvobj::make_persistent<C>("1j");
				s_arr[103] = nvobj::make_persistent<C>(
					"1klmnopqrst");
				s_arr[104] = nvobj::make_persistent<C>("1t");
				s_arr[105] = nvobj::make_persistent<C>("a");
				s_arr[106] = nvobj::make_persistent<C>("a1");
				s_arr[107] = nvobj::make_persistent<C>("a12");
				s_arr[108] = nvobj::make_persistent<C>("a1234");
				s_arr[109] =
					nvobj::make_persistent<C>("a12345");
				s_arr[110] =
					nvobj::make_persistent<C>("a123456789");
				s_arr[111] = nvobj::make_persistent<C>(
					"a1234567890");
				s_arr[112] = nvobj::make_persistent<C>(
					"a1234567890123456789");
				s_arr[113] = nvobj::make_persistent<C>(
					"a12345678901234567890");
				s_arr[114] = nvobj::make_persistent<C>(
					"a12345678901234567890bcde");
				s_arr[115] = nvobj::make_persistent<C>(
					"a12345678901234567890bcdefghij");
				s_arr[116] = nvobj::make_persistent<C>(
					"a12345678901234567890bcdefghijklmnopqrst");
				s_arr[117] = nvobj::make_persistent<C>(
					"a12345678901234567890cde");
				s_arr[118] = nvobj::make_persistent<C>(
					"a12345678901234567890cdefghij");
				s_arr[119] = nvobj::make_persistent<C>(
					"a12345678901234567890cdefghijklmnopqrst");
				s_arr[120] = nvobj::make_persistent<C>(
					"a12345678901234567890de");
				s_arr[121] = nvobj::make_persistent<C>(
					"a12345678901234567890e");
				s_arr[122] = nvobj::make_persistent<C>(
					"a12345678901234567890fghij");
				s_arr[123] = nvobj::make_persistent<C>(
					"a12345678901234567890j");
				s_arr[124] = nvobj::make_persistent<C>(
					"a12345678901234567890klmnopqrst");
				s_arr[125] = nvobj::make_persistent<C>(
					"a12345678901234567890t");
				s_arr[126] = nvobj::make_persistent<C>(
					"a1234567890123456789bcde");
				s_arr[127] = nvobj::make_persistent<C>(
					"a1234567890123456789bcdefghij");
				s_arr[128] = nvobj::make_persistent<C>(
					"a1234567890123456789bcdefghijklmnopqrst");
				s_arr[129] = nvobj::make_persistent<C>(
					"a1234567890123456789cde");
				s_arr[130] = nvobj::make_persistent<C>(
					"a1234567890123456789cdefghij");
				s_arr[131] = nvobj::make_persistent<C>(
					"a1234567890123456789cdefghijklmnopqrst");
				s_arr[132] = nvobj::make_persistent<C>(
					"a1234567890123456789de");
				s_arr[133] = nvobj::make_persistent<C>(
					"a1234567890123456789e");
				s_arr[134] = nvobj::make_persistent<C>(
					"a1234567890123456789fghij");
				s_arr[135] = nvobj::make_persistent<C>(
					"a1234567890123456789j");
				s_arr[136] = nvobj::make_persistent<C>(
					"a1234567890123456789klmnopqrst");
				s_arr[137] = nvobj::make_persistent<C>(
					"a1234567890123456789t");
				s_arr[138] = nvobj::make_persistent<C>(
					"a1234567890bcde");
				s_arr[139] = nvobj::make_persistent<C>(
					"a1234567890bcdefghij");
				s_arr[140] = nvobj::make_persistent<C>(
					"a1234567890bcdefghijklmnopqrst");
				s_arr[141] = nvobj::make_persistent<C>(
					"a1234567890cde");
				s_arr[142] = nvobj::make_persistent<C>(
					"a1234567890cdefghij");
				s_arr[143] = nvobj::make_persistent<C>(
					"a1234567890cdefghijklmnopqrst");
				s_arr[144] = nvobj::make_persistent<C>(
					"a1234567890de");
				s_arr[145] = nvobj::make_persistent<C>(
					"a1234567890e");
				s_arr[146] = nvobj::make_persistent<C>(
					"a1234567890fghij");
				s_arr[147] = nvobj::make_persistent<C>(
					"a1234567890j");
				s_arr[148] = nvobj::make_persistent<C>(
					"a1234567890klmnopqrst");
				s_arr[149] = nvobj::make_persistent<C>(
					"a1234567890t");
				s_arr[150] = nvobj::make_persistent<C>(
					"a123456789bcde");
				s_arr[151] = nvobj::make_persistent<C>(
					"a123456789bcdefghij");
				s_arr[152] = nvobj::make_persistent<C>(
					"a123456789bcdefghijklmnopqrst");
				s_arr[153] = nvobj::make_persistent<C>(
					"a123456789cde");
				s_arr[154] = nvobj::make_persistent<C>(
					"a123456789cdefghij");
				s_arr[155] = nvobj::make_persistent<C>(
					"a123456789cdefghijklmnopqrst");
				s_arr[156] = nvobj::make_persistent<C>(
					"a123456789de");
				s_arr[157] = nvobj::make_persistent<C>(
					"a123456789e");
				s_arr[158] = nvobj::make_persistent<C>(
					"a123456789fghij");
				s_arr[159] = nvobj::make_persistent<C>(
					"a123456789j");
				s_arr[160] = nvobj::make_persistent<C>(
					"a123456789klmnopqrst");
				s_arr[161] = nvobj::make_persistent<C>(
					"a123456789t");
				s_arr[162] =
					nvobj::make_persistent<C>("a12345bcde");
				s_arr[163] = nvobj::make_persistent<C>(
					"a12345bcdefghij");
				s_arr[164] = nvobj::make_persistent<C>(
					"a12345bcdefghijklmnopqrst");
				s_arr[165] =
					nvobj::make_persistent<C>("a12345cde");
				s_arr[166] = nvobj::make_persistent<C>(
					"a12345cdefghij");
				s_arr[167] = nvobj::make_persistent<C>(
					"a12345cdefghijklmnopqrst");
				s_arr[168] =
					nvobj::make_persistent<C>("a12345de");
				s_arr[169] =
					nvobj::make_persistent<C>("a12345e");
				s_arr[170] = nvobj::make_persistent<C>(
					"a12345fghij");
				s_arr[171] =
					nvobj::make_persistent<C>("a12345j");
				s_arr[172] = nvobj::make_persistent<C>(
					"a12345klmnopqrst");
				s_arr[173] =
					nvobj::make_persistent<C>("a12345t");
				s_arr[174] =
					nvobj::make_persistent<C>("a1234bcde");
				s_arr[175] = nvobj::make_persistent<C>(
					"a1234bcdefghij");
				s_arr[176] = nvobj::make_persistent<C>(
					"a1234bcdefghijklmnopqrst");
				s_arr[177] =
					nvobj::make_persistent<C>("a1234cde");
				s_arr[178] = nvobj::make_persistent<C>(
					"a1234cdefghij");
				s_arr[179] = nvobj::make_persistent<C>(
					"a1234cdefghijklmnopqrst");
				s_arr[180] =
					nvobj::make_persistent<C>("a1234de");
				s_arr[181] =
					nvobj::make_persistent<C>("a1234e");
				s_arr[182] =
					nvobj::make_persistent<C>("a1234fghij");
				s_arr[183] =
					nvobj::make_persistent<C>("a1234j");
				s_arr[184] = nvobj::make_persistent<C>(
					"a1234klmnopqrst");
				s_arr[185] =
					nvobj::make_persistent<C>("a1234t");
				s_arr[186] =
					nvobj::make_persistent<C>("a12bcde");
				s_arr[187] = nvobj::make_persistent<C>(
					"a12bcdefghij");
				s_arr[188] = nvobj::make_persistent<C>(
					"a12bcdefghijklmnopqrst");
				s_arr[189] =
					nvobj::make_persistent<C>("a12cde");
				s_arr[190] = nvobj::make_persistent<C>(
					"a12cdefghij");
				s_arr[191] = nvobj::make_persistent<C>(
					"a12cdefghijklmnopqrst");
				s_arr[192] = nvobj::make_persistent<C>("a12de");
				s_arr[193] = nvobj::make_persistent<C>("a12e");
				s_arr[194] =
					nvobj::make_persistent<C>("a12fghij");
				s_arr[195] = nvobj::make_persistent<C>("a12j");
				s_arr[196] = nvobj::make_persistent<C>(
					"a12klmnopqrst");
				s_arr[197] = nvobj::make_persistent<C>("a12t");
				s_arr[198] =
					nvobj::make_persistent<C>("a1bcde");
				s_arr[199] = nvobj::make_persistent<C>(
					"a1bcdefghij");
				s_arr[200] = nvobj::make_persistent<C>(
					"a1bcdefghijklmnopqrst");
				s_arr[201] = nvobj::make_persistent<C>("a1cde");
				s_arr[202] =
					nvobj::make_persistent<C>("a1cdefghij");
				s_arr[203] = nvobj::make_persistent<C>(
					"a1cdefghijklmnopqrst");
				s_arr[204] = nvobj::make_persistent<C>("a1de");
				s_arr[205] = nvobj::make_persistent<C>("a1e");
				s_arr[206] =
					nvobj::make_persistent<C>("a1fghij");
				s_arr[207] = nvobj::make_persistent<C>("a1j");
				s_arr[208] = nvobj::make_persistent<C>(
					"a1klmnopqrst");
				s_arr[209] = nvobj::make_persistent<C>("a1t");
				s_arr[210] = nvobj::make_persistent<C>("ab");
				s_arr[211] = nvobj::make_persistent<C>("ab1");
				s_arr[212] = nvobj::make_persistent<C>("ab12");
				s_arr[213] =
					nvobj::make_persistent<C>("ab1234");
				s_arr[214] =
					nvobj::make_persistent<C>("ab12345");
				s_arr[215] = nvobj::make_persistent<C>(
					"ab123456789");
				s_arr[216] = nvobj::make_persistent<C>(
					"ab1234567890");
				s_arr[217] = nvobj::make_persistent<C>(
					"ab1234567890123456789");
				s_arr[218] = nvobj::make_persistent<C>(
					"ab12345678901234567890");
				s_arr[219] = nvobj::make_persistent<C>(
					"ab12345678901234567890cde");
				s_arr[220] = nvobj::make_persistent<C>(
					"ab12345678901234567890de");
				s_arr[221] = nvobj::make_persistent<C>(
					"ab12345678901234567890e");
				s_arr[222] = nvobj::make_persistent<C>(
					"ab1234567890123456789cde");
				s_arr[223] = nvobj::make_persistent<C>(
					"ab1234567890123456789de");
				s_arr[224] = nvobj::make_persistent<C>(
					"ab1234567890123456789e");
				s_arr[225] = nvobj::make_persistent<C>(
					"ab1234567890cde");
				s_arr[226] = nvobj::make_persistent<C>(
					"ab1234567890de");
				s_arr[227] = nvobj::make_persistent<C>(
					"ab1234567890e");
				s_arr[228] = nvobj::make_persistent<C>(
					"ab123456789cde");
				s_arr[229] = nvobj::make_persistent<C>(
					"ab123456789de");
				s_arr[230] = nvobj::make_persistent<C>(
					"ab123456789e");
				s_arr[231] =
					nvobj::make_persistent<C>("ab12345cde");
				s_arr[232] =
					nvobj::make_persistent<C>("ab12345de");
				s_arr[233] =
					nvobj::make_persistent<C>("ab12345e");
				s_arr[234] =
					nvobj::make_persistent<C>("ab1234cde");
				s_arr[235] =
					nvobj::make_persistent<C>("ab1234de");
				s_arr[236] =
					nvobj::make_persistent<C>("ab1234e");
				s_arr[237] =
					nvobj::make_persistent<C>("ab12cde");
				s_arr[238] =
					nvobj::make_persistent<C>("ab12de");
				s_arr[239] = nvobj::make_persistent<C>("ab12e");
				s_arr[240] =
					nvobj::make_persistent<C>("ab1cde");
				s_arr[241] = nvobj::make_persistent<C>("ab1de");
				s_arr[242] = nvobj::make_persistent<C>("ab1e");
				s_arr[243] = nvobj::make_persistent<C>("abcd");
				s_arr[244] = nvobj::make_persistent<C>("abcd1");
				s_arr[245] =
					nvobj::make_persistent<C>("abcd12");
				s_arr[246] =
					nvobj::make_persistent<C>("abcd1234");
				s_arr[247] =
					nvobj::make_persistent<C>("abcd12345");
				s_arr[248] = nvobj::make_persistent<C>(
					"abcd123456789");
				s_arr[249] = nvobj::make_persistent<C>(
					"abcd1234567890");
				s_arr[250] = nvobj::make_persistent<C>(
					"abcd1234567890123456789");
				s_arr[251] = nvobj::make_persistent<C>(
					"abcd12345678901234567890");
				s_arr[252] = nvobj::make_persistent<C>(
					"abcd12345678901234567890e");
				s_arr[253] = nvobj::make_persistent<C>(
					"abcd1234567890123456789e");
				s_arr[254] = nvobj::make_persistent<C>(
					"abcd1234567890e");
				s_arr[255] = nvobj::make_persistent<C>(
					"abcd123456789e");
				s_arr[256] =
					nvobj::make_persistent<C>("abcd12345e");
				s_arr[257] =
					nvobj::make_persistent<C>("abcd1234e");
				s_arr[258] =
					nvobj::make_persistent<C>("abcd12e");
				s_arr[259] =
					nvobj::make_persistent<C>("abcd1e");
				s_arr[260] = nvobj::make_persistent<C>("abcde");
				s_arr[261] =
					nvobj::make_persistent<C>("abcde1");
				s_arr[262] =
					nvobj::make_persistent<C>("abcde12");
				s_arr[263] =
					nvobj::make_persistent<C>("abcde1234");
				s_arr[264] =
					nvobj::make_persistent<C>("abcde12345");
				s_arr[265] = nvobj::make_persistent<C>(
					"abcde123456789");
				s_arr[266] = nvobj::make_persistent<C>(
					"abcde1234567890");
				s_arr[267] = nvobj::make_persistent<C>(
					"abcde1234567890123456789");
				s_arr[268] = nvobj::make_persistent<C>(
					"abcde12345678901234567890");
				s_arr[269] = nvobj::make_persistent<C>(
					"abcde12345678901234567890fghij");
				s_arr[270] = nvobj::make_persistent<C>(
					"abcde12345678901234567890ghij");
				s_arr[271] = nvobj::make_persistent<C>(
					"abcde12345678901234567890hij");
				s_arr[272] = nvobj::make_persistent<C>(
					"abcde12345678901234567890j");
				s_arr[273] = nvobj::make_persistent<C>(
					"abcde1234567890123456789fghij");
				s_arr[274] = nvobj::make_persistent<C>(
					"abcde1234567890123456789ghij");
				s_arr[275] = nvobj::make_persistent<C>(
					"abcde1234567890123456789hij");
				s_arr[276] = nvobj::make_persistent<C>(
					"abcde1234567890123456789j");
				s_arr[277] = nvobj::make_persistent<C>(
					"abcde1234567890fghij");
				s_arr[278] = nvobj::make_persistent<C>(
					"abcde1234567890ghij");
				s_arr[279] = nvobj::make_persistent<C>(
					"abcde1234567890hij");
				s_arr[280] = nvobj::make_persistent<C>(
					"abcde1234567890j");
				s_arr[281] = nvobj::make_persistent<C>(
					"abcde123456789fghij");
				s_arr[282] = nvobj::make_persistent<C>(
					"abcde123456789ghij");
				s_arr[283] = nvobj::make_persistent<C>(
					"abcde123456789hij");
				s_arr[284] = nvobj::make_persistent<C>(
					"abcde123456789j");
				s_arr[285] = nvobj::make_persistent<C>(
					"abcde12345fghij");
				s_arr[286] = nvobj::make_persistent<C>(
					"abcde12345ghij");
				s_arr[287] = nvobj::make_persistent<C>(
					"abcde12345hij");
				s_arr[288] = nvobj::make_persistent<C>(
					"abcde12345j");
				s_arr[289] = nvobj::make_persistent<C>(
					"abcde1234fghij");
				s_arr[290] = nvobj::make_persistent<C>(
					"abcde1234ghij");
				s_arr[291] = nvobj::make_persistent<C>(
					"abcde1234hij");
				s_arr[292] =
					nvobj::make_persistent<C>("abcde1234j");
				s_arr[293] = nvobj::make_persistent<C>(
					"abcde12fghij");
				s_arr[294] = nvobj::make_persistent<C>(
					"abcde12ghij");
				s_arr[295] =
					nvobj::make_persistent<C>("abcde12hij");
				s_arr[296] =
					nvobj::make_persistent<C>("abcde12j");
				s_arr[297] = nvobj::make_persistent<C>(
					"abcde1fghij");
				s_arr[298] =
					nvobj::make_persistent<C>("abcde1ghij");
				s_arr[299] =
					nvobj::make_persistent<C>("abcde1hij");
				s_arr[300] =
					nvobj::make_persistent<C>("abcde1j");
				s_arr[301] =
					nvobj::make_persistent<C>("abcdefghi");
				s_arr[302] =
					nvobj::make_persistent<C>("abcdefghi1");
				s_arr[303] = nvobj::make_persistent<C>(
					"abcdefghi12");
				s_arr[304] = nvobj::make_persistent<C>(
					"abcdefghi1234");
				s_arr[305] = nvobj::make_persistent<C>(
					"abcdefghi12345");
				s_arr[306] = nvobj::make_persistent<C>(
					"abcdefghi123456789");
				s_arr[307] = nvobj::make_persistent<C>(
					"abcdefghi1234567890");
				s_arr[308] = nvobj::make_persistent<C>(
					"abcdefghi1234567890123456789");
				s_arr[309] = nvobj::make_persistent<C>(
					"abcdefghi12345678901234567890");
				s_arr[310] = nvobj::make_persistent<C>(
					"abcdefghi12345678901234567890j");
				s_arr[311] = nvobj::make_persistent<C>(
					"abcdefghi1234567890123456789j");
				s_arr[312] = nvobj::make_persistent<C>(
					"abcdefghi1234567890j");
				s_arr[313] = nvobj::make_persistent<C>(
					"abcdefghi123456789j");
				s_arr[314] = nvobj::make_persistent<C>(
					"abcdefghi12345j");
				s_arr[315] = nvobj::make_persistent<C>(
					"abcdefghi1234j");
				s_arr[316] = nvobj::make_persistent<C>(
					"abcdefghi12j");
				s_arr[317] = nvobj::make_persistent<C>(
					"abcdefghi1j");
				s_arr[318] =
					nvobj::make_persistent<C>("abcdefghij");
				s_arr[319] = nvobj::make_persistent<C>(
					"abcdefghij1");
				s_arr[320] = nvobj::make_persistent<C>(
					"abcdefghij12");
				s_arr[321] = nvobj::make_persistent<C>(
					"abcdefghij1234");
				s_arr[322] = nvobj::make_persistent<C>(
					"abcdefghij12345");
				s_arr[323] = nvobj::make_persistent<C>(
					"abcdefghij123456789");
				s_arr[324] = nvobj::make_persistent<C>(
					"abcdefghij1234567890");
				s_arr[325] = nvobj::make_persistent<C>(
					"abcdefghij1234567890123456789");
				s_arr[326] = nvobj::make_persistent<C>(
					"abcdefghij12345678901234567890");
				s_arr[327] = nvobj::make_persistent<C>(
					"abcdefghij12345678901234567890klmnopqrst");
				s_arr[328] = nvobj::make_persistent<C>(
					"abcdefghij12345678901234567890lmnopqrst");
				s_arr[329] = nvobj::make_persistent<C>(
					"abcdefghij12345678901234567890pqrst");
				s_arr[330] = nvobj::make_persistent<C>(
					"abcdefghij12345678901234567890t");
				s_arr[331] = nvobj::make_persistent<C>(
					"abcdefghij1234567890123456789klmnopqrst");
				s_arr[332] = nvobj::make_persistent<C>(
					"abcdefghij1234567890123456789lmnopqrst");
				s_arr[333] = nvobj::make_persistent<C>(
					"abcdefghij1234567890123456789pqrst");
				s_arr[334] = nvobj::make_persistent<C>(
					"abcdefghij1234567890123456789t");
				s_arr[335] = nvobj::make_persistent<C>(
					"abcdefghij1234567890klmnopqrst");
				s_arr[336] = nvobj::make_persistent<C>(
					"abcdefghij1234567890lmnopqrst");
				s_arr[337] = nvobj::make_persistent<C>(
					"abcdefghij1234567890pqrst");
				s_arr[338] = nvobj::make_persistent<C>(
					"abcdefghij1234567890t");
				s_arr[339] = nvobj::make_persistent<C>(
					"abcdefghij123456789klmnopqrst");
				s_arr[340] = nvobj::make_persistent<C>(
					"abcdefghij123456789lmnopqrst");
				s_arr[341] = nvobj::make_persistent<C>(
					"abcdefghij123456789pqrst");
				s_arr[342] = nvobj::make_persistent<C>(
					"abcdefghij123456789t");
				s_arr[343] = nvobj::make_persistent<C>(
					"abcdefghij12345klmnopqrst");
				s_arr[344] = nvobj::make_persistent<C>(
					"abcdefghij12345lmnopqrst");
				s_arr[345] = nvobj::make_persistent<C>(
					"abcdefghij12345pqrst");
				s_arr[346] = nvobj::make_persistent<C>(
					"abcdefghij12345t");
				s_arr[347] = nvobj::make_persistent<C>(
					"abcdefghij1234klmnopqrst");
				s_arr[348] = nvobj::make_persistent<C>(
					"abcdefghij1234lmnopqrst");
				s_arr[349] = nvobj::make_persistent<C>(
					"abcdefghij1234pqrst");
				s_arr[350] = nvobj::make_persistent<C>(
					"abcdefghij1234t");
				s_arr[351] = nvobj::make_persistent<C>(
					"abcdefghij12klmnopqrst");
				s_arr[352] = nvobj::make_persistent<C>(
					"abcdefghij12lmnopqrst");
				s_arr[353] = nvobj::make_persistent<C>(
					"abcdefghij12pqrst");
				s_arr[354] = nvobj::make_persistent<C>(
					"abcdefghij12t");
				s_arr[355] = nvobj::make_persistent<C>(
					"abcdefghij1klmnopqrst");
				s_arr[356] = nvobj::make_persistent<C>(
					"abcdefghij1lmnopqrst");
				s_arr[357] = nvobj::make_persistent<C>(
					"abcdefghij1pqrst");
				s_arr[358] = nvobj::make_persistent<C>(
					"abcdefghij1t");
				s_arr[359] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs");
				s_arr[360] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs1");
				s_arr[361] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs12");
				s_arr[362] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs1234");
				s_arr[363] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs12345");
				s_arr[364] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs123456789");
				s_arr[365] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs1234567890");
				s_arr[366] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs1234567890123456789");
				s_arr[367] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs12345678901234567890");
				s_arr[368] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs12345678901234567890t");
				s_arr[369] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs1234567890123456789t");
				s_arr[370] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs1234567890t");
				s_arr[371] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs123456789t");
				s_arr[372] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs12345t");
				s_arr[373] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs1234t");
				s_arr[374] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs12t");
				s_arr[375] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrs1t");
				s_arr[376] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst");
				s_arr[377] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst1");
				s_arr[378] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst12");
				s_arr[379] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst1234");
				s_arr[380] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst12345");
				s_arr[381] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst123456789");
				s_arr[382] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst1234567890");
				s_arr[383] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst1234567890123456789");
				s_arr[384] = nvobj::make_persistent<C>(
					"abcdefghijklmnopqrst12345678901234567890");
				s_arr[385] = nvobj::make_persistent<C>(
					"abcdefghijlmnopqrst");
				s_arr[386] = nvobj::make_persistent<C>(
					"abcdefghijpqrst");
				s_arr[387] = nvobj::make_persistent<C>(
					"abcdefghijt");
				s_arr[388] =
					nvobj::make_persistent<C>("abcdeghij");
				s_arr[389] =
					nvobj::make_persistent<C>("abcdehij");
				s_arr[390] =
					nvobj::make_persistent<C>("abcdej");
				s_arr[391] = nvobj::make_persistent<C>("abde");
				s_arr[392] = nvobj::make_persistent<C>("abe");
				s_arr[393] = nvobj::make_persistent<C>("acde");
				s_arr[394] =
					nvobj::make_persistent<C>("acdefghij");
				s_arr[395] = nvobj::make_persistent<C>(
					"acdefghijklmnopqrst");
				s_arr[396] = nvobj::make_persistent<C>("ade");
				s_arr[397] = nvobj::make_persistent<C>("ae");
				s_arr[398] =
					nvobj::make_persistent<C>("afghij");
				s_arr[399] = nvobj::make_persistent<C>("aj");
				s_arr[400] = nvobj::make_persistent<C>(
					"aklmnopqrst");
				s_arr[401] = nvobj::make_persistent<C>("at");
				s_arr[402] = nvobj::make_persistent<C>("bcde");
				s_arr[403] =
					nvobj::make_persistent<C>("bcdefghij");
				s_arr[404] = nvobj::make_persistent<C>(
					"bcdefghijklmnopqrst");
				s_arr[405] = nvobj::make_persistent<C>(
					"can't happen");
				s_arr[406] = nvobj::make_persistent<C>("cde");
				s_arr[407] = nvobj::make_persistent<C>("e");
				s_arr[408] = nvobj::make_persistent<C>("fghij");
				s_arr[409] = nvobj::make_persistent<C>("j");
				s_arr[410] =
					nvobj::make_persistent<C>("klmnopqrst");
				s_arr[411] = nvobj::make_persistent<C>("t");
			});

			test0<C>(pop);
			test1<C>(pop);
			test2<C>(pop);
			test3<C>(pop);
			test4<C>(pop);
			test5<C>(pop);
			test6<C>(pop);
			test7<C>(pop);
			test8<C>(pop);
			test9<C>(pop);
			test10<C>(pop);
			test11<C>(pop);

			nvobj::transaction::run(pop, [&] {
				for (unsigned i = 0; i < 412; ++i) {
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
