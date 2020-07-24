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

// explicit map(const key_compare& comp);

// key_compare key_comp() const;

#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

template <class C>
class test_compare : private C {
	nvobj::p<int> data_;

public:
	explicit test_compare(int data = 0) : data_(data)
	{
	}

	typename C::result_type
	operator()(
		typename std::add_lvalue_reference<
			const typename C::first_argument_type>::type x,
		typename std::add_lvalue_reference<
			const typename C::second_argument_type>::type y) const
	{
		return C::operator()(x, y);
	}

	bool
	operator==(const test_compare &c) const
	{
		return data_ == c.data_;
	}
};

template <class C>
class non_const_compare {
	// operator() deliberately not marked as 'const'
	bool
	operator()(const C &x, const C &y)
	{
		return x < y;
	}
};

typedef test_compare<std::less<int>> C;
using CM = container_t<int, double, C>;

struct root {
	nvobj::persistent_ptr<CM> s;
};

int
run(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	{
		pmem::obj::transaction::run(pop, [&] {
			robj->s = nvobj::make_persistent<CM>(C(3));
		});
		auto &m = *robj->s;
		UT_ASSERT(m.empty());
		UT_ASSERT(m.begin() == m.end());
		UT_ASSERT(m.key_comp() == C(3));
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<CM>(robj->s); });
	}
#ifdef XXX // XXX: implement min_allocator
	{
		typedef test_compare<std::less<int>> C;
		const std::map<int, double, C,
			       min_allocator<std::pair<const int, double>>>
			m(C(3));
		UT_ASSERT(m.empty());
		UT_ASSERT(m.begin() == m.end());
		UT_ASSERT(m.key_comp() == C(3));
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
		pop = pmem::obj::pool<root>::create(path, "compare.pass",
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
