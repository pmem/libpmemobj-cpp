// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>

#include <vector>

namespace nvobj = pmem::obj;

using pmem_vec = container_t<int>;
using std_vec = std::vector<int>;

struct root {
	nvobj::persistent_ptr<pmem_vec> pptr;
};

void
check_vector(const pmem_vec &v1, const std_vec &v2)
{
	UT_ASSERTeq(v1.size(), v2.size());

	for (unsigned i = 0; i < v1.size(); ++i)
		UT_ASSERTeq(v1[i], v2[i]);
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}
	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest", PMEMOBJ_MIN_POOL * 2, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		std_vec stdvector(10U, 1);

		nvobj::transaction::run(pop, [&] {
			r->pptr = nvobj::make_persistent<pmem_vec>(stdvector);
		});

		pmem_vec &pvector = *(r->pptr);

		check_vector(pvector, stdvector);

		stdvector.assign(20U, 2);
		pvector.assign(stdvector);

		check_vector(pvector, stdvector);

		stdvector = std_vec(30U, 3);
		pvector = stdvector;

		check_vector(pvector, stdvector);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<pmem_vec>(r->pptr);
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
