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

// map(const key_compare& comp, const allocator_type& a);

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

int
run(pmem::obj::pool<root> &pop)
{
#ifdef XXX // XXX: Implement test_allocator, min_allocator, explicit_allocator
	{
		typedef test_compare<std::less<int>> C;
		typedef test_allocator<std::pair<const int, double>> A;
		std::map<int, double, C, A> m(C(4), A(5));
		UT_ASSERT(m.empty());
		UT_ASSERT(m.begin() == m.end());
		UT_ASSERT(m.key_comp() == C(4));
		UT_ASSERT(m.get_allocator() == A(5));
	}
	{
		typedef test_compare<std::less<int>> C;
		typedef min_allocator<std::pair<const int, double>> A;
		std::map<int, double, C, A> m(C(4), A());
		UT_ASSERT(m.empty());
		UT_ASSERT(m.begin() == m.end());
		UT_ASSERT(m.key_comp() == C(4));
		UT_ASSERT(m.get_allocator() == A());
	}
	{
		typedef test_compare<std::less<int>> C;
		typedef explicit_allocator<std::pair<const int, double>> A;
		std::map<int, double, C, A> m(C(4), A{});
		UT_ASSERT(m.empty());
		UT_ASSERT(m.begin() == m.end());
		UT_ASSERT(m.key_comp() == C(4));
		UT_ASSERT(m.get_allocator() == A{});
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
		pop = pmem::obj::pool<root>::create(path, "compare_alloc.pass",
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
