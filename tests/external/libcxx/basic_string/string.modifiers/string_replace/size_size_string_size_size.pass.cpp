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
	nvobj::persistent_ptr<C> s_arr[1211];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &s1, typename S::size_type pos1,
     typename S::size_type n1, const S &str, typename S::size_type pos2,
     typename S::size_type n2, const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<C>(s1); });

	auto &s = *r->s;

	const typename S::size_type old_size = s.size();

	if (pos1 <= old_size && pos2 <= str.size()) {
		s.replace(pos1, n1, str, pos2, n2);
		UT_ASSERT(s == expected);
		typename S::size_type xlen = (std::min)(n1, old_size - pos1);
		typename S::size_type rlen = (std::min)(n2, str.size() - pos2);
		UT_ASSERT(s.size() == old_size - xlen + rlen);
	} else {
		try {
			s.replace(pos1, n1, str, pos2, n2);
			UT_ASSERT(false);
		} catch (std::out_of_range &) {
			UT_ASSERT(pos1 > old_size || pos2 > str.size());
			UT_ASSERT(s == s1);
		}
	}

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<C>(r->s); });
}

template <class S>
void
test_npos(nvobj::pool<struct root> &pop, const S &s1,
	  typename S::size_type pos1, typename S::size_type n1, const S &str,
	  typename S::size_type pos2, const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<C>(s1); });

	auto &s = *r->s;

	const typename S::size_type old_size = s.size();

	if (pos1 <= old_size && pos2 <= str.size()) {
		s.replace(pos1, n1, str, pos2);
		UT_ASSERT(s == expected);
		typename S::size_type xlen = (std::min)(n1, old_size - pos1);

		auto npos = S::npos;
		typename S::size_type rlen =
			(std::min)(npos, str.size() - pos2);
		UT_ASSERT(s.size() == old_size - xlen + rlen);
	} else {
		try {
			s.replace(pos1, n1, str, pos2);
			UT_ASSERT(false);
		} catch (std::out_of_range &) {
			UT_ASSERT(pos1 > old_size || pos2 > str.size());
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

	test(pop, *s_arr[0], 0, 0, *s_arr[0], 0, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[0], 0, 1, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 0, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 0, 1, *s_arr[14]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 0, 2, *s_arr[15]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 0, 4, *s_arr[16]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 0, 5, *s_arr[17]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 0, 6, *s_arr[17]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 1, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 1, 1, *s_arr[118]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 1, 2, *s_arr[119]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 1, 3, *s_arr[120]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 1, 4, *s_arr[121]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 1, 5, *s_arr[121]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 2, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 2, 1, *s_arr[222]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 2, 2, *s_arr[223]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 2, 3, *s_arr[224]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 2, 4, *s_arr[224]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 4, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 4, 1, *s_arr[261]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 4, 2, *s_arr[261]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 5, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 5, 1, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 0, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 0, 1, *s_arr[14]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 0, 5, *s_arr[17]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 0, 9, *s_arr[18]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 0, 10, *s_arr[19]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 0, 11, *s_arr[19]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 1, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 1, 1, *s_arr[118]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 1, 4, *s_arr[121]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 1, 8, *s_arr[122]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 1, 9, *s_arr[123]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 1, 10, *s_arr[123]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 5, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 5, 1, *s_arr[274]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 5, 2, *s_arr[275]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 5, 4, *s_arr[276]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 5, 5, *s_arr[277]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 5, 6, *s_arr[277]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 9, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 9, 1, *s_arr[1]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 9, 2, *s_arr[1]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 10, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 10, 1, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 0, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 0, 1, *s_arr[14]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 0, 10, *s_arr[19]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 0, 19, *s_arr[20]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 0, 20, *s_arr[21]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 0, 21, *s_arr[21]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 1, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 1, 1, *s_arr[118]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 1, 9, *s_arr[123]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 1, 18, *s_arr[124]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 1, 19, *s_arr[125]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 1, 20, *s_arr[125]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 10, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 10, 1, *s_arr[14]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 10, 5, *s_arr[17]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 10, 9, *s_arr[18]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 10, 10, *s_arr[19]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 10, 11, *s_arr[19]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 19, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 19, 1, *s_arr[1]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 19, 2, *s_arr[1]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 20, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 20, 1, *s_arr[0]);
	test(pop, *s_arr[0], 0, 0, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 0, 1, *s_arr[0], 0, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[0], 0, 1, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 0, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 0, 1, *s_arr[14]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 0, 2, *s_arr[15]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 0, 4, *s_arr[16]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 0, 5, *s_arr[17]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 0, 6, *s_arr[17]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 1, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 1, 1, *s_arr[118]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 1, 2, *s_arr[119]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 1, 3, *s_arr[120]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 1, 4, *s_arr[121]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 1, 5, *s_arr[121]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 2, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 2, 1, *s_arr[222]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 2, 2, *s_arr[223]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 2, 3, *s_arr[224]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 2, 4, *s_arr[224]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 4, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 4, 1, *s_arr[261]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 4, 2, *s_arr[261]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 5, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 5, 1, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[17], 6, 0, *s_arr[1204]);
}

template <class S>
void
test1(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[0], 0, 1, *s_arr[19], 0, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 0, 1, *s_arr[14]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 0, 5, *s_arr[17]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 0, 9, *s_arr[18]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 0, 10, *s_arr[19]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 0, 11, *s_arr[19]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 1, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 1, 1, *s_arr[118]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 1, 4, *s_arr[121]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 1, 8, *s_arr[122]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 1, 9, *s_arr[123]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 1, 10, *s_arr[123]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 5, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 5, 1, *s_arr[274]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 5, 2, *s_arr[275]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 5, 4, *s_arr[276]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 5, 5, *s_arr[277]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 5, 6, *s_arr[277]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 9, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 9, 1, *s_arr[1]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 9, 2, *s_arr[1]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 10, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 10, 1, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 0, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 0, 1, *s_arr[14]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 0, 10, *s_arr[19]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 0, 19, *s_arr[20]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 0, 20, *s_arr[21]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 0, 21, *s_arr[21]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 1, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 1, 1, *s_arr[118]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 1, 9, *s_arr[123]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 1, 18, *s_arr[124]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 1, 19, *s_arr[125]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 1, 20, *s_arr[125]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 10, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 10, 1, *s_arr[14]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 10, 5, *s_arr[17]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 10, 9, *s_arr[18]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 10, 10, *s_arr[19]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 10, 11, *s_arr[19]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 19, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 19, 1, *s_arr[1]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 19, 2, *s_arr[1]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 20, 0, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 20, 1, *s_arr[0]);
	test(pop, *s_arr[0], 0, 1, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[0], 0, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[0], 0, 1, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 0, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 0, 1, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 0, 2, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 0, 4, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 0, 5, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 0, 6, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 1, 1, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 1, 2, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 1, 3, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 1, 4, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 1, 5, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 2, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 2, 1, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 2, 2, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 2, 3, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 2, 4, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 4, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 4, 1, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 4, 2, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 5, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 5, 1, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 0, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 0, 1, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 0, 5, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 0, 9, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 0, 10, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 0, 11, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 1, 1, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 1, 4, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 1, 8, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 1, 9, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 1, 10, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 5, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 5, 1, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 5, 2, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 5, 4, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 5, 5, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 5, 6, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 9, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 9, 1, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 9, 2, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 10, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 10, 1, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 0, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 0, 1, *s_arr[1204]);
}

template <class S>
void
test2(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[0], 1, 0, *s_arr[21], 0, 10, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 0, 19, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 0, 20, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 0, 21, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 1, 1, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 1, 9, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 1, 18, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 1, 19, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 1, 20, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 10, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 10, 1, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 10, 5, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 10, 9, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 10, 10, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 10, 11, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 19, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 19, 1, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 19, 2, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 20, 0, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 20, 1, *s_arr[1204]);
	test(pop, *s_arr[0], 1, 0, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 0, *s_arr[0], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[0], 0, 1, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 0, 1, *s_arr[106]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 0, 2, *s_arr[94]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 0, 4, *s_arr[82]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 0, 5, *s_arr[70]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 0, 6, *s_arr[70]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 1, 0, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 1, 1, *s_arr[210]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 1, 2, *s_arr[198]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 1, 3, *s_arr[186]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 1, 4, *s_arr[174]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 1, 5, *s_arr[174]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 2, 0, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 2, 1, *s_arr[249]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 2, 2, *s_arr[237]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 2, 3, *s_arr[225]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 2, 4, *s_arr[225]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 4, 0, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 4, 1, *s_arr[262]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 4, 2, *s_arr[262]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 5, 0, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 5, 1, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 0, 1, *s_arr[106]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 0, 5, *s_arr[70]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 0, 9, *s_arr[58]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 0, 10, *s_arr[46]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 0, 11, *s_arr[46]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 1, 0, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 1, 1, *s_arr[210]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 1, 4, *s_arr[174]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 1, 8, *s_arr[162]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 1, 9, *s_arr[150]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 1, 10, *s_arr[150]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 5, 0, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 5, 1, *s_arr[314]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 5, 2, *s_arr[302]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 5, 4, *s_arr[290]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 5, 5, *s_arr[278]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 5, 6, *s_arr[278]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 9, 0, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 9, 1, *s_arr[2]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 9, 2, *s_arr[2]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 10, 0, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 10, 1, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 0, 1, *s_arr[106]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 0, 10, *s_arr[46]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 0, 19, *s_arr[34]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 0, 20, *s_arr[22]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 0, 21, *s_arr[22]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 1, 0, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 1, 1, *s_arr[210]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 1, 9, *s_arr[150]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 1, 18, *s_arr[138]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 1, 19, *s_arr[126]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 1, 20, *s_arr[126]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 10, 0, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 10, 1, *s_arr[106]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 10, 5, *s_arr[70]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 10, 9, *s_arr[58]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 10, 10, *s_arr[46]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 10, 11, *s_arr[46]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 19, 0, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 19, 1, *s_arr[2]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 19, 2, *s_arr[2]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 20, 0, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 20, 1, *s_arr[804]);
	test(pop, *s_arr[804], 0, 0, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 1, *s_arr[0], 0, 0, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[0], 0, 1, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 0, 0, *s_arr[1201]);
}

template <class S>
void
test3(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[804], 0, 1, *s_arr[17], 0, 1, *s_arr[109]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 0, 2, *s_arr[97]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 0, 4, *s_arr[85]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 0, 5, *s_arr[73]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 0, 6, *s_arr[73]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 1, 0, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 1, 1, *s_arr[213]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 1, 2, *s_arr[201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 1, 3, *s_arr[189]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 1, 4, *s_arr[177]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 1, 5, *s_arr[177]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 2, 0, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 2, 1, *s_arr[252]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 2, 2, *s_arr[240]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 2, 3, *s_arr[228]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 2, 4, *s_arr[228]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 4, 0, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 4, 1, *s_arr[265]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 4, 2, *s_arr[265]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 5, 0, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 5, 1, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 0, 0, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 0, 1, *s_arr[109]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 0, 5, *s_arr[73]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 0, 9, *s_arr[61]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 0, 10, *s_arr[49]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 0, 11, *s_arr[49]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 1, 0, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 1, 1, *s_arr[213]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 1, 4, *s_arr[177]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 1, 8, *s_arr[165]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 1, 9, *s_arr[153]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 1, 10, *s_arr[153]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 5, 0, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 5, 1, *s_arr[317]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 5, 2, *s_arr[305]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 5, 4, *s_arr[293]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 5, 5, *s_arr[281]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 5, 6, *s_arr[281]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 9, 0, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 9, 1, *s_arr[5]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 9, 2, *s_arr[5]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 10, 0, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 10, 1, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 0, 0, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 0, 1, *s_arr[109]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 0, 10, *s_arr[49]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 0, 19, *s_arr[37]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 0, 20, *s_arr[25]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 0, 21, *s_arr[25]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 1, 0, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 1, 1, *s_arr[213]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 1, 9, *s_arr[153]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 1, 18, *s_arr[141]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 1, 19, *s_arr[129]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 1, 20, *s_arr[129]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 10, 0, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 10, 1, *s_arr[109]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 10, 5, *s_arr[73]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 10, 9, *s_arr[61]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 10, 10, *s_arr[49]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 10, 11, *s_arr[49]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 19, 0, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 19, 1, *s_arr[5]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 19, 2, *s_arr[5]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 20, 0, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 20, 1, *s_arr[1201]);
	test(pop, *s_arr[804], 0, 1, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 2, *s_arr[0], 0, 0, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[0], 0, 1, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 0, 0, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 0, 1, *s_arr[112]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 0, 2, *s_arr[100]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 0, 4, *s_arr[88]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 0, 5, *s_arr[76]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 0, 6, *s_arr[76]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 1, 0, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 1, 1, *s_arr[216]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 1, 2, *s_arr[204]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 1, 3, *s_arr[192]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 1, 4, *s_arr[180]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 1, 5, *s_arr[180]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 2, 0, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 2, 1, *s_arr[255]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 2, 2, *s_arr[243]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 2, 3, *s_arr[231]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 2, 4, *s_arr[231]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 4, 0, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 4, 1, *s_arr[268]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 4, 2, *s_arr[268]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 5, 0, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 5, 1, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 0, 0, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 0, 1, *s_arr[112]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 0, 5, *s_arr[76]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 0, 9, *s_arr[64]);
}

template <class S>
void
test4(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[804], 0, 2, *s_arr[19], 0, 10, *s_arr[52]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 0, 11, *s_arr[52]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 1, 0, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 1, 1, *s_arr[216]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 1, 4, *s_arr[180]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 1, 8, *s_arr[168]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 1, 9, *s_arr[156]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 1, 10, *s_arr[156]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 5, 0, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 5, 1, *s_arr[320]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 5, 2, *s_arr[308]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 5, 4, *s_arr[296]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 5, 5, *s_arr[284]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 5, 6, *s_arr[284]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 9, 0, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 9, 1, *s_arr[8]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 9, 2, *s_arr[8]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 10, 0, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 10, 1, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 0, 0, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 0, 1, *s_arr[112]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 0, 10, *s_arr[52]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 0, 19, *s_arr[40]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 0, 20, *s_arr[28]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 0, 21, *s_arr[28]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 1, 0, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 1, 1, *s_arr[216]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 1, 9, *s_arr[156]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 1, 18, *s_arr[144]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 1, 19, *s_arr[132]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 1, 20, *s_arr[132]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 10, 0, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 10, 1, *s_arr[112]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 10, 5, *s_arr[76]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 10, 9, *s_arr[64]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 10, 10, *s_arr[52]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 10, 11, *s_arr[52]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 19, 0, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 19, 1, *s_arr[8]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 19, 2, *s_arr[8]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 20, 0, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 20, 1, *s_arr[1205]);
	test(pop, *s_arr[804], 0, 2, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 4, *s_arr[0], 0, 0, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[0], 0, 1, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 0, 0, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 0, 1, *s_arr[113]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 0, 2, *s_arr[101]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 0, 4, *s_arr[89]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 0, 5, *s_arr[77]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 0, 6, *s_arr[77]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 1, 0, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 1, 1, *s_arr[217]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 1, 2, *s_arr[205]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 1, 3, *s_arr[193]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 1, 4, *s_arr[181]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 1, 5, *s_arr[181]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 2, 0, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 2, 1, *s_arr[256]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 2, 2, *s_arr[244]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 2, 3, *s_arr[232]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 2, 4, *s_arr[232]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 4, 0, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 4, 1, *s_arr[269]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 4, 2, *s_arr[269]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 5, 0, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 5, 1, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 0, 0, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 0, 1, *s_arr[113]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 0, 5, *s_arr[77]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 0, 9, *s_arr[65]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 0, 10, *s_arr[53]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 0, 11, *s_arr[53]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 1, 0, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 1, 1, *s_arr[217]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 1, 4, *s_arr[181]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 1, 8, *s_arr[169]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 1, 9, *s_arr[157]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 1, 10, *s_arr[157]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 5, 0, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 5, 1, *s_arr[321]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 5, 2, *s_arr[309]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 5, 4, *s_arr[297]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 5, 5, *s_arr[285]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 5, 6, *s_arr[285]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 9, 0, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 9, 1, *s_arr[9]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 9, 2, *s_arr[9]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 10, 0, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 10, 1, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 0, 0, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 0, 1, *s_arr[113]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 0, 10, *s_arr[53]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 0, 19, *s_arr[41]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 0, 20, *s_arr[29]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 0, 21, *s_arr[29]);
}

template <class S>
void
test5(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[804], 0, 4, *s_arr[21], 1, 0, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 1, 1, *s_arr[217]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 1, 9, *s_arr[157]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 1, 18, *s_arr[145]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 1, 19, *s_arr[133]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 1, 20, *s_arr[133]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 10, 0, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 10, 1, *s_arr[113]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 10, 5, *s_arr[77]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 10, 9, *s_arr[65]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 10, 10, *s_arr[53]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 10, 11, *s_arr[53]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 19, 0, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 19, 1, *s_arr[9]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 19, 2, *s_arr[9]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 20, 0, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 20, 1, *s_arr[1206]);
	test(pop, *s_arr[804], 0, 4, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 5, *s_arr[0], 0, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[0], 0, 1, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 0, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 0, 1, *s_arr[14]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 0, 2, *s_arr[15]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 0, 4, *s_arr[16]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 0, 5, *s_arr[17]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 0, 6, *s_arr[17]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 1, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 1, 1, *s_arr[118]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 1, 2, *s_arr[119]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 1, 3, *s_arr[120]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 1, 4, *s_arr[121]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 1, 5, *s_arr[121]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 2, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 2, 1, *s_arr[222]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 2, 2, *s_arr[223]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 2, 3, *s_arr[224]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 2, 4, *s_arr[224]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 4, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 4, 1, *s_arr[261]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 4, 2, *s_arr[261]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 5, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 5, 1, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 0, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 0, 1, *s_arr[14]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 0, 5, *s_arr[17]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 0, 9, *s_arr[18]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 0, 10, *s_arr[19]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 0, 11, *s_arr[19]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 1, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 1, 1, *s_arr[118]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 1, 4, *s_arr[121]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 1, 8, *s_arr[122]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 1, 9, *s_arr[123]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 1, 10, *s_arr[123]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 5, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 5, 1, *s_arr[274]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 5, 2, *s_arr[275]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 5, 4, *s_arr[276]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 5, 5, *s_arr[277]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 5, 6, *s_arr[277]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 9, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 9, 1, *s_arr[1]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 9, 2, *s_arr[1]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 10, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 10, 1, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 0, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 0, 1, *s_arr[14]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 0, 10, *s_arr[19]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 0, 19, *s_arr[20]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 0, 20, *s_arr[21]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 0, 21, *s_arr[21]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 1, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 1, 1, *s_arr[118]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 1, 9, *s_arr[123]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 1, 18, *s_arr[124]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 1, 19, *s_arr[125]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 1, 20, *s_arr[125]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 10, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 10, 1, *s_arr[14]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 10, 5, *s_arr[17]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 10, 9, *s_arr[18]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 10, 10, *s_arr[19]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 10, 11, *s_arr[19]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 19, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 19, 1, *s_arr[1]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 19, 2, *s_arr[1]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 20, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 20, 1, *s_arr[0]);
	test(pop, *s_arr[804], 0, 5, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 6, *s_arr[0], 0, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[0], 0, 1, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 0, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 0, 1, *s_arr[14]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 0, 2, *s_arr[15]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 0, 4, *s_arr[16]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 0, 5, *s_arr[17]);
}

template <class S>
void
test6(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[804], 0, 6, *s_arr[17], 0, 6, *s_arr[17]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 1, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 1, 1, *s_arr[118]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 1, 2, *s_arr[119]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 1, 3, *s_arr[120]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 1, 4, *s_arr[121]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 1, 5, *s_arr[121]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 2, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 2, 1, *s_arr[222]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 2, 2, *s_arr[223]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 2, 3, *s_arr[224]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 2, 4, *s_arr[224]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 4, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 4, 1, *s_arr[261]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 4, 2, *s_arr[261]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 5, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 5, 1, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 0, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 0, 1, *s_arr[14]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 0, 5, *s_arr[17]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 0, 9, *s_arr[18]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 0, 10, *s_arr[19]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 0, 11, *s_arr[19]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 1, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 1, 1, *s_arr[118]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 1, 4, *s_arr[121]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 1, 8, *s_arr[122]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 1, 9, *s_arr[123]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 1, 10, *s_arr[123]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 5, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 5, 1, *s_arr[274]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 5, 2, *s_arr[275]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 5, 4, *s_arr[276]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 5, 5, *s_arr[277]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 5, 6, *s_arr[277]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 9, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 9, 1, *s_arr[1]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 9, 2, *s_arr[1]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 10, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 10, 1, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 0, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 0, 1, *s_arr[14]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 0, 10, *s_arr[19]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 0, 19, *s_arr[20]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 0, 20, *s_arr[21]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 0, 21, *s_arr[21]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 1, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 1, 1, *s_arr[118]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 1, 9, *s_arr[123]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 1, 18, *s_arr[124]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 1, 19, *s_arr[125]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 1, 20, *s_arr[125]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 10, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 10, 1, *s_arr[14]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 10, 5, *s_arr[17]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 10, 9, *s_arr[18]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 10, 10, *s_arr[19]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 10, 11, *s_arr[19]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 19, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 19, 1, *s_arr[1]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 19, 2, *s_arr[1]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 20, 0, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 20, 1, *s_arr[0]);
	test(pop, *s_arr[804], 0, 6, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 0, *s_arr[0], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[0], 0, 1, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 0, 1, *s_arr[432]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 0, 2, *s_arr[420]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 0, 4, *s_arr[408]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 0, 5, *s_arr[396]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 0, 6, *s_arr[396]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 1, 0, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 1, 1, *s_arr[536]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 1, 2, *s_arr[524]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 1, 3, *s_arr[512]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 1, 4, *s_arr[500]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 1, 5, *s_arr[500]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 2, 0, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 2, 1, *s_arr[575]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 2, 2, *s_arr[563]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 2, 3, *s_arr[551]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 2, 4, *s_arr[551]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 4, 0, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 4, 1, *s_arr[588]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 4, 2, *s_arr[588]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 5, 0, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 5, 1, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 0, 1, *s_arr[432]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 0, 5, *s_arr[396]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 0, 9, *s_arr[384]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 0, 10, *s_arr[372]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 0, 11, *s_arr[372]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 1, 0, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 1, 1, *s_arr[536]);
}

template <class S>
void
test7(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[804], 1, 0, *s_arr[19], 1, 4, *s_arr[500]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 1, 8, *s_arr[488]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 1, 9, *s_arr[476]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 1, 10, *s_arr[476]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 5, 0, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 5, 1, *s_arr[640]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 5, 2, *s_arr[628]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 5, 4, *s_arr[616]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 5, 5, *s_arr[604]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 5, 6, *s_arr[604]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 9, 0, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 9, 1, *s_arr[328]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 9, 2, *s_arr[328]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 10, 0, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 10, 1, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 0, 1, *s_arr[432]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 0, 10, *s_arr[372]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 0, 19, *s_arr[360]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 0, 20, *s_arr[348]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 0, 21, *s_arr[348]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 1, 0, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 1, 1, *s_arr[536]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 1, 9, *s_arr[476]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 1, 18, *s_arr[464]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 1, 19, *s_arr[452]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 1, 20, *s_arr[452]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 10, 0, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 10, 1, *s_arr[432]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 10, 5, *s_arr[396]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 10, 9, *s_arr[384]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 10, 10, *s_arr[372]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 10, 11, *s_arr[372]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 19, 0, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 19, 1, *s_arr[328]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 19, 2, *s_arr[328]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 20, 0, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 20, 1, *s_arr[804]);
	test(pop, *s_arr[804], 1, 0, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 1, *s_arr[0], 0, 0, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[0], 0, 1, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 0, 0, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 0, 1, *s_arr[435]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 0, 2, *s_arr[423]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 0, 4, *s_arr[411]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 0, 5, *s_arr[399]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 0, 6, *s_arr[399]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 1, 0, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 1, 1, *s_arr[539]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 1, 2, *s_arr[527]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 1, 3, *s_arr[515]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 1, 4, *s_arr[503]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 1, 5, *s_arr[503]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 2, 0, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 2, 1, *s_arr[578]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 2, 2, *s_arr[566]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 2, 3, *s_arr[554]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 2, 4, *s_arr[554]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 4, 0, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 4, 1, *s_arr[591]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 4, 2, *s_arr[591]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 5, 0, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 5, 1, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 0, 0, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 0, 1, *s_arr[435]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 0, 5, *s_arr[399]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 0, 9, *s_arr[387]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 0, 10, *s_arr[375]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 0, 11, *s_arr[375]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 1, 0, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 1, 1, *s_arr[539]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 1, 4, *s_arr[503]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 1, 8, *s_arr[491]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 1, 9, *s_arr[479]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 1, 10, *s_arr[479]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 5, 0, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 5, 1, *s_arr[643]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 5, 2, *s_arr[631]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 5, 4, *s_arr[619]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 5, 5, *s_arr[607]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 5, 6, *s_arr[607]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 9, 0, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 9, 1, *s_arr[331]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 9, 2, *s_arr[331]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 10, 0, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 10, 1, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 0, 0, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 0, 1, *s_arr[435]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 0, 10, *s_arr[375]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 0, 19, *s_arr[363]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 0, 20, *s_arr[351]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 0, 21, *s_arr[351]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 1, 0, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 1, 1, *s_arr[539]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 1, 9, *s_arr[479]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 1, 18, *s_arr[467]);
}

template <class S>
void
test8(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[804], 1, 1, *s_arr[21], 1, 19, *s_arr[455]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 1, 20, *s_arr[455]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 10, 0, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 10, 1, *s_arr[435]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 10, 5, *s_arr[399]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 10, 9, *s_arr[387]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 10, 10, *s_arr[375]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 10, 11, *s_arr[375]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 19, 0, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 19, 1, *s_arr[331]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 19, 2, *s_arr[331]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 20, 0, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 20, 1, *s_arr[1192]);
	test(pop, *s_arr[804], 1, 1, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 2, *s_arr[0], 0, 0, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[0], 0, 1, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 0, 0, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 0, 1, *s_arr[438]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 0, 2, *s_arr[426]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 0, 4, *s_arr[414]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 0, 5, *s_arr[402]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 0, 6, *s_arr[402]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 1, 0, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 1, 1, *s_arr[542]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 1, 2, *s_arr[530]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 1, 3, *s_arr[518]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 1, 4, *s_arr[506]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 1, 5, *s_arr[506]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 2, 0, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 2, 1, *s_arr[581]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 2, 2, *s_arr[569]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 2, 3, *s_arr[557]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 2, 4, *s_arr[557]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 4, 0, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 4, 1, *s_arr[594]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 4, 2, *s_arr[594]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 5, 0, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 5, 1, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 0, 0, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 0, 1, *s_arr[438]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 0, 5, *s_arr[402]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 0, 9, *s_arr[390]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 0, 10, *s_arr[378]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 0, 11, *s_arr[378]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 1, 0, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 1, 1, *s_arr[542]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 1, 4, *s_arr[506]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 1, 8, *s_arr[494]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 1, 9, *s_arr[482]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 1, 10, *s_arr[482]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 5, 0, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 5, 1, *s_arr[646]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 5, 2, *s_arr[634]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 5, 4, *s_arr[622]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 5, 5, *s_arr[610]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 5, 6, *s_arr[610]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 9, 0, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 9, 1, *s_arr[334]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 9, 2, *s_arr[334]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 10, 0, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 10, 1, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 0, 0, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 0, 1, *s_arr[438]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 0, 10, *s_arr[378]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 0, 19, *s_arr[366]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 0, 20, *s_arr[354]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 0, 21, *s_arr[354]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 1, 0, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 1, 1, *s_arr[542]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 1, 9, *s_arr[482]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 1, 18, *s_arr[470]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 1, 19, *s_arr[458]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 1, 20, *s_arr[458]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 10, 0, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 10, 1, *s_arr[438]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 10, 5, *s_arr[402]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 10, 9, *s_arr[390]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 10, 10, *s_arr[378]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 10, 11, *s_arr[378]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 19, 0, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 19, 1, *s_arr[334]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 19, 2, *s_arr[334]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 20, 0, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 20, 1, *s_arr[1195]);
	test(pop, *s_arr[804], 1, 2, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 3, *s_arr[0], 0, 0, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[0], 0, 1, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 0, 0, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 0, 1, *s_arr[439]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 0, 2, *s_arr[427]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 0, 4, *s_arr[415]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 0, 5, *s_arr[403]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 0, 6, *s_arr[403]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 1, 0, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 1, 1, *s_arr[543]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 1, 2, *s_arr[531]);
}

template <class S>
void
test9(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[804], 1, 3, *s_arr[17], 1, 3, *s_arr[519]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 1, 4, *s_arr[507]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 1, 5, *s_arr[507]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 2, 0, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 2, 1, *s_arr[582]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 2, 2, *s_arr[570]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 2, 3, *s_arr[558]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 2, 4, *s_arr[558]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 4, 0, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 4, 1, *s_arr[595]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 4, 2, *s_arr[595]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 5, 0, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 5, 1, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 0, 0, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 0, 1, *s_arr[439]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 0, 5, *s_arr[403]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 0, 9, *s_arr[391]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 0, 10, *s_arr[379]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 0, 11, *s_arr[379]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 1, 0, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 1, 1, *s_arr[543]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 1, 4, *s_arr[507]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 1, 8, *s_arr[495]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 1, 9, *s_arr[483]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 1, 10, *s_arr[483]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 5, 0, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 5, 1, *s_arr[647]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 5, 2, *s_arr[635]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 5, 4, *s_arr[623]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 5, 5, *s_arr[611]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 5, 6, *s_arr[611]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 9, 0, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 9, 1, *s_arr[335]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 9, 2, *s_arr[335]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 10, 0, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 10, 1, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 0, 0, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 0, 1, *s_arr[439]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 0, 10, *s_arr[379]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 0, 19, *s_arr[367]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 0, 20, *s_arr[355]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 0, 21, *s_arr[355]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 1, 0, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 1, 1, *s_arr[543]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 1, 9, *s_arr[483]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 1, 18, *s_arr[471]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 1, 19, *s_arr[459]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 1, 20, *s_arr[459]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 10, 0, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 10, 1, *s_arr[439]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 10, 5, *s_arr[403]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 10, 9, *s_arr[391]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 10, 10, *s_arr[379]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 10, 11, *s_arr[379]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 19, 0, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 19, 1, *s_arr[335]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 19, 2, *s_arr[335]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 20, 0, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 20, 1, *s_arr[1196]);
	test(pop, *s_arr[804], 1, 3, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 4, *s_arr[0], 0, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[0], 0, 1, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 0, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 0, 1, *s_arr[340]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 0, 2, *s_arr[341]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 0, 4, *s_arr[342]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 0, 5, *s_arr[343]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 0, 6, *s_arr[343]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 1, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 1, 1, *s_arr[444]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 1, 2, *s_arr[445]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 1, 3, *s_arr[446]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 1, 4, *s_arr[447]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 1, 5, *s_arr[447]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 2, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 2, 1, *s_arr[548]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 2, 2, *s_arr[549]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 2, 3, *s_arr[550]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 2, 4, *s_arr[550]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 4, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 4, 1, *s_arr[587]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 4, 2, *s_arr[587]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 5, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 5, 1, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 0, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 0, 1, *s_arr[340]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 0, 5, *s_arr[343]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 0, 9, *s_arr[344]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 0, 10, *s_arr[345]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 0, 11, *s_arr[345]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 1, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 1, 1, *s_arr[444]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 1, 4, *s_arr[447]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 1, 8, *s_arr[448]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 1, 9, *s_arr[449]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 1, 10, *s_arr[449]);
}

