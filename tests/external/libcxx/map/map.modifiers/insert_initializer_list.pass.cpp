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

// void insert(initializer_list<value_type> il);

#include "../is_transparent.h"
#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using container = container_t<int, double, TRANSPARENT_COMPARE>;

struct root {
	nvobj::persistent_ptr<container> s;
};

int
run(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	{
		typedef container::value_type V;

		pmem::obj::transaction::run(pop, [&] {
			robj->s = nvobj::make_persistent<container>(
				std::initializer_list<V>({{1, 1},
							  {1, 1.5},
							  {1, 2},
							  {3, 1},
							  {3, 1.5},
							  {3, 2}}));
		});
		auto &m = *robj->s;
		m.insert({
			{2, 1},
			{2, 1.5},
			{2, 2},
		});
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(std::distance(m.begin(), m.end()) == 3);
		UT_ASSERT(m.begin()->MAP_KEY == 1);
		UT_ASSERT(m.begin()->MAP_VALUE == 1);
		UT_ASSERT(std::next(m.begin())->MAP_KEY == 2);
		UT_ASSERT(std::next(m.begin())->MAP_VALUE == 1);
		UT_ASSERT(std::next(m.begin(), 2)->MAP_KEY == 3);
		UT_ASSERT(std::next(m.begin(), 2)->MAP_VALUE == 1);
		pmem::obj::transaction::run(pop, [&] {
			nvobj::delete_persistent<container>(robj->s);
		});
	}
#ifdef XXX // XXX: Implement min_allocator
	{
		typedef std::pair<const int, double> V;
		std::map<int, double, std::less<int>, min_allocator<V>> m = {
			{1, 1}, {1, 1.5}, {1, 2}, {3, 1}, {3, 1.5}, {3, 2}};
		m.insert({
			{2, 1},
			{2, 1.5},
			{2, 2},
		});
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(distance(m.begin(), m.end()) == 3);
		UT_ASSERT(*m.begin() == V(1, 1));
		UT_ASSERT(*next(m.begin()) == V(2, 1));
		UT_ASSERT(*next(m.begin(), 2) == V(3, 1));
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
			path, "insert_initializer_list.pass", PMEMOBJ_MIN_POOL,
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
