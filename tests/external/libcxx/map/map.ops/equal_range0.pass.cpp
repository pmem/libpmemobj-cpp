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

//       iterator find(const key_type& k);
// const_iterator find(const key_type& k) const;
//
//   The member function templates find, count, lower_bound, upper_bound, and
// equal_range shall not participate in overload resolution unless the
// qualified-id Compare::is_transparent is valid and denotes a type

#include "unittest.hpp"

#include <libpmemobj++/experimental/concurrent_map.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include "../is_transparent.h"

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using C = nvobjex::concurrent_map<int, double, transparent_less>;
using C1 = nvobjex::concurrent_map<int, double,
				   transparent_less_not_referenceable>;

struct root {
	nvobj::persistent_ptr<C> s;
	nvobj::persistent_ptr<C1> s1;
};

int
run(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	{
		typedef C M;
		typedef std::pair<typename M::iterator, typename M::iterator> P;
		pmem::obj::transaction::run(
			pop, [&] { robj->s = nvobj::make_persistent<C>(); });
		P result = robj->s->equal_range(C2Int{5});
		UT_ASSERT(result.first == result.second);
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s); });
	}
	{
		typedef C1 M;
		typedef std::pair<typename M::iterator, typename M::iterator> P;
		pmem::obj::transaction::run(
			pop, [&] { robj->s1 = nvobj::make_persistent<C1>(); });
		P result = robj->s1->equal_range(C2Int{5});
		UT_ASSERT(result.first == result.second);
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s1); });
	}

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
		pop = pmem::obj::pool<root>::create(path, "equal_range0.pass",
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
