// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/pool.hpp>

#include <cstring>

namespace nvobj = pmem::obj;

struct X {
	static unsigned count;
	nvobj::p<int> val;

	X() : val(1)
	{
		++count;
	};
	~X()
	{
		--count;
	};
};

unsigned X::count = 0;

using vector_type = container_t<X>;

struct root {
	nvobj::persistent_ptr<vector_type> pptr;
};

/**
 * Test pmem::obj::vector default destructor.
 *
 * Call default destructor out of transaction scope.
 * Expects vector is empty and no exception is thrown.
 */
void
test_dtor(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	using size_type = vector_type::size_type;
	const size_type size = 100;
	try {
		nvobj::transaction::run(pop, [&] {
			r->pptr = nvobj::make_persistent<vector_type>(size);
		});
		UT_ASSERTeq(r->pptr->size(), X::count);
		UT_ASSERTeq(X::count, size);

		r->pptr->~vector_type();

		UT_ASSERT(r->pptr->empty());
		UT_ASSERTeq(X::count, 0);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(path, "VectorTest: vector_dtor",
					     PMEMOBJ_MIN_POOL * 2,
					     S_IWUSR | S_IRUSR);

	test_dtor(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
