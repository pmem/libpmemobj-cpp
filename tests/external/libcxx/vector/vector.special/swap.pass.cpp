//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

#include <utility>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;

using C = pmem_exp::vector<int>;

struct root {
	nvobj::persistent_ptr<C> c1;
	nvobj::persistent_ptr<C> c2;
};
int
main(int argc, char *argv[])
{
	START();

	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
		return 1;
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: swap", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();
	{
		int a1[] = {1, 3, 7, 9, 10};
		int a2[] = {0, 2, 4, 5, 6, 8, 11};
		try {
			nvobj::transaction::run(pop, [&] {
				r->c1 = nvobj::make_persistent<C>(
					a1, a1 + sizeof(a1) / sizeof(a1[0]));
				r->c2 = nvobj::make_persistent<C>(
					a2, a2 + sizeof(a2) / sizeof(a2[0]));
			});

			swap(*r->c1, *r->c2);

			nvobj::transaction::run(pop, [&] {
				nvobj::persistent_ptr<C> tmp1 =
					nvobj::make_persistent<C>(
						a1,
						a1 +
							sizeof(a1) /
								sizeof(a1[0]));
				nvobj::persistent_ptr<C> tmp2 =
					nvobj::make_persistent<C>(
						a2,
						a2 +
							sizeof(a2) /
								sizeof(a2[0]));
				UT_ASSERT(*r->c1 == *tmp2);
				UT_ASSERT(*r->c2 == *tmp1);
				nvobj::delete_persistent<C>(tmp1);
				nvobj::delete_persistent<C>(tmp2);
			});

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C>(r->c1);
				nvobj::delete_persistent<C>(r->c2);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}
	{
		int a1[] = {1, 3, 7, 9, 10};
		int a2[] = {0, 2, 4, 5, 6, 8, 11};
		try {
			nvobj::transaction::run(pop, [&] {
				r->c1 = nvobj::make_persistent<C>(a1, a1);
				r->c2 = nvobj::make_persistent<C>(
					a2, a2 + sizeof(a2) / sizeof(a2[0]));
			});

			swap(*r->c1, *r->c2);

			nvobj::transaction::run(pop, [&] {
				nvobj::persistent_ptr<C> tmp1 =
					nvobj::make_persistent<C>(
						a2,
						a2 +
							sizeof(a2) /
								sizeof(a2[0]));
				UT_ASSERT(*r->c1 == *tmp1);
				UT_ASSERT(r->c2->empty());
				UT_ASSERT(std::distance(r->c2->begin(),
							r->c2->end()) == 0);
				nvobj::delete_persistent<C>(tmp1);
			});

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C>(r->c1);
				nvobj::delete_persistent<C>(r->c2);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}
	{
		int a1[] = {1, 3, 7, 9, 10};
		int a2[] = {0, 2, 4, 5, 6, 8, 11};
		try {
			nvobj::transaction::run(pop, [&] {
				r->c1 = nvobj::make_persistent<C>(
					a1, a1 + sizeof(a1) / sizeof(a1[0]));
				r->c2 = nvobj::make_persistent<C>(a2, a2);
			});

			swap(*r->c1, *r->c2);

			UT_ASSERT(r->c1->empty());
			UT_ASSERT(std::distance(r->c1->begin(), r->c1->end()) ==
				  0);

			nvobj::transaction::run(pop, [&] {
				nvobj::persistent_ptr<C> tmp1 =
					nvobj::make_persistent<C>(
						a1,
						a1 +
							sizeof(a1) /
								sizeof(a1[0]));
				UT_ASSERT(*r->c2 == *tmp1);
				nvobj::delete_persistent<C>(tmp1);
			});

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C>(r->c1);
				nvobj::delete_persistent<C>(r->c2);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}
	{
		int a1[] = {1, 3, 7, 9, 10};
		int a2[] = {0, 2, 4, 5, 6, 8, 11};
		try {
			nvobj::transaction::run(pop, [&] {
				r->c1 = nvobj::make_persistent<C>(a1, a1);
				r->c2 = nvobj::make_persistent<C>(a2, a2);
			});

			swap(*r->c1, *r->c2);

			UT_ASSERT(r->c1->empty());
			UT_ASSERT(std::distance(r->c1->begin(), r->c1->end()) ==
				  0);
			UT_ASSERT(r->c2->empty());
			UT_ASSERT(std::distance(r->c2->begin(), r->c2->end()) ==
				  0);
			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C>(r->c1);
				nvobj::delete_persistent<C>(r->c2);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}
	pop.close();

	return 0;
}
