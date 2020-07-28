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

// void clear() noexcept;

#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using container = container_t<int, double>;

struct root {
	nvobj::persistent_ptr<container> s;
};

int
run(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	{
		typedef container M;
		typedef std::pair<int, double> P;
		P ar[] = {
			P(1, 1.5), P(2, 2.5), P(3, 3.5), P(4, 4.5),
			P(5, 5.5), P(6, 6.5), P(7, 7.5), P(8, 8.5),
		};
		pmem::obj::transaction::run(pop, [&] {
			robj->s = nvobj::make_persistent<M>(
				ar, ar + sizeof(ar) / sizeof(ar[0]));
		});
		auto &m = *robj->s;
		UT_ASSERT(m.size() == 8);
		m.clear();
		UT_ASSERT(m.size() == 0);
		m.clear();
		UT_ASSERT(m.size() == 0);
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s); });
	}
#ifdef XXX // XXX: Implement min_allocator class
	{
		typedef std::map<int, double, std::less<int>,
				 min_allocator<std::pair<const int, double>>>
			M;
		typedef std::pair<int, double> P;
		P ar[] = {
			P(1, 1.5), P(2, 2.5), P(3, 3.5), P(4, 4.5),
			P(5, 5.5), P(6, 6.5), P(7, 7.5), P(8, 8.5),
		};
		pmem::obj::transaction::run(pop, [&] {
			robj->s = nvobj::make_persistent<M>(
				ar, ar + sizeof(ar) / sizeof(ar[0]));
		});
		auto &m = *robj->s;
		UT_ASSERT(m.size() == 8);
		UT_ASSERT_NOEXCEPT(m.clear());
		m.clear();
		UT_ASSERT(m.size() == 0);
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s); });
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
		pop = pmem::obj::pool<root>::create(path, "clear.pass",
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
