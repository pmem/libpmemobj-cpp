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

//       iterator upper_bound(const key_type& k);
// const_iterator upper_bound(const key_type& k) const;
//
//   The member function templates find, count, lower_bound, upper_bound, and
// equal_range shall not participate in overload resolution unless the
// qualified-id Compare::is_transparent is valid and denotes a type

#include "unittest.hpp"

#include "map_wrapper.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include "../is_transparent.h"

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using C = container_t<int, double, TRANSPARENT_COMPARE>;
using C2 = container_t<int, double, TRANSPARENT_COMPARE_NOT_REFERENCEABLE>;

struct root {
	nvobj::persistent_ptr<C> s;
	nvobj::persistent_ptr<C2> s2;
};

int
run(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	{
		typedef C M;
		pmem::obj::transaction::run(
			pop, [&] { robj->s = nvobj::make_persistent<M>(); });
		M &example = *robj->s;
		M::iterator result = example.upper_bound(C2Int{5});
		UT_ASSERT(result == example.end());
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s); });
	}
	{
		typedef C2 M;
		pmem::obj::transaction::run(
			pop, [&] { robj->s2 = nvobj::make_persistent<M>(); });
		M &example = *robj->s2;
		UT_ASSERT(example.upper_bound(C2Int{5}) == example.end());
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s2); });
	}
	{
		typedef C M;
		pmem::obj::transaction::run(
			pop, [&] { robj->s = nvobj::make_persistent<M>(); });
		M &example = *robj->s;
		M::const_iterator result = example.upper_bound(C2Int{5});
		UT_ASSERT(result == example.end());
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s); });
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
		pop = pmem::obj::pool<root>::create(path, "upper_bound.pass",
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