template <class S>
void
test10(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[804], 1, 4, *s_arr[19], 5, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 5, 1, *s_arr[600]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 5, 2, *s_arr[601]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 5, 4, *s_arr[602]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 5, 5, *s_arr[603]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 5, 6, *s_arr[603]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 9, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 9, 1, *s_arr[327]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 9, 2, *s_arr[327]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 10, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 10, 1, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 0, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 0, 1, *s_arr[340]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 0, 10, *s_arr[345]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 0, 19, *s_arr[346]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 0, 20, *s_arr[347]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 0, 21, *s_arr[347]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 1, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 1, 1, *s_arr[444]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 1, 9, *s_arr[449]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 1, 18, *s_arr[450]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 1, 19, *s_arr[451]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 1, 20, *s_arr[451]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 10, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 10, 1, *s_arr[340]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 10, 5, *s_arr[343]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 10, 9, *s_arr[344]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 10, 10, *s_arr[345]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 10, 11, *s_arr[345]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 19, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 19, 1, *s_arr[327]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 19, 2, *s_arr[327]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 20, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 20, 1, *s_arr[326]);
	test(pop, *s_arr[804], 1, 4, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 5, *s_arr[0], 0, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[0], 0, 1, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 0, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 0, 1, *s_arr[340]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 0, 2, *s_arr[341]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 0, 4, *s_arr[342]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 0, 5, *s_arr[343]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 0, 6, *s_arr[343]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 1, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 1, 1, *s_arr[444]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 1, 2, *s_arr[445]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 1, 3, *s_arr[446]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 1, 4, *s_arr[447]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 1, 5, *s_arr[447]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 2, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 2, 1, *s_arr[548]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 2, 2, *s_arr[549]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 2, 3, *s_arr[550]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 2, 4, *s_arr[550]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 4, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 4, 1, *s_arr[587]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 4, 2, *s_arr[587]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 5, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 5, 1, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 0, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 0, 1, *s_arr[340]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 0, 5, *s_arr[343]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 0, 9, *s_arr[344]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 0, 10, *s_arr[345]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 0, 11, *s_arr[345]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 1, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 1, 1, *s_arr[444]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 1, 4, *s_arr[447]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 1, 8, *s_arr[448]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 1, 9, *s_arr[449]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 1, 10, *s_arr[449]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 5, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 5, 1, *s_arr[600]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 5, 2, *s_arr[601]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 5, 4, *s_arr[602]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 5, 5, *s_arr[603]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 5, 6, *s_arr[603]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 9, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 9, 1, *s_arr[327]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 9, 2, *s_arr[327]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 10, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 10, 1, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 0, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 0, 1, *s_arr[340]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 0, 10, *s_arr[345]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 0, 19, *s_arr[346]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 0, 20, *s_arr[347]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 0, 21, *s_arr[347]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 1, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 1, 1, *s_arr[444]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 1, 9, *s_arr[449]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 1, 18, *s_arr[450]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 1, 19, *s_arr[451]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 1, 20, *s_arr[451]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 10, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 10, 1, *s_arr[340]);
}

template <class S>
void
test11(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[804], 1, 5, *s_arr[21], 10, 5, *s_arr[343]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 10, 9, *s_arr[344]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 10, 10, *s_arr[345]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 10, 11, *s_arr[345]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 19, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 19, 1, *s_arr[327]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 19, 2, *s_arr[327]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 20, 0, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 20, 1, *s_arr[326]);
	test(pop, *s_arr[804], 1, 5, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 0, *s_arr[0], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[0], 0, 1, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 0, 1, *s_arr[686]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 0, 2, *s_arr[683]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 0, 4, *s_arr[680]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 0, 5, *s_arr[677]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 0, 6, *s_arr[677]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 1, 0, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 1, 1, *s_arr[718]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 1, 2, *s_arr[715]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 1, 3, *s_arr[712]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 1, 4, *s_arr[709]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 1, 5, *s_arr[709]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 2, 0, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 2, 1, *s_arr[730]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 2, 2, *s_arr[727]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 2, 3, *s_arr[724]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 2, 4, *s_arr[724]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 4, 0, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 4, 1, *s_arr[734]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 4, 2, *s_arr[734]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 5, 0, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 5, 1, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 0, 1, *s_arr[686]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 0, 5, *s_arr[677]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 0, 9, *s_arr[674]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 0, 10, *s_arr[671]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 0, 11, *s_arr[671]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 1, 0, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 1, 1, *s_arr[718]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 1, 4, *s_arr[709]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 1, 8, *s_arr[706]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 1, 9, *s_arr[703]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 1, 10, *s_arr[703]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 5, 0, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 5, 1, *s_arr[750]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 5, 2, *s_arr[747]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 5, 4, *s_arr[744]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 5, 5, *s_arr[741]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 5, 6, *s_arr[741]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 9, 0, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 9, 1, *s_arr[654]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 9, 2, *s_arr[654]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 10, 0, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 10, 1, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 0, 1, *s_arr[686]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 0, 10, *s_arr[671]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 0, 19, *s_arr[668]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 0, 20, *s_arr[665]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 0, 21, *s_arr[665]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 1, 0, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 1, 1, *s_arr[718]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 1, 9, *s_arr[703]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 1, 18, *s_arr[700]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 1, 19, *s_arr[697]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 1, 20, *s_arr[697]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 10, 0, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 10, 1, *s_arr[686]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 10, 5, *s_arr[677]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 10, 9, *s_arr[674]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 10, 10, *s_arr[671]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 10, 11, *s_arr[671]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 19, 0, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 19, 1, *s_arr[654]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 19, 2, *s_arr[654]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 20, 0, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 20, 1, *s_arr[804]);
	test(pop, *s_arr[804], 2, 0, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 1, *s_arr[0], 0, 0, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[0], 0, 1, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 0, 0, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 0, 1, *s_arr[687]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 0, 2, *s_arr[684]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 0, 4, *s_arr[681]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 0, 5, *s_arr[678]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 0, 6, *s_arr[678]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 1, 0, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 1, 1, *s_arr[719]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 1, 2, *s_arr[716]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 1, 3, *s_arr[713]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 1, 4, *s_arr[710]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 1, 5, *s_arr[710]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 2, 0, *s_arr[1190]);
}

template <class S>
void
test12(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[804], 2, 1, *s_arr[17], 2, 1, *s_arr[731]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 2, 2, *s_arr[728]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 2, 3, *s_arr[725]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 2, 4, *s_arr[725]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 4, 0, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 4, 1, *s_arr[735]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 4, 2, *s_arr[735]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 5, 0, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 5, 1, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 0, 0, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 0, 1, *s_arr[687]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 0, 5, *s_arr[678]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 0, 9, *s_arr[675]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 0, 10, *s_arr[672]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 0, 11, *s_arr[672]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 1, 0, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 1, 1, *s_arr[719]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 1, 4, *s_arr[710]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 1, 8, *s_arr[707]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 1, 9, *s_arr[704]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 1, 10, *s_arr[704]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 5, 0, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 5, 1, *s_arr[751]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 5, 2, *s_arr[748]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 5, 4, *s_arr[745]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 5, 5, *s_arr[742]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 5, 6, *s_arr[742]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 9, 0, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 9, 1, *s_arr[655]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 9, 2, *s_arr[655]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 10, 0, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 10, 1, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 0, 0, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 0, 1, *s_arr[687]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 0, 10, *s_arr[672]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 0, 19, *s_arr[669]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 0, 20, *s_arr[666]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 0, 21, *s_arr[666]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 1, 0, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 1, 1, *s_arr[719]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 1, 9, *s_arr[704]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 1, 18, *s_arr[701]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 1, 19, *s_arr[698]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 1, 20, *s_arr[698]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 10, 0, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 10, 1, *s_arr[687]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 10, 5, *s_arr[678]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 10, 9, *s_arr[675]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 10, 10, *s_arr[672]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 10, 11, *s_arr[672]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 19, 0, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 19, 1, *s_arr[655]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 19, 2, *s_arr[655]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 20, 0, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 20, 1, *s_arr[1190]);
	test(pop, *s_arr[804], 2, 1, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 2, *s_arr[0], 0, 0, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[0], 0, 1, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 0, 0, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 0, 1, *s_arr[688]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 0, 2, *s_arr[685]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 0, 4, *s_arr[682]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 0, 5, *s_arr[679]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 0, 6, *s_arr[679]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 1, 0, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 1, 1, *s_arr[720]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 1, 2, *s_arr[717]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 1, 3, *s_arr[714]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 1, 4, *s_arr[711]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 1, 5, *s_arr[711]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 2, 0, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 2, 1, *s_arr[732]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 2, 2, *s_arr[729]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 2, 3, *s_arr[726]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 2, 4, *s_arr[726]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 4, 0, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 4, 1, *s_arr[736]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 4, 2, *s_arr[736]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 5, 0, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 5, 1, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 0, 0, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 0, 1, *s_arr[688]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 0, 5, *s_arr[679]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 0, 9, *s_arr[676]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 0, 10, *s_arr[673]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 0, 11, *s_arr[673]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 1, 0, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 1, 1, *s_arr[720]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 1, 4, *s_arr[711]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 1, 8, *s_arr[708]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 1, 9, *s_arr[705]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 1, 10, *s_arr[705]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 5, 0, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 5, 1, *s_arr[752]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 5, 2, *s_arr[749]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 5, 4, *s_arr[746]);
}

template <class S>
void
test13(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[804], 2, 2, *s_arr[19], 5, 5, *s_arr[743]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 5, 6, *s_arr[743]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 9, 0, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 9, 1, *s_arr[656]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 9, 2, *s_arr[656]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 10, 0, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 10, 1, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 0, 0, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 0, 1, *s_arr[688]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 0, 10, *s_arr[673]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 0, 19, *s_arr[670]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 0, 20, *s_arr[667]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 0, 21, *s_arr[667]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 1, 0, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 1, 1, *s_arr[720]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 1, 9, *s_arr[705]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 1, 18, *s_arr[702]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 1, 19, *s_arr[699]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 1, 20, *s_arr[699]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 10, 0, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 10, 1, *s_arr[688]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 10, 5, *s_arr[679]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 10, 9, *s_arr[676]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 10, 10, *s_arr[673]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 10, 11, *s_arr[673]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 19, 0, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 19, 1, *s_arr[656]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 19, 2, *s_arr[656]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 20, 0, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 20, 1, *s_arr[1191]);
	test(pop, *s_arr[804], 2, 2, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 3, *s_arr[0], 0, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[0], 0, 1, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 0, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 0, 1, *s_arr[657]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 0, 2, *s_arr[658]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 0, 4, *s_arr[659]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 0, 5, *s_arr[660]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 0, 6, *s_arr[660]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 1, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 1, 1, *s_arr[689]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 1, 2, *s_arr[690]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 1, 3, *s_arr[691]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 1, 4, *s_arr[692]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 1, 5, *s_arr[692]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 2, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 2, 1, *s_arr[721]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 2, 2, *s_arr[722]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 2, 3, *s_arr[723]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 2, 4, *s_arr[723]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 4, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 4, 1, *s_arr[733]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 4, 2, *s_arr[733]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 5, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 5, 1, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 0, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 0, 1, *s_arr[657]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 0, 5, *s_arr[660]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 0, 9, *s_arr[661]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 0, 10, *s_arr[662]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 0, 11, *s_arr[662]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 1, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 1, 1, *s_arr[689]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 1, 4, *s_arr[692]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 1, 8, *s_arr[693]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 1, 9, *s_arr[694]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 1, 10, *s_arr[694]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 5, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 5, 1, *s_arr[737]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 5, 2, *s_arr[738]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 5, 4, *s_arr[739]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 5, 5, *s_arr[740]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 5, 6, *s_arr[740]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 9, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 9, 1, *s_arr[653]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 9, 2, *s_arr[653]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 10, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 10, 1, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 0, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 0, 1, *s_arr[657]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 0, 10, *s_arr[662]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 0, 19, *s_arr[663]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 0, 20, *s_arr[664]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 0, 21, *s_arr[664]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 1, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 1, 1, *s_arr[689]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 1, 9, *s_arr[694]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 1, 18, *s_arr[695]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 1, 19, *s_arr[696]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 1, 20, *s_arr[696]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 10, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 10, 1, *s_arr[657]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 10, 5, *s_arr[660]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 10, 9, *s_arr[661]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 10, 10, *s_arr[662]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 10, 11, *s_arr[662]);
}

template <class S>
void
test14(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[804], 2, 3, *s_arr[21], 19, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 19, 1, *s_arr[653]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 19, 2, *s_arr[653]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 20, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 20, 1, *s_arr[652]);
	test(pop, *s_arr[804], 2, 3, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 4, *s_arr[0], 0, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[0], 0, 1, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 0, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 0, 1, *s_arr[657]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 0, 2, *s_arr[658]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 0, 4, *s_arr[659]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 0, 5, *s_arr[660]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 0, 6, *s_arr[660]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 1, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 1, 1, *s_arr[689]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 1, 2, *s_arr[690]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 1, 3, *s_arr[691]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 1, 4, *s_arr[692]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 1, 5, *s_arr[692]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 2, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 2, 1, *s_arr[721]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 2, 2, *s_arr[722]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 2, 3, *s_arr[723]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 2, 4, *s_arr[723]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 4, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 4, 1, *s_arr[733]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 4, 2, *s_arr[733]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 5, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 5, 1, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 0, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 0, 1, *s_arr[657]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 0, 5, *s_arr[660]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 0, 9, *s_arr[661]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 0, 10, *s_arr[662]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 0, 11, *s_arr[662]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 1, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 1, 1, *s_arr[689]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 1, 4, *s_arr[692]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 1, 8, *s_arr[693]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 1, 9, *s_arr[694]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 1, 10, *s_arr[694]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 5, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 5, 1, *s_arr[737]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 5, 2, *s_arr[738]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 5, 4, *s_arr[739]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 5, 5, *s_arr[740]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 5, 6, *s_arr[740]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 9, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 9, 1, *s_arr[653]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 9, 2, *s_arr[653]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 10, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 10, 1, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 0, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 0, 1, *s_arr[657]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 0, 10, *s_arr[662]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 0, 19, *s_arr[663]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 0, 20, *s_arr[664]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 0, 21, *s_arr[664]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 1, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 1, 1, *s_arr[689]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 1, 9, *s_arr[694]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 1, 18, *s_arr[695]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 1, 19, *s_arr[696]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 1, 20, *s_arr[696]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 10, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 10, 1, *s_arr[657]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 10, 5, *s_arr[660]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 10, 9, *s_arr[661]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 10, 10, *s_arr[662]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 10, 11, *s_arr[662]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 19, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 19, 1, *s_arr[653]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 19, 2, *s_arr[653]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 20, 0, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 20, 1, *s_arr[652]);
	test(pop, *s_arr[804], 2, 4, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 4, 0, *s_arr[0], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[0], 0, 1, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 0, 1, *s_arr[771]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 0, 2, *s_arr[770]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 0, 4, *s_arr[769]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 0, 5, *s_arr[768]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 0, 6, *s_arr[768]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 1, 0, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 1, 1, *s_arr[787]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 1, 2, *s_arr[786]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 1, 3, *s_arr[785]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 1, 4, *s_arr[784]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 1, 5, *s_arr[784]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 2, 0, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 2, 1, *s_arr[793]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 2, 2, *s_arr[792]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 2, 3, *s_arr[791]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 2, 4, *s_arr[791]);
}

template <class S>
void
test15(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[804], 4, 0, *s_arr[17], 4, 0, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 4, 1, *s_arr[795]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 4, 2, *s_arr[795]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 5, 0, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 5, 1, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 0, 1, *s_arr[771]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 0, 5, *s_arr[768]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 0, 9, *s_arr[767]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 0, 10, *s_arr[766]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 0, 11, *s_arr[766]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 1, 0, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 1, 1, *s_arr[787]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 1, 4, *s_arr[784]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 1, 8, *s_arr[783]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 1, 9, *s_arr[782]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 1, 10, *s_arr[782]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 5, 0, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 5, 1, *s_arr[803]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 5, 2, *s_arr[802]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 5, 4, *s_arr[801]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 5, 5, *s_arr[800]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 5, 6, *s_arr[800]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 9, 0, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 9, 1, *s_arr[755]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 9, 2, *s_arr[755]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 10, 0, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 10, 1, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 0, 1, *s_arr[771]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 0, 10, *s_arr[766]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 0, 19, *s_arr[765]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 0, 20, *s_arr[764]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 0, 21, *s_arr[764]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 1, 0, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 1, 1, *s_arr[787]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 1, 9, *s_arr[782]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 1, 18, *s_arr[781]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 1, 19, *s_arr[780]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 1, 20, *s_arr[780]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 10, 0, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 10, 1, *s_arr[771]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 10, 5, *s_arr[768]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 10, 9, *s_arr[767]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 10, 10, *s_arr[766]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 10, 11, *s_arr[766]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 19, 0, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 19, 1, *s_arr[755]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 19, 2, *s_arr[755]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 20, 0, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 20, 1, *s_arr[804]);
	test(pop, *s_arr[804], 4, 0, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 4, 1, *s_arr[0], 0, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[0], 0, 1, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 0, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 0, 1, *s_arr[756]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 0, 2, *s_arr[757]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 0, 4, *s_arr[758]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 0, 5, *s_arr[759]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 0, 6, *s_arr[759]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 1, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 1, 1, *s_arr[772]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 1, 2, *s_arr[773]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 1, 3, *s_arr[774]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 1, 4, *s_arr[775]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 1, 5, *s_arr[775]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 2, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 2, 1, *s_arr[788]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 2, 2, *s_arr[789]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 2, 3, *s_arr[790]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 2, 4, *s_arr[790]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 4, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 4, 1, *s_arr[794]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 4, 2, *s_arr[794]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 5, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 5, 1, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 0, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 0, 1, *s_arr[756]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 0, 5, *s_arr[759]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 0, 9, *s_arr[760]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 0, 10, *s_arr[761]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 0, 11, *s_arr[761]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 1, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 1, 1, *s_arr[772]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 1, 4, *s_arr[775]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 1, 8, *s_arr[776]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 1, 9, *s_arr[777]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 1, 10, *s_arr[777]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 5, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 5, 1, *s_arr[796]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 5, 2, *s_arr[797]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 5, 4, *s_arr[798]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 5, 5, *s_arr[799]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 5, 6, *s_arr[799]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 9, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 9, 1, *s_arr[754]);
}

template <class S>
void
test16(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[804], 4, 1, *s_arr[19], 9, 2, *s_arr[754]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 10, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 10, 1, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 0, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 0, 1, *s_arr[756]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 0, 10, *s_arr[761]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 0, 19, *s_arr[762]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 0, 20, *s_arr[763]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 0, 21, *s_arr[763]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 1, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 1, 1, *s_arr[772]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 1, 9, *s_arr[777]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 1, 18, *s_arr[778]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 1, 19, *s_arr[779]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 1, 20, *s_arr[779]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 10, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 10, 1, *s_arr[756]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 10, 5, *s_arr[759]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 10, 9, *s_arr[760]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 10, 10, *s_arr[761]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 10, 11, *s_arr[761]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 19, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 19, 1, *s_arr[754]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 19, 2, *s_arr[754]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 20, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 20, 1, *s_arr[753]);
	test(pop, *s_arr[804], 4, 1, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 4, 2, *s_arr[0], 0, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[0], 0, 1, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 0, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 0, 1, *s_arr[756]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 0, 2, *s_arr[757]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 0, 4, *s_arr[758]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 0, 5, *s_arr[759]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 0, 6, *s_arr[759]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 1, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 1, 1, *s_arr[772]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 1, 2, *s_arr[773]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 1, 3, *s_arr[774]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 1, 4, *s_arr[775]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 1, 5, *s_arr[775]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 2, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 2, 1, *s_arr[788]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 2, 2, *s_arr[789]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 2, 3, *s_arr[790]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 2, 4, *s_arr[790]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 4, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 4, 1, *s_arr[794]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 4, 2, *s_arr[794]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 5, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 5, 1, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 0, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 0, 1, *s_arr[756]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 0, 5, *s_arr[759]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 0, 9, *s_arr[760]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 0, 10, *s_arr[761]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 0, 11, *s_arr[761]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 1, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 1, 1, *s_arr[772]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 1, 4, *s_arr[775]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 1, 8, *s_arr[776]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 1, 9, *s_arr[777]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 1, 10, *s_arr[777]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 5, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 5, 1, *s_arr[796]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 5, 2, *s_arr[797]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 5, 4, *s_arr[798]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 5, 5, *s_arr[799]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 5, 6, *s_arr[799]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 9, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 9, 1, *s_arr[754]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 9, 2, *s_arr[754]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 10, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 10, 1, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 0, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 0, 1, *s_arr[756]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 0, 10, *s_arr[761]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 0, 19, *s_arr[762]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 0, 20, *s_arr[763]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 0, 21, *s_arr[763]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 1, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 1, 1, *s_arr[772]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 1, 9, *s_arr[777]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 1, 18, *s_arr[778]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 1, 19, *s_arr[779]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 1, 20, *s_arr[779]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 10, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 10, 1, *s_arr[756]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 10, 5, *s_arr[759]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 10, 9, *s_arr[760]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 10, 10, *s_arr[761]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 10, 11, *s_arr[761]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 19, 0, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 19, 1, *s_arr[754]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 19, 2, *s_arr[754]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 20, 0, *s_arr[753]);
}

template <class S>
void
test17(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[804], 4, 2, *s_arr[21], 20, 1, *s_arr[753]);
	test(pop, *s_arr[804], 4, 2, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 5, 0, *s_arr[0], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[0], 0, 1, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 0, 1, *s_arr[810]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 0, 2, *s_arr[811]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 0, 4, *s_arr[812]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 0, 5, *s_arr[813]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 0, 6, *s_arr[813]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 1, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 1, 1, *s_arr[850]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 1, 2, *s_arr[851]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 1, 3, *s_arr[852]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 1, 4, *s_arr[853]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 1, 5, *s_arr[853]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 2, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 2, 1, *s_arr[890]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 2, 2, *s_arr[891]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 2, 3, *s_arr[892]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 2, 4, *s_arr[892]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 4, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 4, 1, *s_arr[905]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 4, 2, *s_arr[905]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 5, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 5, 1, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 0, 1, *s_arr[810]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 0, 5, *s_arr[813]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 0, 9, *s_arr[814]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 0, 10, *s_arr[815]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 0, 11, *s_arr[815]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 1, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 1, 1, *s_arr[850]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 1, 4, *s_arr[853]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 1, 8, *s_arr[854]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 1, 9, *s_arr[855]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 1, 10, *s_arr[855]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 5, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 5, 1, *s_arr[910]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 5, 2, *s_arr[911]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 5, 4, *s_arr[912]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 5, 5, *s_arr[913]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 5, 6, *s_arr[913]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 9, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 9, 1, *s_arr[805]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 9, 2, *s_arr[805]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 10, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 10, 1, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 0, 1, *s_arr[810]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 0, 10, *s_arr[815]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 0, 19, *s_arr[816]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 0, 20, *s_arr[817]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 0, 21, *s_arr[817]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 1, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 1, 1, *s_arr[850]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 1, 9, *s_arr[855]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 1, 18, *s_arr[856]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 1, 19, *s_arr[857]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 1, 20, *s_arr[857]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 10, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 10, 1, *s_arr[810]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 10, 5, *s_arr[813]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 10, 9, *s_arr[814]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 10, 10, *s_arr[815]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 10, 11, *s_arr[815]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 19, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 19, 1, *s_arr[805]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 19, 2, *s_arr[805]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 20, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 20, 1, *s_arr[804]);
	test(pop, *s_arr[804], 5, 0, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 5, 1, *s_arr[0], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[0], 0, 1, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 0, 1, *s_arr[810]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 0, 2, *s_arr[811]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 0, 4, *s_arr[812]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 0, 5, *s_arr[813]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 0, 6, *s_arr[813]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 1, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 1, 1, *s_arr[850]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 1, 2, *s_arr[851]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 1, 3, *s_arr[852]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 1, 4, *s_arr[853]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 1, 5, *s_arr[853]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 2, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 2, 1, *s_arr[890]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 2, 2, *s_arr[891]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 2, 3, *s_arr[892]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 2, 4, *s_arr[892]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 4, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 4, 1, *s_arr[905]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 4, 2, *s_arr[905]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 5, 0, *s_arr[804]);
}

template <class S>
void
test18(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[804], 5, 1, *s_arr[17], 5, 1, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 0, 1, *s_arr[810]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 0, 5, *s_arr[813]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 0, 9, *s_arr[814]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 0, 10, *s_arr[815]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 0, 11, *s_arr[815]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 1, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 1, 1, *s_arr[850]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 1, 4, *s_arr[853]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 1, 8, *s_arr[854]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 1, 9, *s_arr[855]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 1, 10, *s_arr[855]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 5, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 5, 1, *s_arr[910]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 5, 2, *s_arr[911]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 5, 4, *s_arr[912]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 5, 5, *s_arr[913]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 5, 6, *s_arr[913]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 9, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 9, 1, *s_arr[805]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 9, 2, *s_arr[805]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 10, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 10, 1, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 0, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 0, 1, *s_arr[810]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 0, 10, *s_arr[815]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 0, 19, *s_arr[816]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 0, 20, *s_arr[817]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 0, 21, *s_arr[817]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 1, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 1, 1, *s_arr[850]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 1, 9, *s_arr[855]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 1, 18, *s_arr[856]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 1, 19, *s_arr[857]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 1, 20, *s_arr[857]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 10, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 10, 1, *s_arr[810]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 10, 5, *s_arr[813]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 10, 9, *s_arr[814]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 10, 10, *s_arr[815]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 10, 11, *s_arr[815]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 19, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 19, 1, *s_arr[805]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 19, 2, *s_arr[805]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 20, 0, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 20, 1, *s_arr[804]);
	test(pop, *s_arr[804], 5, 1, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[0], 0, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[0], 0, 1, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 0, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 0, 1, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 0, 2, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 0, 4, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 0, 5, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 0, 6, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 1, 1, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 1, 2, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 1, 3, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 1, 4, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 1, 5, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 2, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 2, 1, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 2, 2, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 2, 3, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 2, 4, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 4, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 4, 1, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 4, 2, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 5, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 5, 1, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 0, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 0, 1, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 0, 5, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 0, 9, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 0, 10, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 0, 11, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 1, 1, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 1, 4, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 1, 8, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 1, 9, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 1, 10, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 5, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 5, 1, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 5, 2, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 5, 4, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 5, 5, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 5, 6, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 9, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 9, 1, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 9, 2, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 10, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 10, 1, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[19], 11, 0, *s_arr[1204]);
}

template <class S>
void
test19(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[804], 6, 0, *s_arr[21], 0, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 0, 1, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 0, 10, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 0, 19, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 0, 20, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 0, 21, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 1, 1, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 1, 9, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 1, 18, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 1, 19, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 1, 20, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 10, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 10, 1, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 10, 5, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 10, 9, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 10, 10, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 10, 11, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 19, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 19, 1, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 19, 2, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 20, 0, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 20, 1, *s_arr[1204]);
	test(pop, *s_arr[804], 6, 0, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 0, *s_arr[0], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[0], 0, 1, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 0, 1, *s_arr[107]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 0, 2, *s_arr[95]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 0, 4, *s_arr[83]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 0, 5, *s_arr[71]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 0, 6, *s_arr[71]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 1, 0, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 1, 1, *s_arr[211]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 1, 2, *s_arr[199]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 1, 3, *s_arr[187]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 1, 4, *s_arr[175]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 1, 5, *s_arr[175]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 2, 0, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 2, 1, *s_arr[250]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 2, 2, *s_arr[238]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 2, 3, *s_arr[226]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 2, 4, *s_arr[226]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 4, 0, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 4, 1, *s_arr[263]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 4, 2, *s_arr[263]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 5, 0, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 5, 1, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 0, 1, *s_arr[107]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 0, 5, *s_arr[71]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 0, 9, *s_arr[59]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 0, 10, *s_arr[47]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 0, 11, *s_arr[47]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 1, 0, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 1, 1, *s_arr[211]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 1, 4, *s_arr[175]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 1, 8, *s_arr[163]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 1, 9, *s_arr[151]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 1, 10, *s_arr[151]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 5, 0, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 5, 1, *s_arr[315]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 5, 2, *s_arr[303]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 5, 4, *s_arr[291]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 5, 5, *s_arr[279]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 5, 6, *s_arr[279]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 9, 0, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 9, 1, *s_arr[3]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 9, 2, *s_arr[3]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 10, 0, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 10, 1, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 0, 1, *s_arr[107]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 0, 10, *s_arr[47]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 0, 19, *s_arr[35]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 0, 20, *s_arr[23]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 0, 21, *s_arr[23]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 1, 0, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 1, 1, *s_arr[211]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 1, 9, *s_arr[151]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 1, 18, *s_arr[139]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 1, 19, *s_arr[127]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 1, 20, *s_arr[127]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 10, 0, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 10, 1, *s_arr[107]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 10, 5, *s_arr[71]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 10, 9, *s_arr[59]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 10, 10, *s_arr[47]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 10, 11, *s_arr[47]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 19, 0, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 19, 1, *s_arr[3]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 19, 2, *s_arr[3]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 20, 0, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 20, 1, *s_arr[981]);
	test(pop, *s_arr[981], 0, 0, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 1, *s_arr[0], 0, 0, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[0], 0, 1, *s_arr[1202]);
}

template <class S>
void
test20(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[981], 0, 1, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 0, 0, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 0, 1, *s_arr[110]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 0, 2, *s_arr[98]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 0, 4, *s_arr[86]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 0, 5, *s_arr[74]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 0, 6, *s_arr[74]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 1, 0, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 1, 1, *s_arr[214]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 1, 2, *s_arr[202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 1, 3, *s_arr[190]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 1, 4, *s_arr[178]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 1, 5, *s_arr[178]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 2, 0, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 2, 1, *s_arr[253]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 2, 2, *s_arr[241]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 2, 3, *s_arr[229]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 2, 4, *s_arr[229]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 4, 0, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 4, 1, *s_arr[266]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 4, 2, *s_arr[266]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 5, 0, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 5, 1, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 0, 0, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 0, 1, *s_arr[110]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 0, 5, *s_arr[74]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 0, 9, *s_arr[62]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 0, 10, *s_arr[50]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 0, 11, *s_arr[50]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 1, 0, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 1, 1, *s_arr[214]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 1, 4, *s_arr[178]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 1, 8, *s_arr[166]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 1, 9, *s_arr[154]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 1, 10, *s_arr[154]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 5, 0, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 5, 1, *s_arr[318]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 5, 2, *s_arr[306]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 5, 4, *s_arr[294]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 5, 5, *s_arr[282]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 5, 6, *s_arr[282]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 9, 0, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 9, 1, *s_arr[6]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 9, 2, *s_arr[6]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 10, 0, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 10, 1, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 0, 0, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 0, 1, *s_arr[110]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 0, 10, *s_arr[50]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 0, 19, *s_arr[38]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 0, 20, *s_arr[26]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 0, 21, *s_arr[26]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 1, 0, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 1, 1, *s_arr[214]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 1, 9, *s_arr[154]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 1, 18, *s_arr[142]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 1, 19, *s_arr[130]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 1, 20, *s_arr[130]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 10, 0, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 10, 1, *s_arr[110]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 10, 5, *s_arr[74]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 10, 9, *s_arr[62]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 10, 10, *s_arr[50]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 10, 11, *s_arr[50]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 19, 0, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 19, 1, *s_arr[6]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 19, 2, *s_arr[6]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 20, 0, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 20, 1, *s_arr[1202]);
	test(pop, *s_arr[981], 0, 1, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 5, *s_arr[0], 0, 0, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[0], 0, 1, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 0, 0, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 0, 1, *s_arr[114]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 0, 2, *s_arr[102]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 0, 4, *s_arr[90]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 0, 5, *s_arr[78]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 0, 6, *s_arr[78]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 1, 0, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 1, 1, *s_arr[218]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 1, 2, *s_arr[206]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 1, 3, *s_arr[194]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 1, 4, *s_arr[182]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 1, 5, *s_arr[182]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 2, 0, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 2, 1, *s_arr[257]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 2, 2, *s_arr[245]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 2, 3, *s_arr[233]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 2, 4, *s_arr[233]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 4, 0, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 4, 1, *s_arr[270]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 4, 2, *s_arr[270]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 5, 0, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 5, 1, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 0, 0, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 0, 1, *s_arr[114]);
}

template <class S>
void
test21(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[981], 0, 5, *s_arr[19], 0, 5, *s_arr[78]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 0, 9, *s_arr[66]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 0, 10, *s_arr[54]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 0, 11, *s_arr[54]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 1, 0, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 1, 1, *s_arr[218]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 1, 4, *s_arr[182]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 1, 8, *s_arr[170]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 1, 9, *s_arr[158]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 1, 10, *s_arr[158]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 5, 0, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 5, 1, *s_arr[322]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 5, 2, *s_arr[310]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 5, 4, *s_arr[298]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 5, 5, *s_arr[286]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 5, 6, *s_arr[286]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 9, 0, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 9, 1, *s_arr[10]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 9, 2, *s_arr[10]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 10, 0, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 10, 1, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 0, 0, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 0, 1, *s_arr[114]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 0, 10, *s_arr[54]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 0, 19, *s_arr[42]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 0, 20, *s_arr[30]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 0, 21, *s_arr[30]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 1, 0, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 1, 1, *s_arr[218]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 1, 9, *s_arr[158]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 1, 18, *s_arr[146]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 1, 19, *s_arr[134]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 1, 20, *s_arr[134]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 10, 0, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 10, 1, *s_arr[114]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 10, 5, *s_arr[78]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 10, 9, *s_arr[66]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 10, 10, *s_arr[54]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 10, 11, *s_arr[54]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 19, 0, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 19, 1, *s_arr[10]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 19, 2, *s_arr[10]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 20, 0, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 20, 1, *s_arr[1207]);
	test(pop, *s_arr[981], 0, 5, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 9, *s_arr[0], 0, 0, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[0], 0, 1, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 0, 0, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 0, 1, *s_arr[115]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 0, 2, *s_arr[103]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 0, 4, *s_arr[91]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 0, 5, *s_arr[79]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 0, 6, *s_arr[79]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 1, 0, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 1, 1, *s_arr[219]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 1, 2, *s_arr[207]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 1, 3, *s_arr[195]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 1, 4, *s_arr[183]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 1, 5, *s_arr[183]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 2, 0, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 2, 1, *s_arr[258]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 2, 2, *s_arr[246]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 2, 3, *s_arr[234]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 2, 4, *s_arr[234]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 4, 0, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 4, 1, *s_arr[271]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 4, 2, *s_arr[271]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 5, 0, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 5, 1, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 0, 0, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 0, 1, *s_arr[115]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 0, 5, *s_arr[79]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 0, 9, *s_arr[67]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 0, 10, *s_arr[55]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 0, 11, *s_arr[55]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 1, 0, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 1, 1, *s_arr[219]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 1, 4, *s_arr[183]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 1, 8, *s_arr[171]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 1, 9, *s_arr[159]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 1, 10, *s_arr[159]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 5, 0, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 5, 1, *s_arr[323]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 5, 2, *s_arr[311]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 5, 4, *s_arr[299]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 5, 5, *s_arr[287]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 5, 6, *s_arr[287]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 9, 0, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 9, 1, *s_arr[11]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 9, 2, *s_arr[11]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 10, 0, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 10, 1, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 0, 0, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 0, 1, *s_arr[115]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 0, 10, *s_arr[55]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 0, 19, *s_arr[43]);
}

