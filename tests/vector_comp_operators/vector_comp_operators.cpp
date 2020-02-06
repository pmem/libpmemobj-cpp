// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>

#include <vector>

namespace nvobj = pmem::obj;

using vector_type = container_t<int>;

struct root {
	nvobj::persistent_ptr<vector_type> v1;
	nvobj::persistent_ptr<vector_type> v2;
	nvobj::persistent_ptr<vector_type> v3;
};

/**
 * Test pmem::obj::vector comparison operators.
 *
 * Compares elements in two vector vector_types using following operators:
 * ==, !=, <, <=, >, >=.
 */
void
test_comp_operators(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	int arr1[] = {0, 1, 2, 3, 4};
	int arr2[] = {0, 1, 2, 3, 4, 5};

	std::vector<int> stdvec1(std::begin(arr1), std::end(arr1));
	std::vector<int> stdvec2(std::begin(arr2), std::end(arr2));
	std::vector<int> stdvec3(std::begin(arr2) + 1, std::end(arr2));

	try {
		nvobj::transaction::run(pop, [&] {
			r->v1 = nvobj::make_persistent<vector_type>(
				std::begin(arr1), std::end(arr1));
			r->v2 = nvobj::make_persistent<vector_type>(
				std::begin(arr2), std::end(arr2));
			r->v3 = nvobj::make_persistent<vector_type>(
				std::begin(arr2) + 1, std::end(arr2));
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(*(r->v1) == *(r->v1));
	UT_ASSERT(*(r->v1) != *(r->v2));
	UT_ASSERT(*(r->v1) != *(r->v3));
	UT_ASSERT(*(r->v1) < *(r->v2));
	UT_ASSERT(*(r->v1) <= *(r->v2));
	UT_ASSERT(*(r->v1) <= *(r->v1));
	UT_ASSERT(*(r->v1) < *(r->v3));
	UT_ASSERT(*(r->v2) > *(r->v1));
	UT_ASSERT(*(r->v2) >= *(r->v1));
	UT_ASSERT(*(r->v2) >= *(r->v2));
	UT_ASSERT(*(r->v3) > *(r->v1));

	UT_ASSERT(*(r->v1) == stdvec1);
	UT_ASSERT(*(r->v1) != stdvec2);
	UT_ASSERT(*(r->v1) != stdvec3);
	UT_ASSERT(*(r->v1) < stdvec2);
	UT_ASSERT(*(r->v1) <= stdvec2);
	UT_ASSERT(*(r->v1) <= stdvec1);
	UT_ASSERT(*(r->v1) < stdvec3);
	UT_ASSERT(*(r->v2) > stdvec1);
	UT_ASSERT(*(r->v2) >= stdvec1);
	UT_ASSERT(*(r->v2) >= stdvec2);
	UT_ASSERT(*(r->v3) > stdvec1);

	UT_ASSERT(stdvec1 == *(r->v1));
	UT_ASSERT(stdvec1 != *(r->v2));
	UT_ASSERT(stdvec1 != *(r->v3));
	UT_ASSERT(stdvec1 < *(r->v2));
	UT_ASSERT(stdvec1 <= *(r->v2));
	UT_ASSERT(stdvec1 <= *(r->v1));
	UT_ASSERT(stdvec1 < *(r->v3));
	UT_ASSERT(stdvec2 > *(r->v1));
	UT_ASSERT(stdvec2 >= *(r->v1));
	UT_ASSERT(stdvec2 >= *(r->v2));
	UT_ASSERT(stdvec3 > *(r->v1));

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<vector_type>(r->v1);
			nvobj::delete_persistent<vector_type>(r->v2);
			nvobj::delete_persistent<vector_type>(r->v3);
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
	auto pop = nvobj::pool<root>::create(path, "VectorTest: comp_operators",
					     PMEMOBJ_MIN_POOL * 2,
					     S_IWUSR | S_IRUSR);

	test_comp_operators(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
