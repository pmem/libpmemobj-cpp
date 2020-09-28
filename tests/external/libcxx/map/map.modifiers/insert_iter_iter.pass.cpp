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

// template <class InputIterator>
//   void insert(InputIterator first, InputIterator last);

#include "../is_transparent.h"
#include "iterators_support.hpp"
#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using container = container_t<int, double, TRANSPARENT_COMPARE>;

struct root {
	nvobj::persistent_ptr<container> s;
};

int
run(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	{
		typedef container M;
		typedef container::value_type P;
		P ar[] = {
			P(1, 1), P(1, 1.5), P(1, 2),   P(2, 1), P(2, 1.5),
			P(2, 2), P(3, 1),   P(3, 1.5), P(3, 2),
		};
		pmem::obj::transaction::run(
			pop, [&] { robj->s = nvobj::make_persistent<M>(); });
		auto &m = *robj->s;
		m.insert(test_support::input_it<P *>(ar),
			 test_support::input_it<P *>(
				 ar + sizeof(ar) / sizeof(ar[0])));
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(m.begin()->MAP_KEY == 1);
		UT_ASSERT(m.begin()->MAP_VALUE == 1);
		UT_ASSERT(std::next(m.begin())->MAP_KEY == 2);
		UT_ASSERT(std::next(m.begin())->MAP_VALUE == 1);
		UT_ASSERT(std::next(m.begin(), 2)->MAP_KEY == 3);
		UT_ASSERT(std::next(m.begin(), 2)->MAP_VALUE == 1);
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s); });
	}
#ifdef XXX // XXX: Implement min_allocator
	{
		typedef std::map<int, double, std::less<int>,
				 min_allocator<std::pair<const int, double>>>
			M;
		typedef std::pair<int, double> P;
		P ar[] = {
			P(1, 1), P(1, 1.5), P(1, 2),   P(2, 1), P(2, 1.5),
			P(2, 2), P(3, 1),   P(3, 1.5), P(3, 2),
		};
		M m;
		m.insert(input_iterator<P *>(ar),
			 input_iterator<P *>(ar + sizeof(ar) / sizeof(ar[0])));
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(m.begin()->first == 1);
		UT_ASSERT(m.begin()->second == 1);
		UT_ASSERT(next(m.begin())->first == 2);
		UT_ASSERT(next(m.begin())->second == 1);
		UT_ASSERT(next(m.begin(), 2)->first == 3);
		UT_ASSERT(next(m.begin(), 2)->second == 1);
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
		pop = pmem::obj::pool<root>::create(
			path, "insert_iter_iter.pass", PMEMOBJ_MIN_POOL,
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
