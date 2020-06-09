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

// UNSUPPORTED: c++98, c++03

// <map>

// class map

// map& operator=(initializer_list<value_type> il);

#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using C = container_t<int, double>;

struct root {
	nvobj::persistent_ptr<C> s;
};

template <typename T, typename T2>
bool
operator==(const T &a, const T2 &b)
{
	if (a.first == b.first && a.second == b.second)
		return true;
	return false;
}

void
test_basic(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	{
		typedef std::pair<const int, double> V;
		pmem::obj::transaction::run(pop, [&] {
			robj->s = nvobj::make_persistent<C>(
				std::initializer_list<typename C::value_type>{
					{20, 1}});
		});
		auto &m = *robj->s;
		m = {{1, 1}, {1, 1.5}, {1, 2},	 {2, 1}, {2, 1.5},
		     {2, 2}, {3, 1},   {3, 1.5}, {3, 2}};
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(distance(m.begin(), m.end()) == 3);
		UT_ASSERT(*m.begin() == V(1, 1));
		UT_ASSERT(*next(m.begin()) == V(2, 1));
		UT_ASSERT(*next(m.begin(), 2) == V(3, 1));
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C>(robj->s); });
	}
#ifdef XXX
	{ // XXX: implement min_allocator class
		typedef std::pair<const int, double> V;
		std::map<int, double, std::less<int>, min_allocator<V>> m = {
			{20, 1},
		};
		m = {{1, 1}, {1, 1.5}, {1, 2},	 {2, 1}, {2, 1.5},
		     {2, 2}, {3, 1},   {3, 1.5}, {3, 2}};
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(distance(m.begin(), m.end()) == 3);
		UT_ASSERT(*m.begin() == V(1, 1));
		UT_ASSERT(*next(m.begin()) == V(2, 1));
		UT_ASSERT(*next(m.begin(), 2) == V(3, 1));
	}
#endif
}

#ifdef XXX // XXX: implement test_allocator class
void
duplicate_keys_test(pmem::obj::pool<root> &pop)
{
	typedef std::map<int, int, std::less<int>,
			 test_allocator<std::pair<const int, int>>>
		Map;
	{
		LIBCPP_ASSERT(test_alloc_base::alloc_count == 0);
		Map s = {{1, 0}, {2, 0}, {3, 0}};
		LIBCPP_ASSERT(test_alloc_base::alloc_count == 3);
		s = {{4, 0}, {4, 0}, {4, 0}, {4, 0}};
		LIBCPP_ASSERT(test_alloc_base::alloc_count == 1);
		UT_ASSERT(s.size() == 1);
		UT_ASSERT(s.begin()->first == 4);
	}
	LIBCPP_ASSERT(test_alloc_base::alloc_count == 0);
}
#endif

int
run(pmem::obj::pool<root> &pop)
{
	test_basic(pop);
#ifdef XXX
	duplicate_keys_test(pop);
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
			path, "assign_initializer_list.pass", PMEMOBJ_MIN_POOL,
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
