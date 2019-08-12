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

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;

static constexpr int INITIALIZED = 99;

struct CompoundType {
	int counter = 0;

	CompoundType(int c)
	{
		UT_ASSERT(counter != INITIALIZED);
		counter = c;
	}

	CompoundType()
	{
		UT_ASSERT(counter != INITIALIZED);
		counter = INITIALIZED;
	}

	CompoundType(CompoundType &&rhs)
	{
		UT_ASSERT(counter != INITIALIZED);
		counter = rhs.counter;
	}

	CompoundType(const CompoundType &rhs)
	{
		UT_ASSERT(counter != INITIALIZED);
		counter = rhs.counter;
	}

	~CompoundType()
	{
		counter = 0;
	}

	CompoundType &
	operator=(CompoundType &&rhs)
	{
		UT_ASSERT(counter == INITIALIZED);
		counter = rhs.counter;
		return *this;
	}

	CompoundType &
	operator=(const CompoundType &rhs)
	{
		UT_ASSERT(counter == INITIALIZED);
		counter = rhs.counter;
		return *this;
	}

	bool
	operator==(const CompoundType &rhs)
	{
		return counter == rhs.counter;
	}
};

struct root {
	nvobj::persistent_ptr<pmem_exp::vector<int>> v1;
	nvobj::persistent_ptr<pmem_exp::vector<CompoundType>> v2;
};

template <typename C>
void
test_insert1(nvobj::pool_base &pop,
	     nvobj::persistent_ptr<pmem_exp::vector<C>> &ptr)
{
	try {
		nvobj::transaction::run(pop, [&] {
			ptr = nvobj::make_persistent<pmem_exp::vector<C>>(100U);
		});
		typename pmem_exp::vector<C>::iterator i =
			ptr->insert(ptr->cbegin() + 10, C{1});
		UT_ASSERT(ptr->size() == 101);
		UT_ASSERT(i == ptr->begin() + 10);
		unsigned j;
		for (j = 0; j < 10; ++j)
			UT_ASSERT((*ptr)[j] ==
				  C{}); // C{} == default constructed value
		UT_ASSERT((*ptr)[j] == C{1});
		for (++j; j < 101; ++j)
			UT_ASSERT((*ptr)[j] == C{});
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<pmem_exp::vector<C>>(ptr);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

template <typename C>
void
test_insert2(nvobj::pool_base &pop,
	     nvobj::persistent_ptr<pmem_exp::vector<C>> &ptr)
{
	try {
		nvobj::transaction::run(pop, [&] {
			ptr = nvobj::make_persistent<pmem_exp::vector<C>>(100U);
		});
		while (ptr->size() < ptr->capacity())
			ptr->push_back(0); // force reallocation
		size_t sz = ptr->size();
		typename pmem_exp::vector<C>::iterator i =
			ptr->insert(ptr->cbegin() + 10, C{1});
		UT_ASSERT(ptr->size() == sz + 1);
		UT_ASSERT(i == ptr->begin() + 10);
		std::size_t j;
		for (j = 0; j < 10; ++j)
			UT_ASSERT((*ptr)[j] == C{});
		UT_ASSERT((*ptr)[j] == C{1});
		for (++j; j < ptr->size(); ++j)
			UT_ASSERT((*ptr)[j] == C{});
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<pmem_exp::vector<C>>(ptr);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

template <typename C>
void
test_insert3(nvobj::pool_base &pop,
	     nvobj::persistent_ptr<pmem_exp::vector<C>> &ptr)
{
	try {
		nvobj::transaction::run(pop, [&] {
			ptr = nvobj::make_persistent<pmem_exp::vector<C>>(100U);
		});
		while (ptr->size() < ptr->capacity())
			ptr->push_back(0);
		ptr->pop_back();
		ptr->pop_back(); // force no reallocation
		size_t sz = ptr->size();
		typename pmem_exp::vector<C>::iterator i =
			ptr->insert(ptr->cbegin() + 10, C{1});
		UT_ASSERT(ptr->size() == sz + 1);
		UT_ASSERT(i == ptr->begin() + 10);
		std::size_t j;
		for (j = 0; j < 10; ++j)
			UT_ASSERT((*ptr)[j] == C{});
		UT_ASSERT((*ptr)[j] == C{1});
		for (++j; j < ptr->size(); ++j)
			UT_ASSERT((*ptr)[j] == C{});
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<pmem_exp::vector<C>>(ptr);
		});
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
	auto pop =
		nvobj::pool<root>::create(path, "VectorTest: insert_iter_value",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

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