template <class S>
void
test22(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[981], 0, 9, *s_arr[21], 0, 20, *s_arr[31]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 0, 21, *s_arr[31]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 1, 0, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 1, 1, *s_arr[219]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 1, 9, *s_arr[159]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 1, 18, *s_arr[147]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 1, 19, *s_arr[135]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 1, 20, *s_arr[135]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 10, 0, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 10, 1, *s_arr[115]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 10, 5, *s_arr[79]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 10, 9, *s_arr[67]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 10, 10, *s_arr[55]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 10, 11, *s_arr[55]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 19, 0, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 19, 1, *s_arr[11]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 19, 2, *s_arr[11]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 20, 0, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 20, 1, *s_arr[1208]);
	test(pop, *s_arr[981], 0, 9, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 10, *s_arr[0], 0, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[0], 0, 1, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 0, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 0, 1, *s_arr[14]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 0, 2, *s_arr[15]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 0, 4, *s_arr[16]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 0, 5, *s_arr[17]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 0, 6, *s_arr[17]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 1, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 1, 1, *s_arr[118]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 1, 2, *s_arr[119]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 1, 3, *s_arr[120]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 1, 4, *s_arr[121]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 1, 5, *s_arr[121]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 2, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 2, 1, *s_arr[222]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 2, 2, *s_arr[223]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 2, 3, *s_arr[224]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 2, 4, *s_arr[224]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 4, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 4, 1, *s_arr[261]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 4, 2, *s_arr[261]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 5, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 5, 1, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 0, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 0, 1, *s_arr[14]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 0, 5, *s_arr[17]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 0, 9, *s_arr[18]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 0, 10, *s_arr[19]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 0, 11, *s_arr[19]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 1, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 1, 1, *s_arr[118]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 1, 4, *s_arr[121]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 1, 8, *s_arr[122]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 1, 9, *s_arr[123]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 1, 10, *s_arr[123]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 5, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 5, 1, *s_arr[274]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 5, 2, *s_arr[275]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 5, 4, *s_arr[276]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 5, 5, *s_arr[277]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 5, 6, *s_arr[277]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 9, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 9, 1, *s_arr[1]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 9, 2, *s_arr[1]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 10, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 10, 1, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 0, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 0, 1, *s_arr[14]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 0, 10, *s_arr[19]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 0, 19, *s_arr[20]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 0, 20, *s_arr[21]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 0, 21, *s_arr[21]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 1, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 1, 1, *s_arr[118]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 1, 9, *s_arr[123]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 1, 18, *s_arr[124]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 1, 19, *s_arr[125]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 1, 20, *s_arr[125]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 10, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 10, 1, *s_arr[14]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 10, 5, *s_arr[17]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 10, 9, *s_arr[18]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 10, 10, *s_arr[19]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 10, 11, *s_arr[19]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 19, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 19, 1, *s_arr[1]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 19, 2, *s_arr[1]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 20, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 20, 1, *s_arr[0]);
	test(pop, *s_arr[981], 0, 10, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 11, *s_arr[0], 0, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[0], 0, 1, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 0, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 0, 1, *s_arr[14]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 0, 2, *s_arr[15]);
}

template <class S>
void
test23(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[981], 0, 11, *s_arr[17], 0, 4, *s_arr[16]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 0, 5, *s_arr[17]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 0, 6, *s_arr[17]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 1, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 1, 1, *s_arr[118]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 1, 2, *s_arr[119]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 1, 3, *s_arr[120]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 1, 4, *s_arr[121]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 1, 5, *s_arr[121]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 2, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 2, 1, *s_arr[222]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 2, 2, *s_arr[223]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 2, 3, *s_arr[224]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 2, 4, *s_arr[224]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 4, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 4, 1, *s_arr[261]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 4, 2, *s_arr[261]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 5, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 5, 1, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 0, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 0, 1, *s_arr[14]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 0, 5, *s_arr[17]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 0, 9, *s_arr[18]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 0, 10, *s_arr[19]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 0, 11, *s_arr[19]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 1, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 1, 1, *s_arr[118]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 1, 4, *s_arr[121]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 1, 8, *s_arr[122]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 1, 9, *s_arr[123]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 1, 10, *s_arr[123]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 5, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 5, 1, *s_arr[274]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 5, 2, *s_arr[275]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 5, 4, *s_arr[276]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 5, 5, *s_arr[277]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 5, 6, *s_arr[277]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 9, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 9, 1, *s_arr[1]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 9, 2, *s_arr[1]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 10, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 10, 1, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 0, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 0, 1, *s_arr[14]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 0, 10, *s_arr[19]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 0, 19, *s_arr[20]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 0, 20, *s_arr[21]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 0, 21, *s_arr[21]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 1, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 1, 1, *s_arr[118]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 1, 9, *s_arr[123]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 1, 18, *s_arr[124]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 1, 19, *s_arr[125]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 1, 20, *s_arr[125]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 10, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 10, 1, *s_arr[14]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 10, 5, *s_arr[17]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 10, 9, *s_arr[18]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 10, 10, *s_arr[19]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 10, 11, *s_arr[19]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 19, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 19, 1, *s_arr[1]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 19, 2, *s_arr[1]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 20, 0, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 20, 1, *s_arr[0]);
	test(pop, *s_arr[981], 0, 11, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 0, *s_arr[0], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[0], 0, 1, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 0, 1, *s_arr[433]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 0, 2, *s_arr[421]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 0, 4, *s_arr[409]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 0, 5, *s_arr[397]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 0, 6, *s_arr[397]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 1, 0, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 1, 1, *s_arr[537]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 1, 2, *s_arr[525]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 1, 3, *s_arr[513]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 1, 4, *s_arr[501]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 1, 5, *s_arr[501]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 2, 0, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 2, 1, *s_arr[576]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 2, 2, *s_arr[564]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 2, 3, *s_arr[552]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 2, 4, *s_arr[552]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 4, 0, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 4, 1, *s_arr[589]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 4, 2, *s_arr[589]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 5, 0, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 5, 1, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 0, 1, *s_arr[433]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 0, 5, *s_arr[397]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 0, 9, *s_arr[385]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 0, 10, *s_arr[373]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 0, 11, *s_arr[373]);
}

template <class S>
void
test24(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[981], 1, 0, *s_arr[19], 1, 0, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 1, 1, *s_arr[537]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 1, 4, *s_arr[501]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 1, 8, *s_arr[489]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 1, 9, *s_arr[477]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 1, 10, *s_arr[477]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 5, 0, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 5, 1, *s_arr[641]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 5, 2, *s_arr[629]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 5, 4, *s_arr[617]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 5, 5, *s_arr[605]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 5, 6, *s_arr[605]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 9, 0, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 9, 1, *s_arr[329]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 9, 2, *s_arr[329]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 10, 0, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 10, 1, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 0, 1, *s_arr[433]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 0, 10, *s_arr[373]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 0, 19, *s_arr[361]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 0, 20, *s_arr[349]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 0, 21, *s_arr[349]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 1, 0, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 1, 1, *s_arr[537]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 1, 9, *s_arr[477]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 1, 18, *s_arr[465]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 1, 19, *s_arr[453]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 1, 20, *s_arr[453]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 10, 0, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 10, 1, *s_arr[433]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 10, 5, *s_arr[397]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 10, 9, *s_arr[385]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 10, 10, *s_arr[373]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 10, 11, *s_arr[373]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 19, 0, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 19, 1, *s_arr[329]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 19, 2, *s_arr[329]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 20, 0, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 20, 1, *s_arr[981]);
	test(pop, *s_arr[981], 1, 0, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 1, *s_arr[0], 0, 0, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[0], 0, 1, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 0, 0, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 0, 1, *s_arr[436]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 0, 2, *s_arr[424]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 0, 4, *s_arr[412]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 0, 5, *s_arr[400]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 0, 6, *s_arr[400]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 1, 0, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 1, 1, *s_arr[540]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 1, 2, *s_arr[528]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 1, 3, *s_arr[516]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 1, 4, *s_arr[504]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 1, 5, *s_arr[504]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 2, 0, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 2, 1, *s_arr[579]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 2, 2, *s_arr[567]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 2, 3, *s_arr[555]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 2, 4, *s_arr[555]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 4, 0, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 4, 1, *s_arr[592]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 4, 2, *s_arr[592]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 5, 0, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 5, 1, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 0, 0, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 0, 1, *s_arr[436]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 0, 5, *s_arr[400]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 0, 9, *s_arr[388]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 0, 10, *s_arr[376]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 0, 11, *s_arr[376]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 1, 0, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 1, 1, *s_arr[540]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 1, 4, *s_arr[504]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 1, 8, *s_arr[492]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 1, 9, *s_arr[480]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 1, 10, *s_arr[480]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 5, 0, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 5, 1, *s_arr[644]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 5, 2, *s_arr[632]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 5, 4, *s_arr[620]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 5, 5, *s_arr[608]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 5, 6, *s_arr[608]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 9, 0, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 9, 1, *s_arr[332]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 9, 2, *s_arr[332]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 10, 0, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 10, 1, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 0, 0, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 0, 1, *s_arr[436]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 0, 10, *s_arr[376]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 0, 19, *s_arr[364]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 0, 20, *s_arr[352]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 0, 21, *s_arr[352]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 1, 0, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 1, 1, *s_arr[540]);
}

template <class S>
void
test25(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[981], 1, 1, *s_arr[21], 1, 9, *s_arr[480]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 1, 18, *s_arr[468]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 1, 19, *s_arr[456]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 1, 20, *s_arr[456]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 10, 0, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 10, 1, *s_arr[436]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 10, 5, *s_arr[400]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 10, 9, *s_arr[388]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 10, 10, *s_arr[376]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 10, 11, *s_arr[376]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 19, 0, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 19, 1, *s_arr[332]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 19, 2, *s_arr[332]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 20, 0, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 20, 1, *s_arr[1193]);
	test(pop, *s_arr[981], 1, 1, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 4, *s_arr[0], 0, 0, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[0], 0, 1, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 0, 0, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 0, 1, *s_arr[440]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 0, 2, *s_arr[428]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 0, 4, *s_arr[416]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 0, 5, *s_arr[404]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 0, 6, *s_arr[404]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 1, 0, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 1, 1, *s_arr[544]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 1, 2, *s_arr[532]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 1, 3, *s_arr[520]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 1, 4, *s_arr[508]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 1, 5, *s_arr[508]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 2, 0, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 2, 1, *s_arr[583]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 2, 2, *s_arr[571]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 2, 3, *s_arr[559]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 2, 4, *s_arr[559]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 4, 0, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 4, 1, *s_arr[596]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 4, 2, *s_arr[596]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 5, 0, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 5, 1, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 0, 0, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 0, 1, *s_arr[440]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 0, 5, *s_arr[404]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 0, 9, *s_arr[392]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 0, 10, *s_arr[380]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 0, 11, *s_arr[380]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 1, 0, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 1, 1, *s_arr[544]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 1, 4, *s_arr[508]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 1, 8, *s_arr[496]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 1, 9, *s_arr[484]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 1, 10, *s_arr[484]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 5, 0, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 5, 1, *s_arr[648]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 5, 2, *s_arr[636]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 5, 4, *s_arr[624]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 5, 5, *s_arr[612]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 5, 6, *s_arr[612]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 9, 0, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 9, 1, *s_arr[336]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 9, 2, *s_arr[336]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 10, 0, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 10, 1, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 0, 0, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 0, 1, *s_arr[440]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 0, 10, *s_arr[380]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 0, 19, *s_arr[368]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 0, 20, *s_arr[356]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 0, 21, *s_arr[356]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 1, 0, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 1, 1, *s_arr[544]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 1, 9, *s_arr[484]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 1, 18, *s_arr[472]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 1, 19, *s_arr[460]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 1, 20, *s_arr[460]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 10, 0, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 10, 1, *s_arr[440]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 10, 5, *s_arr[404]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 10, 9, *s_arr[392]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 10, 10, *s_arr[380]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 10, 11, *s_arr[380]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 19, 0, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 19, 1, *s_arr[336]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 19, 2, *s_arr[336]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 20, 0, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 20, 1, *s_arr[1197]);
	test(pop, *s_arr[981], 1, 4, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 8, *s_arr[0], 0, 0, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[0], 0, 1, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 0, 0, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 0, 1, *s_arr[441]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 0, 2, *s_arr[429]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 0, 4, *s_arr[417]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 0, 5, *s_arr[405]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 0, 6, *s_arr[405]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 1, 0, *s_arr[1198]);
}

template <class S>
void
test26(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[981], 1, 8, *s_arr[17], 1, 1, *s_arr[545]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 1, 2, *s_arr[533]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 1, 3, *s_arr[521]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 1, 4, *s_arr[509]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 1, 5, *s_arr[509]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 2, 0, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 2, 1, *s_arr[584]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 2, 2, *s_arr[572]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 2, 3, *s_arr[560]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 2, 4, *s_arr[560]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 4, 0, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 4, 1, *s_arr[597]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 4, 2, *s_arr[597]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 5, 0, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 5, 1, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 0, 0, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 0, 1, *s_arr[441]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 0, 5, *s_arr[405]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 0, 9, *s_arr[393]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 0, 10, *s_arr[381]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 0, 11, *s_arr[381]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 1, 0, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 1, 1, *s_arr[545]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 1, 4, *s_arr[509]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 1, 8, *s_arr[497]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 1, 9, *s_arr[485]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 1, 10, *s_arr[485]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 5, 0, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 5, 1, *s_arr[649]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 5, 2, *s_arr[637]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 5, 4, *s_arr[625]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 5, 5, *s_arr[613]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 5, 6, *s_arr[613]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 9, 0, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 9, 1, *s_arr[337]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 9, 2, *s_arr[337]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 10, 0, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 10, 1, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 0, 0, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 0, 1, *s_arr[441]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 0, 10, *s_arr[381]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 0, 19, *s_arr[369]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 0, 20, *s_arr[357]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 0, 21, *s_arr[357]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 1, 0, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 1, 1, *s_arr[545]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 1, 9, *s_arr[485]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 1, 18, *s_arr[473]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 1, 19, *s_arr[461]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 1, 20, *s_arr[461]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 10, 0, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 10, 1, *s_arr[441]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 10, 5, *s_arr[405]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 10, 9, *s_arr[393]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 10, 10, *s_arr[381]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 10, 11, *s_arr[381]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 19, 0, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 19, 1, *s_arr[337]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 19, 2, *s_arr[337]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 20, 0, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 20, 1, *s_arr[1198]);
	test(pop, *s_arr[981], 1, 8, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 9, *s_arr[0], 0, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[0], 0, 1, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 0, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 0, 1, *s_arr[340]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 0, 2, *s_arr[341]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 0, 4, *s_arr[342]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 0, 5, *s_arr[343]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 0, 6, *s_arr[343]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 1, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 1, 1, *s_arr[444]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 1, 2, *s_arr[445]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 1, 3, *s_arr[446]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 1, 4, *s_arr[447]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 1, 5, *s_arr[447]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 2, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 2, 1, *s_arr[548]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 2, 2, *s_arr[549]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 2, 3, *s_arr[550]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 2, 4, *s_arr[550]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 4, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 4, 1, *s_arr[587]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 4, 2, *s_arr[587]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 5, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 5, 1, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 0, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 0, 1, *s_arr[340]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 0, 5, *s_arr[343]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 0, 9, *s_arr[344]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 0, 10, *s_arr[345]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 0, 11, *s_arr[345]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 1, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 1, 1, *s_arr[444]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 1, 4, *s_arr[447]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 1, 8, *s_arr[448]);
}

template <class S>
void
test27(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[981], 1, 9, *s_arr[19], 1, 9, *s_arr[449]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 1, 10, *s_arr[449]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 5, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 5, 1, *s_arr[600]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 5, 2, *s_arr[601]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 5, 4, *s_arr[602]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 5, 5, *s_arr[603]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 5, 6, *s_arr[603]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 9, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 9, 1, *s_arr[327]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 9, 2, *s_arr[327]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 10, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 10, 1, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 0, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 0, 1, *s_arr[340]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 0, 10, *s_arr[345]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 0, 19, *s_arr[346]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 0, 20, *s_arr[347]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 0, 21, *s_arr[347]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 1, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 1, 1, *s_arr[444]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 1, 9, *s_arr[449]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 1, 18, *s_arr[450]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 1, 19, *s_arr[451]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 1, 20, *s_arr[451]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 10, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 10, 1, *s_arr[340]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 10, 5, *s_arr[343]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 10, 9, *s_arr[344]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 10, 10, *s_arr[345]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 10, 11, *s_arr[345]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 19, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 19, 1, *s_arr[327]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 19, 2, *s_arr[327]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 20, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 20, 1, *s_arr[326]);
	test(pop, *s_arr[981], 1, 9, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 10, *s_arr[0], 0, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[0], 0, 1, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 0, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 0, 1, *s_arr[340]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 0, 2, *s_arr[341]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 0, 4, *s_arr[342]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 0, 5, *s_arr[343]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 0, 6, *s_arr[343]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 1, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 1, 1, *s_arr[444]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 1, 2, *s_arr[445]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 1, 3, *s_arr[446]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 1, 4, *s_arr[447]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 1, 5, *s_arr[447]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 2, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 2, 1, *s_arr[548]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 2, 2, *s_arr[549]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 2, 3, *s_arr[550]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 2, 4, *s_arr[550]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 4, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 4, 1, *s_arr[587]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 4, 2, *s_arr[587]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 5, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 5, 1, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 0, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 0, 1, *s_arr[340]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 0, 5, *s_arr[343]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 0, 9, *s_arr[344]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 0, 10, *s_arr[345]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 0, 11, *s_arr[345]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 1, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 1, 1, *s_arr[444]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 1, 4, *s_arr[447]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 1, 8, *s_arr[448]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 1, 9, *s_arr[449]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 1, 10, *s_arr[449]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 5, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 5, 1, *s_arr[600]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 5, 2, *s_arr[601]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 5, 4, *s_arr[602]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 5, 5, *s_arr[603]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 5, 6, *s_arr[603]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 9, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 9, 1, *s_arr[327]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 9, 2, *s_arr[327]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 10, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 10, 1, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 0, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 0, 1, *s_arr[340]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 0, 10, *s_arr[345]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 0, 19, *s_arr[346]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 0, 20, *s_arr[347]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 0, 21, *s_arr[347]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 1, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 1, 1, *s_arr[444]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 1, 9, *s_arr[449]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 1, 18, *s_arr[450]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 1, 19, *s_arr[451]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 1, 20, *s_arr[451]);
}

template <class S>
void
test28(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[981], 1, 10, *s_arr[21], 10, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 10, 1, *s_arr[340]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 10, 5, *s_arr[343]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 10, 9, *s_arr[344]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 10, 10, *s_arr[345]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 10, 11, *s_arr[345]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 19, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 19, 1, *s_arr[327]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 19, 2, *s_arr[327]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 20, 0, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 20, 1, *s_arr[326]);
	test(pop, *s_arr[981], 1, 10, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 0, *s_arr[0], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[0], 0, 1, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 0, 1, *s_arr[846]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 0, 2, *s_arr[842]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 0, 4, *s_arr[838]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 0, 5, *s_arr[834]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 0, 6, *s_arr[834]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 1, 0, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 1, 1, *s_arr[886]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 1, 2, *s_arr[882]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 1, 3, *s_arr[878]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 1, 4, *s_arr[874]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 1, 5, *s_arr[874]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 2, 0, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 2, 1, *s_arr[901]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 2, 2, *s_arr[897]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 2, 3, *s_arr[893]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 2, 4, *s_arr[893]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 4, 0, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 4, 1, *s_arr[906]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 4, 2, *s_arr[906]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 5, 0, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 5, 1, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 0, 1, *s_arr[846]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 0, 5, *s_arr[834]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 0, 9, *s_arr[830]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 0, 10, *s_arr[826]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 0, 11, *s_arr[826]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 1, 0, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 1, 1, *s_arr[886]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 1, 4, *s_arr[874]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 1, 8, *s_arr[870]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 1, 9, *s_arr[866]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 1, 10, *s_arr[866]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 5, 0, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 5, 1, *s_arr[926]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 5, 2, *s_arr[922]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 5, 4, *s_arr[918]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 5, 5, *s_arr[914]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 5, 6, *s_arr[914]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 9, 0, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 9, 1, *s_arr[806]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 9, 2, *s_arr[806]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 10, 0, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 10, 1, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 0, 1, *s_arr[846]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 0, 10, *s_arr[826]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 0, 19, *s_arr[822]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 0, 20, *s_arr[818]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 0, 21, *s_arr[818]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 1, 0, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 1, 1, *s_arr[886]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 1, 9, *s_arr[866]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 1, 18, *s_arr[862]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 1, 19, *s_arr[858]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 1, 20, *s_arr[858]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 10, 0, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 10, 1, *s_arr[846]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 10, 5, *s_arr[834]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 10, 9, *s_arr[830]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 10, 10, *s_arr[826]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 10, 11, *s_arr[826]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 19, 0, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 19, 1, *s_arr[806]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 19, 2, *s_arr[806]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 20, 0, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 20, 1, *s_arr[981]);
	test(pop, *s_arr[981], 5, 0, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 1, *s_arr[0], 0, 0, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[0], 0, 1, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 0, 0, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 0, 1, *s_arr[847]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 0, 2, *s_arr[843]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 0, 4, *s_arr[839]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 0, 5, *s_arr[835]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 0, 6, *s_arr[835]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 1, 0, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 1, 1, *s_arr[887]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 1, 2, *s_arr[883]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 1, 3, *s_arr[879]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 1, 4, *s_arr[875]);
}

template <class S>
void
test29(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[981], 5, 1, *s_arr[17], 1, 5, *s_arr[875]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 2, 0, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 2, 1, *s_arr[902]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 2, 2, *s_arr[898]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 2, 3, *s_arr[894]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 2, 4, *s_arr[894]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 4, 0, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 4, 1, *s_arr[907]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 4, 2, *s_arr[907]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 5, 0, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 5, 1, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 0, 0, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 0, 1, *s_arr[847]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 0, 5, *s_arr[835]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 0, 9, *s_arr[831]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 0, 10, *s_arr[827]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 0, 11, *s_arr[827]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 1, 0, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 1, 1, *s_arr[887]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 1, 4, *s_arr[875]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 1, 8, *s_arr[871]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 1, 9, *s_arr[867]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 1, 10, *s_arr[867]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 5, 0, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 5, 1, *s_arr[927]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 5, 2, *s_arr[923]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 5, 4, *s_arr[919]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 5, 5, *s_arr[915]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 5, 6, *s_arr[915]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 9, 0, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 9, 1, *s_arr[807]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 9, 2, *s_arr[807]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 10, 0, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 10, 1, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 0, 0, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 0, 1, *s_arr[847]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 0, 10, *s_arr[827]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 0, 19, *s_arr[823]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 0, 20, *s_arr[819]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 0, 21, *s_arr[819]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 1, 0, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 1, 1, *s_arr[887]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 1, 9, *s_arr[867]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 1, 18, *s_arr[863]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 1, 19, *s_arr[859]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 1, 20, *s_arr[859]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 10, 0, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 10, 1, *s_arr[847]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 10, 5, *s_arr[835]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 10, 9, *s_arr[831]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 10, 10, *s_arr[827]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 10, 11, *s_arr[827]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 19, 0, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 19, 1, *s_arr[807]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 19, 2, *s_arr[807]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 20, 0, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 20, 1, *s_arr[1187]);
	test(pop, *s_arr[981], 5, 1, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 2, *s_arr[0], 0, 0, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[0], 0, 1, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 0, 0, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 0, 1, *s_arr[848]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 0, 2, *s_arr[844]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 0, 4, *s_arr[840]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 0, 5, *s_arr[836]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 0, 6, *s_arr[836]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 1, 0, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 1, 1, *s_arr[888]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 1, 2, *s_arr[884]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 1, 3, *s_arr[880]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 1, 4, *s_arr[876]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 1, 5, *s_arr[876]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 2, 0, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 2, 1, *s_arr[903]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 2, 2, *s_arr[899]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 2, 3, *s_arr[895]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 2, 4, *s_arr[895]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 4, 0, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 4, 1, *s_arr[908]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 4, 2, *s_arr[908]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 5, 0, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 5, 1, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 0, 0, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 0, 1, *s_arr[848]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 0, 5, *s_arr[836]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 0, 9, *s_arr[832]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 0, 10, *s_arr[828]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 0, 11, *s_arr[828]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 1, 0, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 1, 1, *s_arr[888]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 1, 4, *s_arr[876]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 1, 8, *s_arr[872]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 1, 9, *s_arr[868]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 1, 10, *s_arr[868]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 5, 0, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 5, 1, *s_arr[928]);
}

template <class S>
void
test30(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[981], 5, 2, *s_arr[19], 5, 2, *s_arr[924]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 5, 4, *s_arr[920]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 5, 5, *s_arr[916]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 5, 6, *s_arr[916]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 9, 0, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 9, 1, *s_arr[808]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 9, 2, *s_arr[808]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 10, 0, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 10, 1, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 0, 0, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 0, 1, *s_arr[848]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 0, 10, *s_arr[828]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 0, 19, *s_arr[824]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 0, 20, *s_arr[820]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 0, 21, *s_arr[820]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 1, 0, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 1, 1, *s_arr[888]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 1, 9, *s_arr[868]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 1, 18, *s_arr[864]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 1, 19, *s_arr[860]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 1, 20, *s_arr[860]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 10, 0, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 10, 1, *s_arr[848]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 10, 5, *s_arr[836]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 10, 9, *s_arr[832]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 10, 10, *s_arr[828]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 10, 11, *s_arr[828]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 19, 0, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 19, 1, *s_arr[808]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 19, 2, *s_arr[808]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 20, 0, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 20, 1, *s_arr[1188]);
	test(pop, *s_arr[981], 5, 2, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 4, *s_arr[0], 0, 0, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[0], 0, 1, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 0, 0, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 0, 1, *s_arr[849]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 0, 2, *s_arr[845]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 0, 4, *s_arr[841]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 0, 5, *s_arr[837]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 0, 6, *s_arr[837]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 1, 0, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 1, 1, *s_arr[889]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 1, 2, *s_arr[885]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 1, 3, *s_arr[881]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 1, 4, *s_arr[877]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 1, 5, *s_arr[877]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 2, 0, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 2, 1, *s_arr[904]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 2, 2, *s_arr[900]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 2, 3, *s_arr[896]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 2, 4, *s_arr[896]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 4, 0, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 4, 1, *s_arr[909]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 4, 2, *s_arr[909]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 5, 0, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 5, 1, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 0, 0, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 0, 1, *s_arr[849]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 0, 5, *s_arr[837]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 0, 9, *s_arr[833]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 0, 10, *s_arr[829]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 0, 11, *s_arr[829]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 1, 0, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 1, 1, *s_arr[889]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 1, 4, *s_arr[877]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 1, 8, *s_arr[873]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 1, 9, *s_arr[869]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 1, 10, *s_arr[869]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 5, 0, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 5, 1, *s_arr[929]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 5, 2, *s_arr[925]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 5, 4, *s_arr[921]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 5, 5, *s_arr[917]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 5, 6, *s_arr[917]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 9, 0, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 9, 1, *s_arr[809]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 9, 2, *s_arr[809]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 10, 0, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 10, 1, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 0, 0, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 0, 1, *s_arr[849]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 0, 10, *s_arr[829]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 0, 19, *s_arr[825]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 0, 20, *s_arr[821]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 0, 21, *s_arr[821]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 1, 0, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 1, 1, *s_arr[889]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 1, 9, *s_arr[869]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 1, 18, *s_arr[865]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 1, 19, *s_arr[861]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 1, 20, *s_arr[861]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 10, 0, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 10, 1, *s_arr[849]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 10, 5, *s_arr[837]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 10, 9, *s_arr[833]);
}

template <class S>
void
test31(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[981], 5, 4, *s_arr[21], 10, 10, *s_arr[829]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 10, 11, *s_arr[829]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 19, 0, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 19, 1, *s_arr[809]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 19, 2, *s_arr[809]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 20, 0, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 20, 1, *s_arr[1189]);
	test(pop, *s_arr[981], 5, 4, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 5, *s_arr[0], 0, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[0], 0, 1, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 0, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 0, 1, *s_arr[810]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 0, 2, *s_arr[811]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 0, 4, *s_arr[812]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 0, 5, *s_arr[813]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 0, 6, *s_arr[813]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 1, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 1, 1, *s_arr[850]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 1, 2, *s_arr[851]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 1, 3, *s_arr[852]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 1, 4, *s_arr[853]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 1, 5, *s_arr[853]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 2, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 2, 1, *s_arr[890]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 2, 2, *s_arr[891]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 2, 3, *s_arr[892]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 2, 4, *s_arr[892]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 4, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 4, 1, *s_arr[905]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 4, 2, *s_arr[905]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 5, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 5, 1, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 0, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 0, 1, *s_arr[810]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 0, 5, *s_arr[813]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 0, 9, *s_arr[814]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 0, 10, *s_arr[815]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 0, 11, *s_arr[815]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 1, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 1, 1, *s_arr[850]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 1, 4, *s_arr[853]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 1, 8, *s_arr[854]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 1, 9, *s_arr[855]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 1, 10, *s_arr[855]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 5, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 5, 1, *s_arr[910]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 5, 2, *s_arr[911]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 5, 4, *s_arr[912]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 5, 5, *s_arr[913]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 5, 6, *s_arr[913]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 9, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 9, 1, *s_arr[805]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 9, 2, *s_arr[805]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 10, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 10, 1, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 0, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 0, 1, *s_arr[810]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 0, 10, *s_arr[815]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 0, 19, *s_arr[816]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 0, 20, *s_arr[817]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 0, 21, *s_arr[817]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 1, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 1, 1, *s_arr[850]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 1, 9, *s_arr[855]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 1, 18, *s_arr[856]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 1, 19, *s_arr[857]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 1, 20, *s_arr[857]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 10, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 10, 1, *s_arr[810]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 10, 5, *s_arr[813]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 10, 9, *s_arr[814]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 10, 10, *s_arr[815]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 10, 11, *s_arr[815]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 19, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 19, 1, *s_arr[805]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 19, 2, *s_arr[805]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 20, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 20, 1, *s_arr[804]);
	test(pop, *s_arr[981], 5, 5, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 6, *s_arr[0], 0, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[0], 0, 1, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 0, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 0, 1, *s_arr[810]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 0, 2, *s_arr[811]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 0, 4, *s_arr[812]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 0, 5, *s_arr[813]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 0, 6, *s_arr[813]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 1, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 1, 1, *s_arr[850]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 1, 2, *s_arr[851]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 1, 3, *s_arr[852]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 1, 4, *s_arr[853]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 1, 5, *s_arr[853]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 2, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 2, 1, *s_arr[890]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 2, 2, *s_arr[891]);
}

