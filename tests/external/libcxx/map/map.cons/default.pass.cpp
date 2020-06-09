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

// map();

#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using CM = container_t<int, double>;

struct root {
	nvobj::persistent_ptr<CM> s;
};

int
run(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	{
		pmem::obj::transaction::run(
			pop, [&] { robj->s = nvobj::make_persistent<CM>(); });
		auto &m = *robj->s;
		UT_ASSERT(m.empty());
		UT_ASSERT(m.begin() == m.end());
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<CM>(robj->s); });
	}
#ifdef XXX // XXX: Implement min_allocator and explicit_allocator
	{
		std::map<int, double, std::less<int>,
			 min_allocator<std::pair<const int, double>>>
			m;
		UT_ASSERT(m.empty());
		UT_ASSERT(m.begin() == m.end());
	}
	{
		typedef explicit_allocator<std::pair<const int, double>> A;
		{
			std::map<int, double, std::less<int>, A> m;
			UT_ASSERT(m.empty());
			UT_ASSERT(m.begin() == m.end());
		}
		{
			A a;
			std::map<int, double, std::less<int>, A> m(a);
			UT_ASSERT(m.empty());
			UT_ASSERT(m.begin() == m.end());
		}
	}
	{
		std::map<int, double> m = {};
		UT_ASSERT(m.empty());
		UT_ASSERT(m.begin() == m.end());
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
		pop = pmem::obj::pool<root>::create(path, "default.pass",
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
