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

#include <iterator>

#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using container = container_t<int, double>;

struct root {
	nvobj::persistent_ptr<container> s;
};

#define DELETE_FUNCTION = delete
#define TEST_CONSTEXPR_CXX14

template <class It, class ItTraits = It>
class input_iterator {
	typedef std::iterator_traits<ItTraits> Traits;
	It it_;

	template <class U, class T>
	friend class input_iterator;

public:
	typedef std::input_iterator_tag iterator_category;
	typedef typename Traits::value_type value_type;
	typedef typename Traits::difference_type difference_type;
	typedef It pointer;
	typedef typename Traits::reference reference;

	TEST_CONSTEXPR_CXX14 It
	base() const
	{
		return it_;
	}

	TEST_CONSTEXPR_CXX14
	input_iterator() : it_()
	{
	}
	explicit TEST_CONSTEXPR_CXX14
	input_iterator(It it)
	    : it_(it)
	{
	}
	template <class U, class T>
	TEST_CONSTEXPR_CXX14
	input_iterator(const input_iterator<U, T> &u)
	    : it_(u.it_)
	{
	}

	TEST_CONSTEXPR_CXX14 reference operator*() const
	{
		return *it_;
	}
	TEST_CONSTEXPR_CXX14 pointer operator->() const
	{
		return it_;
	}

	TEST_CONSTEXPR_CXX14 input_iterator &
	operator++()
	{
		++it_;
		return *this;
	}
	TEST_CONSTEXPR_CXX14 input_iterator
	operator++(int)
	{
		input_iterator tmp(*this);
		++(*this);
		return tmp;
	}

	friend TEST_CONSTEXPR_CXX14 bool
	operator==(const input_iterator &x, const input_iterator &y)
	{
		return x.it_ == y.it_;
	}
	friend TEST_CONSTEXPR_CXX14 bool
	operator!=(const input_iterator &x, const input_iterator &y)
	{
		return !(x == y);
	}

	template <class T>
	void operator,(T const &) DELETE_FUNCTION;
};

int
run(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	{
		typedef container M;
		typedef std::pair<int, double> P;
		P ar[] = {
			P(1, 1), P(1, 1.5), P(1, 2),   P(2, 1), P(2, 1.5),
			P(2, 2), P(3, 1),   P(3, 1.5), P(3, 2),
		};
		pmem::obj::transaction::run(
			pop, [&] { robj->s = nvobj::make_persistent<M>(); });
		auto &m = *robj->s;
		m.insert(input_iterator<P *>(ar),
			 input_iterator<P *>(ar + sizeof(ar) / sizeof(ar[0])));
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(m.begin()->first == 1);
		UT_ASSERT(m.begin()->second == 1);
		UT_ASSERT(next(m.begin())->first == 2);
		UT_ASSERT(next(m.begin())->second == 1);
		UT_ASSERT(next(m.begin(), 2)->first == 3);
		UT_ASSERT(next(m.begin(), 2)->second == 1);
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