template <class S>
void
test32(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[981], 5, 6, *s_arr[17], 2, 3, *s_arr[892]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 2, 4, *s_arr[892]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 4, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 4, 1, *s_arr[905]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 4, 2, *s_arr[905]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 5, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 5, 1, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 0, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 0, 1, *s_arr[810]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 0, 5, *s_arr[813]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 0, 9, *s_arr[814]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 0, 10, *s_arr[815]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 0, 11, *s_arr[815]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 1, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 1, 1, *s_arr[850]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 1, 4, *s_arr[853]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 1, 8, *s_arr[854]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 1, 9, *s_arr[855]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 1, 10, *s_arr[855]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 5, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 5, 1, *s_arr[910]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 5, 2, *s_arr[911]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 5, 4, *s_arr[912]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 5, 5, *s_arr[913]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 5, 6, *s_arr[913]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 9, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 9, 1, *s_arr[805]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 9, 2, *s_arr[805]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 10, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 10, 1, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 0, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 0, 1, *s_arr[810]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 0, 10, *s_arr[815]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 0, 19, *s_arr[816]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 0, 20, *s_arr[817]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 0, 21, *s_arr[817]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 1, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 1, 1, *s_arr[850]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 1, 9, *s_arr[855]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 1, 18, *s_arr[856]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 1, 19, *s_arr[857]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 1, 20, *s_arr[857]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 10, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 10, 1, *s_arr[810]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 10, 5, *s_arr[813]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 10, 9, *s_arr[814]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 10, 10, *s_arr[815]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 10, 11, *s_arr[815]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 19, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 19, 1, *s_arr[805]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 19, 2, *s_arr[805]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 20, 0, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 20, 1, *s_arr[804]);
	test(pop, *s_arr[981], 5, 6, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 9, 0, *s_arr[0], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[0], 0, 1, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 0, 1, *s_arr[948]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 0, 2, *s_arr[947]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 0, 4, *s_arr[946]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 0, 5, *s_arr[945]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 0, 6, *s_arr[945]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 1, 0, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 1, 1, *s_arr[964]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 1, 2, *s_arr[963]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 1, 3, *s_arr[962]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 1, 4, *s_arr[961]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 1, 5, *s_arr[961]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 2, 0, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 2, 1, *s_arr[970]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 2, 2, *s_arr[969]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 2, 3, *s_arr[968]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 2, 4, *s_arr[968]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 4, 0, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 4, 1, *s_arr[972]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 4, 2, *s_arr[972]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 5, 0, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 5, 1, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 0, 1, *s_arr[948]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 0, 5, *s_arr[945]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 0, 9, *s_arr[944]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 0, 10, *s_arr[943]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 0, 11, *s_arr[943]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 1, 0, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 1, 1, *s_arr[964]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 1, 4, *s_arr[961]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 1, 8, *s_arr[960]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 1, 9, *s_arr[959]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 1, 10, *s_arr[959]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 5, 0, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 5, 1, *s_arr[980]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 5, 2, *s_arr[979]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 5, 4, *s_arr[978]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 5, 5, *s_arr[977]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 5, 6, *s_arr[977]);
}

template <class S>
void
test33(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[981], 9, 0, *s_arr[19], 9, 0, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 9, 1, *s_arr[932]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 9, 2, *s_arr[932]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 10, 0, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 10, 1, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 0, 1, *s_arr[948]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 0, 10, *s_arr[943]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 0, 19, *s_arr[942]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 0, 20, *s_arr[941]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 0, 21, *s_arr[941]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 1, 0, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 1, 1, *s_arr[964]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 1, 9, *s_arr[959]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 1, 18, *s_arr[958]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 1, 19, *s_arr[957]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 1, 20, *s_arr[957]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 10, 0, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 10, 1, *s_arr[948]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 10, 5, *s_arr[945]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 10, 9, *s_arr[944]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 10, 10, *s_arr[943]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 10, 11, *s_arr[943]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 19, 0, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 19, 1, *s_arr[932]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 19, 2, *s_arr[932]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 20, 0, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 20, 1, *s_arr[981]);
	test(pop, *s_arr[981], 9, 0, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 9, 1, *s_arr[0], 0, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[0], 0, 1, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 0, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 0, 1, *s_arr[933]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 0, 2, *s_arr[934]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 0, 4, *s_arr[935]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 0, 5, *s_arr[936]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 0, 6, *s_arr[936]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 1, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 1, 1, *s_arr[949]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 1, 2, *s_arr[950]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 1, 3, *s_arr[951]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 1, 4, *s_arr[952]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 1, 5, *s_arr[952]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 2, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 2, 1, *s_arr[965]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 2, 2, *s_arr[966]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 2, 3, *s_arr[967]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 2, 4, *s_arr[967]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 4, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 4, 1, *s_arr[971]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 4, 2, *s_arr[971]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 5, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 5, 1, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 0, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 0, 1, *s_arr[933]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 0, 5, *s_arr[936]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 0, 9, *s_arr[937]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 0, 10, *s_arr[938]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 0, 11, *s_arr[938]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 1, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 1, 1, *s_arr[949]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 1, 4, *s_arr[952]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 1, 8, *s_arr[953]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 1, 9, *s_arr[954]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 1, 10, *s_arr[954]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 5, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 5, 1, *s_arr[973]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 5, 2, *s_arr[974]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 5, 4, *s_arr[975]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 5, 5, *s_arr[976]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 5, 6, *s_arr[976]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 9, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 9, 1, *s_arr[931]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 9, 2, *s_arr[931]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 10, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 10, 1, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 0, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 0, 1, *s_arr[933]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 0, 10, *s_arr[938]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 0, 19, *s_arr[939]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 0, 20, *s_arr[940]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 0, 21, *s_arr[940]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 1, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 1, 1, *s_arr[949]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 1, 9, *s_arr[954]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 1, 18, *s_arr[955]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 1, 19, *s_arr[956]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 1, 20, *s_arr[956]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 10, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 10, 1, *s_arr[933]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 10, 5, *s_arr[936]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 10, 9, *s_arr[937]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 10, 10, *s_arr[938]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 10, 11, *s_arr[938]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 19, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 19, 1, *s_arr[931]);
}

template <class S>
void
test34(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[981], 9, 1, *s_arr[21], 19, 2, *s_arr[931]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 20, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 20, 1, *s_arr[930]);
	test(pop, *s_arr[981], 9, 1, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 9, 2, *s_arr[0], 0, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[0], 0, 1, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 0, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 0, 1, *s_arr[933]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 0, 2, *s_arr[934]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 0, 4, *s_arr[935]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 0, 5, *s_arr[936]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 0, 6, *s_arr[936]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 1, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 1, 1, *s_arr[949]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 1, 2, *s_arr[950]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 1, 3, *s_arr[951]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 1, 4, *s_arr[952]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 1, 5, *s_arr[952]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 2, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 2, 1, *s_arr[965]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 2, 2, *s_arr[966]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 2, 3, *s_arr[967]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 2, 4, *s_arr[967]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 4, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 4, 1, *s_arr[971]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 4, 2, *s_arr[971]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 5, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 5, 1, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 0, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 0, 1, *s_arr[933]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 0, 5, *s_arr[936]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 0, 9, *s_arr[937]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 0, 10, *s_arr[938]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 0, 11, *s_arr[938]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 1, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 1, 1, *s_arr[949]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 1, 4, *s_arr[952]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 1, 8, *s_arr[953]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 1, 9, *s_arr[954]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 1, 10, *s_arr[954]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 5, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 5, 1, *s_arr[973]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 5, 2, *s_arr[974]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 5, 4, *s_arr[975]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 5, 5, *s_arr[976]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 5, 6, *s_arr[976]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 9, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 9, 1, *s_arr[931]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 9, 2, *s_arr[931]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 10, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 10, 1, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 0, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 0, 1, *s_arr[933]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 0, 10, *s_arr[938]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 0, 19, *s_arr[939]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 0, 20, *s_arr[940]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 0, 21, *s_arr[940]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 1, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 1, 1, *s_arr[949]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 1, 9, *s_arr[954]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 1, 18, *s_arr[955]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 1, 19, *s_arr[956]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 1, 20, *s_arr[956]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 10, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 10, 1, *s_arr[933]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 10, 5, *s_arr[936]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 10, 9, *s_arr[937]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 10, 10, *s_arr[938]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 10, 11, *s_arr[938]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 19, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 19, 1, *s_arr[931]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 19, 2, *s_arr[931]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 20, 0, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 20, 1, *s_arr[930]);
	test(pop, *s_arr[981], 9, 2, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 10, 0, *s_arr[0], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[0], 0, 1, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 0, 1, *s_arr[987]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 0, 2, *s_arr[988]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 0, 4, *s_arr[989]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 0, 5, *s_arr[990]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 0, 6, *s_arr[990]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 1, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 1, 1, *s_arr[1027]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 1, 2, *s_arr[1028]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 1, 3, *s_arr[1029]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 1, 4, *s_arr[1030]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 1, 5, *s_arr[1030]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 2, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 2, 1, *s_arr[1067]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 2, 2, *s_arr[1068]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 2, 3, *s_arr[1069]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 2, 4, *s_arr[1069]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 4, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 4, 1, *s_arr[1082]);
}

template <class S>
void
test35(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[981], 10, 0, *s_arr[17], 4, 2, *s_arr[1082]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 5, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 5, 1, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 0, 1, *s_arr[987]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 0, 5, *s_arr[990]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 0, 9, *s_arr[991]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 0, 10, *s_arr[992]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 0, 11, *s_arr[992]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 1, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 1, 1, *s_arr[1027]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 1, 4, *s_arr[1030]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 1, 8, *s_arr[1031]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 1, 9, *s_arr[1032]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 1, 10, *s_arr[1032]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 5, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 5, 1, *s_arr[1087]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 5, 2, *s_arr[1088]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 5, 4, *s_arr[1089]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 5, 5, *s_arr[1090]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 5, 6, *s_arr[1090]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 9, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 9, 1, *s_arr[982]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 9, 2, *s_arr[982]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 10, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 10, 1, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 0, 1, *s_arr[987]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 0, 10, *s_arr[992]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 0, 19, *s_arr[993]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 0, 20, *s_arr[994]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 0, 21, *s_arr[994]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 1, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 1, 1, *s_arr[1027]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 1, 9, *s_arr[1032]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 1, 18, *s_arr[1033]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 1, 19, *s_arr[1034]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 1, 20, *s_arr[1034]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 10, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 10, 1, *s_arr[987]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 10, 5, *s_arr[990]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 10, 9, *s_arr[991]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 10, 10, *s_arr[992]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 10, 11, *s_arr[992]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 19, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 19, 1, *s_arr[982]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 19, 2, *s_arr[982]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 20, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 20, 1, *s_arr[981]);
	test(pop, *s_arr[981], 10, 0, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 10, 1, *s_arr[0], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[0], 0, 1, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 0, 1, *s_arr[987]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 0, 2, *s_arr[988]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 0, 4, *s_arr[989]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 0, 5, *s_arr[990]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 0, 6, *s_arr[990]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 1, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 1, 1, *s_arr[1027]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 1, 2, *s_arr[1028]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 1, 3, *s_arr[1029]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 1, 4, *s_arr[1030]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 1, 5, *s_arr[1030]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 2, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 2, 1, *s_arr[1067]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 2, 2, *s_arr[1068]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 2, 3, *s_arr[1069]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 2, 4, *s_arr[1069]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 4, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 4, 1, *s_arr[1082]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 4, 2, *s_arr[1082]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 5, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 5, 1, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 0, 1, *s_arr[987]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 0, 5, *s_arr[990]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 0, 9, *s_arr[991]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 0, 10, *s_arr[992]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 0, 11, *s_arr[992]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 1, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 1, 1, *s_arr[1027]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 1, 4, *s_arr[1030]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 1, 8, *s_arr[1031]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 1, 9, *s_arr[1032]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 1, 10, *s_arr[1032]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 5, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 5, 1, *s_arr[1087]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 5, 2, *s_arr[1088]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 5, 4, *s_arr[1089]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 5, 5, *s_arr[1090]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 5, 6, *s_arr[1090]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 9, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 9, 1, *s_arr[982]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 9, 2, *s_arr[982]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 10, 0, *s_arr[981]);
}

template <class S>
void
test36(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[981], 10, 1, *s_arr[19], 10, 1, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 0, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 0, 1, *s_arr[987]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 0, 10, *s_arr[992]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 0, 19, *s_arr[993]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 0, 20, *s_arr[994]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 0, 21, *s_arr[994]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 1, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 1, 1, *s_arr[1027]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 1, 9, *s_arr[1032]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 1, 18, *s_arr[1033]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 1, 19, *s_arr[1034]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 1, 20, *s_arr[1034]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 10, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 10, 1, *s_arr[987]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 10, 5, *s_arr[990]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 10, 9, *s_arr[991]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 10, 10, *s_arr[992]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 10, 11, *s_arr[992]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 19, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 19, 1, *s_arr[982]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 19, 2, *s_arr[982]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 20, 0, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 20, 1, *s_arr[981]);
	test(pop, *s_arr[981], 10, 1, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[0], 0, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[0], 0, 1, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 0, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 0, 1, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 0, 2, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 0, 4, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 0, 5, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 0, 6, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 1, 1, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 1, 2, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 1, 3, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 1, 4, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 1, 5, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 2, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 2, 1, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 2, 2, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 2, 3, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 2, 4, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 4, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 4, 1, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 4, 2, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 5, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 5, 1, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 0, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 0, 1, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 0, 5, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 0, 9, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 0, 10, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 0, 11, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 1, 1, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 1, 4, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 1, 8, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 1, 9, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 1, 10, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 5, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 5, 1, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 5, 2, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 5, 4, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 5, 5, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 5, 6, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 9, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 9, 1, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 9, 2, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 10, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 10, 1, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 0, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 0, 1, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 0, 10, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 0, 19, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 0, 20, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 0, 21, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 1, 1, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 1, 9, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 1, 18, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 1, 19, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 1, 20, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 10, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 10, 1, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 10, 5, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 10, 9, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 10, 10, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 10, 11, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 19, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 19, 1, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 19, 2, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 20, 0, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 20, 1, *s_arr[1204]);
	test(pop, *s_arr[981], 11, 0, *s_arr[21], 21, 0, *s_arr[1204]);
}

template <class S>
void
test37(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[1158], 0, 0, *s_arr[0], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[0], 0, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 0, 1, *s_arr[108]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 0, 2, *s_arr[96]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 0, 4, *s_arr[84]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 0, 5, *s_arr[72]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 0, 6, *s_arr[72]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 1, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 1, 1, *s_arr[212]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 1, 2, *s_arr[200]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 1, 3, *s_arr[188]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 1, 4, *s_arr[176]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 1, 5, *s_arr[176]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 2, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 2, 1, *s_arr[251]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 2, 2, *s_arr[239]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 2, 3, *s_arr[227]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 2, 4, *s_arr[227]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 4, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 4, 1, *s_arr[264]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 4, 2, *s_arr[264]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 5, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 5, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 0, 1, *s_arr[108]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 0, 5, *s_arr[72]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 0, 9, *s_arr[60]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 0, 10, *s_arr[48]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 0, 11, *s_arr[48]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 1, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 1, 1, *s_arr[212]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 1, 4, *s_arr[176]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 1, 8, *s_arr[164]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 1, 9, *s_arr[152]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 1, 10, *s_arr[152]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 5, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 5, 1, *s_arr[316]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 5, 2, *s_arr[304]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 5, 4, *s_arr[292]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 5, 5, *s_arr[280]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 5, 6, *s_arr[280]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 9, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 9, 1, *s_arr[4]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 9, 2, *s_arr[4]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 10, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 10, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 0, 1, *s_arr[108]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 0, 10, *s_arr[48]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 0, 19, *s_arr[36]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 0, 20, *s_arr[24]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 0, 21, *s_arr[24]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 1, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 1, 1, *s_arr[212]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 1, 9, *s_arr[152]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 1, 18, *s_arr[140]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 1, 19, *s_arr[128]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 1, 20, *s_arr[128]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 10, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 10, 1, *s_arr[108]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 10, 5, *s_arr[72]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 10, 9, *s_arr[60]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 10, 10, *s_arr[48]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 10, 11, *s_arr[48]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 19, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 19, 1, *s_arr[4]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 19, 2, *s_arr[4]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 20, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 20, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 0, 0, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[0], 0, 0, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[0], 0, 1, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 0, 0, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 0, 1, *s_arr[111]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 0, 2, *s_arr[99]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 0, 4, *s_arr[87]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 0, 5, *s_arr[75]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 0, 6, *s_arr[75]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 1, 0, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 1, 1, *s_arr[215]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 1, 2, *s_arr[203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 1, 3, *s_arr[191]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 1, 4, *s_arr[179]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 1, 5, *s_arr[179]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 2, 0, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 2, 1, *s_arr[254]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 2, 2, *s_arr[242]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 2, 3, *s_arr[230]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 2, 4, *s_arr[230]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 4, 0, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 4, 1, *s_arr[267]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 4, 2, *s_arr[267]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 5, 0, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 5, 1, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[17], 6, 0, *s_arr[1204]);
}

template <class S>
void
test38(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 0, 0, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 0, 1, *s_arr[111]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 0, 5, *s_arr[75]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 0, 9, *s_arr[63]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 0, 10, *s_arr[51]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 0, 11, *s_arr[51]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 1, 0, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 1, 1, *s_arr[215]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 1, 4, *s_arr[179]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 1, 8, *s_arr[167]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 1, 9, *s_arr[155]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 1, 10, *s_arr[155]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 5, 0, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 5, 1, *s_arr[319]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 5, 2, *s_arr[307]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 5, 4, *s_arr[295]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 5, 5, *s_arr[283]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 5, 6, *s_arr[283]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 9, 0, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 9, 1, *s_arr[7]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 9, 2, *s_arr[7]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 10, 0, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 10, 1, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 0, 0, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 0, 1, *s_arr[111]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 0, 10, *s_arr[51]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 0, 19, *s_arr[39]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 0, 20, *s_arr[27]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 0, 21, *s_arr[27]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 1, 0, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 1, 1, *s_arr[215]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 1, 9, *s_arr[155]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 1, 18, *s_arr[143]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 1, 19, *s_arr[131]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 1, 20, *s_arr[131]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 10, 0, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 10, 1, *s_arr[111]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 10, 5, *s_arr[75]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 10, 9, *s_arr[63]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 10, 10, *s_arr[51]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 10, 11, *s_arr[51]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 19, 0, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 19, 1, *s_arr[7]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 19, 2, *s_arr[7]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 20, 0, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 20, 1, *s_arr[1203]);
	test(pop, *s_arr[1158], 0, 1, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[0], 0, 0, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[0], 0, 1, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 0, 0, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 0, 1, *s_arr[116]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 0, 2, *s_arr[104]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 0, 4, *s_arr[92]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 0, 5, *s_arr[80]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 0, 6, *s_arr[80]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 1, 0, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 1, 1, *s_arr[220]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 1, 2, *s_arr[208]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 1, 3, *s_arr[196]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 1, 4, *s_arr[184]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 1, 5, *s_arr[184]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 2, 0, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 2, 1, *s_arr[259]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 2, 2, *s_arr[247]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 2, 3, *s_arr[235]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 2, 4, *s_arr[235]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 4, 0, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 4, 1, *s_arr[272]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 4, 2, *s_arr[272]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 5, 0, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 5, 1, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 0, 0, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 0, 1, *s_arr[116]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 0, 5, *s_arr[80]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 0, 9, *s_arr[68]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 0, 10, *s_arr[56]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 0, 11, *s_arr[56]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 1, 0, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 1, 1, *s_arr[220]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 1, 4, *s_arr[184]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 1, 8, *s_arr[172]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 1, 9, *s_arr[160]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 1, 10, *s_arr[160]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 5, 0, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 5, 1, *s_arr[324]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 5, 2, *s_arr[312]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 5, 4, *s_arr[300]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 5, 5, *s_arr[288]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 5, 6, *s_arr[288]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 9, 0, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 9, 1, *s_arr[12]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 9, 2, *s_arr[12]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 10, 0, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 10, 1, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 0, 0, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 0, 1, *s_arr[116]);
}

template <class S>
void
test39(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 0, 10, *s_arr[56]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 0, 19, *s_arr[44]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 0, 20, *s_arr[32]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 0, 21, *s_arr[32]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 1, 0, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 1, 1, *s_arr[220]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 1, 9, *s_arr[160]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 1, 18, *s_arr[148]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 1, 19, *s_arr[136]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 1, 20, *s_arr[136]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 10, 0, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 10, 1, *s_arr[116]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 10, 5, *s_arr[80]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 10, 9, *s_arr[68]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 10, 10, *s_arr[56]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 10, 11, *s_arr[56]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 19, 0, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 19, 1, *s_arr[12]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 19, 2, *s_arr[12]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 20, 0, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 20, 1, *s_arr[1209]);
	test(pop, *s_arr[1158], 0, 10, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[0], 0, 0, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[0], 0, 1, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 0, 0, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 0, 1, *s_arr[117]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 0, 2, *s_arr[105]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 0, 4, *s_arr[93]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 0, 5, *s_arr[81]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 0, 6, *s_arr[81]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 1, 0, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 1, 1, *s_arr[221]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 1, 2, *s_arr[209]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 1, 3, *s_arr[197]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 1, 4, *s_arr[185]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 1, 5, *s_arr[185]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 2, 0, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 2, 1, *s_arr[260]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 2, 2, *s_arr[248]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 2, 3, *s_arr[236]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 2, 4, *s_arr[236]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 4, 0, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 4, 1, *s_arr[273]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 4, 2, *s_arr[273]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 5, 0, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 5, 1, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 0, 0, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 0, 1, *s_arr[117]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 0, 5, *s_arr[81]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 0, 9, *s_arr[69]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 0, 10, *s_arr[57]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 0, 11, *s_arr[57]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 1, 0, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 1, 1, *s_arr[221]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 1, 4, *s_arr[185]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 1, 8, *s_arr[173]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 1, 9, *s_arr[161]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 1, 10, *s_arr[161]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 5, 0, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 5, 1, *s_arr[325]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 5, 2, *s_arr[313]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 5, 4, *s_arr[301]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 5, 5, *s_arr[289]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 5, 6, *s_arr[289]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 9, 0, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 9, 1, *s_arr[13]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 9, 2, *s_arr[13]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 10, 0, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 10, 1, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 0, 0, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 0, 1, *s_arr[117]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 0, 10, *s_arr[57]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 0, 19, *s_arr[45]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 0, 20, *s_arr[33]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 0, 21, *s_arr[33]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 1, 0, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 1, 1, *s_arr[221]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 1, 9, *s_arr[161]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 1, 18, *s_arr[149]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 1, 19, *s_arr[137]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 1, 20, *s_arr[137]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 10, 0, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 10, 1, *s_arr[117]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 10, 5, *s_arr[81]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 10, 9, *s_arr[69]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 10, 10, *s_arr[57]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 10, 11, *s_arr[57]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 19, 0, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 19, 1, *s_arr[13]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 19, 2, *s_arr[13]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 20, 0, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 20, 1, *s_arr[1210]);
	test(pop, *s_arr[1158], 0, 19, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[0], 0, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[0], 0, 1, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 0, 0, *s_arr[0]);
}

template <class S>
void
test40(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 0, 1, *s_arr[14]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 0, 2, *s_arr[15]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 0, 4, *s_arr[16]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 0, 5, *s_arr[17]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 0, 6, *s_arr[17]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 1, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 1, 1, *s_arr[118]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 1, 2, *s_arr[119]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 1, 3, *s_arr[120]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 1, 4, *s_arr[121]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 1, 5, *s_arr[121]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 2, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 2, 1, *s_arr[222]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 2, 2, *s_arr[223]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 2, 3, *s_arr[224]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 2, 4, *s_arr[224]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 4, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 4, 1, *s_arr[261]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 4, 2, *s_arr[261]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 5, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 5, 1, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 0, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 0, 1, *s_arr[14]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 0, 5, *s_arr[17]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 0, 9, *s_arr[18]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 0, 10, *s_arr[19]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 0, 11, *s_arr[19]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 1, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 1, 1, *s_arr[118]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 1, 4, *s_arr[121]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 1, 8, *s_arr[122]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 1, 9, *s_arr[123]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 1, 10, *s_arr[123]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 5, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 5, 1, *s_arr[274]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 5, 2, *s_arr[275]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 5, 4, *s_arr[276]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 5, 5, *s_arr[277]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 5, 6, *s_arr[277]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 9, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 9, 1, *s_arr[1]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 9, 2, *s_arr[1]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 10, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 10, 1, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 0, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 0, 1, *s_arr[14]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 0, 10, *s_arr[19]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 0, 19, *s_arr[20]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 0, 20, *s_arr[21]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 0, 21, *s_arr[21]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 1, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 1, 1, *s_arr[118]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 1, 9, *s_arr[123]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 1, 18, *s_arr[124]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 1, 19, *s_arr[125]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 1, 20, *s_arr[125]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 10, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 10, 1, *s_arr[14]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 10, 5, *s_arr[17]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 10, 9, *s_arr[18]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 10, 10, *s_arr[19]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 10, 11, *s_arr[19]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 19, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 19, 1, *s_arr[1]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 19, 2, *s_arr[1]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 20, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 20, 1, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 20, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[0], 0, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[0], 0, 1, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 0, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 0, 1, *s_arr[14]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 0, 2, *s_arr[15]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 0, 4, *s_arr[16]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 0, 5, *s_arr[17]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 0, 6, *s_arr[17]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 1, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 1, 1, *s_arr[118]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 1, 2, *s_arr[119]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 1, 3, *s_arr[120]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 1, 4, *s_arr[121]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 1, 5, *s_arr[121]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 2, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 2, 1, *s_arr[222]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 2, 2, *s_arr[223]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 2, 3, *s_arr[224]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 2, 4, *s_arr[224]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 4, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 4, 1, *s_arr[261]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 4, 2, *s_arr[261]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 5, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 5, 1, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 0, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 0, 1, *s_arr[14]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 0, 5, *s_arr[17]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 0, 9, *s_arr[18]);
}

template <class S>
void
test41(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 0, 10, *s_arr[19]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 0, 11, *s_arr[19]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 1, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 1, 1, *s_arr[118]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 1, 4, *s_arr[121]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 1, 8, *s_arr[122]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 1, 9, *s_arr[123]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 1, 10, *s_arr[123]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 5, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 5, 1, *s_arr[274]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 5, 2, *s_arr[275]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 5, 4, *s_arr[276]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 5, 5, *s_arr[277]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 5, 6, *s_arr[277]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 9, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 9, 1, *s_arr[1]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 9, 2, *s_arr[1]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 10, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 10, 1, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 0, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 0, 1, *s_arr[14]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 0, 10, *s_arr[19]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 0, 19, *s_arr[20]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 0, 20, *s_arr[21]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 0, 21, *s_arr[21]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 1, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 1, 1, *s_arr[118]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 1, 9, *s_arr[123]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 1, 18, *s_arr[124]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 1, 19, *s_arr[125]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 1, 20, *s_arr[125]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 10, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 10, 1, *s_arr[14]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 10, 5, *s_arr[17]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 10, 9, *s_arr[18]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 10, 10, *s_arr[19]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 10, 11, *s_arr[19]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 19, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 19, 1, *s_arr[1]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 19, 2, *s_arr[1]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 20, 0, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 20, 1, *s_arr[0]);
	test(pop, *s_arr[1158], 0, 21, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[0], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[0], 0, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 0, 1, *s_arr[434]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 0, 2, *s_arr[422]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 0, 4, *s_arr[410]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 0, 5, *s_arr[398]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 0, 6, *s_arr[398]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 1, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 1, 1, *s_arr[538]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 1, 2, *s_arr[526]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 1, 3, *s_arr[514]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 1, 4, *s_arr[502]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 1, 5, *s_arr[502]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 2, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 2, 1, *s_arr[577]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 2, 2, *s_arr[565]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 2, 3, *s_arr[553]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 2, 4, *s_arr[553]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 4, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 4, 1, *s_arr[590]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 4, 2, *s_arr[590]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 5, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 5, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 0, 1, *s_arr[434]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 0, 5, *s_arr[398]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 0, 9, *s_arr[386]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 0, 10, *s_arr[374]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 0, 11, *s_arr[374]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 1, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 1, 1, *s_arr[538]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 1, 4, *s_arr[502]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 1, 8, *s_arr[490]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 1, 9, *s_arr[478]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 1, 10, *s_arr[478]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 5, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 5, 1, *s_arr[642]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 5, 2, *s_arr[630]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 5, 4, *s_arr[618]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 5, 5, *s_arr[606]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 5, 6, *s_arr[606]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 9, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 9, 1, *s_arr[330]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 9, 2, *s_arr[330]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 10, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 10, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 0, 1, *s_arr[434]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 0, 10, *s_arr[374]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 0, 19, *s_arr[362]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 0, 20, *s_arr[350]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 0, 21, *s_arr[350]);
}

template <class S>
void
test42(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 1, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 1, 1, *s_arr[538]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 1, 9, *s_arr[478]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 1, 18, *s_arr[466]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 1, 19, *s_arr[454]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 1, 20, *s_arr[454]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 10, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 10, 1, *s_arr[434]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 10, 5, *s_arr[398]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 10, 9, *s_arr[386]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 10, 10, *s_arr[374]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 10, 11, *s_arr[374]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 19, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 19, 1, *s_arr[330]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 19, 2, *s_arr[330]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 20, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 20, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 1, 0, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[0], 0, 0, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[0], 0, 1, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 0, 0, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 0, 1, *s_arr[437]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 0, 2, *s_arr[425]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 0, 4, *s_arr[413]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 0, 5, *s_arr[401]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 0, 6, *s_arr[401]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 1, 0, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 1, 1, *s_arr[541]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 1, 2, *s_arr[529]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 1, 3, *s_arr[517]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 1, 4, *s_arr[505]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 1, 5, *s_arr[505]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 2, 0, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 2, 1, *s_arr[580]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 2, 2, *s_arr[568]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 2, 3, *s_arr[556]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 2, 4, *s_arr[556]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 4, 0, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 4, 1, *s_arr[593]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 4, 2, *s_arr[593]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 5, 0, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 5, 1, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 0, 0, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 0, 1, *s_arr[437]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 0, 5, *s_arr[401]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 0, 9, *s_arr[389]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 0, 10, *s_arr[377]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 0, 11, *s_arr[377]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 1, 0, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 1, 1, *s_arr[541]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 1, 4, *s_arr[505]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 1, 8, *s_arr[493]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 1, 9, *s_arr[481]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 1, 10, *s_arr[481]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 5, 0, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 5, 1, *s_arr[645]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 5, 2, *s_arr[633]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 5, 4, *s_arr[621]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 5, 5, *s_arr[609]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 5, 6, *s_arr[609]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 9, 0, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 9, 1, *s_arr[333]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 9, 2, *s_arr[333]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 10, 0, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 10, 1, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 0, 0, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 0, 1, *s_arr[437]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 0, 10, *s_arr[377]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 0, 19, *s_arr[365]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 0, 20, *s_arr[353]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 0, 21, *s_arr[353]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 1, 0, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 1, 1, *s_arr[541]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 1, 9, *s_arr[481]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 1, 18, *s_arr[469]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 1, 19, *s_arr[457]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 1, 20, *s_arr[457]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 10, 0, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 10, 1, *s_arr[437]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 10, 5, *s_arr[401]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 10, 9, *s_arr[389]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 10, 10, *s_arr[377]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 10, 11, *s_arr[377]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 19, 0, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 19, 1, *s_arr[333]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 19, 2, *s_arr[333]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 20, 0, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 20, 1, *s_arr[1194]);
	test(pop, *s_arr[1158], 1, 1, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[0], 0, 0, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[0], 0, 1, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 0, 0, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 0, 1, *s_arr[442]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 0, 2, *s_arr[430]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 0, 4, *s_arr[418]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 0, 5, *s_arr[406]);
}

template <class S>
void
test43(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 0, 6, *s_arr[406]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 1, 0, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 1, 1, *s_arr[546]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 1, 2, *s_arr[534]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 1, 3, *s_arr[522]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 1, 4, *s_arr[510]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 1, 5, *s_arr[510]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 2, 0, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 2, 1, *s_arr[585]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 2, 2, *s_arr[573]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 2, 3, *s_arr[561]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 2, 4, *s_arr[561]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 4, 0, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 4, 1, *s_arr[598]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 4, 2, *s_arr[598]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 5, 0, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 5, 1, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 0, 0, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 0, 1, *s_arr[442]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 0, 5, *s_arr[406]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 0, 9, *s_arr[394]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 0, 10, *s_arr[382]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 0, 11, *s_arr[382]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 1, 0, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 1, 1, *s_arr[546]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 1, 4, *s_arr[510]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 1, 8, *s_arr[498]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 1, 9, *s_arr[486]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 1, 10, *s_arr[486]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 5, 0, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 5, 1, *s_arr[650]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 5, 2, *s_arr[638]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 5, 4, *s_arr[626]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 5, 5, *s_arr[614]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 5, 6, *s_arr[614]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 9, 0, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 9, 1, *s_arr[338]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 9, 2, *s_arr[338]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 10, 0, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 10, 1, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 0, 0, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 0, 1, *s_arr[442]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 0, 10, *s_arr[382]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 0, 19, *s_arr[370]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 0, 20, *s_arr[358]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 0, 21, *s_arr[358]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 1, 0, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 1, 1, *s_arr[546]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 1, 9, *s_arr[486]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 1, 18, *s_arr[474]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 1, 19, *s_arr[462]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 1, 20, *s_arr[462]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 10, 0, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 10, 1, *s_arr[442]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 10, 5, *s_arr[406]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 10, 9, *s_arr[394]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 10, 10, *s_arr[382]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 10, 11, *s_arr[382]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 19, 0, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 19, 1, *s_arr[338]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 19, 2, *s_arr[338]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 20, 0, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 20, 1, *s_arr[1199]);
	test(pop, *s_arr[1158], 1, 9, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[0], 0, 0, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[0], 0, 1, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 0, 0, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 0, 1, *s_arr[443]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 0, 2, *s_arr[431]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 0, 4, *s_arr[419]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 0, 5, *s_arr[407]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 0, 6, *s_arr[407]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 1, 0, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 1, 1, *s_arr[547]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 1, 2, *s_arr[535]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 1, 3, *s_arr[523]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 1, 4, *s_arr[511]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 1, 5, *s_arr[511]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 2, 0, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 2, 1, *s_arr[586]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 2, 2, *s_arr[574]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 2, 3, *s_arr[562]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 2, 4, *s_arr[562]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 4, 0, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 4, 1, *s_arr[599]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 4, 2, *s_arr[599]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 5, 0, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 5, 1, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 0, 0, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 0, 1, *s_arr[443]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 0, 5, *s_arr[407]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 0, 9, *s_arr[395]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 0, 10, *s_arr[383]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 0, 11, *s_arr[383]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 1, 0, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 1, 1, *s_arr[547]);
}

