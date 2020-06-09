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

// map& operator=(const map& m);

#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using CM = container_t<int, double>;

struct root {
	nvobj::persistent_ptr<CM> s;
};

static std::vector<int> ca_allocs;
static std::vector<int> ca_deallocs;

template <class T>
class counting_allocatorT {
public:
	typedef T value_type;
	int foo{0};
	counting_allocatorT(int f) noexcept : foo(f)
	{
	}

	using propagate_on_container_copy_assignment = std::true_type;
	template <class U>
	counting_allocatorT(const counting_allocatorT<U> &other) noexcept
	{
		foo = other.foo;
	}
	template <class U>
	bool
	operator==(const counting_allocatorT<U> &other) const noexcept
	{
		return foo == other.foo;
	}
	template <class U>
	bool
	operator!=(const counting_allocatorT<U> &other) const noexcept
	{
		return foo != other.foo;
	}

	T *
	allocate(const size_t n) const
	{
		ca_allocs.push_back(foo);
		void *const pv = ::malloc(n * sizeof(T));
		return static_cast<T *>(pv);
	}
	void
	deallocate(T *const p, size_t) const noexcept
	{
		ca_deallocs.push_back(foo);
		free(p);
	}
};

template <class T>
class counting_allocatorF {
public:
	typedef T value_type;
	int foo{0};
	counting_allocatorF(int f) noexcept : foo(f)
	{
	}

	using propagate_on_container_copy_assignment = std::false_type;
	template <class U>
	counting_allocatorF(const counting_allocatorF<U> &other) noexcept
	{
		foo = other.foo;
	}
	template <class U>
	bool
	operator==(const counting_allocatorF<U> &other) const noexcept
	{
		return foo == other.foo;
	}
	template <class U>
	bool
	operator!=(const counting_allocatorF<U> &other) const noexcept
	{
		return foo != other.foo;
	}

	T *
	allocate(const size_t n) const
	{
		ca_allocs.push_back(foo);
		void *const pv = ::malloc(n * sizeof(T));
		return static_cast<T *>(pv);
	}
	void
	deallocate(T *const p, size_t) const noexcept
	{
		ca_deallocs.push_back(foo);
		free(p);
	}
};

bool
balanced_allocs()
{
	std::vector<int> temp1, temp2;

	std::cout << "Allocations = " << ca_allocs.size()
		  << ", deallocatons = " << ca_deallocs.size() << std::endl;
	if (ca_allocs.size() != ca_deallocs.size())
		return false;

	temp1 = ca_allocs;
	std::sort(temp1.begin(), temp1.end());
	temp2.clear();
	std::unique_copy(temp1.begin(), temp1.end(),
			 std::back_inserter<std::vector<int>>(temp2));
	std::cout << "There were " << temp2.size() << " different allocators\n";

	for (std::vector<int>::const_iterator it = temp2.begin();
	     it != temp2.end(); ++it) {
		std::cout << *it << ": "
			  << std::count(ca_allocs.begin(), ca_allocs.end(), *it)
			  << " vs "
			  << std::count(ca_deallocs.begin(), ca_deallocs.end(),
					*it)
			  << std::endl;
		if (std::count(ca_allocs.begin(), ca_allocs.end(), *it) !=
		    std::count(ca_deallocs.begin(), ca_deallocs.end(), *it))
			return false;
	}

	temp1 = ca_allocs;
	std::sort(temp1.begin(), temp1.end());
	temp2.clear();
	std::unique_copy(temp1.begin(), temp1.end(),
			 std::back_inserter<std::vector<int>>(temp2));
	std::cout << "There were " << temp2.size()
		  << " different (de)allocators\n";
	for (std::vector<int>::const_iterator it = ca_deallocs.begin();
	     it != ca_deallocs.end(); ++it) {
		std::cout << *it << ": "
			  << std::count(ca_allocs.begin(), ca_allocs.end(), *it)
			  << " vs "
			  << std::count(ca_deallocs.begin(), ca_deallocs.end(),
					*it)
			  << std::endl;
		if (std::count(ca_allocs.begin(), ca_allocs.end(), *it) !=
		    std::count(ca_deallocs.begin(), ca_deallocs.end(), *it))
			return false;
	}

	return true;
}

