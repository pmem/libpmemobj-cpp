// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>

namespace nvobj = pmem::obj;
using C = container_t<int>;

struct root {
	nvobj::persistent_ptr<C> v;
};

/* Check if access method can be called out of transaction scope */
void
check_access_out_of_tx(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		r->v->cdata();
		r->v->data();
		static_cast<const C &>(*r->v).data();
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

/*
 * Check if access methods, iterators and dereference operator add
 * elements to transaction. Expect no pmemcheck errors.
 */
void
check_add_to_tx(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			auto p = r->v->data();
			for (unsigned i = 0; i < r->v->size(); ++i)
				*(p + i) = 2;
		});
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
	auto pop =
		nvobj::pool<root>::create(path, "VectorTest: iterators",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	nvobj::transaction::run(
		pop, [&] { r->v = nvobj::make_persistent<C>(10U, 1); });

	check_access_out_of_tx(pop);
	check_add_to_tx(pop);

	nvobj::delete_persistent_atomic<C>(r->v);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