template <class S>
void
test44(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 1, 4, *s_arr[511]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 1, 8, *s_arr[499]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 1, 9, *s_arr[487]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 1, 10, *s_arr[487]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 5, 0, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 5, 1, *s_arr[651]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 5, 2, *s_arr[639]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 5, 4, *s_arr[627]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 5, 5, *s_arr[615]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 5, 6, *s_arr[615]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 9, 0, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 9, 1, *s_arr[339]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 9, 2, *s_arr[339]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 10, 0, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 10, 1, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 0, 0, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 0, 1, *s_arr[443]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 0, 10, *s_arr[383]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 0, 19, *s_arr[371]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 0, 20, *s_arr[359]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 0, 21, *s_arr[359]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 1, 0, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 1, 1, *s_arr[547]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 1, 9, *s_arr[487]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 1, 18, *s_arr[475]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 1, 19, *s_arr[463]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 1, 20, *s_arr[463]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 10, 0, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 10, 1, *s_arr[443]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 10, 5, *s_arr[407]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 10, 9, *s_arr[395]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 10, 10, *s_arr[383]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 10, 11, *s_arr[383]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 19, 0, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 19, 1, *s_arr[339]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 19, 2, *s_arr[339]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 20, 0, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 20, 1, *s_arr[1200]);
	test(pop, *s_arr[1158], 1, 18, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[0], 0, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[0], 0, 1, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 0, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 0, 1, *s_arr[340]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 0, 2, *s_arr[341]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 0, 4, *s_arr[342]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 0, 5, *s_arr[343]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 0, 6, *s_arr[343]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 1, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 1, 1, *s_arr[444]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 1, 2, *s_arr[445]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 1, 3, *s_arr[446]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 1, 4, *s_arr[447]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 1, 5, *s_arr[447]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 2, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 2, 1, *s_arr[548]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 2, 2, *s_arr[549]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 2, 3, *s_arr[550]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 2, 4, *s_arr[550]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 4, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 4, 1, *s_arr[587]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 4, 2, *s_arr[587]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 5, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 5, 1, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 0, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 0, 1, *s_arr[340]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 0, 5, *s_arr[343]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 0, 9, *s_arr[344]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 0, 10, *s_arr[345]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 0, 11, *s_arr[345]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 1, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 1, 1, *s_arr[444]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 1, 4, *s_arr[447]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 1, 8, *s_arr[448]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 1, 9, *s_arr[449]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 1, 10, *s_arr[449]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 5, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 5, 1, *s_arr[600]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 5, 2, *s_arr[601]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 5, 4, *s_arr[602]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 5, 5, *s_arr[603]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 5, 6, *s_arr[603]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 9, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 9, 1, *s_arr[327]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 9, 2, *s_arr[327]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 10, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 10, 1, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 0, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 0, 1, *s_arr[340]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 0, 10, *s_arr[345]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 0, 19, *s_arr[346]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 0, 20, *s_arr[347]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 0, 21, *s_arr[347]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 1, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 1, 1, *s_arr[444]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 1, 9, *s_arr[449]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 1, 18, *s_arr[450]);
}

template <class S>
void
test45(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 1, 19, *s_arr[451]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 1, 20, *s_arr[451]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 10, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 10, 1, *s_arr[340]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 10, 5, *s_arr[343]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 10, 9, *s_arr[344]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 10, 10, *s_arr[345]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 10, 11, *s_arr[345]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 19, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 19, 1, *s_arr[327]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 19, 2, *s_arr[327]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 20, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 20, 1, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 19, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[0], 0, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[0], 0, 1, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 0, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 0, 1, *s_arr[340]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 0, 2, *s_arr[341]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 0, 4, *s_arr[342]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 0, 5, *s_arr[343]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 0, 6, *s_arr[343]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 1, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 1, 1, *s_arr[444]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 1, 2, *s_arr[445]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 1, 3, *s_arr[446]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 1, 4, *s_arr[447]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 1, 5, *s_arr[447]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 2, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 2, 1, *s_arr[548]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 2, 2, *s_arr[549]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 2, 3, *s_arr[550]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 2, 4, *s_arr[550]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 4, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 4, 1, *s_arr[587]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 4, 2, *s_arr[587]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 5, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 5, 1, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 0, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 0, 1, *s_arr[340]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 0, 5, *s_arr[343]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 0, 9, *s_arr[344]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 0, 10, *s_arr[345]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 0, 11, *s_arr[345]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 1, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 1, 1, *s_arr[444]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 1, 4, *s_arr[447]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 1, 8, *s_arr[448]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 1, 9, *s_arr[449]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 1, 10, *s_arr[449]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 5, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 5, 1, *s_arr[600]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 5, 2, *s_arr[601]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 5, 4, *s_arr[602]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 5, 5, *s_arr[603]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 5, 6, *s_arr[603]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 9, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 9, 1, *s_arr[327]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 9, 2, *s_arr[327]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 10, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 10, 1, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 0, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 0, 1, *s_arr[340]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 0, 10, *s_arr[345]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 0, 19, *s_arr[346]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 0, 20, *s_arr[347]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 0, 21, *s_arr[347]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 1, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 1, 1, *s_arr[444]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 1, 9, *s_arr[449]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 1, 18, *s_arr[450]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 1, 19, *s_arr[451]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 1, 20, *s_arr[451]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 10, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 10, 1, *s_arr[340]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 10, 5, *s_arr[343]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 10, 9, *s_arr[344]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 10, 10, *s_arr[345]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 10, 11, *s_arr[345]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 19, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 19, 1, *s_arr[327]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 19, 2, *s_arr[327]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 20, 0, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 20, 1, *s_arr[326]);
	test(pop, *s_arr[1158], 1, 20, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[0], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[0], 0, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 0, 1, *s_arr[1023]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 0, 2, *s_arr[1019]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 0, 4, *s_arr[1015]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 0, 5, *s_arr[1011]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 0, 6, *s_arr[1011]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 1, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 1, 1, *s_arr[1063]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 1, 2, *s_arr[1059]);
}

template <class S>
void
test46(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 1, 3, *s_arr[1055]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 1, 4, *s_arr[1051]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 1, 5, *s_arr[1051]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 2, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 2, 1, *s_arr[1078]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 2, 2, *s_arr[1074]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 2, 3, *s_arr[1070]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 2, 4, *s_arr[1070]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 4, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 4, 1, *s_arr[1083]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 4, 2, *s_arr[1083]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 5, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 5, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 0, 1, *s_arr[1023]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 0, 5, *s_arr[1011]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 0, 9, *s_arr[1007]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 0, 10, *s_arr[1003]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 0, 11, *s_arr[1003]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 1, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 1, 1, *s_arr[1063]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 1, 4, *s_arr[1051]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 1, 8, *s_arr[1047]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 1, 9, *s_arr[1043]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 1, 10, *s_arr[1043]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 5, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 5, 1, *s_arr[1103]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 5, 2, *s_arr[1099]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 5, 4, *s_arr[1095]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 5, 5, *s_arr[1091]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 5, 6, *s_arr[1091]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 9, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 9, 1, *s_arr[983]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 9, 2, *s_arr[983]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 10, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 10, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 0, 1, *s_arr[1023]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 0, 10, *s_arr[1003]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 0, 19, *s_arr[999]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 0, 20, *s_arr[995]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 0, 21, *s_arr[995]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 1, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 1, 1, *s_arr[1063]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 1, 9, *s_arr[1043]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 1, 18, *s_arr[1039]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 1, 19, *s_arr[1035]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 1, 20, *s_arr[1035]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 10, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 10, 1, *s_arr[1023]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 10, 5, *s_arr[1011]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 10, 9, *s_arr[1007]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 10, 10, *s_arr[1003]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 10, 11, *s_arr[1003]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 19, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 19, 1, *s_arr[983]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 19, 2, *s_arr[983]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 20, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 20, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 10, 0, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[0], 0, 0, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[0], 0, 1, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 0, 0, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 0, 1, *s_arr[1024]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 0, 2, *s_arr[1020]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 0, 4, *s_arr[1016]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 0, 5, *s_arr[1012]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 0, 6, *s_arr[1012]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 1, 0, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 1, 1, *s_arr[1064]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 1, 2, *s_arr[1060]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 1, 3, *s_arr[1056]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 1, 4, *s_arr[1052]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 1, 5, *s_arr[1052]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 2, 0, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 2, 1, *s_arr[1079]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 2, 2, *s_arr[1075]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 2, 3, *s_arr[1071]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 2, 4, *s_arr[1071]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 4, 0, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 4, 1, *s_arr[1084]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 4, 2, *s_arr[1084]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 5, 0, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 5, 1, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 0, 0, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 0, 1, *s_arr[1024]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 0, 5, *s_arr[1012]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 0, 9, *s_arr[1008]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 0, 10, *s_arr[1004]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 0, 11, *s_arr[1004]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 1, 0, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 1, 1, *s_arr[1064]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 1, 4, *s_arr[1052]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 1, 8, *s_arr[1048]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 1, 9, *s_arr[1044]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 1, 10, *s_arr[1044]);
}

template <class S>
void
test47(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 5, 0, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 5, 1, *s_arr[1104]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 5, 2, *s_arr[1100]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 5, 4, *s_arr[1096]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 5, 5, *s_arr[1092]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 5, 6, *s_arr[1092]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 9, 0, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 9, 1, *s_arr[984]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 9, 2, *s_arr[984]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 10, 0, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 10, 1, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 0, 0, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 0, 1, *s_arr[1024]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 0, 10, *s_arr[1004]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 0, 19, *s_arr[1000]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 0, 20, *s_arr[996]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 0, 21, *s_arr[996]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 1, 0, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 1, 1, *s_arr[1064]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 1, 9, *s_arr[1044]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 1, 18, *s_arr[1040]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 1, 19, *s_arr[1036]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 1, 20, *s_arr[1036]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 10, 0, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 10, 1, *s_arr[1024]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 10, 5, *s_arr[1012]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 10, 9, *s_arr[1008]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 10, 10, *s_arr[1004]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 10, 11, *s_arr[1004]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 19, 0, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 19, 1, *s_arr[984]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 19, 2, *s_arr[984]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 20, 0, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 20, 1, *s_arr[1184]);
	test(pop, *s_arr[1158], 10, 1, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[0], 0, 0, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[0], 0, 1, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 0, 0, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 0, 1, *s_arr[1025]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 0, 2, *s_arr[1021]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 0, 4, *s_arr[1017]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 0, 5, *s_arr[1013]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 0, 6, *s_arr[1013]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 1, 0, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 1, 1, *s_arr[1065]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 1, 2, *s_arr[1061]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 1, 3, *s_arr[1057]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 1, 4, *s_arr[1053]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 1, 5, *s_arr[1053]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 2, 0, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 2, 1, *s_arr[1080]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 2, 2, *s_arr[1076]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 2, 3, *s_arr[1072]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 2, 4, *s_arr[1072]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 4, 0, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 4, 1, *s_arr[1085]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 4, 2, *s_arr[1085]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 5, 0, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 5, 1, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 0, 0, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 0, 1, *s_arr[1025]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 0, 5, *s_arr[1013]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 0, 9, *s_arr[1009]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 0, 10, *s_arr[1005]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 0, 11, *s_arr[1005]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 1, 0, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 1, 1, *s_arr[1065]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 1, 4, *s_arr[1053]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 1, 8, *s_arr[1049]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 1, 9, *s_arr[1045]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 1, 10, *s_arr[1045]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 5, 0, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 5, 1, *s_arr[1105]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 5, 2, *s_arr[1101]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 5, 4, *s_arr[1097]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 5, 5, *s_arr[1093]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 5, 6, *s_arr[1093]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 9, 0, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 9, 1, *s_arr[985]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 9, 2, *s_arr[985]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 10, 0, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 10, 1, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 0, 0, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 0, 1, *s_arr[1025]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 0, 10, *s_arr[1005]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 0, 19, *s_arr[1001]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 0, 20, *s_arr[997]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 0, 21, *s_arr[997]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 1, 0, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 1, 1, *s_arr[1065]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 1, 9, *s_arr[1045]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 1, 18, *s_arr[1041]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 1, 19, *s_arr[1037]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 1, 20, *s_arr[1037]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 10, 0, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 10, 1, *s_arr[1025]);
}

template <class S>
void
test48(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 10, 5, *s_arr[1013]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 10, 9, *s_arr[1009]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 10, 10, *s_arr[1005]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 10, 11, *s_arr[1005]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 19, 0, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 19, 1, *s_arr[985]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 19, 2, *s_arr[985]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 20, 0, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 20, 1, *s_arr[1185]);
	test(pop, *s_arr[1158], 10, 5, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[0], 0, 0, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[0], 0, 1, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 0, 0, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 0, 1, *s_arr[1026]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 0, 2, *s_arr[1022]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 0, 4, *s_arr[1018]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 0, 5, *s_arr[1014]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 0, 6, *s_arr[1014]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 1, 0, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 1, 1, *s_arr[1066]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 1, 2, *s_arr[1062]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 1, 3, *s_arr[1058]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 1, 4, *s_arr[1054]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 1, 5, *s_arr[1054]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 2, 0, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 2, 1, *s_arr[1081]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 2, 2, *s_arr[1077]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 2, 3, *s_arr[1073]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 2, 4, *s_arr[1073]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 4, 0, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 4, 1, *s_arr[1086]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 4, 2, *s_arr[1086]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 5, 0, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 5, 1, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 0, 0, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 0, 1, *s_arr[1026]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 0, 5, *s_arr[1014]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 0, 9, *s_arr[1010]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 0, 10, *s_arr[1006]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 0, 11, *s_arr[1006]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 1, 0, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 1, 1, *s_arr[1066]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 1, 4, *s_arr[1054]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 1, 8, *s_arr[1050]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 1, 9, *s_arr[1046]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 1, 10, *s_arr[1046]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 5, 0, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 5, 1, *s_arr[1106]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 5, 2, *s_arr[1102]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 5, 4, *s_arr[1098]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 5, 5, *s_arr[1094]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 5, 6, *s_arr[1094]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 9, 0, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 9, 1, *s_arr[986]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 9, 2, *s_arr[986]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 10, 0, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 10, 1, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 0, 0, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 0, 1, *s_arr[1026]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 0, 10, *s_arr[1006]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 0, 19, *s_arr[1002]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 0, 20, *s_arr[998]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 0, 21, *s_arr[998]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 1, 0, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 1, 1, *s_arr[1066]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 1, 9, *s_arr[1046]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 1, 18, *s_arr[1042]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 1, 19, *s_arr[1038]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 1, 20, *s_arr[1038]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 10, 0, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 10, 1, *s_arr[1026]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 10, 5, *s_arr[1014]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 10, 9, *s_arr[1010]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 10, 10, *s_arr[1006]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 10, 11, *s_arr[1006]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 19, 0, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 19, 1, *s_arr[986]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 19, 2, *s_arr[986]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 20, 0, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 20, 1, *s_arr[1186]);
	test(pop, *s_arr[1158], 10, 9, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[0], 0, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[0], 0, 1, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 0, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 0, 1, *s_arr[987]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 0, 2, *s_arr[988]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 0, 4, *s_arr[989]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 0, 5, *s_arr[990]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 0, 6, *s_arr[990]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 1, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 1, 1, *s_arr[1027]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 1, 2, *s_arr[1028]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 1, 3, *s_arr[1029]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 1, 4, *s_arr[1030]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 1, 5, *s_arr[1030]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 2, 0, *s_arr[981]);
}

template <class S>
void
test49(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 2, 1, *s_arr[1067]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 2, 2, *s_arr[1068]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 2, 3, *s_arr[1069]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 2, 4, *s_arr[1069]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 4, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 4, 1, *s_arr[1082]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 4, 2, *s_arr[1082]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 5, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 5, 1, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 0, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 0, 1, *s_arr[987]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 0, 5, *s_arr[990]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 0, 9, *s_arr[991]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 0, 10, *s_arr[992]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 0, 11, *s_arr[992]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 1, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 1, 1, *s_arr[1027]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 1, 4, *s_arr[1030]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 1, 8, *s_arr[1031]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 1, 9, *s_arr[1032]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 1, 10, *s_arr[1032]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 5, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 5, 1, *s_arr[1087]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 5, 2, *s_arr[1088]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 5, 4, *s_arr[1089]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 5, 5, *s_arr[1090]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 5, 6, *s_arr[1090]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 9, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 9, 1, *s_arr[982]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 9, 2, *s_arr[982]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 10, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 10, 1, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 0, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 0, 1, *s_arr[987]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 0, 10, *s_arr[992]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 0, 19, *s_arr[993]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 0, 20, *s_arr[994]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 0, 21, *s_arr[994]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 1, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 1, 1, *s_arr[1027]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 1, 9, *s_arr[1032]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 1, 18, *s_arr[1033]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 1, 19, *s_arr[1034]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 1, 20, *s_arr[1034]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 10, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 10, 1, *s_arr[987]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 10, 5, *s_arr[990]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 10, 9, *s_arr[991]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 10, 10, *s_arr[992]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 10, 11, *s_arr[992]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 19, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 19, 1, *s_arr[982]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 19, 2, *s_arr[982]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 20, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 20, 1, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 10, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[0], 0, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[0], 0, 1, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 0, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 0, 1, *s_arr[987]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 0, 2, *s_arr[988]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 0, 4, *s_arr[989]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 0, 5, *s_arr[990]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 0, 6, *s_arr[990]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 1, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 1, 1, *s_arr[1027]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 1, 2, *s_arr[1028]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 1, 3, *s_arr[1029]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 1, 4, *s_arr[1030]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 1, 5, *s_arr[1030]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 2, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 2, 1, *s_arr[1067]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 2, 2, *s_arr[1068]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 2, 3, *s_arr[1069]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 2, 4, *s_arr[1069]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 4, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 4, 1, *s_arr[1082]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 4, 2, *s_arr[1082]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 5, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 5, 1, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 0, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 0, 1, *s_arr[987]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 0, 5, *s_arr[990]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 0, 9, *s_arr[991]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 0, 10, *s_arr[992]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 0, 11, *s_arr[992]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 1, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 1, 1, *s_arr[1027]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 1, 4, *s_arr[1030]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 1, 8, *s_arr[1031]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 1, 9, *s_arr[1032]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 1, 10, *s_arr[1032]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 5, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 5, 1, *s_arr[1087]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 5, 2, *s_arr[1088]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 5, 4, *s_arr[1089]);
}

template <class S>
void
test50(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 5, 5, *s_arr[1090]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 5, 6, *s_arr[1090]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 9, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 9, 1, *s_arr[982]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 9, 2, *s_arr[982]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 10, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 10, 1, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 0, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 0, 1, *s_arr[987]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 0, 10, *s_arr[992]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 0, 19, *s_arr[993]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 0, 20, *s_arr[994]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 0, 21, *s_arr[994]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 1, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 1, 1, *s_arr[1027]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 1, 9, *s_arr[1032]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 1, 18, *s_arr[1033]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 1, 19, *s_arr[1034]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 1, 20, *s_arr[1034]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 10, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 10, 1, *s_arr[987]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 10, 5, *s_arr[990]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 10, 9, *s_arr[991]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 10, 10, *s_arr[992]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 10, 11, *s_arr[992]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 19, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 19, 1, *s_arr[982]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 19, 2, *s_arr[982]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 20, 0, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 20, 1, *s_arr[981]);
	test(pop, *s_arr[1158], 10, 11, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[0], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[0], 0, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 0, 1, *s_arr[1125]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 0, 2, *s_arr[1124]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 0, 4, *s_arr[1123]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 0, 5, *s_arr[1122]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 0, 6, *s_arr[1122]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 1, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 1, 1, *s_arr[1141]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 1, 2, *s_arr[1140]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 1, 3, *s_arr[1139]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 1, 4, *s_arr[1138]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 1, 5, *s_arr[1138]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 2, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 2, 1, *s_arr[1147]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 2, 2, *s_arr[1146]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 2, 3, *s_arr[1145]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 2, 4, *s_arr[1145]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 4, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 4, 1, *s_arr[1149]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 4, 2, *s_arr[1149]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 5, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 5, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 0, 1, *s_arr[1125]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 0, 5, *s_arr[1122]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 0, 9, *s_arr[1121]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 0, 10, *s_arr[1120]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 0, 11, *s_arr[1120]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 1, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 1, 1, *s_arr[1141]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 1, 4, *s_arr[1138]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 1, 8, *s_arr[1137]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 1, 9, *s_arr[1136]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 1, 10, *s_arr[1136]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 5, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 5, 1, *s_arr[1157]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 5, 2, *s_arr[1156]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 5, 4, *s_arr[1155]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 5, 5, *s_arr[1154]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 5, 6, *s_arr[1154]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 9, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 9, 1, *s_arr[1109]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 9, 2, *s_arr[1109]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 10, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 10, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 0, 1, *s_arr[1125]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 0, 10, *s_arr[1120]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 0, 19, *s_arr[1119]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 0, 20, *s_arr[1118]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 0, 21, *s_arr[1118]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 1, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 1, 1, *s_arr[1141]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 1, 9, *s_arr[1136]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 1, 18, *s_arr[1135]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 1, 19, *s_arr[1134]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 1, 20, *s_arr[1134]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 10, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 10, 1, *s_arr[1125]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 10, 5, *s_arr[1122]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 10, 9, *s_arr[1121]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 10, 10, *s_arr[1120]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 10, 11, *s_arr[1120]);
}

template <class S>
void
test51(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 19, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 19, 1, *s_arr[1109]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 19, 2, *s_arr[1109]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 20, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 20, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 19, 0, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[0], 0, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[0], 0, 1, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 0, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 0, 1, *s_arr[1110]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 0, 2, *s_arr[1111]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 0, 4, *s_arr[1112]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 0, 5, *s_arr[1113]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 0, 6, *s_arr[1113]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 1, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 1, 1, *s_arr[1126]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 1, 2, *s_arr[1127]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 1, 3, *s_arr[1128]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 1, 4, *s_arr[1129]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 1, 5, *s_arr[1129]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 2, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 2, 1, *s_arr[1142]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 2, 2, *s_arr[1143]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 2, 3, *s_arr[1144]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 2, 4, *s_arr[1144]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 4, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 4, 1, *s_arr[1148]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 4, 2, *s_arr[1148]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 5, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 5, 1, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 0, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 0, 1, *s_arr[1110]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 0, 5, *s_arr[1113]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 0, 9, *s_arr[1114]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 0, 10, *s_arr[1115]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 0, 11, *s_arr[1115]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 1, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 1, 1, *s_arr[1126]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 1, 4, *s_arr[1129]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 1, 8, *s_arr[1130]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 1, 9, *s_arr[1131]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 1, 10, *s_arr[1131]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 5, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 5, 1, *s_arr[1150]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 5, 2, *s_arr[1151]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 5, 4, *s_arr[1152]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 5, 5, *s_arr[1153]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 5, 6, *s_arr[1153]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 9, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 9, 1, *s_arr[1108]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 9, 2, *s_arr[1108]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 10, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 10, 1, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 0, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 0, 1, *s_arr[1110]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 0, 10, *s_arr[1115]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 0, 19, *s_arr[1116]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 0, 20, *s_arr[1117]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 0, 21, *s_arr[1117]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 1, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 1, 1, *s_arr[1126]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 1, 9, *s_arr[1131]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 1, 18, *s_arr[1132]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 1, 19, *s_arr[1133]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 1, 20, *s_arr[1133]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 10, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 10, 1, *s_arr[1110]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 10, 5, *s_arr[1113]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 10, 9, *s_arr[1114]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 10, 10, *s_arr[1115]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 10, 11, *s_arr[1115]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 19, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 19, 1, *s_arr[1108]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 19, 2, *s_arr[1108]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 20, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 20, 1, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 1, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[0], 0, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[0], 0, 1, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 0, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 0, 1, *s_arr[1110]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 0, 2, *s_arr[1111]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 0, 4, *s_arr[1112]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 0, 5, *s_arr[1113]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 0, 6, *s_arr[1113]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 1, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 1, 1, *s_arr[1126]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 1, 2, *s_arr[1127]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 1, 3, *s_arr[1128]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 1, 4, *s_arr[1129]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 1, 5, *s_arr[1129]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 2, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 2, 1, *s_arr[1142]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 2, 2, *s_arr[1143]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 2, 3, *s_arr[1144]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 2, 4, *s_arr[1144]);
}

template <class S>
void
test52(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 4, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 4, 1, *s_arr[1148]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 4, 2, *s_arr[1148]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 5, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 5, 1, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 0, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 0, 1, *s_arr[1110]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 0, 5, *s_arr[1113]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 0, 9, *s_arr[1114]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 0, 10, *s_arr[1115]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 0, 11, *s_arr[1115]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 1, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 1, 1, *s_arr[1126]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 1, 4, *s_arr[1129]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 1, 8, *s_arr[1130]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 1, 9, *s_arr[1131]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 1, 10, *s_arr[1131]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 5, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 5, 1, *s_arr[1150]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 5, 2, *s_arr[1151]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 5, 4, *s_arr[1152]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 5, 5, *s_arr[1153]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 5, 6, *s_arr[1153]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 9, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 9, 1, *s_arr[1108]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 9, 2, *s_arr[1108]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 10, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 10, 1, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 0, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 0, 1, *s_arr[1110]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 0, 10, *s_arr[1115]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 0, 19, *s_arr[1116]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 0, 20, *s_arr[1117]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 0, 21, *s_arr[1117]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 1, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 1, 1, *s_arr[1126]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 1, 9, *s_arr[1131]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 1, 18, *s_arr[1132]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 1, 19, *s_arr[1133]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 1, 20, *s_arr[1133]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 10, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 10, 1, *s_arr[1110]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 10, 5, *s_arr[1113]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 10, 9, *s_arr[1114]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 10, 10, *s_arr[1115]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 10, 11, *s_arr[1115]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 19, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 19, 1, *s_arr[1108]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 19, 2, *s_arr[1108]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 20, 0, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 20, 1, *s_arr[1107]);
	test(pop, *s_arr[1158], 19, 2, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[0], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[0], 0, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 0, 1, *s_arr[1160]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 0, 2, *s_arr[1161]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 0, 4, *s_arr[1162]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 0, 5, *s_arr[1163]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 0, 6, *s_arr[1163]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 1, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 1, 1, *s_arr[1168]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 1, 2, *s_arr[1169]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 1, 3, *s_arr[1170]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 1, 4, *s_arr[1171]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 1, 5, *s_arr[1171]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 2, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 2, 1, *s_arr[1176]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 2, 2, *s_arr[1177]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 2, 3, *s_arr[1178]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 2, 4, *s_arr[1178]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 4, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 4, 1, *s_arr[1179]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 4, 2, *s_arr[1179]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 5, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 5, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 0, 1, *s_arr[1160]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 0, 5, *s_arr[1163]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 0, 9, *s_arr[1164]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 0, 10, *s_arr[1165]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 0, 11, *s_arr[1165]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 1, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 1, 1, *s_arr[1168]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 1, 4, *s_arr[1171]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 1, 8, *s_arr[1172]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 1, 9, *s_arr[1173]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 1, 10, *s_arr[1173]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 5, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 5, 1, *s_arr[1180]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 5, 2, *s_arr[1181]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 5, 4, *s_arr[1182]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 5, 5, *s_arr[1183]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 5, 6, *s_arr[1183]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 9, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 9, 1, *s_arr[1159]);
}

template <class S>
void
test53(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 9, 2, *s_arr[1159]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 10, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 10, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 0, 1, *s_arr[1160]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 0, 10, *s_arr[1165]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 0, 19, *s_arr[1166]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 0, 20, *s_arr[1167]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 0, 21, *s_arr[1167]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 1, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 1, 1, *s_arr[1168]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 1, 9, *s_arr[1173]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 1, 18, *s_arr[1174]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 1, 19, *s_arr[1175]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 1, 20, *s_arr[1175]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 10, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 10, 1, *s_arr[1160]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 10, 5, *s_arr[1163]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 10, 9, *s_arr[1164]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 10, 10, *s_arr[1165]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 10, 11, *s_arr[1165]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 19, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 19, 1, *s_arr[1159]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 19, 2, *s_arr[1159]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 20, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 20, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 0, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[0], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[0], 0, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 0, 1, *s_arr[1160]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 0, 2, *s_arr[1161]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 0, 4, *s_arr[1162]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 0, 5, *s_arr[1163]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 0, 6, *s_arr[1163]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 1, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 1, 1, *s_arr[1168]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 1, 2, *s_arr[1169]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 1, 3, *s_arr[1170]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 1, 4, *s_arr[1171]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 1, 5, *s_arr[1171]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 2, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 2, 1, *s_arr[1176]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 2, 2, *s_arr[1177]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 2, 3, *s_arr[1178]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 2, 4, *s_arr[1178]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 4, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 4, 1, *s_arr[1179]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 4, 2, *s_arr[1179]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 5, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 5, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 0, 1, *s_arr[1160]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 0, 5, *s_arr[1163]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 0, 9, *s_arr[1164]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 0, 10, *s_arr[1165]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 0, 11, *s_arr[1165]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 1, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 1, 1, *s_arr[1168]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 1, 4, *s_arr[1171]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 1, 8, *s_arr[1172]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 1, 9, *s_arr[1173]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 1, 10, *s_arr[1173]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 5, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 5, 1, *s_arr[1180]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 5, 2, *s_arr[1181]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 5, 4, *s_arr[1182]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 5, 5, *s_arr[1183]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 5, 6, *s_arr[1183]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 9, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 9, 1, *s_arr[1159]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 9, 2, *s_arr[1159]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 10, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 10, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 0, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 0, 1, *s_arr[1160]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 0, 10, *s_arr[1165]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 0, 19, *s_arr[1166]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 0, 20, *s_arr[1167]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 0, 21, *s_arr[1167]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 1, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 1, 1, *s_arr[1168]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 1, 9, *s_arr[1173]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 1, 18, *s_arr[1174]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 1, 19, *s_arr[1175]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 1, 20, *s_arr[1175]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 10, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 10, 1, *s_arr[1160]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 10, 5, *s_arr[1163]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 10, 9, *s_arr[1164]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 10, 10, *s_arr[1165]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 10, 11, *s_arr[1165]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 19, 0, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 19, 1, *s_arr[1159]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 19, 2, *s_arr[1159]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 20, 0, *s_arr[1158]);
}

template <class S>
void
test54(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 20, 1, *s_arr[1158]);
	test(pop, *s_arr[1158], 20, 1, *s_arr[21], 21, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[0], 0, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[0], 0, 1, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[0], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 0, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 0, 1, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 0, 2, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 0, 4, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 0, 5, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 0, 6, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 1, 1, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 1, 2, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 1, 3, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 1, 4, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 1, 5, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 2, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 2, 1, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 2, 2, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 2, 3, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 2, 4, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 4, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 4, 1, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 4, 2, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 5, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 5, 1, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[17], 6, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 0, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 0, 1, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 0, 5, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 0, 9, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 0, 10, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 0, 11, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 1, 1, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 1, 4, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 1, 8, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 1, 9, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 1, 10, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 5, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 5, 1, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 5, 2, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 5, 4, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 5, 5, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 5, 6, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 9, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 9, 1, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 9, 2, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 10, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 10, 1, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[19], 11, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 0, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 0, 1, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 0, 10, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 0, 19, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 0, 20, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 0, 21, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 1, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 1, 1, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 1, 9, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 1, 18, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 1, 19, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 1, 20, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 10, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 10, 1, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 10, 5, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 10, 9, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 10, 10, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 10, 11, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 19, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 19, 1, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 19, 2, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 20, 0, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 20, 1, *s_arr[1204]);
	test(pop, *s_arr[1158], 21, 0, *s_arr[21], 21, 0, *s_arr[1204]);
}

