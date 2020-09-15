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

// #include <map>
// #include <cassert>

#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/experimental/concurrent_map.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include "../is_transparent.h"
#include "../private_constructor.h"

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using Type_A = container_t<int, double, TRANSPARENT_COMPARE>;
using Type_B = container_t<int, double, TRANSPARENT_COMPARE_NOT_REFERENCEABLE>;

struct root {
	nvobj::persistent_ptr<Type_A> s;
	nvobj::persistent_ptr<Type_B> s2;
};

int
run(pmem::obj::pool<root> &pop)
{
	{
		auto r = pop.root();
		nvobj::transaction::run(pop, [&] {
			r->s = nvobj::make_persistent<Type_A>();
			UT_ASSERTeq(r->s->count(C2Int{5}), 0);
			nvobj::delete_persistent<Type_A>(r->s);
		});
	}
	{
		auto r = pop.root();
		nvobj::transaction::run(pop, [&] {
			r->s2 = nvobj::make_persistent<Type_B>();
			UT_ASSERTeq(r->s2->count(C2Int{5}), 0);
			nvobj::delete_persistent<Type_B>(r->s2);
		});
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
		pop = pmem::obj::pool<root>::create(path, "count0.pass",
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
