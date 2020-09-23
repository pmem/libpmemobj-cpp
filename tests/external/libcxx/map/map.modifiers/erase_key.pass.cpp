//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Copyright 2020, Intel Corporation
//
// Modified to test pmem::obj containers
//

// <map>

// class map

// size_type erase(const key_type& k);

#include "../is_transparent.h"
#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using container = container_t<int, double, TRANSPARENT_COMPARE>;
using container2 =
	container_t<pmem::obj::string, double, TRANSPARENT_COMPARE_STRING>;

struct root {
	nvobj::persistent_ptr<container> s;
	nvobj::persistent_ptr<container2> s2;
};

int
run(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	{
		typedef container M;
		typedef M::value_type P;
		typedef M::size_type R;
		P ar[] = {
			P(1, 1.5), P(2, 2.5), P(3, 3.5), P(4, 4.5),
			P(5, 5.5), P(6, 6.5), P(7, 7.5), P(8, 8.5),
		};
		pmem::obj::transaction::run(pop, [&] {
			robj->s = nvobj::make_persistent<M>(
				ar, ar + sizeof(ar) / sizeof(ar[0]));
		});
		auto &m = *robj->s;
		UT_ASSERT(m.size() == 8);
		R s = erase(m, 9);
		UT_ASSERT(s == 0);
		UT_ASSERT(m.size() == 8);
		UT_ASSERT((*m.begin()).MAP_KEY == 1);
		UT_ASSERT((*m.begin()).MAP_VALUE == 1.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 2);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_KEY == 3);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 3.5);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_KEY == 4);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_VALUE == 4.5);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_VALUE == 5.5);
		UT_ASSERT((*std::next(m.begin(), 5)).MAP_KEY == 6);
		UT_ASSERT((*std::next(m.begin(), 5)).MAP_VALUE == 6.5);
		UT_ASSERT((*std::next(m.begin(), 6)).MAP_KEY == 7);
		UT_ASSERT((*std::next(m.begin(), 6)).MAP_VALUE == 7.5);
		UT_ASSERT((*std::next(m.begin(), 7)).MAP_KEY == 8);
		UT_ASSERT((*std::next(m.begin(), 7)).MAP_VALUE == 8.5);

		s = erase(m, 4);
		UT_ASSERT(m.size() == 7);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 1);
		UT_ASSERT((*m.begin()).MAP_VALUE == 1.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 2);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_KEY == 3);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 3.5);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_VALUE == 5.5);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_KEY == 6);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_VALUE == 6.5);
		UT_ASSERT((*std::next(m.begin(), 5)).MAP_KEY == 7);
		UT_ASSERT((*std::next(m.begin(), 5)).MAP_VALUE == 7.5);
		UT_ASSERT((*std::next(m.begin(), 6)).MAP_KEY == 8);
		UT_ASSERT((*std::next(m.begin(), 6)).MAP_VALUE == 8.5);

		s = erase(m, 1);
		UT_ASSERT(m.size() == 6);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 2);
		UT_ASSERT((*m.begin()).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 3);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 3.5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 5.5);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_KEY == 6);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_VALUE == 6.5);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_KEY == 7);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_VALUE == 7.5);
		UT_ASSERT((*std::next(m.begin(), 5)).MAP_KEY == 8);
		UT_ASSERT((*std::next(m.begin(), 5)).MAP_VALUE == 8.5);

		s = erase(m, 8);
		UT_ASSERT(m.size() == 5);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 2);
		UT_ASSERT((*m.begin()).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 3);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 3.5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 5.5);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_KEY == 6);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_VALUE == 6.5);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_KEY == 7);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_VALUE == 7.5);

		s = erase(m, 3);
		UT_ASSERT(m.size() == 4);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 2);
		UT_ASSERT((*m.begin()).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 5.5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_KEY == 6);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 6.5);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_KEY == 7);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_VALUE == 7.5);

		s = erase(m, 6);
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 2);
		UT_ASSERT((*m.begin()).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 5.5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_KEY == 7);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 7.5);

		s = erase(m, 7);
		UT_ASSERT(m.size() == 2);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 2);
		UT_ASSERT((*m.begin()).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 5.5);

		s = erase(m, 2);
		UT_ASSERT(m.size() == 1);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 5);
		UT_ASSERT((*m.begin()).MAP_VALUE == 5.5);

		s = erase(m, 5);
		UT_ASSERT(m.size() == 0);
		UT_ASSERT(s == 1);
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s); });
	}
	{ // erase(const K &k)
		typedef container M;
		typedef M::value_type P;
		typedef M::size_type R;
		P ar[] = {
			P(1, 1.5), P(2, 2.5), P(3, 3.5), P(4, 4.5),
			P(5, 5.5), P(6, 6.5), P(7, 7.5), P(8, 8.5),
		};
		pmem::obj::transaction::run(pop, [&] {
			robj->s = nvobj::make_persistent<M>(
				ar, ar + sizeof(ar) / sizeof(ar[0]));
		});
		auto &m = *robj->s;

		UT_ASSERT(m.size() == 8);
		R s = erase(m, C2Int{9});
		UT_ASSERT(s == 0);
		UT_ASSERT(m.size() == 8);
		UT_ASSERT((*m.begin()).MAP_KEY == 1);
		UT_ASSERT((*m.begin()).MAP_VALUE == 1.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 2);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_KEY == 3);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 3.5);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_KEY == 4);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_VALUE == 4.5);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_VALUE == 5.5);
		UT_ASSERT((*std::next(m.begin(), 5)).MAP_KEY == 6);
		UT_ASSERT((*std::next(m.begin(), 5)).MAP_VALUE == 6.5);
		UT_ASSERT((*std::next(m.begin(), 6)).MAP_KEY == 7);
		UT_ASSERT((*std::next(m.begin(), 6)).MAP_VALUE == 7.5);
		UT_ASSERT((*std::next(m.begin(), 7)).MAP_KEY == 8);
		UT_ASSERT((*std::next(m.begin(), 7)).MAP_VALUE == 8.5);

		s = erase(m, C2Int{4});
		UT_ASSERT(m.size() == 7);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 1);
		UT_ASSERT((*m.begin()).MAP_VALUE == 1.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 2);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_KEY == 3);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 3.5);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_VALUE == 5.5);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_KEY == 6);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_VALUE == 6.5);
		UT_ASSERT((*std::next(m.begin(), 5)).MAP_KEY == 7);
		UT_ASSERT((*std::next(m.begin(), 5)).MAP_VALUE == 7.5);
		UT_ASSERT((*std::next(m.begin(), 6)).MAP_KEY == 8);
		UT_ASSERT((*std::next(m.begin(), 6)).MAP_VALUE == 8.5);

		s = erase(m, C2Int{1});
		UT_ASSERT(m.size() == 6);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 2);
		UT_ASSERT((*m.begin()).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 3);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 3.5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 5.5);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_KEY == 6);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_VALUE == 6.5);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_KEY == 7);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_VALUE == 7.5);
		UT_ASSERT((*std::next(m.begin(), 5)).MAP_KEY == 8);
		UT_ASSERT((*std::next(m.begin(), 5)).MAP_VALUE == 8.5);

		s = erase(m, C2Int{8});
		UT_ASSERT(m.size() == 5);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 2);
		UT_ASSERT((*m.begin()).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 3);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 3.5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 5.5);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_KEY == 6);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_VALUE == 6.5);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_KEY == 7);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_VALUE == 7.5);

		s = erase(m, C2Int{3});
		UT_ASSERT(m.size() == 4);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 2);
		UT_ASSERT((*m.begin()).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 5.5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_KEY == 6);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 6.5);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_KEY == 7);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_VALUE == 7.5);

		s = erase(m, C2Int{6});
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 2);
		UT_ASSERT((*m.begin()).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 5.5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_KEY == 7);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 7.5);

		s = erase(m, C2Int{7});
		UT_ASSERT(m.size() == 2);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 2);
		UT_ASSERT((*m.begin()).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 5.5);

		s = erase(m, C2Int{2});
		UT_ASSERT(m.size() == 1);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 5);
		UT_ASSERT((*m.begin()).MAP_VALUE == 5.5);

		s = erase(m, C2Int{5});
		UT_ASSERT(m.size() == 0);
		UT_ASSERT(s == 1);
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s); });
	}
	{
		typedef container2 M;
		typedef M::size_type R;

		pmem::obj::transaction::run(
			pop, [&] { robj->s2 = nvobj::make_persistent<M>(); });
		auto &m = *robj->s2;

		for (size_t i = 0; i < 5; i++) {
			m.try_emplace(std::string(i, 'x'), i + 0.5);
		}
		UT_ASSERT(m.size() == 5);

		R s = erase(m, std::string(5, 'x'));
		UT_ASSERT(s == 0);
		UT_ASSERT(m.size() == 5);
		UT_ASSERT((*m.begin()).MAP_KEY.compare(std::string(0, 'x')) ==
			  0);
		UT_ASSERT((*m.begin()).MAP_VALUE == 0.5);
		UT_ASSERT((*std::next(m.begin()))
				  .MAP_KEY.compare(std::string(1, 'x')) == 0);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 1.5);
		UT_ASSERT((*std::next(m.begin(), 2))
				  .MAP_KEY.compare(std::string(2, 'x')) == 0);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin(), 3))
				  .MAP_KEY.compare(std::string(3, 'x')) == 0);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_VALUE == 3.5);
		UT_ASSERT((*std::next(m.begin(), 4))
				  .MAP_KEY.compare(std::string(4, 'x')) == 0);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_VALUE == 4.5);

		s = erase(m, std::string(0, 'x'));
		UT_ASSERT(s == 1);
		UT_ASSERT(m.size() == 4);
		UT_ASSERT((*m.begin()).MAP_KEY.compare(std::string(1, 'x')) ==
			  0);
		UT_ASSERT((*m.begin()).MAP_VALUE == 1.5);
		UT_ASSERT((*std::next(m.begin(), 1))
				  .MAP_KEY.compare(std::string(2, 'x')) == 0);
		UT_ASSERT((*std::next(m.begin(), 1)).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin(), 2))
				  .MAP_KEY.compare(std::string(3, 'x')) == 0);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 3.5);
		UT_ASSERT((*std::next(m.begin(), 3))
				  .MAP_KEY.compare(std::string(4, 'x')) == 0);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_VALUE == 4.5);

		s = erase(m, std::string(1, 'x'));
		UT_ASSERT(s == 1);
		UT_ASSERT(m.size() == 3);
		UT_ASSERT((*m.begin()).MAP_KEY.compare(std::string(2, 'x')) ==
			  0);
		UT_ASSERT((*m.begin()).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin(), 1))
				  .MAP_KEY.compare(std::string(3, 'x')) == 0);
		UT_ASSERT((*std::next(m.begin(), 1)).MAP_VALUE == 3.5);
		UT_ASSERT((*std::next(m.begin(), 2))
				  .MAP_KEY.compare(std::string(4, 'x')) == 0);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 4.5);

		s = erase(m, std::string(2, 'x'));
		UT_ASSERT(s == 1);
		UT_ASSERT(m.size() == 2);
		UT_ASSERT((*m.begin()).MAP_KEY.compare(std::string(3, 'x')) ==
			  0);
		UT_ASSERT((*m.begin()).MAP_VALUE == 3.5);
		UT_ASSERT((*std::next(m.begin(), 1))
				  .MAP_KEY.compare(std::string(4, 'x')) == 0);
		UT_ASSERT((*std::next(m.begin(), 1)).MAP_VALUE == 4.5);

		s = erase(m, std::string(3, 'x'));
		UT_ASSERT(s == 1);
		UT_ASSERT(m.size() == 1);
		UT_ASSERT((*m.begin()).MAP_KEY.compare(std::string(4, 'x')) ==
			  0);
		UT_ASSERT((*m.begin()).MAP_VALUE == 4.5);

		s = erase(m, std::string(4, 'x'));
		UT_ASSERT(s == 1);
		UT_ASSERT(m.size() == 0);

		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s2); });
	}
