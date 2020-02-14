//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019-2020, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;
using vector_type = container_t<int>;

struct root {
	nvobj::persistent_ptr<vector_type> v_pptr;
};

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(path, "VectorTest: iterators.pass",
					     PMEMOBJ_MIN_POOL * 2,
					     S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->v_pptr = nvobj::make_persistent<vector_type>();
			{
				vector_type::iterator i = r->v_pptr->begin();
				vector_type::iterator j = r->v_pptr->end();
				UT_ASSERT(std::distance(i, j) == 0);
				UT_ASSERT(i == j);
			}
			{
				vector_type::const_iterator i =
					r->v_pptr->begin();
				vector_type::const_iterator j =
					r->v_pptr->end();
				UT_ASSERT(std::distance(i, j) == 0);
				UT_ASSERT(i == j);
			}
			{
				vector_type::const_iterator i =
					r->v_pptr->cbegin();
				vector_type::const_iterator j =
					r->v_pptr->cend();
				UT_ASSERT(std::distance(i, j) == 0);
				UT_ASSERT(i == j);
				UT_ASSERT(i == r->v_pptr->end());
			}
			{
				vector_type::iterator i;
				vector_type::const_iterator j;
				(void)i;
				(void)j;
			}
			nvobj::delete_persistent<vector_type>(r->v_pptr);

			const int a[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
			r->v_pptr = nvobj::make_persistent<vector_type>(
				std::begin(a), std::end(a));

			{
				vector_type::iterator i = r->v_pptr->begin();
				UT_ASSERT(*i == 0);
				++i;
				UT_ASSERT(*i == 1);
				*i = 10;
				UT_ASSERT(*i == 10);
				UT_ASSERT(std::distance(r->v_pptr->begin(),
							r->v_pptr->end()) ==
					  10);
			}
			{
				vector_type::iterator ii1{}, ii2{};
				vector_type::iterator ii4 = ii1;
				vector_type::const_iterator cii{};
				UT_ASSERT(ii1 == ii2);
				UT_ASSERT(ii1 == ii4);
				UT_ASSERT(!(ii1 != ii2));
				UT_ASSERT((ii1 == cii));
				UT_ASSERT((cii == ii1));
				UT_ASSERT(!(ii1 != cii));
				UT_ASSERT(!(cii != ii1));
				UT_ASSERT(!(ii1 < cii));
				UT_ASSERT(!(cii < ii1));
				UT_ASSERT((ii1 <= cii));
				UT_ASSERT((cii <= ii1));
				UT_ASSERT(!(ii1 > cii));
				UT_ASSERT(!(cii > ii1));
				UT_ASSERT((ii1 >= cii));
				UT_ASSERT((cii >= ii1));
				UT_ASSERT(cii - ii1 == 0);
				UT_ASSERT(ii1 - cii == 0);
			}
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
