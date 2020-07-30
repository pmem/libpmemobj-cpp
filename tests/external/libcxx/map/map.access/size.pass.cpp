//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <map>

// class map

// size_type size() const;

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
		pmem::obj::transaction::run(
			pop, [&] { robj->s = nvobj::make_persistent<M>(); });
		auto &m = *robj->s;
		UT_ASSERT(m.size() == 0);
		m.insert(M::value_type(2, 1.5));
		UT_ASSERT(m.size() == 1);
		m.insert(M::value_type(1, 1.5));
		UT_ASSERT(m.size() == 2);
		m.insert(M::value_type(3, 1.5));
		UT_ASSERT(m.size() == 3);
		erase(m, m.begin());
		UT_ASSERT(m.size() == 2);
		erase(m, m.begin());
		UT_ASSERT(m.size() == 1);
		erase(m, m.begin());
		UT_ASSERT(m.size() == 0);
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s); });
	}
#ifdef XXX // XXX: Implement min_allocator
	{
		typedef std::map<int, double, std::less<int>,
				 min_allocator<std::pair<const int, double>>>
			M;
		M m;
		UT_ASSERT(m.size() == 0);
		m.insert(M::value_type(2, 1.5));
		UT_ASSERT(m.size() == 1);
		m.insert(M::value_type(1, 1.5));
		UT_ASSERT(m.size() == 2);
		m.insert(M::value_type(3, 1.5));
		UT_ASSERT(m.size() == 3);
		m.erase(m.begin());
		UT_ASSERT(m.size() == 2);
		m.erase(m.begin());
		UT_ASSERT(m.size() == 1);
		m.erase(m.begin());
		UT_ASSERT(m.size() == 0);
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
		pop = pmem::obj::pool<root>::create(
			path, "size.pass", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
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
