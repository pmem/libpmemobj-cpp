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

#include "unittest.hpp"

#include <libpmemobj++/experimental/concurrent_map.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using CM = nvobjex::concurrent_map<int, double>;

struct root {
	nvobj::persistent_ptr<CM> s;
};

int
run(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	(void)robj;
	{
#ifdef XXX // Implement test_compare class
		typedef test_compare<std::less<int>> C;
		const std::map<int, double, C> m(C(3));
		assert(m.empty());
		assert(m.begin() == m.end());
		assert(m.key_comp() == C(3));
#endif
	}
#if TEST_STD_VER >= 11
	{
		typedef test_compare<std::less<int>> C;
		const std::map<int, double, C,
			       min_allocator<std::pair<const int, double>>>
			m(C(3));
		assert(m.empty());
		assert(m.begin() == m.end());
		assert(m.key_comp() == C(3));
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
