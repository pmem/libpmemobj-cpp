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

// template<typename K>
//         pair<iterator,iterator>             equal_range(const K& x); // C++14
// template<typename K>
//         pair<const_iterator,const_iterator> equal_range(const K& x) const;
//         // C++14

#include "unittest.hpp"

#include <libpmemobj++/experimental/concurrent_map.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

struct Comp;
namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;
using C = nvobjex::concurrent_map<std::pair<int, int>, int, Comp>;

struct root {
	nvobj::persistent_ptr<C> s;
};

struct Comp {
	using is_transparent = void;

	bool
	operator()(const std::pair<int, int> &lhs,
		   const std::pair<int, int> &rhs) const
	{
		return lhs < rhs;
	}

	bool
	operator()(const std::pair<int, int> &lhs, int rhs) const
	{
		return lhs.first < rhs;
	}

	bool
	operator()(int lhs, const std::pair<int, int> &rhs) const
	{
		return lhs < rhs.first;
	}
};

int
run(pmem::obj::pool<root> &pop)
{
	auto r = pop.root();
	pmem::obj::transaction::run(pop, [&] {
		r->s = nvobj::make_persistent<C>(
			std::initializer_list<typename C::value_type>{
				{{2, 1}, 1},
				{{1, 2}, 2},
				{{1, 3}, 3},
				{{1, 4}, 4},
				{{2, 2}, 5}});
	});
	auto &s = *r->s;
	auto er = s.equal_range(1);
	long nels = 0;

	for (auto it = er.first; it != er.second; it++) {
		UT_ASSERTeq(it->first.first, 1);
		nels++;
	}

	UT_ASSERTeq(nels, 3);
	pmem::obj::transaction::run(pop,
				    [&] { nvobj::delete_persistent<C>(r->s); });
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
			path, "equal_range_transparent.pass", PMEMOBJ_MIN_POOL,
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
