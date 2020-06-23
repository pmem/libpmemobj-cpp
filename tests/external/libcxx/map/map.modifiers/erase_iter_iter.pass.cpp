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

// iterator erase(const_iterator first, const_iterator last);

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
		typedef M::value_type P;
		typedef M::iterator I;
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
		I i = erase(m, m.cbegin(), m.cbegin());
		UT_ASSERT(m.size() == 8);
		UT_ASSERT(i == m.begin());
		UT_ASSERT(m.begin()->first == 1);
		UT_ASSERT(m.begin()->second == 1.5);
		UT_ASSERT(std::next(m.begin())->first == 2);
		UT_ASSERT(std::next(m.begin())->second == 2.5);
		UT_ASSERT(std::next(m.begin(), 2)->first == 3);
		UT_ASSERT(std::next(m.begin(), 2)->second == 3.5);
		UT_ASSERT(std::next(m.begin(), 3)->first == 4);
		UT_ASSERT(std::next(m.begin(), 3)->second == 4.5);
		UT_ASSERT(std::next(m.begin(), 4)->first == 5);
		UT_ASSERT(std::next(m.begin(), 4)->second == 5.5);
		UT_ASSERT(std::next(m.begin(), 5)->first == 6);
		UT_ASSERT(std::next(m.begin(), 5)->second == 6.5);
		UT_ASSERT(std::next(m.begin(), 6)->first == 7);
		UT_ASSERT(std::next(m.begin(), 6)->second == 7.5);
		UT_ASSERT(std::next(m.begin(), 7)->first == 8);
		UT_ASSERT(std::next(m.begin(), 7)->second == 8.5);

		i = erase(m, m.cbegin(), std::next(m.cbegin(), 2));
		UT_ASSERT(m.size() == 6);
		UT_ASSERT(i == m.begin());
		UT_ASSERT(std::next(m.begin(), 0)->first == 3);
		UT_ASSERT(std::next(m.begin(), 0)->second == 3.5);
		UT_ASSERT(std::next(m.begin(), 1)->first == 4);
		UT_ASSERT(std::next(m.begin(), 1)->second == 4.5);
		UT_ASSERT(std::next(m.begin(), 2)->first == 5);
		UT_ASSERT(std::next(m.begin(), 2)->second == 5.5);
		UT_ASSERT(std::next(m.begin(), 3)->first == 6);
		UT_ASSERT(std::next(m.begin(), 3)->second == 6.5);
		UT_ASSERT(std::next(m.begin(), 4)->first == 7);
		UT_ASSERT(std::next(m.begin(), 4)->second == 7.5);
		UT_ASSERT(std::next(m.begin(), 5)->first == 8);
		UT_ASSERT(std::next(m.begin(), 5)->second == 8.5);

		i = erase(m, std::next(m.cbegin(), 2),
			  std::next(m.cbegin(), 6));
		UT_ASSERT(m.size() == 2);
		UT_ASSERT(i == std::next(m.begin(), 2));
		UT_ASSERT(std::next(m.begin(), 0)->first == 3);
		UT_ASSERT(std::next(m.begin(), 0)->second == 3.5);
		UT_ASSERT(std::next(m.begin(), 1)->first == 4);
		UT_ASSERT(std::next(m.begin(), 1)->second == 4.5);

		i = erase(m, m.cbegin(), m.cend());
		UT_ASSERT(m.size() == 0);
		UT_ASSERT(i == m.begin());
		UT_ASSERT(i == m.end());
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s); });
	}
#ifdef XXX // XXX: implement min_allocator
	{
		typedef std::map<int, double, std::less<int>,
				 min_allocator<std::pair<const int, double>>>
			M;
		typedef std::pair<int, double> P;
		typedef M::iterator I;
		P ar[] = {
			P(1, 1.5), P(2, 2.5), P(3, 3.5), P(4, 4.5),
			P(5, 5.5), P(6, 6.5), P(7, 7.5), P(8, 8.5),
		};
		M m(ar, ar + sizeof(ar) / sizeof(ar[0]));
		UT_ASSERT(m.size() == 8);
		I i = erase(m, m.cbegin(), m.cbegin());
		UT_ASSERT(m.size() == 8);
		UT_ASSERT(i == m.begin());
		UT_ASSERT(m.begin()->first == 1);
		UT_ASSERT(m.begin()->second == 1.5);
		UT_ASSERT(std::next(m.begin())->first == 2);
		UT_ASSERT(std::next(m.begin())->second == 2.5);
		UT_ASSERT(std::next(m.begin(), 2)->first == 3);
		UT_ASSERT(std::next(m.begin(), 2)->second == 3.5);
		UT_ASSERT(std::next(m.begin(), 3)->first == 4);
		UT_ASSERT(std::next(m.begin(), 3)->second == 4.5);
		UT_ASSERT(std::next(m.begin(), 4)->first == 5);
		UT_ASSERT(std::next(m.begin(), 4)->second == 5.5);
		UT_ASSERT(std::next(m.begin(), 5)->first == 6);
		UT_ASSERT(std::next(m.begin(), 5)->second == 6.5);
		UT_ASSERT(std::next(m.begin(), 6)->first == 7);
		UT_ASSERT(std::next(m.begin(), 6)->second == 7.5);
		UT_ASSERT(std::next(m.begin(), 7)->first == 8);
		UT_ASSERT(std::next(m.begin(), 7)->second == 8.5);

		i = erase(m, m.cbegin(), std::next(m.cbegin(), 2));
		UT_ASSERT(m.size() == 6);
		UT_ASSERT(i == m.begin());
		UT_ASSERT(std::next(m.begin(), 0)->first == 3);
		UT_ASSERT(std::next(m.begin(), 0)->second == 3.5);
		UT_ASSERT(std::next(m.begin(), 1)->first == 4);
		UT_ASSERT(std::next(m.begin(), 1)->second == 4.5);
		UT_ASSERT(std::next(m.begin(), 2)->first == 5);
		UT_ASSERT(std::next(m.begin(), 2)->second == 5.5);
		UT_ASSERT(std::next(m.begin(), 3)->first == 6);
		UT_ASSERT(std::next(m.begin(), 3)->second == 6.5);
		UT_ASSERT(std::next(m.begin(), 4)->first == 7);
		UT_ASSERT(std::next(m.begin(), 4)->second == 7.5);
		UT_ASSERT(std::next(m.begin(), 5)->first == 8);
		UT_ASSERT(std::next(m.begin(), 5)->second == 8.5);

		i = erase(m, std::next(m.cbegin(), 2),
			  std::next(m.cbegin(), 6));
		UT_ASSERT(m.size() == 2);
		UT_ASSERT(i == std::next(m.begin(), 2));
		UT_ASSERT(std::next(m.begin(), 0)->first == 3);
		UT_ASSERT(std::next(m.begin(), 0)->second == 3.5);
		UT_ASSERT(std::next(m.begin(), 1)->first == 4);
		UT_ASSERT(std::next(m.begin(), 1)->second == 4.5);

		i = erase(m, m.cbegin(), m.cend());
		UT_ASSERT(m.size() == 0);
		UT_ASSERT(i == m.begin());
		UT_ASSERT(i == m.end());
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
			path, "erase_iter_iter.pass", PMEMOBJ_MIN_POOL,
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
