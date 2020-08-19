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
//

// <map>

// class map

//       iterator begin();
// const_iterator begin() const;
//       iterator end();
// const_iterator end()   const;
//
//       reverse_iterator rbegin();
// const_reverse_iterator rbegin() const;
//       reverse_iterator rend();
// const_reverse_iterator rend()   const;
//
// const_iterator         cbegin()  const;
// const_iterator         cend()    const;
// const_reverse_iterator crbegin() const;
// const_reverse_iterator crend()   const;

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include "map_wrapper.hpp"
#include "unittest.hpp"

namespace nvobj = pmem::obj;

using M = container_t<int, nvobj::p<double>>;

// XXX: currently it's not possible to have persistent_ptr<const M>
// see https://github.com/pmem/pmemkv/issues/728
struct const_M {
	template <typename... Args>
	const_M(Args &&... args) : m(std::forward<Args>(args)...)
	{
	}

	const M m;
};

struct root {
	nvobj::persistent_ptr<M> s1;
	nvobj::persistent_ptr<const_M> s2;
};

void
run(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();

	{
		typedef std::pair<const int, double> V;
		V ar[] = {V(1, 1),   V(1, 1.5), V(1, 2),   V(2, 1),   V(2, 1.5),
			  V(2, 2),   V(3, 1),	V(3, 1.5), V(3, 2),   V(4, 1),
			  V(4, 1.5), V(4, 2),	V(5, 1),   V(5, 1.5), V(5, 2),
			  V(6, 1),   V(6, 1.5), V(6, 2),   V(7, 1),   V(7, 1.5),
			  V(7, 2),   V(8, 1),	V(8, 1.5), V(8, 2)};
		pmem::obj::transaction::run(
			pop, [&] { robj->s1 = nvobj::make_persistent<M>(); });

		for (auto &e : ar)
			robj->s1->emplace(e);

		auto &m = *(robj->s1);

		UT_ASSERT(static_cast<std::size_t>(std::distance(
				  m.begin(), m.end())) == m.size());

		typename M::iterator i;
		i = m.begin();
		typename M::const_iterator k = i;
		UT_ASSERT(i == k);
		for (int j = 1; static_cast<std::size_t>(j) <= m.size();
		     ++j, ++i) {
			UT_ASSERT((*i).MAP_KEY == j);
			UT_ASSERT((*i).MAP_VALUE == 1);
			(*i).MAP_VALUE = 2.5;
			pop.persist((*i).MAP_VALUE);
			UT_ASSERT((*i).MAP_VALUE == 2.5);
		}

#ifndef LIBPMEMOBJ_CPP_TESTS_CONCURRENT_MAP // XXX: concurrent map does not
					    // support reverse_it
		UT_ASSERT(static_cast<std::size_t>(std::distance(
				  m.rbegin(), m.rend())) == m.size());

		typename M::reverse_iterator ri;
		ri = m.rbegin();
		for (int j = 1; static_cast<std::size_t>(j) <= m.size();
		     ++j, ++ri) {
			UT_ASSERT((*ri).MAP_KEY == (int)m.size() - j + 1);
			UT_ASSERT(ri->MAP_KEY == (int)m.size() - j + 1);
			UT_ASSERT((*ri).MAP_VALUE == 2.5);
			UT_ASSERT(ri->MAP_VALUE == 2.5);
			(*ri).MAP_VALUE = 3.5;
			pop.persist((*ri).MAP_VALUE);
			UT_ASSERT((*ri).MAP_VALUE == 3.5);
		}
#endif

		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s1); });
	}
	{
		typedef std::pair<const int, double> V;
		V ar[] = {V(1, 1),   V(1, 1.5), V(1, 2),   V(2, 1),   V(2, 1.5),
			  V(2, 2),   V(3, 1),	V(3, 1.5), V(3, 2),   V(4, 1),
			  V(4, 1.5), V(4, 2),	V(5, 1),   V(5, 1.5), V(5, 2),
			  V(6, 1),   V(6, 1.5), V(6, 2),   V(7, 1),   V(7, 1.5),
			  V(7, 2),   V(8, 1),	V(8, 1.5), V(8, 2)};
		pmem::obj::transaction::run(pop, [&] {
			robj->s2 = nvobj::make_persistent<const_M>(
				ar, ar + sizeof(ar) / sizeof(ar[0]));
		});

		const auto &m = robj->s2->m;
		UT_ASSERT(static_cast<std::size_t>(std::distance(
				  m.begin(), m.end())) == m.size());
		UT_ASSERT(static_cast<std::size_t>(std::distance(
				  m.cbegin(), m.cend())) == m.size());

		typename M::const_iterator i;
		i = m.begin();
		for (int j = 1; static_cast<std::size_t>(j) <= m.size();
		     ++j, ++i) {
			UT_ASSERT((*i).MAP_KEY == j);
			UT_ASSERT((*i).MAP_VALUE == 1);
		}

#ifndef LIBPMEMOBJ_CPP_TESTS_CONCURRENT_MAP // XXX: concurrent map does not
					    // support reverse_it
		UT_ASSERT(static_cast<std::size_t>(std::distance(
				  m.rbegin(), m.rend())) == m.size());
		UT_ASSERT(static_cast<std::size_t>(std::distance(
				  m.crbegin(), m.crend())) == m.size());

		typename M::const_reverse_iterator ri;
		ri = m.rbegin();
		for (int j = 1; static_cast<std::size_t>(j) <= m.size();
		     ++j, ++ri) {
			UT_ASSERT((*ri).MAP_KEY == (int)m.size() - j + 1);
			UT_ASSERT((*ri).MAP_VALUE == 1);
		}
#endif

		pmem::obj::transaction::run(pop, [&] {
			nvobj::delete_persistent<const_M>(robj->s2);
		});
	}
