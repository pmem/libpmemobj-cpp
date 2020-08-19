//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// Copyright 2020, Intel Corporation
//
// Modified to test pmem::obj containers

// <map>

// class map

// void swap(map& m);

#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using container = container_t<int, double>;

struct root {
	nvobj::persistent_ptr<container> m1;
	nvobj::persistent_ptr<container> m2;
	nvobj::persistent_ptr<container> m1_save;
	nvobj::persistent_ptr<container> m2_save;
};

template <typename C>
void
containers_eq(const C &c1, const C &c2)
{
#if XXX // implement operator==
	UT_ASSERT(c1 == c2);
#else
	UT_ASSERTeq(c1.size(), c2.size());
	for (const auto &e : c1) {
		auto it = c2.find(e.MAP_KEY);

		UT_ASSERT(it != c2.end());
		UT_ASSERT(it->MAP_KEY == e.MAP_KEY);
		UT_ASSERT(it->MAP_VALUE == e.MAP_VALUE);
	}
#endif
}

void
run(nvobj::pool<root> pop)
{
	auto robj = pop.root();

	typedef std::pair<const int, double> V;
	{
		typedef container M;
		{
			pmem::obj::transaction::run(pop, [&] {
				robj->m1 = nvobj::make_persistent<M>();
				robj->m2 = nvobj::make_persistent<M>();
				robj->m1_save =
					nvobj::make_persistent<M>(*robj->m1);
				robj->m2_save =
					nvobj::make_persistent<M>(*robj->m2);
			});

			robj->m1->swap(*robj->m2);

			containers_eq(*robj->m1, *robj->m2_save);
			containers_eq(*robj->m2, *robj->m1_save);

			pmem::obj::transaction::run(pop, [&] {
				nvobj::delete_persistent<M>(robj->m1);
				nvobj::delete_persistent<M>(robj->m2);
				nvobj::delete_persistent<M>(robj->m1_save);
				nvobj::delete_persistent<M>(robj->m2_save);
			});
		}
		{
			V ar2[] = {V(5, 5), V(6, 6),   V(7, 7),	  V(8, 8),
				   V(9, 9), V(10, 10), V(11, 11), V(12, 12)};

			pmem::obj::transaction::run(pop, [&] {
				robj->m1 = nvobj::make_persistent<M>();
				robj->m2 = nvobj::make_persistent<M>(
					ar2,
					ar2 + sizeof(ar2) / sizeof(ar2[0]));
				robj->m1_save =
					nvobj::make_persistent<M>(*robj->m1);
				robj->m2_save =
					nvobj::make_persistent<M>(*robj->m2);
			});

			robj->m1->swap(*robj->m2);

			containers_eq(*robj->m1, *robj->m2_save);
			containers_eq(*robj->m2, *robj->m1_save);

			pmem::obj::transaction::run(pop, [&] {
				nvobj::delete_persistent<M>(robj->m1);
				nvobj::delete_persistent<M>(robj->m2);
				nvobj::delete_persistent<M>(robj->m1_save);
				nvobj::delete_persistent<M>(robj->m2_save);
			});
		}
		{
			V ar1[] = {V(1, 1), V(2, 2), V(3, 3), V(4, 4)};

			pmem::obj::transaction::run(pop, [&] {
				robj->m1 = nvobj::make_persistent<M>(
					ar1,
					ar1 + sizeof(ar1) / sizeof(ar1[0]));
				robj->m2 = nvobj::make_persistent<M>();
				robj->m1_save =
					nvobj::make_persistent<M>(*robj->m1);
				robj->m2_save =
					nvobj::make_persistent<M>(*robj->m2);
			});

			robj->m1->swap(*robj->m2);

			containers_eq(*robj->m1, *robj->m2_save);
			containers_eq(*robj->m2, *robj->m1_save);

			pmem::obj::transaction::run(pop, [&] {
				nvobj::delete_persistent<M>(robj->m1);
				nvobj::delete_persistent<M>(robj->m2);
				nvobj::delete_persistent<M>(robj->m1_save);
				nvobj::delete_persistent<M>(robj->m2_save);
			});
		}
		{
			V ar1[] = {V(1, 1), V(2, 2), V(3, 3), V(4, 4)};
			V ar2[] = {V(5, 5), V(6, 6),   V(7, 7),	  V(8, 8),
				   V(9, 9), V(10, 10), V(11, 11), V(12, 12)};

			pmem::obj::transaction::run(pop, [&] {
				robj->m1 = nvobj::make_persistent<M>(
					ar1,
					ar1 + sizeof(ar1) / sizeof(ar1[0]));
				robj->m2 = nvobj::make_persistent<M>(
					ar2,
					ar2 + sizeof(ar2) / sizeof(ar2[0]));
				robj->m1_save =
					nvobj::make_persistent<M>(*robj->m1);
				robj->m2_save =
					nvobj::make_persistent<M>(*robj->m2);
			});

			std::unordered_map<int, typename M::iterator> its_1;
			std::unordered_map<int, typename M::iterator> its_2;

			for (auto it = robj->m1->begin(); it != robj->m1->end();
			     ++it)
				its_1.emplace(it->MAP_KEY, it);

			for (auto it = robj->m2->begin(); it != robj->m2->end();
			     ++it)
				its_2.emplace(it->MAP_KEY, it);

			robj->m1->swap(*robj->m2);

			for (auto &e : its_1) {
				auto m_it = e.second;
				UT_ASSERT(e.first == m_it->MAP_KEY);
				UT_ASSERT(e.first == m_it->MAP_VALUE);
			}

			for (auto &e : its_2) {
				auto m_it = e.second;
				UT_ASSERT(e.first == m_it->MAP_KEY);
				UT_ASSERT(e.first == m_it->MAP_VALUE);
			}

			containers_eq(*robj->m1, *robj->m2_save);
			containers_eq(*robj->m2, *robj->m1_save);

			pmem::obj::transaction::run(pop, [&] {
				nvobj::delete_persistent<M>(robj->m1);
				nvobj::delete_persistent<M>(robj->m2);
				nvobj::delete_persistent<M>(robj->m1_save);
				nvobj::delete_persistent<M>(robj->m2_save);
			});
		}
	}
#if XXX // implement min_allocator
	{
		typedef std::map<int, double, std::less<int>, min_allocator<V>>
			M;
		{
			M m1;
			M m2;
			M m1_save = m1;
			M m2_save = m2;
			m1.swap(m2);
			assert(m1 == m2_save);
			assert(m2 == m1_save);
		}
		{
			V ar2[] = {V(5, 5), V(6, 6),   V(7, 7),	  V(8, 8),
				   V(9, 9), V(10, 10), V(11, 11), V(12, 12)};
			M m1;
			M m2(ar2, ar2 + sizeof(ar2) / sizeof(ar2[0]));
			M m1_save = m1;
			M m2_save = m2;
			m1.swap(m2);
			assert(m1 == m2_save);
			assert(m2 == m1_save);
		}
		{
			V ar1[] = {V(1, 1), V(2, 2), V(3, 3), V(4, 4)};
			M m1(ar1, ar1 + sizeof(ar1) / sizeof(ar1[0]));
			M m2;
			M m1_save = m1;
			M m2_save = m2;
			m1.swap(m2);
			assert(m1 == m2_save);
			assert(m2 == m1_save);
		}
		{
			V ar1[] = {V(1, 1), V(2, 2), V(3, 3), V(4, 4)};
			V ar2[] = {V(5, 5), V(6, 6),   V(7, 7),	  V(8, 8),
				   V(9, 9), V(10, 10), V(11, 11), V(12, 12)};
			M m1(ar1, ar1 + sizeof(ar1) / sizeof(ar1[0]));
			M m2(ar2, ar2 + sizeof(ar2) / sizeof(ar2[0]));
			M m1_save = m1;
			M m2_save = m2;
			m1.swap(m2);
			assert(m1 == m2_save);
			assert(m2 == m1_save);
		}
	}
#endif
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<root> pop;
	try {
		pop = pmem::obj::pool<root>::create(path, "member_swap.pass",
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
