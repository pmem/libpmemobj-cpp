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

#include "helper_classes.hpp"
#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

struct root {
	nvobj::persistent_ptr<container_t<int>> v1;
	nvobj::persistent_ptr<container_t<CompoundType>> v2;
};

template <typename C>
void
common_insert_test(nvobj::pool_base &pop,
		   nvobj::persistent_ptr<container_t<C>> &ptr,
		   size_t expected_size)
{
	try {
		typename container_t<C>::iterator i =
			ptr->insert(ptr->cbegin() + 10, C{1});
		UT_ASSERT(ptr->size() == expected_size);
		UT_ASSERT(i == ptr->begin() + 10);
		unsigned j;
		for (j = 0; j < 10; ++j)
			UT_ASSERT((*ptr)[j] ==
				  C{}); // C{} == default constructed value
		UT_ASSERT((*ptr)[j] == C{1});
		for (++j; j < expected_size; ++j)
			UT_ASSERT((*ptr)[j] == C{});
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<container_t<C>>(ptr);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

template <typename C>
void
test_insert1(nvobj::pool_base &pop, nvobj::persistent_ptr<container_t<C>> &ptr)
{
	try {
		nvobj::transaction::run(pop, [&] {
			ptr = nvobj::make_persistent<container_t<C>>(100U);
		});
		common_insert_test(pop, ptr, 101);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

template <typename C>
void
test_insert2(nvobj::pool_base &pop, nvobj::persistent_ptr<container_t<C>> &ptr)
{
	try {
		nvobj::transaction::run(pop, [&] {
			ptr = nvobj::make_persistent<container_t<C>>(100U);
		});
		while (ptr->size() < ptr->capacity())
			ptr->push_back(C{}); // force reallocation
		common_insert_test(pop, ptr, ptr->size() + 1);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

template <typename C>
void
test_insert3(nvobj::pool_base &pop, nvobj::persistent_ptr<container_t<C>> &ptr)
{
	try {
		nvobj::transaction::run(pop, [&] {
			ptr = nvobj::make_persistent<container_t<C>>(100U);
		});
		while (ptr->size() < ptr->capacity())
			ptr->push_back(C{});
		ptr->pop_back();
		ptr->pop_back(); // force no reallocation
		common_insert_test(pop, ptr, ptr->size() + 1);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

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
		path, "VectorTest: insert_iter_value", PMEMOBJ_MIN_POOL * 2,
		S_IWUSR | S_IRUSR);

	auto r = pop.root();

	test_insert1(pop, r->v1);
	test_insert2(pop, r->v1);
	test_insert3(pop, r->v1);

	test_insert1(pop, r->v2);
	test_insert2(pop, r->v2);
	test_insert3(pop, r->v2);

	pop.close();

	return 0;
}
