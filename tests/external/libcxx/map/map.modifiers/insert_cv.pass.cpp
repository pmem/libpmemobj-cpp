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

// pair<iterator, bool> insert(const value_type& v);

#include "../is_transparent.h"
#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using container = container_t<int, double, TRANSPARENT_COMPARE>;

struct root {
	nvobj::persistent_ptr<container> s;
};

template <class Container>
void
do_insert_cv_test(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	typedef Container M;
	typedef std::pair<typename M::iterator, bool> R;
	typedef typename M::value_type VT;
	pmem::obj::transaction::run(
		pop, [&] { robj->s = nvobj::make_persistent<M>(); });
	auto &m = *robj->s;

	const VT v1(2, 2.5);
	R r = m.insert(v1);
	UT_ASSERT(r.second);
	UT_ASSERT(r.first == m.begin());
	UT_ASSERT(m.size() == 1);
	UT_ASSERT(r.first->MAP_KEY == 2);
	UT_ASSERT(r.first->MAP_VALUE == 2.5);

	const VT v2(1, 1.5);
	r = m.insert(v2);
	UT_ASSERT(r.second);
	UT_ASSERT(r.first == m.begin());
	UT_ASSERT(m.size() == 2);
	UT_ASSERT(r.first->MAP_KEY == 1);
	UT_ASSERT(r.first->MAP_VALUE == 1.5);

	const VT v3(3, 3.5);
	r = m.insert(v3);
	UT_ASSERT(r.second);
	UT_ASSERT(m.size() == 3);
	UT_ASSERT(r.first->MAP_KEY == 3);
	UT_ASSERT(r.first->MAP_VALUE == 3.5);

	const VT v4(3, 4.5);
	r = m.insert(v4);
	UT_ASSERT(!r.second);
	UT_ASSERT(m.size() == 3);
	UT_ASSERT(r.first->MAP_KEY == 3);
	UT_ASSERT(r.first->MAP_VALUE == 3.5);
	pmem::obj::transaction::run(
		pop, [&] { nvobj::delete_persistent<M>(robj->s); });
}

int
run(pmem::obj::pool<root> &pop)
{
	do_insert_cv_test<container>(pop);
#ifdef XXX // XXX: Implement min_allocator class
	{
		typedef std::map<int, double, std::less<int>,
				 min_allocator<std::pair<const int, double>>>
			M;
		do_insert_cv_test<M>();
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
		pop = pmem::obj::pool<root>::create(path, "insert_cv.pass",
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
