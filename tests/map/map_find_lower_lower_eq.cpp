// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "../common/map_wrapper.hpp"
#include "../common/unittest.hpp"
#include "../container_generic/container_txabort.hpp"
#include "../external/libcxx/map/is_transparent.h"
#include "../external/libcxx/map/private_constructor.h"

#define LAYOUT "layout"

namespace nvobj = pmem::obj;

using map_type1 = container_t<int, nvobj::p<int>>;
using map_type2 =
	container_t<PrivateConstructor, nvobj::p<int>, transparent_less>;

const size_t TEST_ELEMENTS = 1024;

struct root {
	nvobj::persistent_ptr<map_type1> pptr1;
	nvobj::persistent_ptr<map_type2> pptr2;
};

/* Test lower() overloads */
template <bool IsConst>
void
test_find_lower(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	{
		using M = typename std::conditional<IsConst, const map_type1 &,
						    map_type1 &>::type;
		using It = typename std::conditional<IsConst,
						     map_type1::const_iterator,
						     map_type1::iterator>::type;

		nvobj::transaction::run(pop, [&] {
			using V = typename map_type1::value_type;
			V ar[] = {V(1, 1), V(2, 2), V(3, 3), V(4, 4),
				  V(6, 6), V(8, 8), V(9, 9)};
			r->pptr1 = nvobj::make_persistent<map_type1>(
				ar, ar + sizeof(ar) / sizeof(ar[0]));
		});

		It it;
		M m = *(r->pptr1);

		it = m.find_lower(0);
		UT_ASSERT(it == m.end());
		it = m.find_lower(1);
		UT_ASSERT(it == m.end());
		it = m.find_lower(2);
		UT_ASSERT(it == m.begin());
		it = m.find_lower(3);
		UT_ASSERT(it == std::next(m.begin(), 1));
		it = m.find_lower(4);
		UT_ASSERT(it == std::next(m.begin(), 2));
		it = m.find_lower(5);
		UT_ASSERT(it == std::next(m.begin(), 3));
		it = m.find_lower(6);
		UT_ASSERT(it == std::next(m.begin(), 3));
		it = m.find_lower(7);
		UT_ASSERT(it == std::next(m.begin(), 4));
		it = m.find_lower(8);
		UT_ASSERT(it == std::next(m.begin(), 4));
		it = m.find_lower(9);
		UT_ASSERT(it == std::next(m.begin(), 5));
		it = m.find_lower(10);
		UT_ASSERT(it == std::next(m.begin(), 6));

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<map_type1>(r->pptr1);
		});
	}

	{
		using M = typename std::conditional<IsConst, const map_type2 &,
						    map_type2 &>::type;
		using It = typename std::conditional<IsConst,
						     map_type2::const_iterator,
						     map_type2::iterator>::type;

		nvobj::transaction::run(pop, [&] {
			r->pptr2 = nvobj::make_persistent<map_type2>();
		});

		r->pptr2->insert({PrivateConstructor::make(1), 1});
		r->pptr2->insert({PrivateConstructor::make(2), 2});
		r->pptr2->insert({PrivateConstructor::make(3), 3});
		r->pptr2->insert({PrivateConstructor::make(4), 4});
		r->pptr2->insert({PrivateConstructor::make(6), 6});
		r->pptr2->insert({PrivateConstructor::make(8), 8});
		r->pptr2->insert({PrivateConstructor::make(9), 9});

		It it;
		M m = *(r->pptr2);

		it = m.find_lower(0);
		UT_ASSERT(it == m.end());
		it = m.find_lower(1);
		UT_ASSERT(it == m.end());
		it = m.find_lower(2);
		UT_ASSERT(it == m.begin());
		it = m.find_lower(3);
		UT_ASSERT(it == std::next(m.begin(), 1));
		it = m.find_lower(4);
		UT_ASSERT(it == std::next(m.begin(), 2));
		it = m.find_lower(5);
		UT_ASSERT(it == std::next(m.begin(), 3));
		it = m.find_lower(6);
		UT_ASSERT(it == std::next(m.begin(), 3));
		it = m.find_lower(7);
		UT_ASSERT(it == std::next(m.begin(), 4));
		it = m.find_lower(8);
		UT_ASSERT(it == std::next(m.begin(), 4));
		it = m.find_lower(9);
		UT_ASSERT(it == std::next(m.begin(), 5));
		it = m.find_lower(10);
		UT_ASSERT(it == std::next(m.begin(), 6));

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<map_type2>(r->pptr2);
		});
	}
}

