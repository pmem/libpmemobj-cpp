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
#include "iterators_support.hpp"
#include "unittest.hpp"

#include <libpmemobj++/experimental/array.hpp>
#include <libpmemobj++/experimental/string.hpp>
#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;

struct Throws;

struct root {
	nvobj::persistent_ptr<pmem_exp::vector<int>> v1;
	nvobj::persistent_ptr<pmem_exp::vector<CompoundType>> v2;
	nvobj::persistent_ptr<pmem_exp::vector<pmem_exp::string>> v3;

	nvobj::persistent_ptr<pmem_exp::array<pmem_exp::string, 5>>
		string_test_arr;
	nvobj::persistent_ptr<pmem_exp::string> default_str;
};

template <typename C, typename Iterator, typename DefaultValueT>
void
test_insert_with_realloc(nvobj::pool_base &pop,
			 nvobj::persistent_ptr<pmem_exp::vector<C>> &ptr,
			 const DefaultValueT &def, Iterator begin, Iterator end)
{
	try {
		nvobj::transaction::run(pop, [&] {
			ptr = nvobj::make_persistent<pmem_exp::vector<C>>(100U,
									  def);
		});
		typename pmem_exp::vector<C>::iterator i =
			ptr->insert(ptr->cbegin() + 10, begin, end);

		UT_ASSERT(ptr->size() == 100 + 5);
		UT_ASSERT(i == ptr->begin() + 10);
		unsigned j;
		for (j = 0; j < 10; ++j)
			UT_ASSERT((*ptr)[j] == def);
		for (auto it = begin; it != end; ++j, ++it)
			UT_ASSERT((*ptr)[j] == *it);
		for (; j < 105; ++j)
			UT_ASSERT((*ptr)[j] == def);
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<pmem_exp::vector<C>>(ptr);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

template <typename C, typename Iterator, typename DefaultValueT>
void
test_insert_after_realloc(nvobj::pool_base &pop,
			  nvobj::persistent_ptr<pmem_exp::vector<C>> &ptr,
			  const DefaultValueT &def, Iterator begin,
			  Iterator end)
{
	try {
		nvobj::transaction::run(pop, [&] {
			ptr = nvobj::make_persistent<pmem_exp::vector<C>>(100U,
									  def);
		});
		while (ptr->size() < ptr->capacity())
			ptr->push_back(0); // force reallocation
		size_t sz = ptr->size();
		typename pmem_exp::vector<C>::iterator i =
			ptr->insert(ptr->cbegin() + 10, begin, end);
		UT_ASSERT(ptr->size() == sz + 5);
		UT_ASSERT(i == ptr->begin() + 10);
		std::size_t j;
		for (j = 0; j < 10; ++j)
			UT_ASSERT((*ptr)[j] == def);
		for (auto it = begin; it != end; ++j, ++it)
			UT_ASSERT((*ptr)[j] == *it);
		for (; j < ptr->size(); ++j)
			UT_ASSERT((*ptr)[j] == def);
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<pmem_exp::vector<C>>(ptr);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

template <typename C, typename Iterator, typename DefaultValueT>
void
test_insert_with_reserve1(nvobj::pool_base &pop,
			  nvobj::persistent_ptr<pmem_exp::vector<C>> &ptr,
			  const DefaultValueT &def, Iterator begin,
			  Iterator end)
{
	try {
		nvobj::transaction::run(pop, [&] {
			ptr = nvobj::make_persistent<pmem_exp::vector<C>>(100U,
									  def);
		});
		ptr->reserve(128); // force no reallocation
		size_t sz = ptr->size();
		typename pmem_exp::vector<C>::iterator i =
			ptr->insert(ptr->cbegin() + 10, begin, end);
		UT_ASSERT(ptr->size() == sz + 5);
		UT_ASSERT(i == ptr->begin() + 10);
		std::size_t j;
		for (j = 0; j < 10; ++j)
			UT_ASSERT((*ptr)[j] == def);
		for (auto it = begin; it != end; ++j, ++it)
			UT_ASSERT((*ptr)[j] == *it);
		for (; j < ptr->size(); ++j)
			UT_ASSERT((*ptr)[j] == def);
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<pmem_exp::vector<C>>(ptr);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

template <typename C, typename Iterator, typename DefaultValueT>
void
test_insert_with_reserve2(nvobj::pool_base &pop,
			  nvobj::persistent_ptr<pmem_exp::vector<C>> &ptr,
			  const DefaultValueT &def, Iterator begin,
			  Iterator end)
{
	try {
		nvobj::transaction::run(pop, [&] {
			ptr = nvobj::make_persistent<pmem_exp::vector<C>>(100U,
									  def);
		});
		ptr->reserve(128); // force no reallocation
		size_t sz = ptr->size();
		typename pmem_exp::vector<C>::iterator i =
			ptr->insert(ptr->cbegin() + 98, begin, end);
		UT_ASSERT(ptr->size() == sz + 5);
		UT_ASSERT(i == ptr->begin() + 98);
		std::size_t j;
		for (j = 0; j < 98; ++j)
			UT_ASSERT((*ptr)[j] == def);
		for (auto it = begin; it != end; ++j, ++it)
			UT_ASSERT((*ptr)[j] == *it);
		for (; j < ptr->size(); ++j)
			UT_ASSERT((*ptr)[j] == def);
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<pmem_exp::vector<C>>(ptr);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

template <typename C, typename Iterator, typename DefaultValueT>
void
test_insert_with_reserve3(nvobj::pool_base &pop,
			  nvobj::persistent_ptr<pmem_exp::vector<C>> &ptr,
			  const DefaultValueT &def, Iterator begin,
			  Iterator end)
{
	try {
		nvobj::transaction::run(pop, [&] {
			ptr = nvobj::make_persistent<pmem_exp::vector<C>>(100U,
									  def);
		});
		ptr->reserve(128); // force no reallocation
		size_t sz = ptr->size();
		typename pmem_exp::vector<C>::iterator i =
			ptr->insert(ptr->cend(), begin, end);
		UT_ASSERT(ptr->size() == sz + 5);
		UT_ASSERT(i == ptr->begin() + 100);
		std::size_t j;
		for (j = 0; j < 100; ++j)
			UT_ASSERT((*ptr)[j] == def);
		for (auto it = begin; it != end; ++j, ++it)
			UT_ASSERT((*ptr)[j] == *it);
		for (; j < ptr->size(); ++j)
			UT_ASSERT((*ptr)[j] == def);
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
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: insert_iter_iter_iter", PMEMOBJ_MIN_POOL,
		S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			pop.root()->string_test_arr = nvobj::make_persistent<
				pmem_exp::array<pmem_exp::string, 5>>();

			(*pop.root()->string_test_arr)[0] = "1";
			(*pop.root()->string_test_arr)[1] = "2";
			(*pop.root()->string_test_arr)[2] = "3";
			(*pop.root()->string_test_arr)[3] = "4";
			(*pop.root()->string_test_arr)[4] = "5";

			pop.root()->default_str =
				nvobj::make_persistent<pmem_exp::string>("0");
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	int int_table[5] = {1, 2, 3, 4, 5};
	CompoundType compund_table[5] = {1, 2, 3, 4, 5};

	test_insert_with_realloc(
		pop, r->v1, 99,
		test_support::forward_it<const int *>(int_table),
		test_support::forward_it<const int *>(int_table + 5));
	test_insert_after_realloc(
		pop, r->v1, 99,
		test_support::forward_it<const int *>(int_table),
		test_support::forward_it<const int *>(int_table + 5));
	test_insert_with_reserve1(
		pop, r->v1, 99,
		test_support::forward_it<const int *>(int_table),
		test_support::forward_it<const int *>(int_table + 5));
	test_insert_with_reserve2(
		pop, r->v1, 99,
		test_support::forward_it<const int *>(int_table),
		test_support::forward_it<const int *>(int_table + 5));
	test_insert_with_reserve3(
		pop, r->v1, 99,
		test_support::forward_it<const int *>(int_table),
		test_support::forward_it<const int *>(int_table + 5));

	test_insert_with_realloc(
		pop, r->v1, 99, test_support::input_it<const int *>(int_table),
		test_support::input_it<const int *>(int_table + 5));
	test_insert_after_realloc(
		pop, r->v1, 99, test_support::input_it<const int *>(int_table),
		test_support::input_it<const int *>(int_table + 5));
	test_insert_with_reserve1(
		pop, r->v1, 99, test_support::input_it<const int *>(int_table),
		test_support::input_it<const int *>(int_table + 5));
	test_insert_with_reserve2(
		pop, r->v1, 99, test_support::input_it<const int *>(int_table),
		test_support::input_it<const int *>(int_table + 5));
	test_insert_with_reserve3(
		pop, r->v1, 99, test_support::input_it<const int *>(int_table),
		test_support::input_it<const int *>(int_table + 5));

	test_insert_with_realloc(
		pop, r->v2, CompoundType{},
		test_support::forward_it<const CompoundType *>(compund_table),
		test_support::forward_it<const CompoundType *>(compund_table +
							       5));
	test_insert_after_realloc(
		pop, r->v2, CompoundType{},
		test_support::forward_it<const CompoundType *>(compund_table),
		test_support::forward_it<const CompoundType *>(compund_table +
							       5));
	test_insert_with_reserve1(
		pop, r->v2, CompoundType{},
		test_support::forward_it<const CompoundType *>(compund_table),
		test_support::forward_it<const CompoundType *>(compund_table +
							       5));
	test_insert_with_reserve2(
		pop, r->v2, CompoundType{},
		test_support::forward_it<const CompoundType *>(compund_table),
		test_support::forward_it<const CompoundType *>(compund_table +
							       5));
	test_insert_with_reserve3(
		pop, r->v2, CompoundType{},
		test_support::forward_it<const CompoundType *>(compund_table),
		test_support::forward_it<const CompoundType *>(compund_table +
							       5));

	/* XXX: Enable when no_uninit_check is implemented
	test_insert_with_realloc(pop, r->v3, *(pop.root()->default_str),
				 pop.root()->string_test_arr->cbegin(),
				 pop.root()->string_test_arr->cend());
	test_insert_after_realloc(pop, r->v3, *(pop.root()->default_str),
				  pop.root()->string_test_arr->cbegin(),
				  pop.root()->string_test_arr->cend());
	test_insert_with_reserve1(pop, r->v3, *(pop.root()->default_str),
				  pop.root()->string_test_arr->cbegin(),
				  pop.root()->string_test_arr->cend());
	test_insert_with_reserve2(pop, r->v3, *(pop.root()->default_str),
				  pop.root()->string_test_arr->cbegin(),
				  pop.root()->string_test_arr->cend());
	test_insert_with_reserve3(pop, r->v3, *(pop.root()->default_str),
				  pop.root()->string_test_arr->cbegin(),
				  pop.root()->string_test_arr->cend());
	*/

	pop.close();

	return 0;
}