#ifdef XXX // XXX: Implement min_alocator and generic std:less
	{
		typedef std::map<int, double, std::less<int>,
				 min_allocator<std::pair<const int, double>>>
			M;
		typedef std::pair<int, double> P;
		typedef M::size_type R;
		P ar[] = {
			P(1, 1.5), P(2, 2.5), P(3, 3.5), P(4, 4.5),
			P(5, 5.5), P(6, 6.5), P(7, 7.5), P(8, 8.5),
		};
		M m(ar, ar + sizeof(ar) / sizeof(ar[0]));
		UT_ASSERT(m.size() == 8);
		R s = m.erase(9);
		UT_ASSERT(s == 0);
		UT_ASSERT(m.size() == 8);
		UT_ASSERT((*m.begin()).MAP_KEY == 1);
		UT_ASSERT((*m.begin()).MAP_VALUE == 1.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 2);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_KEY == 3);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 3.5);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_KEY == 4);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_VALUE == 4.5);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_VALUE == 5.5);
		UT_ASSERT((*std::next(m.begin(), 5)).MAP_KEY == 6);
		UT_ASSERT((*std::next(m.begin(), 5)).MAP_VALUE == 6.5);
		UT_ASSERT((*std::next(m.begin(), 6)).MAP_KEY == 7);
		UT_ASSERT((*std::next(m.begin(), 6)).MAP_VALUE == 7.5);
		UT_ASSERT((*std::next(m.begin(), 7)).MAP_KEY == 8);
		UT_ASSERT((*std::next(m.begin(), 7)).MAP_VALUE == 8.5);

		s = m.erase(4);
		UT_ASSERT(m.size() == 7);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 1);
		UT_ASSERT((*m.begin()).MAP_VALUE == 1.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 2);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_KEY == 3);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 3.5);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_VALUE == 5.5);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_KEY == 6);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_VALUE == 6.5);
		UT_ASSERT((*std::next(m.begin(), 5)).MAP_KEY == 7);
		UT_ASSERT((*std::next(m.begin(), 5)).MAP_VALUE == 7.5);
		UT_ASSERT((*std::next(m.begin(), 6)).MAP_KEY == 8);
		UT_ASSERT((*std::next(m.begin(), 6)).MAP_VALUE == 8.5);

		s = m.erase(1);
		UT_ASSERT(m.size() == 6);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 2);
		UT_ASSERT((*m.begin()).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 3);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 3.5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 5.5);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_KEY == 6);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_VALUE == 6.5);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_KEY == 7);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_VALUE == 7.5);
		UT_ASSERT((*std::next(m.begin(), 5)).MAP_KEY == 8);
		UT_ASSERT((*std::next(m.begin(), 5)).MAP_VALUE == 8.5);

		s = m.erase(8);
		UT_ASSERT(m.size() == 5);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 2);
		UT_ASSERT((*m.begin()).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 3);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 3.5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 5.5);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_KEY == 6);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_VALUE == 6.5);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_KEY == 7);
		UT_ASSERT((*std::next(m.begin(), 4)).MAP_VALUE == 7.5);

		s = m.erase(3);
		UT_ASSERT(m.size() == 4);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 2);
		UT_ASSERT((*m.begin()).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 5.5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_KEY == 6);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 6.5);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_KEY == 7);
		UT_ASSERT((*std::next(m.begin(), 3)).MAP_VALUE == 7.5);

		s = m.erase(6);
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 2);
		UT_ASSERT((*m.begin()).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 5.5);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_KEY == 7);
		UT_ASSERT((*std::next(m.begin(), 2)).MAP_VALUE == 7.5);

		s = m.erase(7);
		UT_ASSERT(m.size() == 2);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 2);
		UT_ASSERT((*m.begin()).MAP_VALUE == 2.5);
		UT_ASSERT((*std::next(m.begin())).MAP_KEY == 5);
		UT_ASSERT((*std::next(m.begin())).MAP_VALUE == 5.5);

		s = m.erase(2);
		UT_ASSERT(m.size() == 1);
		UT_ASSERT(s == 1);
		UT_ASSERT((*m.begin()).MAP_KEY == 5);
		UT_ASSERT((*m.begin()).MAP_VALUE == 5.5);

		s = m.erase(5);
		UT_ASSERT(m.size() == 0);
		UT_ASSERT(s == 1);
	}
#endif

	return 0;
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<root> pop;
	try {
		pop = pmem::obj::pool<root>::create(path, "erase_key.pass",
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}
	try {
		run(pop);
		pop.close();
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
