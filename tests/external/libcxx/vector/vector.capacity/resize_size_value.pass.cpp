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
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: resize_size_value", PMEMOBJ_MIN_POOL * 2,
		S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->v = nvobj::make_persistent<C>(100U); });

		r->v->resize(50, 1);
		UT_ASSERT(r->v->size() == 50);
		UT_ASSERT(r->v->capacity() == expected_capacity<size_t>(100));

		nvobj::transaction::run(pop, [&] {
			nvobj::persistent_ptr<C> v_tmp =
				nvobj::make_persistent<C>(50U);
			UT_ASSERT(*r->v == *v_tmp);
			nvobj::delete_persistent<C>(v_tmp);
		});

		r->v->resize(200, 1);
		UT_ASSERT(r->v->size() == 200);
		UT_ASSERT(r->v->capacity() >= expected_capacity<size_t>(200));

		for (unsigned i = 0; i < 50; ++i)
			UT_ASSERT((*r->v)[i] == 0);
		for (unsigned i = 50; i < 200; ++i)
			UT_ASSERT((*r->v)[i] == 1);

		/* test count == capacity() */
		r->v->resize(r->v->capacity(), 1);
		UT_ASSERT(r->v->size() == expected_capacity<size_t>(200));
		UT_ASSERT(r->v->capacity() >= expected_capacity<size_t>(200));

		for (unsigned i = 0; i < 50; ++i)
			UT_ASSERT((*r->v)[i] == 0);
		for (unsigned i = 50; i < expected_capacity<size_t>(200); ++i)
			UT_ASSERT((*r->v)[i] == 1);

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C>(r->v); });

	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