int
run(pmem::obj::pool<root> &pop)
{
#ifdef XXX // Implement test_compare and test_allocator
	{
		typedef std::pair<const int, double> V;
		V ar[] = {V(1, 1), V(1, 1.5), V(1, 2),	 V(2, 1), V(2, 1.5),
			  V(2, 2), V(3, 1),   V(3, 1.5), V(3, 2)};
		typedef test_compare<std::less<int>> C;
		typedef test_allocator<V> A;
		std::map<int, double, C, A> mo(
			ar, ar + sizeof(ar) / sizeof(ar[0]), C(5), A(2));
		std::map<int, double, C, A> m(
			ar, ar + sizeof(ar) / sizeof(ar[0]) / 2, C(3), A(7));
		m = mo;
		UT_ASSERT(m.get_allocator() == A(7));
		UT_ASSERT(m.key_comp() == C(5));
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(distance(m.begin(), m.end()) == 3);
		UT_ASSERT(*m.begin() == V(1, 1));
		UT_ASSERT(*next(m.begin()) == V(2, 1));
		UT_ASSERT(*next(m.begin(), 2) == V(3, 1));

		UT_ASSERT(mo.get_allocator() == A(2));
		UT_ASSERT(mo.key_comp() == C(5));
		UT_ASSERT(mo.size() == 3);
		UT_ASSERT(distance(mo.begin(), mo.end()) == 3);
		UT_ASSERT(*mo.begin() == V(1, 1));
		UT_ASSERT(*next(mo.begin()) == V(2, 1));
		UT_ASSERT(*next(mo.begin(), 2) == V(3, 1));
	}
	{
		typedef std::pair<const int, double> V;
		const V ar[] = {
			V(1, 1),
			V(2, 1),
			V(3, 1),
		};
		std::map<int, double> m(ar, ar + sizeof(ar) / sizeof(ar[0]));
		std::map<int, double> *p = &m;
		m = *p;

		UT_ASSERT(m.size() == 3);
		UT_ASSERT(std::equal(m.begin(), m.end(), ar));
	}
	{
		typedef std::pair<const int, double> V;
		V ar[] = {V(1, 1), V(1, 1.5), V(1, 2),	 V(2, 1), V(2, 1.5),
			  V(2, 2), V(3, 1),   V(3, 1.5), V(3, 2)};
		typedef test_compare<std::less<int>> C;
		typedef other_allocator<V> A;
		std::map<int, double, C, A> mo(
			ar, ar + sizeof(ar) / sizeof(ar[0]), C(5), A(2));
		std::map<int, double, C, A> m(
			ar, ar + sizeof(ar) / sizeof(ar[0]) / 2, C(3), A(7));
		m = mo;
		UT_ASSERT(m.get_allocator() == A(2));
		UT_ASSERT(m.key_comp() == C(5));
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(distance(m.begin(), m.end()) == 3);
		UT_ASSERT(*m.begin() == V(1, 1));
		UT_ASSERT(*next(m.begin()) == V(2, 1));
		UT_ASSERT(*next(m.begin(), 2) == V(3, 1));

		UT_ASSERT(mo.get_allocator() == A(2));
		UT_ASSERT(mo.key_comp() == C(5));
		UT_ASSERT(mo.size() == 3);
		UT_ASSERT(distance(mo.begin(), mo.end()) == 3);
		UT_ASSERT(*mo.begin() == V(1, 1));
		UT_ASSERT(*next(mo.begin()) == V(2, 1));
		UT_ASSERT(*next(mo.begin(), 2) == V(3, 1));
	}
#endif
#ifdef XXX
	{
		typedef std::pair<const int, double> V;
		V ar[] = {V(1, 1), V(1, 1.5), V(1, 2),	 V(2, 1), V(2, 1.5),
			  V(2, 2), V(3, 1),   V(3, 1.5), V(3, 2)};
		typedef test_compare<std::less<int>> C;
		typedef min_allocator<V> A;
		std::map<int, double, C, A> mo(
			ar, ar + sizeof(ar) / sizeof(ar[0]), C(5), A());
		std::map<int, double, C, A> m(
			ar, ar + sizeof(ar) / sizeof(ar[0]) / 2, C(3), A());
		m = mo;
		UT_ASSERT(m.get_allocator() == A());
		UT_ASSERT(m.key_comp() == C(5));
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(distance(m.begin(), m.end()) == 3);
		UT_ASSERT(*m.begin() == V(1, 1));
		UT_ASSERT(*next(m.begin()) == V(2, 1));
		UT_ASSERT(*next(m.begin(), 2) == V(3, 1));

		UT_ASSERT(mo.get_allocator() == A());
		UT_ASSERT(mo.key_comp() == C(5));
		UT_ASSERT(mo.size() == 3);
		UT_ASSERT(distance(mo.begin(), mo.end()) == 3);
		UT_ASSERT(*mo.begin() == V(1, 1));
		UT_ASSERT(*next(mo.begin()) == V(2, 1));
		UT_ASSERT(*next(mo.begin(), 2) == V(3, 1));
	}
	{
		typedef std::pair<const int, double> V;
		V ar[] = {V(1, 1), V(1, 1.5), V(1, 2),	 V(2, 1), V(2, 1.5),
			  V(2, 2), V(3, 1),   V(3, 1.5), V(3, 2)};
		typedef test_compare<std::less<int>> C;
		typedef min_allocator<V> A;
		std::map<int, double, C, A> mo(
			ar, ar + sizeof(ar) / sizeof(ar[0]), C(5), A());
		std::map<int, double, C, A> m(
			ar, ar + sizeof(ar) / sizeof(ar[0]) / 2, C(3), A());
		m = mo;
		UT_ASSERT(m.get_allocator() == A());
		UT_ASSERT(m.key_comp() == C(5));
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(distance(m.begin(), m.end()) == 3);
		UT_ASSERT(*m.begin() == V(1, 1));
		UT_ASSERT(*next(m.begin()) == V(2, 1));
		UT_ASSERT(*next(m.begin(), 2) == V(3, 1));

		UT_ASSERT(mo.get_allocator() == A());
		UT_ASSERT(mo.key_comp() == C(5));
		UT_ASSERT(mo.size() == 3);
		UT_ASSERT(distance(mo.begin(), mo.end()) == 3);
		UT_ASSERT(*mo.begin() == V(1, 1));
		UT_ASSERT(*next(mo.begin()) == V(2, 1));
		UT_ASSERT(*next(mo.begin(), 2) == V(3, 1));
	}

	UT_ASSERT(balanced_allocs());
	{
		typedef std::pair<const int, double> V;
		V ar[] = {V(1, 1), V(1, 1.5), V(1, 2),	 V(2, 1), V(2, 1.5),
			  V(2, 2), V(3, 1),   V(3, 1.5), V(3, 2)};
		typedef test_compare<std::less<int>> C;
		typedef counting_allocatorT<V> A;
		std::map<int, double, C, A> mo(
			ar, ar + sizeof(ar) / sizeof(ar[0]), C(5), A(1));
		std::map<int, double, C, A> m(
			ar, ar + sizeof(ar) / sizeof(ar[0]) / 2, C(3), A(2));
		m = mo;
		UT_ASSERT(m.key_comp() == C(5));
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(distance(m.begin(), m.end()) == 3);
		UT_ASSERT(*m.begin() == V(1, 1));
		UT_ASSERT(*next(m.begin()) == V(2, 1));
		UT_ASSERT(*next(m.begin(), 2) == V(3, 1));

		UT_ASSERT(mo.key_comp() == C(5));
		UT_ASSERT(mo.size() == 3);
		UT_ASSERT(distance(mo.begin(), mo.end()) == 3);
		UT_ASSERT(*mo.begin() == V(1, 1));
		UT_ASSERT(*next(mo.begin()) == V(2, 1));
		UT_ASSERT(*next(mo.begin(), 2) == V(3, 1));
	}
	UT_ASSERT(balanced_allocs());
	{
		typedef std::pair<const int, double> V;
		V ar[] = {V(1, 1), V(1, 1.5), V(1, 2),	 V(2, 1), V(2, 1.5),
			  V(2, 2), V(3, 1),   V(3, 1.5), V(3, 2)};
		typedef test_compare<std::less<int>> C;
		typedef counting_allocatorF<V> A;
		std::map<int, double, C, A> mo(
			ar, ar + sizeof(ar) / sizeof(ar[0]), C(5), A(100));
		std::map<int, double, C, A> m(
			ar, ar + sizeof(ar) / sizeof(ar[0]) / 2, C(3), A(200));
		m = mo;
		UT_ASSERT(m.key_comp() == C(5));
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(distance(m.begin(), m.end()) == 3);
		UT_ASSERT(*m.begin() == V(1, 1));
		UT_ASSERT(*next(m.begin()) == V(2, 1));
		UT_ASSERT(*next(m.begin(), 2) == V(3, 1));

		UT_ASSERT(mo.key_comp() == C(5));
		UT_ASSERT(mo.size() == 3);
		UT_ASSERT(distance(mo.begin(), mo.end()) == 3);
		UT_ASSERT(*mo.begin() == V(1, 1));
		UT_ASSERT(*next(mo.begin()) == V(2, 1));
		UT_ASSERT(*next(mo.begin(), 2) == V(3, 1));
	}
	UT_ASSERT(balanced_allocs());
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
		pop = pmem::obj::pool<root>::create(path, "copy_assign.pass",
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