#if TEST_STD_VER >= 11 // XXX: implement min_allocator
	{
		typedef std::pair<const int, double> V;
		V ar[] = {V(1, 1),   V(1, 1.5), V(1, 2),   V(2, 1),   V(2, 1.5),
			  V(2, 2),   V(3, 1),	V(3, 1.5), V(3, 2),   V(4, 1),
			  V(4, 1.5), V(4, 2),	V(5, 1),   V(5, 1.5), V(5, 2),
			  V(6, 1),   V(6, 1.5), V(6, 2),   V(7, 1),   V(7, 1.5),
			  V(7, 2),   V(8, 1),	V(8, 1.5), V(8, 2)};
		std::map<int, double, std::less<int>, min_allocator<V>> m(
			ar, ar + sizeof(ar) / sizeof(ar[0]));
		UT_ASSERT(static_cast<std::size_t>(std::distance(
				  m.begin(), m.end())) == m.size());
		UT_ASSERT(static_cast<std::size_t>(std::distance(
				  m.rbegin(), m.rend())) == m.size());
		std::map<int, double, std::less<int>,
			 min_allocator<V>>::iterator i;
		i = m.begin();
		std::map<int, double, std::less<int>,
			 min_allocator<V>>::const_iterator k = i;
		UT_ASSERT(i == k);
		for (int j = 1; static_cast<std::size_t>(j) <= m.size();
		     ++j, ++i) {
			UT_ASSERT((*i).MAP_KEY == j);
			UT_ASSERT((*i).MAP_VALUE == 1);
			(*i).MAP_VALUE = 2.5;
			UT_ASSERT((*i).MAP_VALUE == 2.5);
		}
	}
	{
		typedef std::pair<const int, double> V;
		V ar[] = {V(1, 1),   V(1, 1.5), V(1, 2),   V(2, 1),   V(2, 1.5),
			  V(2, 2),   V(3, 1),	V(3, 1.5), V(3, 2),   V(4, 1),
			  V(4, 1.5), V(4, 2),	V(5, 1),   V(5, 1.5), V(5, 2),
			  V(6, 1),   V(6, 1.5), V(6, 2),   V(7, 1),   V(7, 1.5),
			  V(7, 2),   V(8, 1),	V(8, 1.5), V(8, 2)};
		const std::map<int, double, std::less<int>, min_allocator<V>> m(
			ar, ar + sizeof(ar) / sizeof(ar[0]));
		UT_ASSERT(static_cast<std::size_t>(std::distance(
				  m.begin(), m.end())) == m.size());
		UT_ASSERT(static_cast<std::size_t>(std::distance(
				  m.cbegin(), m.cend())) == m.size());
		UT_ASSERT(static_cast<std::size_t>(std::distance(
				  m.rbegin(), m.rend())) == m.size());
		UT_ASSERT(static_cast<std::size_t>(std::distance(
				  m.crbegin(), m.crend())) == m.size());
		std::map<int, double, std::less<int>,
			 min_allocator<V>>::const_iterator i;
		i = m.begin();
		for (int j = 1; static_cast<std::size_t>(j) <= m.size();
		     ++j, ++i) {
			UT_ASSERT((*i).MAP_KEY == j);
			UT_ASSERT((*i).MAP_VALUE == 1);
		}
	}