/* Test lower_eq() overloads */
template <bool IsConst>
void
test_find_lower_eq(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	{
		using M = typename std::conditional<IsConst, const map_type1 &,
						    map_type1 &>::type;
		using It = typename std::conditional<IsConst,
						     map_type1::const_iterator,
						     map_type1::iterator>::type;

		nvobj::transaction::run(pop, [&] {
			using V = typename map_type1::value_type;
			V ar[] = {V(1, 1), V(2, 2), V(3, 3), V(4, 4),
				  V(6, 6), V(8, 8), V(9, 9)};
			r->pptr1 = nvobj::make_persistent<map_type1>(
				ar, ar + sizeof(ar) / sizeof(ar[0]));
		});

		It it;
		M m = *(r->pptr1);

		it = m.find_lower_eq(0);
		UT_ASSERT(it == m.end());
		it = m.find_lower_eq(1);
		UT_ASSERT(it == m.begin());
		it = m.find_lower_eq(2);
		UT_ASSERT(it == std::next(m.begin(), 1));
		it = m.find_lower_eq(3);
		UT_ASSERT(it == std::next(m.begin(), 2));
		it = m.find_lower_eq(4);
		UT_ASSERT(it == std::next(m.begin(), 3));
		it = m.find_lower_eq(5);
		UT_ASSERT(it == std::next(m.begin(), 3));
		it = m.find_lower_eq(6);
		UT_ASSERT(it == std::next(m.begin(), 4));
		it = m.find_lower_eq(7);
		UT_ASSERT(it == std::next(m.begin(), 4));
		it = m.find_lower_eq(8);
		UT_ASSERT(it == std::next(m.begin(), 5));
		it = m.find_lower_eq(9);
		UT_ASSERT(it == std::next(m.begin(), 6));
		it = m.find_lower_eq(10);
		UT_ASSERT(it == std::next(m.begin(), 6));

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<map_type1>(r->pptr1);
		});
	}

	{
		using M = typename std::conditional<IsConst, const map_type2 &,
						    map_type2 &>::type;
		using It = typename std::conditional<IsConst,
						     map_type2::const_iterator,
						     map_type2::iterator>::type;

		nvobj::transaction::run(pop, [&] {
			r->pptr2 = nvobj::make_persistent<map_type2>();
		});

		r->pptr2->insert({PrivateConstructor::make(1), 1});
		r->pptr2->insert({PrivateConstructor::make(2), 2});
		r->pptr2->insert({PrivateConstructor::make(3), 3});
		r->pptr2->insert({PrivateConstructor::make(4), 4});
		r->pptr2->insert({PrivateConstructor::make(6), 6});
		r->pptr2->insert({PrivateConstructor::make(8), 8});
		r->pptr2->insert({PrivateConstructor::make(9), 9});

		It it;
		M m = *(r->pptr2);

		it = m.find_lower_eq(0);
		UT_ASSERT(it == m.end());
		it = m.find_lower_eq(1);
		UT_ASSERT(it == m.begin());
		it = m.find_lower_eq(2);
		UT_ASSERT(it == std::next(m.begin(), 1));
		it = m.find_lower_eq(3);
		UT_ASSERT(it == std::next(m.begin(), 2));
		it = m.find_lower_eq(4);
		UT_ASSERT(it == std::next(m.begin(), 3));
		it = m.find_lower_eq(5);
		UT_ASSERT(it == std::next(m.begin(), 3));
		it = m.find_lower_eq(6);
		UT_ASSERT(it == std::next(m.begin(), 4));
		it = m.find_lower_eq(7);
		UT_ASSERT(it == std::next(m.begin(), 4));
		it = m.find_lower_eq(8);
		UT_ASSERT(it == std::next(m.begin(), 5));
		it = m.find_lower_eq(9);
		UT_ASSERT(it == std::next(m.begin(), 6));
		it = m.find_lower_eq(10);
		UT_ASSERT(it == std::next(m.begin(), 6));

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<map_type2>(r->pptr2);
		});
	}
}

/* Verify lower, lower_eq, higher, higher_eq, lower_bound and upper_bound
 * properties. */
template <bool IsConst>
void
test_properties(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	{
		using M = typename std::conditional<IsConst, const map_type1 &,
						    map_type1 &>::type;
		using It = typename std::conditional<IsConst,
						     map_type1::const_iterator,
						     map_type1::iterator>::type;

		nvobj::transaction::run(pop, [&] {
			r->pptr1 = nvobj::make_persistent<map_type1>();
		});

		It it;
		M m = *(r->pptr1);

		for (size_t i = 0; i < TEST_ELEMENTS; i++)
			r->pptr1->insert({3 * i, 3 * i});

		for (size_t i = 2; i < TEST_ELEMENTS * 3; i++) {
			auto lo = m.find_lower(i);
			auto lq = m.find_lower_eq(i);
			auto lb = m.lower_bound(i);
			auto ub = m.upper_bound(i);
			auto hi = m.find_higher(i);
			auto he = m.find_higher_eq(i);

			UT_ASSERT(he == lb);
			UT_ASSERT(hi == ub);

			if (ub != m.end()) {
				UT_ASSERT(lo->first < lb->first);
				UT_ASSERT(lq->first < ub->first);

				if (lb == ub)
					UT_ASSERT(lq->first == lo->first);
				else
					UT_ASSERT(lq->first > lo->first);
			}

			++lo;
			UT_ASSERT(lo == lb);

			++lq;
			UT_ASSERT(lq == ub);
		}

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<map_type1>(r->pptr1);
		});
	}
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<root>::create(
			path, LAYOUT, PMEMOBJ_MIN_POOL * 20, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_find_lower<true>(pop);
	test_find_lower<false>(pop);

	test_find_lower_eq<true>(pop);
	test_find_lower_eq<false>(pop);

	test_properties<true>(pop);
	test_properties<false>(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