template <class S>
void
test55(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test_npos(pop, *s_arr[981], 9, 1, *s_arr[21], 10, *s_arr[938]);
	test_npos(pop, *s_arr[981], 9, 1, *s_arr[21], 19, *s_arr[931]);
	test_npos(pop, *s_arr[981], 9, 1, *s_arr[21], 20, *s_arr[930]);
	test_npos(pop, *s_arr[981], 9, 1, *s_arr[21], 20, *s_arr[930]);
	test_npos(pop, *s_arr[981], 9, 1, *s_arr[21], 21, *s_arr[1204]);
	test_npos(pop, *s_arr[981], 9, 2, *s_arr[0], 0, *s_arr[930]);
	test_npos(pop, *s_arr[981], 9, 2, *s_arr[0], 1, *s_arr[1204]);
	test_npos(pop, *s_arr[981], 9, 2, *s_arr[17], 0, *s_arr[936]);
	test_npos(pop, *s_arr[981], 9, 2, *s_arr[17], 1, *s_arr[952]);
	test_npos(pop, *s_arr[981], 9, 2, *s_arr[17], 2, *s_arr[967]);
	test_npos(pop, *s_arr[981], 9, 2, *s_arr[17], 4, *s_arr[971]);
	test_npos(pop, *s_arr[981], 9, 2, *s_arr[17], 5, *s_arr[930]);
	test_npos(pop, *s_arr[981], 9, 2, *s_arr[17], 6, *s_arr[1204]);
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
		nvobj::transaction::run(pop, [&] {
			s_arr[0] = nvobj::make_persistent<C>("");
			s_arr[1] = nvobj::make_persistent<C>("0");
			s_arr[2] = nvobj::make_persistent<C>("0abcde");
			s_arr[3] = nvobj::make_persistent<C>("0abcdefghij");
			s_arr[4] = nvobj::make_persistent<C>(
				"0abcdefghijklmnopqrst");
			s_arr[5] = nvobj::make_persistent<C>("0bcde");
			s_arr[6] = nvobj::make_persistent<C>("0bcdefghij");
			s_arr[7] = nvobj::make_persistent<C>(
				"0bcdefghijklmnopqrst");
			s_arr[8] = nvobj::make_persistent<C>("0cde");
			s_arr[9] = nvobj::make_persistent<C>("0e");
			s_arr[10] = nvobj::make_persistent<C>("0fghij");
			s_arr[11] = nvobj::make_persistent<C>("0j");
			s_arr[12] = nvobj::make_persistent<C>("0klmnopqrst");
			s_arr[13] = nvobj::make_persistent<C>("0t");
			s_arr[14] = nvobj::make_persistent<C>("1");
			s_arr[15] = nvobj::make_persistent<C>("12");
			s_arr[16] = nvobj::make_persistent<C>("1234");
			s_arr[17] = nvobj::make_persistent<C>("12345");
			s_arr[18] = nvobj::make_persistent<C>("123456789");
			s_arr[19] = nvobj::make_persistent<C>("1234567890");
			s_arr[20] = nvobj::make_persistent<C>(
				"1234567890123456789");
			s_arr[21] = nvobj::make_persistent<C>(
				"12345678901234567890");
			s_arr[22] = nvobj::make_persistent<C>(
				"12345678901234567890abcde");
			s_arr[23] = nvobj::make_persistent<C>(
				"12345678901234567890abcdefghij");
			s_arr[24] = nvobj::make_persistent<C>(
				"12345678901234567890abcdefghijklmnopqrst");
			s_arr[25] = nvobj::make_persistent<C>(
				"12345678901234567890bcde");
			s_arr[26] = nvobj::make_persistent<C>(
				"12345678901234567890bcdefghij");
			s_arr[27] = nvobj::make_persistent<C>(
				"12345678901234567890bcdefghijklmnopqrst");
			s_arr[28] = nvobj::make_persistent<C>(
				"12345678901234567890cde");
			s_arr[29] = nvobj::make_persistent<C>(
				"12345678901234567890e");
			s_arr[30] = nvobj::make_persistent<C>(
				"12345678901234567890fghij");
			s_arr[31] = nvobj::make_persistent<C>(
				"12345678901234567890j");
			s_arr[32] = nvobj::make_persistent<C>(
				"12345678901234567890klmnopqrst");
			s_arr[33] = nvobj::make_persistent<C>(
				"12345678901234567890t");
			s_arr[34] = nvobj::make_persistent<C>(
				"1234567890123456789abcde");
			s_arr[35] = nvobj::make_persistent<C>(
				"1234567890123456789abcdefghij");
			s_arr[36] = nvobj::make_persistent<C>(
				"1234567890123456789abcdefghijklmnopqrst");
			s_arr[37] = nvobj::make_persistent<C>(
				"1234567890123456789bcde");
			s_arr[38] = nvobj::make_persistent<C>(
				"1234567890123456789bcdefghij");
			s_arr[39] = nvobj::make_persistent<C>(
				"1234567890123456789bcdefghijklmnopqrst");
			s_arr[40] = nvobj::make_persistent<C>(
				"1234567890123456789cde");
			s_arr[41] = nvobj::make_persistent<C>(
				"1234567890123456789e");
			s_arr[42] = nvobj::make_persistent<C>(
				"1234567890123456789fghij");
			s_arr[43] = nvobj::make_persistent<C>(
				"1234567890123456789j");
			s_arr[44] = nvobj::make_persistent<C>(
				"1234567890123456789klmnopqrst");
			s_arr[45] = nvobj::make_persistent<C>(
				"1234567890123456789t");
			s_arr[46] =
				nvobj::make_persistent<C>("1234567890abcde");
			s_arr[47] = nvobj::make_persistent<C>(
				"1234567890abcdefghij");
			s_arr[48] = nvobj::make_persistent<C>(
				"1234567890abcdefghijklmnopqrst");
			s_arr[49] = nvobj::make_persistent<C>("1234567890bcde");
			s_arr[50] = nvobj::make_persistent<C>(
				"1234567890bcdefghij");
			s_arr[51] = nvobj::make_persistent<C>(
				"1234567890bcdefghijklmnopqrst");
			s_arr[52] = nvobj::make_persistent<C>("1234567890cde");
			s_arr[53] = nvobj::make_persistent<C>("1234567890e");
			s_arr[54] =
				nvobj::make_persistent<C>("1234567890fghij");
			s_arr[55] = nvobj::make_persistent<C>("1234567890j");
			s_arr[56] = nvobj::make_persistent<C>(
				"1234567890klmnopqrst");
			s_arr[57] = nvobj::make_persistent<C>("1234567890t");
			s_arr[58] = nvobj::make_persistent<C>("123456789abcde");
			s_arr[59] = nvobj::make_persistent<C>(
				"123456789abcdefghij");
			s_arr[60] = nvobj::make_persistent<C>(
				"123456789abcdefghijklmnopqrst");
			s_arr[61] = nvobj::make_persistent<C>("123456789bcde");
			s_arr[62] =
				nvobj::make_persistent<C>("123456789bcdefghij");
			s_arr[63] = nvobj::make_persistent<C>(
				"123456789bcdefghijklmnopqrst");
			s_arr[64] = nvobj::make_persistent<C>("123456789cde");
			s_arr[65] = nvobj::make_persistent<C>("123456789e");
			s_arr[66] = nvobj::make_persistent<C>("123456789fghij");
			s_arr[67] = nvobj::make_persistent<C>("123456789j");
			s_arr[68] = nvobj::make_persistent<C>(
				"123456789klmnopqrst");
			s_arr[69] = nvobj::make_persistent<C>("123456789t");
			s_arr[70] = nvobj::make_persistent<C>("12345abcde");
			s_arr[71] =
				nvobj::make_persistent<C>("12345abcdefghij");
			s_arr[72] = nvobj::make_persistent<C>(
				"12345abcdefghijklmnopqrst");
			s_arr[73] = nvobj::make_persistent<C>("12345bcde");
			s_arr[74] = nvobj::make_persistent<C>("12345bcdefghij");
			s_arr[75] = nvobj::make_persistent<C>(
				"12345bcdefghijklmnopqrst");
			s_arr[76] = nvobj::make_persistent<C>("12345cde");
			s_arr[77] = nvobj::make_persistent<C>("12345e");
			s_arr[78] = nvobj::make_persistent<C>("12345fghij");
			s_arr[79] = nvobj::make_persistent<C>("12345j");
			s_arr[80] =
				nvobj::make_persistent<C>("12345klmnopqrst");
			s_arr[81] = nvobj::make_persistent<C>("12345t");
			s_arr[82] = nvobj::make_persistent<C>("1234abcde");
			s_arr[83] = nvobj::make_persistent<C>("1234abcdefghij");
			s_arr[84] = nvobj::make_persistent<C>(
				"1234abcdefghijklmnopqrst");
			s_arr[85] = nvobj::make_persistent<C>("1234bcde");
			s_arr[86] = nvobj::make_persistent<C>("1234bcdefghij");
			s_arr[87] = nvobj::make_persistent<C>(
				"1234bcdefghijklmnopqrst");
			s_arr[88] = nvobj::make_persistent<C>("1234cde");
			s_arr[89] = nvobj::make_persistent<C>("1234e");
			s_arr[90] = nvobj::make_persistent<C>("1234fghij");
			s_arr[91] = nvobj::make_persistent<C>("1234j");
			s_arr[92] = nvobj::make_persistent<C>("1234klmnopqrst");
			s_arr[93] = nvobj::make_persistent<C>("1234t");
			s_arr[94] = nvobj::make_persistent<C>("12abcde");
			s_arr[95] = nvobj::make_persistent<C>("12abcdefghij");
			s_arr[96] = nvobj::make_persistent<C>(
				"12abcdefghijklmnopqrst");
			s_arr[97] = nvobj::make_persistent<C>("12bcde");
			s_arr[98] = nvobj::make_persistent<C>("12bcdefghij");
			s_arr[99] = nvobj::make_persistent<C>(
				"12bcdefghijklmnopqrst");
			s_arr[100] = nvobj::make_persistent<C>("12cde");
			s_arr[101] = nvobj::make_persistent<C>("12e");
			s_arr[102] = nvobj::make_persistent<C>("12fghij");
			s_arr[103] = nvobj::make_persistent<C>("12j");
			s_arr[104] = nvobj::make_persistent<C>("12klmnopqrst");
			s_arr[105] = nvobj::make_persistent<C>("12t");
			s_arr[106] = nvobj::make_persistent<C>("1abcde");
			s_arr[107] = nvobj::make_persistent<C>("1abcdefghij");
			s_arr[108] = nvobj::make_persistent<C>(
				"1abcdefghijklmnopqrst");
			s_arr[109] = nvobj::make_persistent<C>("1bcde");
			s_arr[110] = nvobj::make_persistent<C>("1bcdefghij");
			s_arr[111] = nvobj::make_persistent<C>(
				"1bcdefghijklmnopqrst");
			s_arr[112] = nvobj::make_persistent<C>("1cde");
			s_arr[113] = nvobj::make_persistent<C>("1e");
			s_arr[114] = nvobj::make_persistent<C>("1fghij");
			s_arr[115] = nvobj::make_persistent<C>("1j");
			s_arr[116] = nvobj::make_persistent<C>("1klmnopqrst");
			s_arr[117] = nvobj::make_persistent<C>("1t");
			s_arr[118] = nvobj::make_persistent<C>("2");
			s_arr[119] = nvobj::make_persistent<C>("23");
			s_arr[120] = nvobj::make_persistent<C>("234");
			s_arr[121] = nvobj::make_persistent<C>("2345");
			s_arr[122] = nvobj::make_persistent<C>("23456789");
			s_arr[123] = nvobj::make_persistent<C>("234567890");
			s_arr[124] =
				nvobj::make_persistent<C>("234567890123456789");
			s_arr[125] = nvobj::make_persistent<C>(
				"2345678901234567890");
			s_arr[126] = nvobj::make_persistent<C>(
				"2345678901234567890abcde");
			s_arr[127] = nvobj::make_persistent<C>(
				"2345678901234567890abcdefghij");
			s_arr[128] = nvobj::make_persistent<C>(
				"2345678901234567890abcdefghijklmnopqrst");
			s_arr[129] = nvobj::make_persistent<C>(
				"2345678901234567890bcde");
			s_arr[130] = nvobj::make_persistent<C>(
				"2345678901234567890bcdefghij");
			s_arr[131] = nvobj::make_persistent<C>(
				"2345678901234567890bcdefghijklmnopqrst");
			s_arr[132] = nvobj::make_persistent<C>(
				"2345678901234567890cde");
			s_arr[133] = nvobj::make_persistent<C>(
				"2345678901234567890e");
			s_arr[134] = nvobj::make_persistent<C>(
				"2345678901234567890fghij");
			s_arr[135] = nvobj::make_persistent<C>(
				"2345678901234567890j");
			s_arr[136] = nvobj::make_persistent<C>(
				"2345678901234567890klmnopqrst");
			s_arr[137] = nvobj::make_persistent<C>(
				"2345678901234567890t");
			s_arr[138] = nvobj::make_persistent<C>(
				"234567890123456789abcde");
			s_arr[139] = nvobj::make_persistent<C>(
				"234567890123456789abcdefghij");
			s_arr[140] = nvobj::make_persistent<C>(
				"234567890123456789abcdefghijklmnopqrst");
			s_arr[141] = nvobj::make_persistent<C>(
				"234567890123456789bcde");
			s_arr[142] = nvobj::make_persistent<C>(
				"234567890123456789bcdefghij");
			s_arr[143] = nvobj::make_persistent<C>(
				"234567890123456789bcdefghijklmnopqrst");
			s_arr[144] = nvobj::make_persistent<C>(
				"234567890123456789cde");
			s_arr[145] = nvobj::make_persistent<C>(
				"234567890123456789e");
			s_arr[146] = nvobj::make_persistent<C>(
				"234567890123456789fghij");
			s_arr[147] = nvobj::make_persistent<C>(
				"234567890123456789j");
			s_arr[148] = nvobj::make_persistent<C>(
				"234567890123456789klmnopqrst");
			s_arr[149] = nvobj::make_persistent<C>(
				"234567890123456789t");
			s_arr[150] =
				nvobj::make_persistent<C>("234567890abcde");
			s_arr[151] = nvobj::make_persistent<C>(
				"234567890abcdefghij");
			s_arr[152] = nvobj::make_persistent<C>(
				"234567890abcdefghijklmnopqrst");
			s_arr[153] = nvobj::make_persistent<C>("234567890bcde");
			s_arr[154] =
				nvobj::make_persistent<C>("234567890bcdefghij");
			s_arr[155] = nvobj::make_persistent<C>(
				"234567890bcdefghijklmnopqrst");
			s_arr[156] = nvobj::make_persistent<C>("234567890cde");
			s_arr[157] = nvobj::make_persistent<C>("234567890e");
			s_arr[158] =
				nvobj::make_persistent<C>("234567890fghij");
			s_arr[159] = nvobj::make_persistent<C>("234567890j");
			s_arr[160] = nvobj::make_persistent<C>(
				"234567890klmnopqrst");
			s_arr[161] = nvobj::make_persistent<C>("234567890t");
			s_arr[162] = nvobj::make_persistent<C>("23456789abcde");
			s_arr[163] =
				nvobj::make_persistent<C>("23456789abcdefghij");
			s_arr[164] = nvobj::make_persistent<C>(
				"23456789abcdefghijklmnopqrst");
			s_arr[165] = nvobj::make_persistent<C>("23456789bcde");
			s_arr[166] =
				nvobj::make_persistent<C>("23456789bcdefghij");
			s_arr[167] = nvobj::make_persistent<C>(
				"23456789bcdefghijklmnopqrst");
			s_arr[168] = nvobj::make_persistent<C>("23456789cde");
			s_arr[169] = nvobj::make_persistent<C>("23456789e");
			s_arr[170] = nvobj::make_persistent<C>("23456789fghij");
			s_arr[171] = nvobj::make_persistent<C>("23456789j");
			s_arr[172] =
				nvobj::make_persistent<C>("23456789klmnopqrst");
			s_arr[173] = nvobj::make_persistent<C>("23456789t");
			s_arr[174] = nvobj::make_persistent<C>("2345abcde");
			s_arr[175] =
				nvobj::make_persistent<C>("2345abcdefghij");
			s_arr[176] = nvobj::make_persistent<C>(
				"2345abcdefghijklmnopqrst");
			s_arr[177] = nvobj::make_persistent<C>("2345bcde");
			s_arr[178] = nvobj::make_persistent<C>("2345bcdefghij");
			s_arr[179] = nvobj::make_persistent<C>(
				"2345bcdefghijklmnopqrst");
			s_arr[180] = nvobj::make_persistent<C>("2345cde");
			s_arr[181] = nvobj::make_persistent<C>("2345e");
			s_arr[182] = nvobj::make_persistent<C>("2345fghij");
			s_arr[183] = nvobj::make_persistent<C>("2345j");
			s_arr[184] =
				nvobj::make_persistent<C>("2345klmnopqrst");
			s_arr[185] = nvobj::make_persistent<C>("2345t");
			s_arr[186] = nvobj::make_persistent<C>("234abcde");
			s_arr[187] = nvobj::make_persistent<C>("234abcdefghij");
			s_arr[188] = nvobj::make_persistent<C>(
				"234abcdefghijklmnopqrst");
			s_arr[189] = nvobj::make_persistent<C>("234bcde");
			s_arr[190] = nvobj::make_persistent<C>("234bcdefghij");
			s_arr[191] = nvobj::make_persistent<C>(
				"234bcdefghijklmnopqrst");
			s_arr[192] = nvobj::make_persistent<C>("234cde");
			s_arr[193] = nvobj::make_persistent<C>("234e");
			s_arr[194] = nvobj::make_persistent<C>("234fghij");
			s_arr[195] = nvobj::make_persistent<C>("234j");
			s_arr[196] = nvobj::make_persistent<C>("234klmnopqrst");
			s_arr[197] = nvobj::make_persistent<C>("234t");
			s_arr[198] = nvobj::make_persistent<C>("23abcde");
			s_arr[199] = nvobj::make_persistent<C>("23abcdefghij");
			s_arr[200] = nvobj::make_persistent<C>(
				"23abcdefghijklmnopqrst");
			s_arr[201] = nvobj::make_persistent<C>("23bcde");
			s_arr[202] = nvobj::make_persistent<C>("23bcdefghij");
			s_arr[203] = nvobj::make_persistent<C>(
				"23bcdefghijklmnopqrst");
			s_arr[204] = nvobj::make_persistent<C>("23cde");
			s_arr[205] = nvobj::make_persistent<C>("23e");
			s_arr[206] = nvobj::make_persistent<C>("23fghij");
			s_arr[207] = nvobj::make_persistent<C>("23j");
			s_arr[208] = nvobj::make_persistent<C>("23klmnopqrst");
			s_arr[209] = nvobj::make_persistent<C>("23t");
			s_arr[210] = nvobj::make_persistent<C>("2abcde");
			s_arr[211] = nvobj::make_persistent<C>("2abcdefghij");
			s_arr[212] = nvobj::make_persistent<C>(
				"2abcdefghijklmnopqrst");
			s_arr[213] = nvobj::make_persistent<C>("2bcde");
			s_arr[214] = nvobj::make_persistent<C>("2bcdefghij");
			s_arr[215] = nvobj::make_persistent<C>(
				"2bcdefghijklmnopqrst");
			s_arr[216] = nvobj::make_persistent<C>("2cde");
			s_arr[217] = nvobj::make_persistent<C>("2e");
			s_arr[218] = nvobj::make_persistent<C>("2fghij");
			s_arr[219] = nvobj::make_persistent<C>("2j");
			s_arr[220] = nvobj::make_persistent<C>("2klmnopqrst");
			s_arr[221] = nvobj::make_persistent<C>("2t");
			s_arr[222] = nvobj::make_persistent<C>("3");
			s_arr[223] = nvobj::make_persistent<C>("34");
			s_arr[224] = nvobj::make_persistent<C>("345");
			s_arr[225] = nvobj::make_persistent<C>("345abcde");
			s_arr[226] = nvobj::make_persistent<C>("345abcdefghij");
			s_arr[227] = nvobj::make_persistent<C>(
				"345abcdefghijklmnopqrst");
			s_arr[228] = nvobj::make_persistent<C>("345bcde");
			s_arr[229] = nvobj::make_persistent<C>("345bcdefghij");
			s_arr[230] = nvobj::make_persistent<C>(
				"345bcdefghijklmnopqrst");
			s_arr[231] = nvobj::make_persistent<C>("345cde");
			s_arr[232] = nvobj::make_persistent<C>("345e");
			s_arr[233] = nvobj::make_persistent<C>("345fghij");
			s_arr[234] = nvobj::make_persistent<C>("345j");
			s_arr[235] = nvobj::make_persistent<C>("345klmnopqrst");
			s_arr[236] = nvobj::make_persistent<C>("345t");
			s_arr[237] = nvobj::make_persistent<C>("34abcde");
			s_arr[238] = nvobj::make_persistent<C>("34abcdefghij");
			s_arr[239] = nvobj::make_persistent<C>(
				"34abcdefghijklmnopqrst");
			s_arr[240] = nvobj::make_persistent<C>("34bcde");
			s_arr[241] = nvobj::make_persistent<C>("34bcdefghij");
			s_arr[242] = nvobj::make_persistent<C>(
				"34bcdefghijklmnopqrst");
			s_arr[243] = nvobj::make_persistent<C>("34cde");
			s_arr[244] = nvobj::make_persistent<C>("34e");
			s_arr[245] = nvobj::make_persistent<C>("34fghij");
			s_arr[246] = nvobj::make_persistent<C>("34j");
			s_arr[247] = nvobj::make_persistent<C>("34klmnopqrst");
			s_arr[248] = nvobj::make_persistent<C>("34t");
			s_arr[249] = nvobj::make_persistent<C>("3abcde");
			s_arr[250] = nvobj::make_persistent<C>("3abcdefghij");
			s_arr[251] = nvobj::make_persistent<C>(
				"3abcdefghijklmnopqrst");
			s_arr[252] = nvobj::make_persistent<C>("3bcde");
			s_arr[253] = nvobj::make_persistent<C>("3bcdefghij");
			s_arr[254] = nvobj::make_persistent<C>(
				"3bcdefghijklmnopqrst");
			s_arr[255] = nvobj::make_persistent<C>("3cde");
			s_arr[256] = nvobj::make_persistent<C>("3e");
			s_arr[257] = nvobj::make_persistent<C>("3fghij");
			s_arr[258] = nvobj::make_persistent<C>("3j");
			s_arr[259] = nvobj::make_persistent<C>("3klmnopqrst");
			s_arr[260] = nvobj::make_persistent<C>("3t");
			s_arr[261] = nvobj::make_persistent<C>("5");
			s_arr[262] = nvobj::make_persistent<C>("5abcde");
			s_arr[263] = nvobj::make_persistent<C>("5abcdefghij");
			s_arr[264] = nvobj::make_persistent<C>(
				"5abcdefghijklmnopqrst");
			s_arr[265] = nvobj::make_persistent<C>("5bcde");
			s_arr[266] = nvobj::make_persistent<C>("5bcdefghij");
			s_arr[267] = nvobj::make_persistent<C>(
				"5bcdefghijklmnopqrst");
			s_arr[268] = nvobj::make_persistent<C>("5cde");
			s_arr[269] = nvobj::make_persistent<C>("5e");
			s_arr[270] = nvobj::make_persistent<C>("5fghij");
			s_arr[271] = nvobj::make_persistent<C>("5j");
			s_arr[272] = nvobj::make_persistent<C>("5klmnopqrst");
			s_arr[273] = nvobj::make_persistent<C>("5t");
			s_arr[274] = nvobj::make_persistent<C>("6");
			s_arr[275] = nvobj::make_persistent<C>("67");
			s_arr[276] = nvobj::make_persistent<C>("6789");
			s_arr[277] = nvobj::make_persistent<C>("67890");
			s_arr[278] = nvobj::make_persistent<C>("67890abcde");
			s_arr[279] =
				nvobj::make_persistent<C>("67890abcdefghij");
			s_arr[280] = nvobj::make_persistent<C>(
				"67890abcdefghijklmnopqrst");
			s_arr[281] = nvobj::make_persistent<C>("67890bcde");
			s_arr[282] =
				nvobj::make_persistent<C>("67890bcdefghij");
			s_arr[283] = nvobj::make_persistent<C>(
				"67890bcdefghijklmnopqrst");
			s_arr[284] = nvobj::make_persistent<C>("67890cde");
			s_arr[285] = nvobj::make_persistent<C>("67890e");
			s_arr[286] = nvobj::make_persistent<C>("67890fghij");
			s_arr[287] = nvobj::make_persistent<C>("67890j");
			s_arr[288] =
				nvobj::make_persistent<C>("67890klmnopqrst");
			s_arr[289] = nvobj::make_persistent<C>("67890t");
			s_arr[290] = nvobj::make_persistent<C>("6789abcde");
			s_arr[291] =
				nvobj::make_persistent<C>("6789abcdefghij");
			s_arr[292] = nvobj::make_persistent<C>(
				"6789abcdefghijklmnopqrst");
			s_arr[293] = nvobj::make_persistent<C>("6789bcde");
			s_arr[294] = nvobj::make_persistent<C>("6789bcdefghij");
			s_arr[295] = nvobj::make_persistent<C>(
				"6789bcdefghijklmnopqrst");
			s_arr[296] = nvobj::make_persistent<C>("6789cde");
			s_arr[297] = nvobj::make_persistent<C>("6789e");
			s_arr[298] = nvobj::make_persistent<C>("6789fghij");
			s_arr[299] = nvobj::make_persistent<C>("6789j");
			s_arr[300] =
				nvobj::make_persistent<C>("6789klmnopqrst");
			s_arr[301] = nvobj::make_persistent<C>("6789t");
			s_arr[302] = nvobj::make_persistent<C>("67abcde");
			s_arr[303] = nvobj::make_persistent<C>("67abcdefghij");
			s_arr[304] = nvobj::make_persistent<C>(
				"67abcdefghijklmnopqrst");
			s_arr[305] = nvobj::make_persistent<C>("67bcde");
			s_arr[306] = nvobj::make_persistent<C>("67bcdefghij");
			s_arr[307] = nvobj::make_persistent<C>(
				"67bcdefghijklmnopqrst");
			s_arr[308] = nvobj::make_persistent<C>("67cde");
			s_arr[309] = nvobj::make_persistent<C>("67e");
			s_arr[310] = nvobj::make_persistent<C>("67fghij");
			s_arr[311] = nvobj::make_persistent<C>("67j");
			s_arr[312] = nvobj::make_persistent<C>("67klmnopqrst");
			s_arr[313] = nvobj::make_persistent<C>("67t");
			s_arr[314] = nvobj::make_persistent<C>("6abcde");
			s_arr[315] = nvobj::make_persistent<C>("6abcdefghij");
			s_arr[316] = nvobj::make_persistent<C>(
				"6abcdefghijklmnopqrst");
			s_arr[317] = nvobj::make_persistent<C>("6bcde");
			s_arr[318] = nvobj::make_persistent<C>("6bcdefghij");
			s_arr[319] = nvobj::make_persistent<C>(
				"6bcdefghijklmnopqrst");
			s_arr[320] = nvobj::make_persistent<C>("6cde");
			s_arr[321] = nvobj::make_persistent<C>("6e");
			s_arr[322] = nvobj::make_persistent<C>("6fghij");
			s_arr[323] = nvobj::make_persistent<C>("6j");
			s_arr[324] = nvobj::make_persistent<C>("6klmnopqrst");
			s_arr[325] = nvobj::make_persistent<C>("6t");
			s_arr[326] = nvobj::make_persistent<C>("a");
			s_arr[327] = nvobj::make_persistent<C>("a0");
			s_arr[328] = nvobj::make_persistent<C>("a0bcde");
			s_arr[329] = nvobj::make_persistent<C>("a0bcdefghij");
			s_arr[330] = nvobj::make_persistent<C>(
				"a0bcdefghijklmnopqrst");
			s_arr[331] = nvobj::make_persistent<C>("a0cde");
			s_arr[332] = nvobj::make_persistent<C>("a0cdefghij");
			s_arr[333] = nvobj::make_persistent<C>(
				"a0cdefghijklmnopqrst");
			s_arr[334] = nvobj::make_persistent<C>("a0de");
			s_arr[335] = nvobj::make_persistent<C>("a0e");
			s_arr[336] = nvobj::make_persistent<C>("a0fghij");
			s_arr[337] = nvobj::make_persistent<C>("a0j");
			s_arr[338] = nvobj::make_persistent<C>("a0klmnopqrst");
			s_arr[339] = nvobj::make_persistent<C>("a0t");
			s_arr[340] = nvobj::make_persistent<C>("a1");
			s_arr[341] = nvobj::make_persistent<C>("a12");
			s_arr[342] = nvobj::make_persistent<C>("a1234");
			s_arr[343] = nvobj::make_persistent<C>("a12345");
			s_arr[344] = nvobj::make_persistent<C>("a123456789");
			s_arr[345] = nvobj::make_persistent<C>("a1234567890");
			s_arr[346] = nvobj::make_persistent<C>(
				"a1234567890123456789");
			s_arr[347] = nvobj::make_persistent<C>(
				"a12345678901234567890");
			s_arr[348] = nvobj::make_persistent<C>(
				"a12345678901234567890bcde");
			s_arr[349] = nvobj::make_persistent<C>(
				"a12345678901234567890bcdefghij");
			s_arr[350] = nvobj::make_persistent<C>(
				"a12345678901234567890bcdefghijklmnopqrst");
			s_arr[351] = nvobj::make_persistent<C>(
				"a12345678901234567890cde");
			s_arr[352] = nvobj::make_persistent<C>(
				"a12345678901234567890cdefghij");
			s_arr[353] = nvobj::make_persistent<C>(
				"a12345678901234567890cdefghijklmnopqrst");
			s_arr[354] = nvobj::make_persistent<C>(
				"a12345678901234567890de");
			s_arr[355] = nvobj::make_persistent<C>(
				"a12345678901234567890e");
			s_arr[356] = nvobj::make_persistent<C>(
				"a12345678901234567890fghij");
			s_arr[357] = nvobj::make_persistent<C>(
				"a12345678901234567890j");
			s_arr[358] = nvobj::make_persistent<C>(
				"a12345678901234567890klmnopqrst");
			s_arr[359] = nvobj::make_persistent<C>(
				"a12345678901234567890t");
			s_arr[360] = nvobj::make_persistent<C>(
				"a1234567890123456789bcde");
			s_arr[361] = nvobj::make_persistent<C>(
				"a1234567890123456789bcdefghij");
			s_arr[362] = nvobj::make_persistent<C>(
				"a1234567890123456789bcdefghijklmnopqrst");
			s_arr[363] = nvobj::make_persistent<C>(
				"a1234567890123456789cde");
			s_arr[364] = nvobj::make_persistent<C>(
				"a1234567890123456789cdefghij");
			s_arr[365] = nvobj::make_persistent<C>(
				"a1234567890123456789cdefghijklmnopqrst");
			s_arr[366] = nvobj::make_persistent<C>(
				"a1234567890123456789de");
			s_arr[367] = nvobj::make_persistent<C>(
				"a1234567890123456789e");
			s_arr[368] = nvobj::make_persistent<C>(
				"a1234567890123456789fghij");
			s_arr[369] = nvobj::make_persistent<C>(
				"a1234567890123456789j");
			s_arr[370] = nvobj::make_persistent<C>(
				"a1234567890123456789klmnopqrst");
			s_arr[371] = nvobj::make_persistent<C>(
				"a1234567890123456789t");
			s_arr[372] =
				nvobj::make_persistent<C>("a1234567890bcde");
			s_arr[373] = nvobj::make_persistent<C>(
				"a1234567890bcdefghij");
			s_arr[374] = nvobj::make_persistent<C>(
				"a1234567890bcdefghijklmnopqrst");
			s_arr[375] =
				nvobj::make_persistent<C>("a1234567890cde");
			s_arr[376] = nvobj::make_persistent<C>(
				"a1234567890cdefghij");
			s_arr[377] = nvobj::make_persistent<C>(
				"a1234567890cdefghijklmnopqrst");
			s_arr[378] = nvobj::make_persistent<C>("a1234567890de");
			s_arr[379] = nvobj::make_persistent<C>("a1234567890e");
			s_arr[380] =
				nvobj::make_persistent<C>("a1234567890fghij");
			s_arr[381] = nvobj::make_persistent<C>("a1234567890j");
			s_arr[382] = nvobj::make_persistent<C>(
				"a1234567890klmnopqrst");
			s_arr[383] = nvobj::make_persistent<C>("a1234567890t");
			s_arr[384] =
				nvobj::make_persistent<C>("a123456789bcde");
			s_arr[385] = nvobj::make_persistent<C>(
				"a123456789bcdefghij");
			s_arr[386] = nvobj::make_persistent<C>(
				"a123456789bcdefghijklmnopqrst");
			s_arr[387] = nvobj::make_persistent<C>("a123456789cde");
			s_arr[388] =
				nvobj::make_persistent<C>("a123456789cdefghij");
			s_arr[389] = nvobj::make_persistent<C>(
				"a123456789cdefghijklmnopqrst");
			s_arr[390] = nvobj::make_persistent<C>("a123456789de");
			s_arr[391] = nvobj::make_persistent<C>("a123456789e");
			s_arr[392] =
				nvobj::make_persistent<C>("a123456789fghij");
			s_arr[393] = nvobj::make_persistent<C>("a123456789j");
			s_arr[394] = nvobj::make_persistent<C>(
				"a123456789klmnopqrst");
			s_arr[395] = nvobj::make_persistent<C>("a123456789t");
			s_arr[396] = nvobj::make_persistent<C>("a12345bcde");
			s_arr[397] =
				nvobj::make_persistent<C>("a12345bcdefghij");
			s_arr[398] = nvobj::make_persistent<C>(
				"a12345bcdefghijklmnopqrst");
			s_arr[399] = nvobj::make_persistent<C>("a12345cde");
			s_arr[400] =
				nvobj::make_persistent<C>("a12345cdefghij");
			s_arr[401] = nvobj::make_persistent<C>(
				"a12345cdefghijklmnopqrst");
			s_arr[402] = nvobj::make_persistent<C>("a12345de");
			s_arr[403] = nvobj::make_persistent<C>("a12345e");
			s_arr[404] = nvobj::make_persistent<C>("a12345fghij");
			s_arr[405] = nvobj::make_persistent<C>("a12345j");
			s_arr[406] =
				nvobj::make_persistent<C>("a12345klmnopqrst");
			s_arr[407] = nvobj::make_persistent<C>("a12345t");
			s_arr[408] = nvobj::make_persistent<C>("a1234bcde");
			s_arr[409] =
				nvobj::make_persistent<C>("a1234bcdefghij");
			s_arr[410] = nvobj::make_persistent<C>(
				"a1234bcdefghijklmnopqrst");
			s_arr[411] = nvobj::make_persistent<C>("a1234cde");
			s_arr[412] = nvobj::make_persistent<C>("a1234cdefghij");
			s_arr[413] = nvobj::make_persistent<C>(
				"a1234cdefghijklmnopqrst");
			s_arr[414] = nvobj::make_persistent<C>("a1234de");
			s_arr[415] = nvobj::make_persistent<C>("a1234e");
			s_arr[416] = nvobj::make_persistent<C>("a1234fghij");
			s_arr[417] = nvobj::make_persistent<C>("a1234j");
			s_arr[418] =
				nvobj::make_persistent<C>("a1234klmnopqrst");
			s_arr[419] = nvobj::make_persistent<C>("a1234t");
			s_arr[420] = nvobj::make_persistent<C>("a12bcde");
			s_arr[421] = nvobj::make_persistent<C>("a12bcdefghij");
			s_arr[422] = nvobj::make_persistent<C>(
				"a12bcdefghijklmnopqrst");
			s_arr[423] = nvobj::make_persistent<C>("a12cde");
			s_arr[424] = nvobj::make_persistent<C>("a12cdefghij");
			s_arr[425] = nvobj::make_persistent<C>(
				"a12cdefghijklmnopqrst");
			s_arr[426] = nvobj::make_persistent<C>("a12de");
			s_arr[427] = nvobj::make_persistent<C>("a12e");
			s_arr[428] = nvobj::make_persistent<C>("a12fghij");
			s_arr[429] = nvobj::make_persistent<C>("a12j");
			s_arr[430] = nvobj::make_persistent<C>("a12klmnopqrst");
			s_arr[431] = nvobj::make_persistent<C>("a12t");
			s_arr[432] = nvobj::make_persistent<C>("a1bcde");
			s_arr[433] = nvobj::make_persistent<C>("a1bcdefghij");
			s_arr[434] = nvobj::make_persistent<C>(
				"a1bcdefghijklmnopqrst");
			s_arr[435] = nvobj::make_persistent<C>("a1cde");
			s_arr[436] = nvobj::make_persistent<C>("a1cdefghij");
			s_arr[437] = nvobj::make_persistent<C>(
				"a1cdefghijklmnopqrst");
			s_arr[438] = nvobj::make_persistent<C>("a1de");
			s_arr[439] = nvobj::make_persistent<C>("a1e");
			s_arr[440] = nvobj::make_persistent<C>("a1fghij");
			s_arr[441] = nvobj::make_persistent<C>("a1j");
			s_arr[442] = nvobj::make_persistent<C>("a1klmnopqrst");
			s_arr[443] = nvobj::make_persistent<C>("a1t");
			s_arr[444] = nvobj::make_persistent<C>("a2");
			s_arr[445] = nvobj::make_persistent<C>("a23");
			s_arr[446] = nvobj::make_persistent<C>("a234");
			s_arr[447] = nvobj::make_persistent<C>("a2345");
			s_arr[448] = nvobj::make_persistent<C>("a23456789");
			s_arr[449] = nvobj::make_persistent<C>("a234567890");
			s_arr[450] = nvobj::make_persistent<C>(
				"a234567890123456789");
			s_arr[451] = nvobj::make_persistent<C>(
				"a2345678901234567890");
			s_arr[452] = nvobj::make_persistent<C>(
				"a2345678901234567890bcde");
			s_arr[453] = nvobj::make_persistent<C>(
				"a2345678901234567890bcdefghij");
			s_arr[454] = nvobj::make_persistent<C>(
				"a2345678901234567890bcdefghijklmnopqrst");
			s_arr[455] = nvobj::make_persistent<C>(
				"a2345678901234567890cde");
			s_arr[456] = nvobj::make_persistent<C>(
				"a2345678901234567890cdefghij");
			s_arr[457] = nvobj::make_persistent<C>(
				"a2345678901234567890cdefghijklmnopqrst");
			s_arr[458] = nvobj::make_persistent<C>(
				"a2345678901234567890de");
			s_arr[459] = nvobj::make_persistent<C>(
				"a2345678901234567890e");
			s_arr[460] = nvobj::make_persistent<C>(
				"a2345678901234567890fghij");
			s_arr[461] = nvobj::make_persistent<C>(
				"a2345678901234567890j");
			s_arr[462] = nvobj::make_persistent<C>(
				"a2345678901234567890klmnopqrst");
			s_arr[463] = nvobj::make_persistent<C>(
				"a2345678901234567890t");
			s_arr[464] = nvobj::make_persistent<C>(
				"a234567890123456789bcde");
			s_arr[465] = nvobj::make_persistent<C>(
				"a234567890123456789bcdefghij");
			s_arr[466] = nvobj::make_persistent<C>(
				"a234567890123456789bcdefghijklmnopqrst");
			s_arr[467] = nvobj::make_persistent<C>(
				"a234567890123456789cde");
			s_arr[468] = nvobj::make_persistent<C>(
				"a234567890123456789cdefghij");
			s_arr[469] = nvobj::make_persistent<C>(
				"a234567890123456789cdefghijklmnopqrst");
			s_arr[470] = nvobj::make_persistent<C>(
				"a234567890123456789de");
			s_arr[471] = nvobj::make_persistent<C>(
				"a234567890123456789e");
			s_arr[472] = nvobj::make_persistent<C>(
				"a234567890123456789fghij");
			s_arr[473] = nvobj::make_persistent<C>(
				"a234567890123456789j");
			s_arr[474] = nvobj::make_persistent<C>(
				"a234567890123456789klmnopqrst");
			s_arr[475] = nvobj::make_persistent<C>(
				"a234567890123456789t");
			s_arr[476] =
				nvobj::make_persistent<C>("a234567890bcde");
			s_arr[477] = nvobj::make_persistent<C>(
				"a234567890bcdefghij");
			s_arr[478] = nvobj::make_persistent<C>(
				"a234567890bcdefghijklmnopqrst");
			s_arr[479] = nvobj::make_persistent<C>("a234567890cde");
			s_arr[480] =
				nvobj::make_persistent<C>("a234567890cdefghij");
			s_arr[481] = nvobj::make_persistent<C>(
				"a234567890cdefghijklmnopqrst");
			s_arr[482] = nvobj::make_persistent<C>("a234567890de");
			s_arr[483] = nvobj::make_persistent<C>("a234567890e");
			s_arr[484] =
				nvobj::make_persistent<C>("a234567890fghij");
			s_arr[485] = nvobj::make_persistent<C>("a234567890j");
			s_arr[486] = nvobj::make_persistent<C>(
				"a234567890klmnopqrst");
			s_arr[487] = nvobj::make_persistent<C>("a234567890t");
			s_arr[488] = nvobj::make_persistent<C>("a23456789bcde");
			s_arr[489] =
				nvobj::make_persistent<C>("a23456789bcdefghij");
			s_arr[490] = nvobj::make_persistent<C>(
				"a23456789bcdefghijklmnopqrst");
			s_arr[491] = nvobj::make_persistent<C>("a23456789cde");
			s_arr[492] =
				nvobj::make_persistent<C>("a23456789cdefghij");
			s_arr[493] = nvobj::make_persistent<C>(
				"a23456789cdefghijklmnopqrst");
			s_arr[494] = nvobj::make_persistent<C>("a23456789de");
			s_arr[495] = nvobj::make_persistent<C>("a23456789e");
			s_arr[496] =
				nvobj::make_persistent<C>("a23456789fghij");
			s_arr[497] = nvobj::make_persistent<C>("a23456789j");
			s_arr[498] = nvobj::make_persistent<C>(
				"a23456789klmnopqrst");
			s_arr[499] = nvobj::make_persistent<C>("a23456789t");
			s_arr[500] = nvobj::make_persistent<C>("a2345bcde");
			s_arr[501] =
				nvobj::make_persistent<C>("a2345bcdefghij");
			s_arr[502] = nvobj::make_persistent<C>(
				"a2345bcdefghijklmnopqrst");
			s_arr[503] = nvobj::make_persistent<C>("a2345cde");
			s_arr[504] = nvobj::make_persistent<C>("a2345cdefghij");
			s_arr[505] = nvobj::make_persistent<C>(
				"a2345cdefghijklmnopqrst");
			s_arr[506] = nvobj::make_persistent<C>("a2345de");
			s_arr[507] = nvobj::make_persistent<C>("a2345e");
			s_arr[508] = nvobj::make_persistent<C>("a2345fghij");
			s_arr[509] = nvobj::make_persistent<C>("a2345j");
			s_arr[510] =
				nvobj::make_persistent<C>("a2345klmnopqrst");
			s_arr[511] = nvobj::make_persistent<C>("a2345t");
			s_arr[512] = nvobj::make_persistent<C>("a234bcde");
			s_arr[513] = nvobj::make_persistent<C>("a234bcdefghij");
			s_arr[514] = nvobj::make_persistent<C>(
				"a234bcdefghijklmnopqrst");
			s_arr[515] = nvobj::make_persistent<C>("a234cde");
			s_arr[516] = nvobj::make_persistent<C>("a234cdefghij");
			s_arr[517] = nvobj::make_persistent<C>(
				"a234cdefghijklmnopqrst");
			s_arr[518] = nvobj::make_persistent<C>("a234de");
			s_arr[519] = nvobj::make_persistent<C>("a234e");
			s_arr[520] = nvobj::make_persistent<C>("a234fghij");
			s_arr[521] = nvobj::make_persistent<C>("a234j");
			s_arr[522] =
				nvobj::make_persistent<C>("a234klmnopqrst");
			s_arr[523] = nvobj::make_persistent<C>("a234t");
			s_arr[524] = nvobj::make_persistent<C>("a23bcde");
			s_arr[525] = nvobj::make_persistent<C>("a23bcdefghij");
			s_arr[526] = nvobj::make_persistent<C>(
				"a23bcdefghijklmnopqrst");
			s_arr[527] = nvobj::make_persistent<C>("a23cde");
			s_arr[528] = nvobj::make_persistent<C>("a23cdefghij");
			s_arr[529] = nvobj::make_persistent<C>(
				"a23cdefghijklmnopqrst");
			s_arr[530] = nvobj::make_persistent<C>("a23de");
			s_arr[531] = nvobj::make_persistent<C>("a23e");
			s_arr[532] = nvobj::make_persistent<C>("a23fghij");
			s_arr[533] = nvobj::make_persistent<C>("a23j");
			s_arr[534] = nvobj::make_persistent<C>("a23klmnopqrst");
			s_arr[535] = nvobj::make_persistent<C>("a23t");
			s_arr[536] = nvobj::make_persistent<C>("a2bcde");
			s_arr[537] = nvobj::make_persistent<C>("a2bcdefghij");
			s_arr[538] = nvobj::make_persistent<C>(
				"a2bcdefghijklmnopqrst");
			s_arr[539] = nvobj::make_persistent<C>("a2cde");
			s_arr[540] = nvobj::make_persistent<C>("a2cdefghij");
			s_arr[541] = nvobj::make_persistent<C>(
				"a2cdefghijklmnopqrst");
			s_arr[542] = nvobj::make_persistent<C>("a2de");
			s_arr[543] = nvobj::make_persistent<C>("a2e");
			s_arr[544] = nvobj::make_persistent<C>("a2fghij");
			s_arr[545] = nvobj::make_persistent<C>("a2j");
			s_arr[546] = nvobj::make_persistent<C>("a2klmnopqrst");
			s_arr[547] = nvobj::make_persistent<C>("a2t");
			s_arr[548] = nvobj::make_persistent<C>("a3");
			s_arr[549] = nvobj::make_persistent<C>("a34");
			s_arr[550] = nvobj::make_persistent<C>("a345");
			s_arr[551] = nvobj::make_persistent<C>("a345bcde");
			s_arr[552] = nvobj::make_persistent<C>("a345bcdefghij");
			s_arr[553] = nvobj::make_persistent<C>(
				"a345bcdefghijklmnopqrst");
			s_arr[554] = nvobj::make_persistent<C>("a345cde");
			s_arr[555] = nvobj::make_persistent<C>("a345cdefghij");
			s_arr[556] = nvobj::make_persistent<C>(
				"a345cdefghijklmnopqrst");
			s_arr[557] = nvobj::make_persistent<C>("a345de");
			s_arr[558] = nvobj::make_persistent<C>("a345e");
			s_arr[559] = nvobj::make_persistent<C>("a345fghij");
			s_arr[560] = nvobj::make_persistent<C>("a345j");
			s_arr[561] =
				nvobj::make_persistent<C>("a345klmnopqrst");
			s_arr[562] = nvobj::make_persistent<C>("a345t");
			s_arr[563] = nvobj::make_persistent<C>("a34bcde");
			s_arr[564] = nvobj::make_persistent<C>("a34bcdefghij");
			s_arr[565] = nvobj::make_persistent<C>(
				"a34bcdefghijklmnopqrst");
			s_arr[566] = nvobj::make_persistent<C>("a34cde");
			s_arr[567] = nvobj::make_persistent<C>("a34cdefghij");
			s_arr[568] = nvobj::make_persistent<C>(
				"a34cdefghijklmnopqrst");
			s_arr[569] = nvobj::make_persistent<C>("a34de");
			s_arr[570] = nvobj::make_persistent<C>("a34e");
			s_arr[571] = nvobj::make_persistent<C>("a34fghij");
			s_arr[572] = nvobj::make_persistent<C>("a34j");
			s_arr[573] = nvobj::make_persistent<C>("a34klmnopqrst");
			s_arr[574] = nvobj::make_persistent<C>("a34t");
			s_arr[575] = nvobj::make_persistent<C>("a3bcde");
			s_arr[576] = nvobj::make_persistent<C>("a3bcdefghij");
			s_arr[577] = nvobj::make_persistent<C>(
				"a3bcdefghijklmnopqrst");
			s_arr[578] = nvobj::make_persistent<C>("a3cde");
			s_arr[579] = nvobj::make_persistent<C>("a3cdefghij");
			s_arr[580] = nvobj::make_persistent<C>(
				"a3cdefghijklmnopqrst");
			s_arr[581] = nvobj::make_persistent<C>("a3de");
			s_arr[582] = nvobj::make_persistent<C>("a3e");
			s_arr[583] = nvobj::make_persistent<C>("a3fghij");
			s_arr[584] = nvobj::make_persistent<C>("a3j");
			s_arr[585] = nvobj::make_persistent<C>("a3klmnopqrst");
			s_arr[586] = nvobj::make_persistent<C>("a3t");
			s_arr[587] = nvobj::make_persistent<C>("a5");
			s_arr[588] = nvobj::make_persistent<C>("a5bcde");
			s_arr[589] = nvobj::make_persistent<C>("a5bcdefghij");
			s_arr[590] = nvobj::make_persistent<C>(
				"a5bcdefghijklmnopqrst");
			s_arr[591] = nvobj::make_persistent<C>("a5cde");
			s_arr[592] = nvobj::make_persistent<C>("a5cdefghij");
			s_arr[593] = nvobj::make_persistent<C>(
				"a5cdefghijklmnopqrst");
			s_arr[594] = nvobj::make_persistent<C>("a5de");
			s_arr[595] = nvobj::make_persistent<C>("a5e");
			s_arr[596] = nvobj::make_persistent<C>("a5fghij");
			s_arr[597] = nvobj::make_persistent<C>("a5j");
			s_arr[598] = nvobj::make_persistent<C>("a5klmnopqrst");
			s_arr[599] = nvobj::make_persistent<C>("a5t");
			s_arr[600] = nvobj::make_persistent<C>("a6");
			s_arr[601] = nvobj::make_persistent<C>("a67");
			s_arr[602] = nvobj::make_persistent<C>("a6789");
			s_arr[603] = nvobj::make_persistent<C>("a67890");
			s_arr[604] = nvobj::make_persistent<C>("a67890bcde");
			s_arr[605] =
				nvobj::make_persistent<C>("a67890bcdefghij");
			s_arr[606] = nvobj::make_persistent<C>(
				"a67890bcdefghijklmnopqrst");
			s_arr[607] = nvobj::make_persistent<C>("a67890cde");
			s_arr[608] =
				nvobj::make_persistent<C>("a67890cdefghij");
			s_arr[609] = nvobj::make_persistent<C>(
				"a67890cdefghijklmnopqrst");
			s_arr[610] = nvobj::make_persistent<C>("a67890de");
			s_arr[611] = nvobj::make_persistent<C>("a67890e");
			s_arr[612] = nvobj::make_persistent<C>("a67890fghij");
			s_arr[613] = nvobj::make_persistent<C>("a67890j");
			s_arr[614] =
				nvobj::make_persistent<C>("a67890klmnopqrst");
			s_arr[615] = nvobj::make_persistent<C>("a67890t");
			s_arr[616] = nvobj::make_persistent<C>("a6789bcde");
			s_arr[617] =
				nvobj::make_persistent<C>("a6789bcdefghij");
			s_arr[618] = nvobj::make_persistent<C>(
				"a6789bcdefghijklmnopqrst");
			s_arr[619] = nvobj::make_persistent<C>("a6789cde");
			s_arr[620] = nvobj::make_persistent<C>("a6789cdefghij");
			s_arr[621] = nvobj::make_persistent<C>(
				"a6789cdefghijklmnopqrst");
			s_arr[622] = nvobj::make_persistent<C>("a6789de");
			s_arr[623] = nvobj::make_persistent<C>("a6789e");
			s_arr[624] = nvobj::make_persistent<C>("a6789fghij");
			s_arr[625] = nvobj::make_persistent<C>("a6789j");
			s_arr[626] =
				nvobj::make_persistent<C>("a6789klmnopqrst");
			s_arr[627] = nvobj::make_persistent<C>("a6789t");
			s_arr[628] = nvobj::make_persistent<C>("a67bcde");
			s_arr[629] = nvobj::make_persistent<C>("a67bcdefghij");
			s_arr[630] = nvobj::make_persistent<C>(
				"a67bcdefghijklmnopqrst");
			s_arr[631] = nvobj::make_persistent<C>("a67cde");
			s_arr[632] = nvobj::make_persistent<C>("a67cdefghij");
			s_arr[633] = nvobj::make_persistent<C>(
				"a67cdefghijklmnopqrst");
			s_arr[634] = nvobj::make_persistent<C>("a67de");
			s_arr[635] = nvobj::make_persistent<C>("a67e");
			s_arr[636] = nvobj::make_persistent<C>("a67fghij");
			s_arr[637] = nvobj::make_persistent<C>("a67j");
			s_arr[638] = nvobj::make_persistent<C>("a67klmnopqrst");
			s_arr[639] = nvobj::make_persistent<C>("a67t");
			s_arr[640] = nvobj::make_persistent<C>("a6bcde");
			s_arr[641] = nvobj::make_persistent<C>("a6bcdefghij");
			s_arr[642] = nvobj::make_persistent<C>(
				"a6bcdefghijklmnopqrst");
			s_arr[643] = nvobj::make_persistent<C>("a6cde");
			s_arr[644] = nvobj::make_persistent<C>("a6cdefghij");
			s_arr[645] = nvobj::make_persistent<C>(
				"a6cdefghijklmnopqrst");
			s_arr[646] = nvobj::make_persistent<C>("a6de");
			s_arr[647] = nvobj::make_persistent<C>("a6e");
			s_arr[648] = nvobj::make_persistent<C>("a6fghij");
			s_arr[649] = nvobj::make_persistent<C>("a6j");
			s_arr[650] = nvobj::make_persistent<C>("a6klmnopqrst");
			s_arr[651] = nvobj::make_persistent<C>("a6t");
			s_arr[652] = nvobj::make_persistent<C>("ab");
			s_arr[653] = nvobj::make_persistent<C>("ab0");
			s_arr[654] = nvobj::make_persistent<C>("ab0cde");
			s_arr[655] = nvobj::make_persistent<C>("ab0de");
			s_arr[656] = nvobj::make_persistent<C>("ab0e");
			s_arr[657] = nvobj::make_persistent<C>("ab1");
			s_arr[658] = nvobj::make_persistent<C>("ab12");
			s_arr[659] = nvobj::make_persistent<C>("ab1234");
			s_arr[660] = nvobj::make_persistent<C>("ab12345");
			s_arr[661] = nvobj::make_persistent<C>("ab123456789");
			s_arr[662] = nvobj::make_persistent<C>("ab1234567890");
			s_arr[663] = nvobj::make_persistent<C>(
				"ab1234567890123456789");
			s_arr[664] = nvobj::make_persistent<C>(
				"ab12345678901234567890");
			s_arr[665] = nvobj::make_persistent<C>(
				"ab12345678901234567890cde");
			s_arr[666] = nvobj::make_persistent<C>(
				"ab12345678901234567890de");
			s_arr[667] = nvobj::make_persistent<C>(
				"ab12345678901234567890e");
			s_arr[668] = nvobj::make_persistent<C>(
				"ab1234567890123456789cde");
			s_arr[669] = nvobj::make_persistent<C>(
				"ab1234567890123456789de");
			s_arr[670] = nvobj::make_persistent<C>(
				"ab1234567890123456789e");
			s_arr[671] =
				nvobj::make_persistent<C>("ab1234567890cde");
			s_arr[672] =
				nvobj::make_persistent<C>("ab1234567890de");
			s_arr[673] = nvobj::make_persistent<C>("ab1234567890e");
			s_arr[674] =
				nvobj::make_persistent<C>("ab123456789cde");
			s_arr[675] = nvobj::make_persistent<C>("ab123456789de");
			s_arr[676] = nvobj::make_persistent<C>("ab123456789e");
			s_arr[677] = nvobj::make_persistent<C>("ab12345cde");
			s_arr[678] = nvobj::make_persistent<C>("ab12345de");
			s_arr[679] = nvobj::make_persistent<C>("ab12345e");
			s_arr[680] = nvobj::make_persistent<C>("ab1234cde");
			s_arr[681] = nvobj::make_persistent<C>("ab1234de");
			s_arr[682] = nvobj::make_persistent<C>("ab1234e");
			s_arr[683] = nvobj::make_persistent<C>("ab12cde");
			s_arr[684] = nvobj::make_persistent<C>("ab12de");
			s_arr[685] = nvobj::make_persistent<C>("ab12e");
			s_arr[686] = nvobj::make_persistent<C>("ab1cde");
			s_arr[687] = nvobj::make_persistent<C>("ab1de");
			s_arr[688] = nvobj::make_persistent<C>("ab1e");
			s_arr[689] = nvobj::make_persistent<C>("ab2");
			s_arr[690] = nvobj::make_persistent<C>("ab23");
			s_arr[691] = nvobj::make_persistent<C>("ab234");
			s_arr[692] = nvobj::make_persistent<C>("ab2345");
			s_arr[693] = nvobj::make_persistent<C>("ab23456789");
			s_arr[694] = nvobj::make_persistent<C>("ab234567890");
			s_arr[695] = nvobj::make_persistent<C>(
				"ab234567890123456789");
			s_arr[696] = nvobj::make_persistent<C>(
				"ab2345678901234567890");
			s_arr[697] = nvobj::make_persistent<C>(
				"ab2345678901234567890cde");
			s_arr[698] = nvobj::make_persistent<C>(
				"ab2345678901234567890de");
			s_arr[699] = nvobj::make_persistent<C>(
				"ab2345678901234567890e");
			s_arr[700] = nvobj::make_persistent<C>(
				"ab234567890123456789cde");
			s_arr[701] = nvobj::make_persistent<C>(
				"ab234567890123456789de");
			s_arr[702] = nvobj::make_persistent<C>(
				"ab234567890123456789e");
			s_arr[703] =
				nvobj::make_persistent<C>("ab234567890cde");
			s_arr[704] = nvobj::make_persistent<C>("ab234567890de");
			s_arr[705] = nvobj::make_persistent<C>("ab234567890e");
			s_arr[706] = nvobj::make_persistent<C>("ab23456789cde");
			s_arr[707] = nvobj::make_persistent<C>("ab23456789de");
			s_arr[708] = nvobj::make_persistent<C>("ab23456789e");
			s_arr[709] = nvobj::make_persistent<C>("ab2345cde");
			s_arr[710] = nvobj::make_persistent<C>("ab2345de");
			s_arr[711] = nvobj::make_persistent<C>("ab2345e");
			s_arr[712] = nvobj::make_persistent<C>("ab234cde");
			s_arr[713] = nvobj::make_persistent<C>("ab234de");
			s_arr[714] = nvobj::make_persistent<C>("ab234e");
			s_arr[715] = nvobj::make_persistent<C>("ab23cde");
			s_arr[716] = nvobj::make_persistent<C>("ab23de");
			s_arr[717] = nvobj::make_persistent<C>("ab23e");
			s_arr[718] = nvobj::make_persistent<C>("ab2cde");
			s_arr[719] = nvobj::make_persistent<C>("ab2de");
			s_arr[720] = nvobj::make_persistent<C>("ab2e");
			s_arr[721] = nvobj::make_persistent<C>("ab3");
			s_arr[722] = nvobj::make_persistent<C>("ab34");
			s_arr[723] = nvobj::make_persistent<C>("ab345");
			s_arr[724] = nvobj::make_persistent<C>("ab345cde");
			s_arr[725] = nvobj::make_persistent<C>("ab345de");
			s_arr[726] = nvobj::make_persistent<C>("ab345e");
			s_arr[727] = nvobj::make_persistent<C>("ab34cde");
			s_arr[728] = nvobj::make_persistent<C>("ab34de");
			s_arr[729] = nvobj::make_persistent<C>("ab34e");
			s_arr[730] = nvobj::make_persistent<C>("ab3cde");
			s_arr[731] = nvobj::make_persistent<C>("ab3de");
			s_arr[732] = nvobj::make_persistent<C>("ab3e");
			s_arr[733] = nvobj::make_persistent<C>("ab5");
			s_arr[734] = nvobj::make_persistent<C>("ab5cde");
			s_arr[735] = nvobj::make_persistent<C>("ab5de");
			s_arr[736] = nvobj::make_persistent<C>("ab5e");
			s_arr[737] = nvobj::make_persistent<C>("ab6");
			s_arr[738] = nvobj::make_persistent<C>("ab67");
			s_arr[739] = nvobj::make_persistent<C>("ab6789");
			s_arr[740] = nvobj::make_persistent<C>("ab67890");
			s_arr[741] = nvobj::make_persistent<C>("ab67890cde");
			s_arr[742] = nvobj::make_persistent<C>("ab67890de");
			s_arr[743] = nvobj::make_persistent<C>("ab67890e");
			s_arr[744] = nvobj::make_persistent<C>("ab6789cde");
			s_arr[745] = nvobj::make_persistent<C>("ab6789de");
			s_arr[746] = nvobj::make_persistent<C>("ab6789e");
			s_arr[747] = nvobj::make_persistent<C>("ab67cde");
			s_arr[748] = nvobj::make_persistent<C>("ab67de");
			s_arr[749] = nvobj::make_persistent<C>("ab67e");
			s_arr[750] = nvobj::make_persistent<C>("ab6cde");
			s_arr[751] = nvobj::make_persistent<C>("ab6de");
			s_arr[752] = nvobj::make_persistent<C>("ab6e");
			s_arr[753] = nvobj::make_persistent<C>("abcd");
			s_arr[754] = nvobj::make_persistent<C>("abcd0");
			s_arr[755] = nvobj::make_persistent<C>("abcd0e");
			s_arr[756] = nvobj::make_persistent<C>("abcd1");
			s_arr[757] = nvobj::make_persistent<C>("abcd12");
			s_arr[758] = nvobj::make_persistent<C>("abcd1234");
			s_arr[759] = nvobj::make_persistent<C>("abcd12345");
			s_arr[760] = nvobj::make_persistent<C>("abcd123456789");
			s_arr[761] =
				nvobj::make_persistent<C>("abcd1234567890");
			s_arr[762] = nvobj::make_persistent<C>(
				"abcd1234567890123456789");
			s_arr[763] = nvobj::make_persistent<C>(
				"abcd12345678901234567890");
			s_arr[764] = nvobj::make_persistent<C>(
				"abcd12345678901234567890e");
			s_arr[765] = nvobj::make_persistent<C>(
				"abcd1234567890123456789e");
			s_arr[766] =
				nvobj::make_persistent<C>("abcd1234567890e");
			s_arr[767] =
				nvobj::make_persistent<C>("abcd123456789e");
			s_arr[768] = nvobj::make_persistent<C>("abcd12345e");
			s_arr[769] = nvobj::make_persistent<C>("abcd1234e");
			s_arr[770] = nvobj::make_persistent<C>("abcd12e");
			s_arr[771] = nvobj::make_persistent<C>("abcd1e");
			s_arr[772] = nvobj::make_persistent<C>("abcd2");
			s_arr[773] = nvobj::make_persistent<C>("abcd23");
			s_arr[774] = nvobj::make_persistent<C>("abcd234");
			s_arr[775] = nvobj::make_persistent<C>("abcd2345");
			s_arr[776] = nvobj::make_persistent<C>("abcd23456789");
			s_arr[777] = nvobj::make_persistent<C>("abcd234567890");
			s_arr[778] = nvobj::make_persistent<C>(
				"abcd234567890123456789");
			s_arr[779] = nvobj::make_persistent<C>(
				"abcd2345678901234567890");
			s_arr[780] = nvobj::make_persistent<C>(
				"abcd2345678901234567890e");
			s_arr[781] = nvobj::make_persistent<C>(
				"abcd234567890123456789e");
			s_arr[782] =
				nvobj::make_persistent<C>("abcd234567890e");
			s_arr[783] = nvobj::make_persistent<C>("abcd23456789e");
			s_arr[784] = nvobj::make_persistent<C>("abcd2345e");
			s_arr[785] = nvobj::make_persistent<C>("abcd234e");
			s_arr[786] = nvobj::make_persistent<C>("abcd23e");
			s_arr[787] = nvobj::make_persistent<C>("abcd2e");
			s_arr[788] = nvobj::make_persistent<C>("abcd3");
			s_arr[789] = nvobj::make_persistent<C>("abcd34");
			s_arr[790] = nvobj::make_persistent<C>("abcd345");
			s_arr[791] = nvobj::make_persistent<C>("abcd345e");
			s_arr[792] = nvobj::make_persistent<C>("abcd34e");
			s_arr[793] = nvobj::make_persistent<C>("abcd3e");
			s_arr[794] = nvobj::make_persistent<C>("abcd5");
			s_arr[795] = nvobj::make_persistent<C>("abcd5e");
			s_arr[796] = nvobj::make_persistent<C>("abcd6");
			s_arr[797] = nvobj::make_persistent<C>("abcd67");
			s_arr[798] = nvobj::make_persistent<C>("abcd6789");
			s_arr[799] = nvobj::make_persistent<C>("abcd67890");
			s_arr[800] = nvobj::make_persistent<C>("abcd67890e");
			s_arr[801] = nvobj::make_persistent<C>("abcd6789e");
			s_arr[802] = nvobj::make_persistent<C>("abcd67e");
			s_arr[803] = nvobj::make_persistent<C>("abcd6e");
			s_arr[804] = nvobj::make_persistent<C>("abcde");
			s_arr[805] = nvobj::make_persistent<C>("abcde0");
			s_arr[806] = nvobj::make_persistent<C>("abcde0fghij");
			s_arr[807] = nvobj::make_persistent<C>("abcde0ghij");
			s_arr[808] = nvobj::make_persistent<C>("abcde0hij");
			s_arr[809] = nvobj::make_persistent<C>("abcde0j");
			s_arr[810] = nvobj::make_persistent<C>("abcde1");
			s_arr[811] = nvobj::make_persistent<C>("abcde12");
			s_arr[812] = nvobj::make_persistent<C>("abcde1234");
			s_arr[813] = nvobj::make_persistent<C>("abcde12345");
			s_arr[814] =
				nvobj::make_persistent<C>("abcde123456789");
			s_arr[815] =
				nvobj::make_persistent<C>("abcde1234567890");
			s_arr[816] = nvobj::make_persistent<C>(
				"abcde1234567890123456789");
			s_arr[817] = nvobj::make_persistent<C>(
				"abcde12345678901234567890");
			s_arr[818] = nvobj::make_persistent<C>(
				"abcde12345678901234567890fghij");
			s_arr[819] = nvobj::make_persistent<C>(
				"abcde12345678901234567890ghij");
			s_arr[820] = nvobj::make_persistent<C>(
				"abcde12345678901234567890hij");
			s_arr[821] = nvobj::make_persistent<C>(
				"abcde12345678901234567890j");
			s_arr[822] = nvobj::make_persistent<C>(
				"abcde1234567890123456789fghij");
			s_arr[823] = nvobj::make_persistent<C>(
				"abcde1234567890123456789ghij");
			s_arr[824] = nvobj::make_persistent<C>(
				"abcde1234567890123456789hij");
			s_arr[825] = nvobj::make_persistent<C>(
				"abcde1234567890123456789j");
			s_arr[826] = nvobj::make_persistent<C>(
				"abcde1234567890fghij");
			s_arr[827] = nvobj::make_persistent<C>(
				"abcde1234567890ghij");
			s_arr[828] =
				nvobj::make_persistent<C>("abcde1234567890hij");
			s_arr[829] =
				nvobj::make_persistent<C>("abcde1234567890j");
			s_arr[830] = nvobj::make_persistent<C>(
				"abcde123456789fghij");
			s_arr[831] =
				nvobj::make_persistent<C>("abcde123456789ghij");
			s_arr[832] =
				nvobj::make_persistent<C>("abcde123456789hij");
			s_arr[833] =
				nvobj::make_persistent<C>("abcde123456789j");
			s_arr[834] =
				nvobj::make_persistent<C>("abcde12345fghij");
			s_arr[835] =
				nvobj::make_persistent<C>("abcde12345ghij");
			s_arr[836] = nvobj::make_persistent<C>("abcde12345hij");
			s_arr[837] = nvobj::make_persistent<C>("abcde12345j");
			s_arr[838] =
				nvobj::make_persistent<C>("abcde1234fghij");
			s_arr[839] = nvobj::make_persistent<C>("abcde1234ghij");
			s_arr[840] = nvobj::make_persistent<C>("abcde1234hij");
			s_arr[841] = nvobj::make_persistent<C>("abcde1234j");
			s_arr[842] = nvobj::make_persistent<C>("abcde12fghij");
			s_arr[843] = nvobj::make_persistent<C>("abcde12ghij");
			s_arr[844] = nvobj::make_persistent<C>("abcde12hij");
			s_arr[845] = nvobj::make_persistent<C>("abcde12j");
			s_arr[846] = nvobj::make_persistent<C>("abcde1fghij");
			s_arr[847] = nvobj::make_persistent<C>("abcde1ghij");
			s_arr[848] = nvobj::make_persistent<C>("abcde1hij");
			s_arr[849] = nvobj::make_persistent<C>("abcde1j");
			s_arr[850] = nvobj::make_persistent<C>("abcde2");
			s_arr[851] = nvobj::make_persistent<C>("abcde23");
			s_arr[852] = nvobj::make_persistent<C>("abcde234");
			s_arr[853] = nvobj::make_persistent<C>("abcde2345");
			s_arr[854] = nvobj::make_persistent<C>("abcde23456789");
			s_arr[855] =
				nvobj::make_persistent<C>("abcde234567890");
			s_arr[856] = nvobj::make_persistent<C>(
				"abcde234567890123456789");
			s_arr[857] = nvobj::make_persistent<C>(
				"abcde2345678901234567890");
			s_arr[858] = nvobj::make_persistent<C>(
				"abcde2345678901234567890fghij");
			s_arr[859] = nvobj::make_persistent<C>(
				"abcde2345678901234567890ghij");
			s_arr[860] = nvobj::make_persistent<C>(
				"abcde2345678901234567890hij");
			s_arr[861] = nvobj::make_persistent<C>(
				"abcde2345678901234567890j");
			s_arr[862] = nvobj::make_persistent<C>(
				"abcde234567890123456789fghij");
			s_arr[863] = nvobj::make_persistent<C>(
				"abcde234567890123456789ghij");
			s_arr[864] = nvobj::make_persistent<C>(
				"abcde234567890123456789hij");
			s_arr[865] = nvobj::make_persistent<C>(
				"abcde234567890123456789j");
			s_arr[866] = nvobj::make_persistent<C>(
				"abcde234567890fghij");
			s_arr[867] =
				nvobj::make_persistent<C>("abcde234567890ghij");
			s_arr[868] =
				nvobj::make_persistent<C>("abcde234567890hij");
			s_arr[869] =
				nvobj::make_persistent<C>("abcde234567890j");
			s_arr[870] =
				nvobj::make_persistent<C>("abcde23456789fghij");
			s_arr[871] =
				nvobj::make_persistent<C>("abcde23456789ghij");
			s_arr[872] =
				nvobj::make_persistent<C>("abcde23456789hij");
			s_arr[873] =
				nvobj::make_persistent<C>("abcde23456789j");
			s_arr[874] =
				nvobj::make_persistent<C>("abcde2345fghij");
			s_arr[875] = nvobj::make_persistent<C>("abcde2345ghij");
			s_arr[876] = nvobj::make_persistent<C>("abcde2345hij");
			s_arr[877] = nvobj::make_persistent<C>("abcde2345j");
			s_arr[878] = nvobj::make_persistent<C>("abcde234fghij");
			s_arr[879] = nvobj::make_persistent<C>("abcde234ghij");
			s_arr[880] = nvobj::make_persistent<C>("abcde234hij");
			s_arr[881] = nvobj::make_persistent<C>("abcde234j");
			s_arr[882] = nvobj::make_persistent<C>("abcde23fghij");
			s_arr[883] = nvobj::make_persistent<C>("abcde23ghij");
			s_arr[884] = nvobj::make_persistent<C>("abcde23hij");
			s_arr[885] = nvobj::make_persistent<C>("abcde23j");
			s_arr[886] = nvobj::make_persistent<C>("abcde2fghij");
			s_arr[887] = nvobj::make_persistent<C>("abcde2ghij");
			s_arr[888] = nvobj::make_persistent<C>("abcde2hij");
			s_arr[889] = nvobj::make_persistent<C>("abcde2j");
			s_arr[890] = nvobj::make_persistent<C>("abcde3");
			s_arr[891] = nvobj::make_persistent<C>("abcde34");
			s_arr[892] = nvobj::make_persistent<C>("abcde345");
			s_arr[893] = nvobj::make_persistent<C>("abcde345fghij");
			s_arr[894] = nvobj::make_persistent<C>("abcde345ghij");
			s_arr[895] = nvobj::make_persistent<C>("abcde345hij");
			s_arr[896] = nvobj::make_persistent<C>("abcde345j");
			s_arr[897] = nvobj::make_persistent<C>("abcde34fghij");
			s_arr[898] = nvobj::make_persistent<C>("abcde34ghij");
			s_arr[899] = nvobj::make_persistent<C>("abcde34hij");
			s_arr[900] = nvobj::make_persistent<C>("abcde34j");
			s_arr[901] = nvobj::make_persistent<C>("abcde3fghij");
			s_arr[902] = nvobj::make_persistent<C>("abcde3ghij");
			s_arr[903] = nvobj::make_persistent<C>("abcde3hij");
			s_arr[904] = nvobj::make_persistent<C>("abcde3j");
			s_arr[905] = nvobj::make_persistent<C>("abcde5");
			s_arr[906] = nvobj::make_persistent<C>("abcde5fghij");
			s_arr[907] = nvobj::make_persistent<C>("abcde5ghij");
			s_arr[908] = nvobj::make_persistent<C>("abcde5hij");
			s_arr[909] = nvobj::make_persistent<C>("abcde5j");
			s_arr[910] = nvobj::make_persistent<C>("abcde6");
			s_arr[911] = nvobj::make_persistent<C>("abcde67");
			s_arr[912] = nvobj::make_persistent<C>("abcde6789");
			s_arr[913] = nvobj::make_persistent<C>("abcde67890");
			s_arr[914] =
				nvobj::make_persistent<C>("abcde67890fghij");
			s_arr[915] =
				nvobj::make_persistent<C>("abcde67890ghij");
			s_arr[916] = nvobj::make_persistent<C>("abcde67890hij");
			s_arr[917] = nvobj::make_persistent<C>("abcde67890j");
			s_arr[918] =
				nvobj::make_persistent<C>("abcde6789fghij");
			s_arr[919] = nvobj::make_persistent<C>("abcde6789ghij");
			s_arr[920] = nvobj::make_persistent<C>("abcde6789hij");
			s_arr[921] = nvobj::make_persistent<C>("abcde6789j");
			s_arr[922] = nvobj::make_persistent<C>("abcde67fghij");
			s_arr[923] = nvobj::make_persistent<C>("abcde67ghij");
			s_arr[924] = nvobj::make_persistent<C>("abcde67hij");
			s_arr[925] = nvobj::make_persistent<C>("abcde67j");
			s_arr[926] = nvobj::make_persistent<C>("abcde6fghij");
			s_arr[927] = nvobj::make_persistent<C>("abcde6ghij");
			s_arr[928] = nvobj::make_persistent<C>("abcde6hij");
			s_arr[929] = nvobj::make_persistent<C>("abcde6j");
			s_arr[930] = nvobj::make_persistent<C>("abcdefghi");
			s_arr[931] = nvobj::make_persistent<C>("abcdefghi0");
			s_arr[932] = nvobj::make_persistent<C>("abcdefghi0j");
			s_arr[933] = nvobj::make_persistent<C>("abcdefghi1");
			s_arr[934] = nvobj::make_persistent<C>("abcdefghi12");
			s_arr[935] = nvobj::make_persistent<C>("abcdefghi1234");
			s_arr[936] =
				nvobj::make_persistent<C>("abcdefghi12345");
			s_arr[937] =
				nvobj::make_persistent<C>("abcdefghi123456789");
			s_arr[938] = nvobj::make_persistent<C>(
				"abcdefghi1234567890");
			s_arr[939] = nvobj::make_persistent<C>(
				"abcdefghi1234567890123456789");
			s_arr[940] = nvobj::make_persistent<C>(
				"abcdefghi12345678901234567890");
			s_arr[941] = nvobj::make_persistent<C>(
				"abcdefghi12345678901234567890j");
			s_arr[942] = nvobj::make_persistent<C>(
				"abcdefghi1234567890123456789j");
			s_arr[943] = nvobj::make_persistent<C>(
				"abcdefghi1234567890j");
			s_arr[944] = nvobj::make_persistent<C>(
				"abcdefghi123456789j");
			s_arr[945] =
				nvobj::make_persistent<C>("abcdefghi12345j");
			s_arr[946] =
				nvobj::make_persistent<C>("abcdefghi1234j");
			s_arr[947] = nvobj::make_persistent<C>("abcdefghi12j");
			s_arr[948] = nvobj::make_persistent<C>("abcdefghi1j");
			s_arr[949] = nvobj::make_persistent<C>("abcdefghi2");
			s_arr[950] = nvobj::make_persistent<C>("abcdefghi23");
			s_arr[951] = nvobj::make_persistent<C>("abcdefghi234");
			s_arr[952] = nvobj::make_persistent<C>("abcdefghi2345");
			s_arr[953] =
				nvobj::make_persistent<C>("abcdefghi23456789");
			s_arr[954] =
				nvobj::make_persistent<C>("abcdefghi234567890");
			s_arr[955] = nvobj::make_persistent<C>(
				"abcdefghi234567890123456789");
			s_arr[956] = nvobj::make_persistent<C>(
				"abcdefghi2345678901234567890");
			s_arr[957] = nvobj::make_persistent<C>(
				"abcdefghi2345678901234567890j");
			s_arr[958] = nvobj::make_persistent<C>(
				"abcdefghi234567890123456789j");
			s_arr[959] = nvobj::make_persistent<C>(
				"abcdefghi234567890j");
			s_arr[960] =
				nvobj::make_persistent<C>("abcdefghi23456789j");
			s_arr[961] =
				nvobj::make_persistent<C>("abcdefghi2345j");
			s_arr[962] = nvobj::make_persistent<C>("abcdefghi234j");
			s_arr[963] = nvobj::make_persistent<C>("abcdefghi23j");
			s_arr[964] = nvobj::make_persistent<C>("abcdefghi2j");
			s_arr[965] = nvobj::make_persistent<C>("abcdefghi3");
			s_arr[966] = nvobj::make_persistent<C>("abcdefghi34");
			s_arr[967] = nvobj::make_persistent<C>("abcdefghi345");
			s_arr[968] = nvobj::make_persistent<C>("abcdefghi345j");
			s_arr[969] = nvobj::make_persistent<C>("abcdefghi34j");
			s_arr[970] = nvobj::make_persistent<C>("abcdefghi3j");
			s_arr[971] = nvobj::make_persistent<C>("abcdefghi5");
			s_arr[972] = nvobj::make_persistent<C>("abcdefghi5j");
			s_arr[973] = nvobj::make_persistent<C>("abcdefghi6");
			s_arr[974] = nvobj::make_persistent<C>("abcdefghi67");
			s_arr[975] = nvobj::make_persistent<C>("abcdefghi6789");
			s_arr[976] =
				nvobj::make_persistent<C>("abcdefghi67890");
			s_arr[977] =
				nvobj::make_persistent<C>("abcdefghi67890j");
			s_arr[978] =
				nvobj::make_persistent<C>("abcdefghi6789j");
			s_arr[979] = nvobj::make_persistent<C>("abcdefghi67j");
			s_arr[980] = nvobj::make_persistent<C>("abcdefghi6j");
			s_arr[981] = nvobj::make_persistent<C>("abcdefghij");
			s_arr[982] = nvobj::make_persistent<C>("abcdefghij0");
			s_arr[983] = nvobj::make_persistent<C>(
				"abcdefghij0klmnopqrst");
			s_arr[984] = nvobj::make_persistent<C>(
				"abcdefghij0lmnopqrst");
			s_arr[985] =
				nvobj::make_persistent<C>("abcdefghij0pqrst");
			s_arr[986] = nvobj::make_persistent<C>("abcdefghij0t");
			s_arr[987] = nvobj::make_persistent<C>("abcdefghij1");
			s_arr[988] = nvobj::make_persistent<C>("abcdefghij12");
			s_arr[989] =
				nvobj::make_persistent<C>("abcdefghij1234");
			s_arr[990] =
				nvobj::make_persistent<C>("abcdefghij12345");
			s_arr[991] = nvobj::make_persistent<C>(
				"abcdefghij123456789");
			s_arr[992] = nvobj::make_persistent<C>(
				"abcdefghij1234567890");
			s_arr[993] = nvobj::make_persistent<C>(
				"abcdefghij1234567890123456789");
			s_arr[994] = nvobj::make_persistent<C>(
				"abcdefghij12345678901234567890");
			s_arr[995] = nvobj::make_persistent<C>(
				"abcdefghij12345678901234567890klmnopqrst");
			s_arr[996] = nvobj::make_persistent<C>(
				"abcdefghij12345678901234567890lmnopqrst");
			s_arr[997] = nvobj::make_persistent<C>(
				"abcdefghij12345678901234567890pqrst");
			s_arr[998] = nvobj::make_persistent<C>(
				"abcdefghij12345678901234567890t");
			s_arr[999] = nvobj::make_persistent<C>(
				"abcdefghij1234567890123456789klmnopqrst");
			s_arr[1000] = nvobj::make_persistent<C>(
				"abcdefghij1234567890123456789lmnopqrst");
			s_arr[1001] = nvobj::make_persistent<C>(
				"abcdefghij1234567890123456789pqrst");
			s_arr[1002] = nvobj::make_persistent<C>(
				"abcdefghij1234567890123456789t");
			s_arr[1003] = nvobj::make_persistent<C>(
				"abcdefghij1234567890klmnopqrst");
			s_arr[1004] = nvobj::make_persistent<C>(
				"abcdefghij1234567890lmnopqrst");
			s_arr[1005] = nvobj::make_persistent<C>(
				"abcdefghij1234567890pqrst");
			s_arr[1006] = nvobj::make_persistent<C>(
				"abcdefghij1234567890t");
			s_arr[1007] = nvobj::make_persistent<C>(
				"abcdefghij123456789klmnopqrst");
			s_arr[1008] = nvobj::make_persistent<C>(
				"abcdefghij123456789lmnopqrst");
			s_arr[1009] = nvobj::make_persistent<C>(
				"abcdefghij123456789pqrst");
			s_arr[1010] = nvobj::make_persistent<C>(
				"abcdefghij123456789t");
			s_arr[1011] = nvobj::make_persistent<C>(
				"abcdefghij12345klmnopqrst");
			s_arr[1012] = nvobj::make_persistent<C>(
				"abcdefghij12345lmnopqrst");
			s_arr[1013] = nvobj::make_persistent<C>(
				"abcdefghij12345pqrst");
			s_arr[1014] =
				nvobj::make_persistent<C>("abcdefghij12345t");
			s_arr[1015] = nvobj::make_persistent<C>(
				"abcdefghij1234klmnopqrst");
			s_arr[1016] = nvobj::make_persistent<C>(
				"abcdefghij1234lmnopqrst");
			s_arr[1017] = nvobj::make_persistent<C>(
				"abcdefghij1234pqrst");
			s_arr[1018] =
				nvobj::make_persistent<C>("abcdefghij1234t");
			s_arr[1019] = nvobj::make_persistent<C>(
				"abcdefghij12klmnopqrst");
			s_arr[1020] = nvobj::make_persistent<C>(
				"abcdefghij12lmnopqrst");
			s_arr[1021] =
				nvobj::make_persistent<C>("abcdefghij12pqrst");
			s_arr[1022] =
				nvobj::make_persistent<C>("abcdefghij12t");
			s_arr[1023] = nvobj::make_persistent<C>(
				"abcdefghij1klmnopqrst");
			s_arr[1024] = nvobj::make_persistent<C>(
				"abcdefghij1lmnopqrst");
			s_arr[1025] =
				nvobj::make_persistent<C>("abcdefghij1pqrst");
			s_arr[1026] = nvobj::make_persistent<C>("abcdefghij1t");
			s_arr[1027] = nvobj::make_persistent<C>("abcdefghij2");
			s_arr[1028] = nvobj::make_persistent<C>("abcdefghij23");
			s_arr[1029] =
				nvobj::make_persistent<C>("abcdefghij234");
			s_arr[1030] =
				nvobj::make_persistent<C>("abcdefghij2345");
			s_arr[1031] =
				nvobj::make_persistent<C>("abcdefghij23456789");
			s_arr[1032] = nvobj::make_persistent<C>(
				"abcdefghij234567890");
			s_arr[1033] = nvobj::make_persistent<C>(
				"abcdefghij234567890123456789");
			s_arr[1034] = nvobj::make_persistent<C>(
				"abcdefghij2345678901234567890");
			s_arr[1035] = nvobj::make_persistent<C>(
				"abcdefghij2345678901234567890klmnopqrst");
			s_arr[1036] = nvobj::make_persistent<C>(
				"abcdefghij2345678901234567890lmnopqrst");
			s_arr[1037] = nvobj::make_persistent<C>(
				"abcdefghij2345678901234567890pqrst");
			s_arr[1038] = nvobj::make_persistent<C>(
				"abcdefghij2345678901234567890t");
			s_arr[1039] = nvobj::make_persistent<C>(
				"abcdefghij234567890123456789klmnopqrst");
			s_arr[1040] = nvobj::make_persistent<C>(
				"abcdefghij234567890123456789lmnopqrst");
			s_arr[1041] = nvobj::make_persistent<C>(
				"abcdefghij234567890123456789pqrst");
			s_arr[1042] = nvobj::make_persistent<C>(
				"abcdefghij234567890123456789t");
			s_arr[1043] = nvobj::make_persistent<C>(
				"abcdefghij234567890klmnopqrst");
			s_arr[1044] = nvobj::make_persistent<C>(
				"abcdefghij234567890lmnopqrst");
			s_arr[1045] = nvobj::make_persistent<C>(
				"abcdefghij234567890pqrst");
			s_arr[1046] = nvobj::make_persistent<C>(
				"abcdefghij234567890t");
			s_arr[1047] = nvobj::make_persistent<C>(
				"abcdefghij23456789klmnopqrst");
			s_arr[1048] = nvobj::make_persistent<C>(
				"abcdefghij23456789lmnopqrst");
			s_arr[1049] = nvobj::make_persistent<C>(
				"abcdefghij23456789pqrst");
			s_arr[1050] = nvobj::make_persistent<C>(
				"abcdefghij23456789t");
			s_arr[1051] = nvobj::make_persistent<C>(
				"abcdefghij2345klmnopqrst");
			s_arr[1052] = nvobj::make_persistent<C>(
				"abcdefghij2345lmnopqrst");
			s_arr[1053] = nvobj::make_persistent<C>(
				"abcdefghij2345pqrst");
			s_arr[1054] =
				nvobj::make_persistent<C>("abcdefghij2345t");
			s_arr[1055] = nvobj::make_persistent<C>(
				"abcdefghij234klmnopqrst");
			s_arr[1056] = nvobj::make_persistent<C>(
				"abcdefghij234lmnopqrst");
			s_arr[1057] =
				nvobj::make_persistent<C>("abcdefghij234pqrst");
			s_arr[1058] =
				nvobj::make_persistent<C>("abcdefghij234t");
			s_arr[1059] = nvobj::make_persistent<C>(
				"abcdefghij23klmnopqrst");
			s_arr[1060] = nvobj::make_persistent<C>(
				"abcdefghij23lmnopqrst");
			s_arr[1061] =
				nvobj::make_persistent<C>("abcdefghij23pqrst");
			s_arr[1062] =
				nvobj::make_persistent<C>("abcdefghij23t");
			s_arr[1063] = nvobj::make_persistent<C>(
				"abcdefghij2klmnopqrst");
			s_arr[1064] = nvobj::make_persistent<C>(
				"abcdefghij2lmnopqrst");
			s_arr[1065] =
				nvobj::make_persistent<C>("abcdefghij2pqrst");
			s_arr[1066] = nvobj::make_persistent<C>("abcdefghij2t");
			s_arr[1067] = nvobj::make_persistent<C>("abcdefghij3");
			s_arr[1068] = nvobj::make_persistent<C>("abcdefghij34");
			s_arr[1069] =
				nvobj::make_persistent<C>("abcdefghij345");
			s_arr[1070] = nvobj::make_persistent<C>(
				"abcdefghij345klmnopqrst");
			s_arr[1071] = nvobj::make_persistent<C>(
				"abcdefghij345lmnopqrst");
			s_arr[1072] =
				nvobj::make_persistent<C>("abcdefghij345pqrst");
			s_arr[1073] =
				nvobj::make_persistent<C>("abcdefghij345t");
			s_arr[1074] = nvobj::make_persistent<C>(
				"abcdefghij34klmnopqrst");
			s_arr[1075] = nvobj::make_persistent<C>(
				"abcdefghij34lmnopqrst");
			s_arr[1076] =
				nvobj::make_persistent<C>("abcdefghij34pqrst");
			s_arr[1077] =
				nvobj::make_persistent<C>("abcdefghij34t");
			s_arr[1078] = nvobj::make_persistent<C>(
				"abcdefghij3klmnopqrst");
			s_arr[1079] = nvobj::make_persistent<C>(
				"abcdefghij3lmnopqrst");
			s_arr[1080] =
				nvobj::make_persistent<C>("abcdefghij3pqrst");
			s_arr[1081] = nvobj::make_persistent<C>("abcdefghij3t");
			s_arr[1082] = nvobj::make_persistent<C>("abcdefghij5");
			s_arr[1083] = nvobj::make_persistent<C>(
				"abcdefghij5klmnopqrst");
			s_arr[1084] = nvobj::make_persistent<C>(
				"abcdefghij5lmnopqrst");
			s_arr[1085] =
				nvobj::make_persistent<C>("abcdefghij5pqrst");
			s_arr[1086] = nvobj::make_persistent<C>("abcdefghij5t");
			s_arr[1087] = nvobj::make_persistent<C>("abcdefghij6");
			s_arr[1088] = nvobj::make_persistent<C>("abcdefghij67");
			s_arr[1089] =
				nvobj::make_persistent<C>("abcdefghij6789");
			s_arr[1090] =
				nvobj::make_persistent<C>("abcdefghij67890");
			s_arr[1091] = nvobj::make_persistent<C>(
				"abcdefghij67890klmnopqrst");
			s_arr[1092] = nvobj::make_persistent<C>(
				"abcdefghij67890lmnopqrst");
			s_arr[1093] = nvobj::make_persistent<C>(
				"abcdefghij67890pqrst");
			s_arr[1094] =
				nvobj::make_persistent<C>("abcdefghij67890t");
			s_arr[1095] = nvobj::make_persistent<C>(
				"abcdefghij6789klmnopqrst");
			s_arr[1096] = nvobj::make_persistent<C>(
				"abcdefghij6789lmnopqrst");
			s_arr[1097] = nvobj::make_persistent<C>(
				"abcdefghij6789pqrst");
			s_arr[1098] =
				nvobj::make_persistent<C>("abcdefghij6789t");
			s_arr[1099] = nvobj::make_persistent<C>(
				"abcdefghij67klmnopqrst");
			s_arr[1100] = nvobj::make_persistent<C>(
				"abcdefghij67lmnopqrst");
			s_arr[1101] =
				nvobj::make_persistent<C>("abcdefghij67pqrst");
			s_arr[1102] =
				nvobj::make_persistent<C>("abcdefghij67t");
			s_arr[1103] = nvobj::make_persistent<C>(
				"abcdefghij6klmnopqrst");
			s_arr[1104] = nvobj::make_persistent<C>(
				"abcdefghij6lmnopqrst");
			s_arr[1105] =
				nvobj::make_persistent<C>("abcdefghij6pqrst");
			s_arr[1106] = nvobj::make_persistent<C>("abcdefghij6t");
			s_arr[1107] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs");
			s_arr[1108] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs0");
			s_arr[1109] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs0t");
			s_arr[1110] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs1");
			s_arr[1111] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs12");
			s_arr[1112] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs1234");
			s_arr[1113] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs12345");
			s_arr[1114] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs123456789");
			s_arr[1115] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs1234567890");
			s_arr[1116] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs1234567890123456789");
			s_arr[1117] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs12345678901234567890");
			s_arr[1118] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs12345678901234567890t");
			s_arr[1119] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs1234567890123456789t");
			s_arr[1120] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs1234567890t");
			s_arr[1121] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs123456789t");
			s_arr[1122] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs12345t");
			s_arr[1123] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs1234t");
			s_arr[1124] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs12t");
			s_arr[1125] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs1t");
			s_arr[1126] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs2");
			s_arr[1127] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs23");
			s_arr[1128] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs234");
			s_arr[1129] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs2345");
			s_arr[1130] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs23456789");
			s_arr[1131] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs234567890");
			s_arr[1132] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs234567890123456789");
			s_arr[1133] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs2345678901234567890");
			s_arr[1134] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs2345678901234567890t");
			s_arr[1135] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs234567890123456789t");
			s_arr[1136] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs234567890t");
			s_arr[1137] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs23456789t");
			s_arr[1138] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs2345t");
			s_arr[1139] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs234t");
			s_arr[1140] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs23t");
			s_arr[1141] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs2t");
			s_arr[1142] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs3");
			s_arr[1143] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs34");
			s_arr[1144] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs345");
			s_arr[1145] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs345t");
			s_arr[1146] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs34t");
			s_arr[1147] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs3t");
			s_arr[1148] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs5");
			s_arr[1149] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs5t");
			s_arr[1150] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs6");
			s_arr[1151] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs67");
			s_arr[1152] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs6789");
			s_arr[1153] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs67890");
			s_arr[1154] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs67890t");
			s_arr[1155] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs6789t");
			s_arr[1156] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs67t");
			s_arr[1157] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrs6t");
			s_arr[1158] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst");
			s_arr[1159] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst0");
			s_arr[1160] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst1");
			s_arr[1161] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst12");
			s_arr[1162] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst1234");
			s_arr[1163] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst12345");
			s_arr[1164] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst123456789");
			s_arr[1165] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst1234567890");
			s_arr[1166] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst1234567890123456789");
			s_arr[1167] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst12345678901234567890");
			s_arr[1168] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst2");
			s_arr[1169] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst23");
			s_arr[1170] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst234");
			s_arr[1171] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst2345");
			s_arr[1172] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst23456789");
			s_arr[1173] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst234567890");
			s_arr[1174] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst234567890123456789");
			s_arr[1175] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst2345678901234567890");
			s_arr[1176] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst3");
			s_arr[1177] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst34");
			s_arr[1178] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst345");
			s_arr[1179] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst5");
			s_arr[1180] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst6");
			s_arr[1181] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst67");
			s_arr[1182] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst6789");
			s_arr[1183] = nvobj::make_persistent<C>(
				"abcdefghijklmnopqrst67890");
			s_arr[1184] = nvobj::make_persistent<C>(
				"abcdefghijlmnopqrst");
			s_arr[1185] =
				nvobj::make_persistent<C>("abcdefghijpqrst");
			s_arr[1186] = nvobj::make_persistent<C>("abcdefghijt");
			s_arr[1187] = nvobj::make_persistent<C>("abcdeghij");
			s_arr[1188] = nvobj::make_persistent<C>("abcdehij");
			s_arr[1189] = nvobj::make_persistent<C>("abcdej");
			s_arr[1190] = nvobj::make_persistent<C>("abde");
			s_arr[1191] = nvobj::make_persistent<C>("abe");
			s_arr[1192] = nvobj::make_persistent<C>("acde");
			s_arr[1193] = nvobj::make_persistent<C>("acdefghij");
			s_arr[1194] = nvobj::make_persistent<C>(
				"acdefghijklmnopqrst");
			s_arr[1195] = nvobj::make_persistent<C>("ade");
			s_arr[1196] = nvobj::make_persistent<C>("ae");
			s_arr[1197] = nvobj::make_persistent<C>("afghij");
			s_arr[1198] = nvobj::make_persistent<C>("aj");
			s_arr[1199] = nvobj::make_persistent<C>("aklmnopqrst");
			s_arr[1200] = nvobj::make_persistent<C>("at");
			s_arr[1201] = nvobj::make_persistent<C>("bcde");
			s_arr[1202] = nvobj::make_persistent<C>("bcdefghij");
			s_arr[1203] = nvobj::make_persistent<C>(
				"bcdefghijklmnopqrst");
			s_arr[1204] = nvobj::make_persistent<C>("can't happen");
			s_arr[1205] = nvobj::make_persistent<C>("cde");
			s_arr[1206] = nvobj::make_persistent<C>("e");
			s_arr[1207] = nvobj::make_persistent<C>("fghij");
			s_arr[1208] = nvobj::make_persistent<C>("j");
			s_arr[1209] = nvobj::make_persistent<C>("klmnopqrst");
			s_arr[1210] = nvobj::make_persistent<C>("t");
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
		test12<C>(pop);
		test13<C>(pop);
		test14<C>(pop);
		test15<C>(pop);
		test16<C>(pop);
		test17<C>(pop);
		test18<C>(pop);
		test19<C>(pop);
		test20<C>(pop);
		test21<C>(pop);
		test22<C>(pop);
		test23<C>(pop);
		test24<C>(pop);
		test25<C>(pop);
		test26<C>(pop);
		test27<C>(pop);
		test28<C>(pop);
		test29<C>(pop);
		test30<C>(pop);
		test31<C>(pop);
		test32<C>(pop);
		test33<C>(pop);
		test34<C>(pop);
		test35<C>(pop);
		test36<C>(pop);
		test37<C>(pop);
		test38<C>(pop);
		test39<C>(pop);
		test40<C>(pop);
		test41<C>(pop);
		test42<C>(pop);
		test43<C>(pop);
		test44<C>(pop);
		test45<C>(pop);
		test46<C>(pop);
		test47<C>(pop);
		test48<C>(pop);
		test49<C>(pop);
		test50<C>(pop);
		test51<C>(pop);
		test52<C>(pop);
		test53<C>(pop);
		test54<C>(pop);
		test55<C>(pop);

		nvobj::transaction::run(pop, [&] {
			for (unsigned i = 0; i < 1211; ++i) {
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