#endif
#if TEST_STD_VER > 11
	{ // N3644 testing
		typedef std::map<int, double> C;
		C::iterator ii1{}, ii2{};
		C::iterator ii4 = ii1;
		C::const_iterator cii{};
		UT_ASSERT(ii1 == ii2);
		UT_ASSERT(ii1 == ii4);

		UT_ASSERT(!(ii1 != ii2));

		UT_ASSERT((ii1 == cii));
		UT_ASSERT((cii == ii1));
		UT_ASSERT(!(ii1 != cii));
		UT_ASSERT(!(cii != ii1));
	}
#endif

	/* Test bidirectional iterator requirements. */
	{
		typedef std::pair<const int, double> V;
		V ar[] = {V(1, 1), V(2, 1), V(3, 1), V(4, 1),
			  V(5, 1), V(6, 1), V(7, 1), V(8, 1)};
		pmem::obj::transaction::run(pop, [&] {
			robj->s2 = nvobj::make_persistent<const_M>(
				ar, ar + sizeof(ar) / sizeof(ar[0]));
		});

		const auto &m = robj->s2->m;

		/* Test ++a */
		auto it = m.begin();
#ifndef LIBPMEMOBJ_CPP_TESTS_CONCURRENT_MAP // XXX: concurrent map it--
		UT_ASSERT((*(++it)).MAP_KEY == 2);
		it = m.begin();
		UT_ASSERT((++it)->MAP_KEY == 2);
		UT_ASSERT(++(--m.end()) == m.end());
#endif
		it = m.begin();
		auto it2 = ++it;
		UT_ASSERT(it == it2);

		/* Test a++ */
		it = m.begin();
		it2 = it++;
		UT_ASSERT(it2 == m.begin());
#ifndef LIBPMEMOBJ_CPP_TESTS_CONCURRENT_MAP // XXX: concurrent map it--
		UT_ASSERT(--it == it2);
#endif
		it = ++m.begin();
		UT_ASSERT((*it++).MAP_KEY == 2);
		UT_ASSERT((*it).MAP_KEY == 3);

#ifndef LIBPMEMOBJ_CPP_TESTS_CONCURRENT_MAP // XXX: concurrent map it--
		/* Test --a */
		UT_ASSERT((*(--m.end())).MAP_KEY == 8);
		UT_ASSERT(--(++m.begin()) == m.begin());
		it = m.end();
		it2 = --it;
		UT_ASSERT(it == it2);

		/* Test a-- */
		it = m.end();
		it2 = it--;
		UT_ASSERT(it2 == m.end());
		UT_ASSERT(++it == it2);
		it = --m.end();
		UT_ASSERT((*it--).MAP_KEY == 8);
		UT_ASSERT((*it).MAP_KEY == 7);
#endif

		pmem::obj::transaction::run(pop, [&] {
			nvobj::delete_persistent<const_M>(robj->s2);
		});
	}
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<root> pop;
	try {
		pop = pmem::obj::pool<root>::create(path, "iterator.pass",
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
