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

// UNSUPPORTED: c++98, c++03

// <map>

// class map

// map(map&& m);

#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using C = container_t<int, double>;

struct root {
	nvobj::persistent_ptr<C> s;
};

int
run(pmem::obj::pool<root> &pop)
{
#ifdef XXX // XXX: Implement test_allocator and min_allocator
    typedef std::pair<const int, double> V;
    {
        typedef test_compare<std::less<int> > C;
        typedef test_allocator<V> A;
        std::map<int, double, C, A> mo(C(5), A(7));
        std::map<int, double, C, A> m = std::move(mo);
        UT_ASSERT(m.get_allocator() == A(7));
        UT_ASSERT(m.key_comp() == C(5));
        UT_ASSERT(m.size() == 0);
        UT_ASSERT(distance(m.begin(), m.end()) == 0);

        UT_ASSERT(mo.get_allocator() == A(test_alloc_base::moved_value));
        UT_ASSERT(mo.key_comp() == C(5));
        UT_ASSERT(mo.size() == 0);
        UT_ASSERT(distance(mo.begin(), mo.end()) == 0);
    }
    {
        V ar[] =
        {
            V(1, 1),
            V(1, 1.5),
            V(1, 2),
            V(2, 1),
            V(2, 1.5),
            V(2, 2),
            V(3, 1),
            V(3, 1.5),
            V(3, 2),
        };
        typedef test_compare<std::less<int> > C;
        typedef test_allocator<V> A;
        std::map<int, double, C, A> mo(ar, ar+sizeof(ar)/sizeof(ar[0]), C(5), A(7));
        std::map<int, double, C, A> m = std::move(mo);
        UT_ASSERT(m.get_allocator() == A(7));
        UT_ASSERT(m.key_comp() == C(5));
        UT_ASSERT(m.size() == 3);
        UT_ASSERT(distance(m.begin(), m.end()) == 3);
        UT_ASSERT(*m.begin() == V(1, 1));
        UT_ASSERT(*next(m.begin()) == V(2, 1));
        UT_ASSERT(*next(m.begin(), 2) == V(3, 1));

        UT_ASSERT(mo.get_allocator() == A(test_alloc_base::moved_value));
        UT_ASSERT(mo.key_comp() == C(5));
        UT_ASSERT(mo.size() == 0);
        UT_ASSERT(distance(mo.begin(), mo.end()) == 0);
    }
    {
        typedef test_compare<std::less<int> > C;
        typedef min_allocator<V> A;
        std::map<int, double, C, A> mo(C(5), A());
        std::map<int, double, C, A> m = std::move(mo);
        UT_ASSERT(m.get_allocator() == A());
        UT_ASSERT(m.key_comp() == C(5));
        UT_ASSERT(m.size() == 0);
        UT_ASSERT(distance(m.begin(), m.end()) == 0);

        UT_ASSERT(mo.get_allocator() == A());
        UT_ASSERT(mo.key_comp() == C(5));
        UT_ASSERT(mo.size() == 0);
        UT_ASSERT(distance(mo.begin(), mo.end()) == 0);
    }
    {
        V ar[] =
        {
            V(1, 1),
            V(1, 1.5),
            V(1, 2),
            V(2, 1),
            V(2, 1.5),
            V(2, 2),
            V(3, 1),
            V(3, 1.5),
            V(3, 2),
        };
        typedef test_compare<std::less<int> > C;
        typedef min_allocator<V> A;
        std::map<int, double, C, A> mo(ar, ar+sizeof(ar)/sizeof(ar[0]), C(5), A());
        std::map<int, double, C, A> m = std::move(mo);
        UT_ASSERT(m.get_allocator() == A());
        UT_ASSERT(m.key_comp() == C(5));
        UT_ASSERT(m.size() == 3);
        UT_ASSERT(distance(m.begin(), m.end()) == 3);
        UT_ASSERT(*m.begin() == V(1, 1));
        UT_ASSERT(*next(m.begin()) == V(2, 1));
        UT_ASSERT(*next(m.begin(), 2) == V(3, 1));

        UT_ASSERT(mo.get_allocator() == A());
        UT_ASSERT(mo.key_comp() == C(5));
        UT_ASSERT(mo.size() == 0);
        UT_ASSERT(distance(mo.begin(), mo.end()) == 0);
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
		pop = pmem::obj::pool<root>::create(path, "move.pass",
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
