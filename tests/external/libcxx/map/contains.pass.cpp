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

// bool contains(const key_type& x) const;

#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

struct E {
	int a = 1;
	double b = 1;
	char c = 1;
};

using container = container_t<char, int>;
using container2 = container_t<char, char>;
using container3 = container_t<int, E>;

struct root {
	nvobj::persistent_ptr<container> s;
	nvobj::persistent_ptr<container2> s2;
	nvobj::persistent_ptr<container3> s3;
};

template <typename T, typename P, typename B, typename... Pairs>
void
test(nvobj::persistent_ptr<T> &rs, nvobj::pool<root> &pop, B bad, Pairs... args)
{
	nvobj::transaction::run(pop, [&] { rs = nvobj::make_persistent<T>(); });
	auto &map = *rs;
	P pairs[] = {args...};

	for (auto &p : pairs)
		map.insert(p);
	for (auto &p : pairs)
		UT_ASSERT(map.contains(p.first));

	UT_ASSERT(!map.contains(bad));
	nvobj::transaction::run(pop, [&] { nvobj::delete_persistent<T>(rs); });
}

int
run(nvobj::pool<root> &pop)
{
	auto robj = pop.root();
	{
		test<container, std::pair<char, int>>(
			robj->s, pop, 'e', std::make_pair('a', 10),
			std::make_pair('b', 11), std::make_pair('c', 12),
			std::make_pair('d', 13));

		test<container2, std::pair<char, char>>(
			robj->s2, pop, 'e', std::make_pair('a', 'a'),
			std::make_pair('b', 'a'), std::make_pair('c', 'a'),
			std::make_pair('d', 'b'));

		test<container3, std::pair<int, E>>(
			robj->s3, pop, -1, std::make_pair(1, E{}),
			std::make_pair(2, E{}), std::make_pair(3, E{}),
			std::make_pair(4, E{}));
	}
#ifdef XXX // XXX: Implement min_allocator
	{
		test<std::multimap<char, int>, std::pair<char, int>>(
			'e', std::make_pair('a', 10), std::make_pair('b', 11),
			std::make_pair('c', 12), std::make_pair('d', 13));

		test<std::multimap<char, char>, std::pair<char, char>>(
			'e', std::make_pair('a', 'a'), std::make_pair('b', 'a'),
			std::make_pair('c', 'a'), std::make_pair('d', 'b'));

		test<std::multimap<int, E>, std::pair<int, E>>(
			-1, std::make_pair(1, E{}), std::make_pair(2, E{}),
			std::make_pair(3, E{}), std::make_pair(4, E{}));
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

	nvobj::pool<root> pop;
	try {
		pop = nvobj::pool<root>::create(path, "contains.pass",
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
