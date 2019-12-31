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

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using C = container_t<int>;

struct root {
	nvobj::persistent_ptr<C> v;
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
	auto pop = nvobj::pool<root>::create(path, "VectorTest: capacity",
					     PMEMOBJ_MIN_POOL * 2,
					     S_IWUSR | S_IRUSR);

	auto r = pop.root();
	{
		try {
			nvobj::transaction::run(pop, [&] {
				r->v = nvobj::make_persistent<C>();
			});

			UT_ASSERT(r->v->capacity() ==
				  expected_capacity<size_t>(0));

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C>(r->v);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}
	{
		try {
			nvobj::transaction::run(pop, [&] {
				r->v = nvobj::make_persistent<C>(
					expected_capacity(100U));
			});

			UT_ASSERT(r->v->capacity() ==
				  expected_capacity<size_t>(100));

			r->v->push_back(0);

			UT_ASSERT(r->v->capacity() >= 101);

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C>(r->v);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}

	pop.close();

	return 0;
}
