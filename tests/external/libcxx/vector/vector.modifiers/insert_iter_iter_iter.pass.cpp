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

#include "iterators_support.hpp"
#include "unittest.hpp"

#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;

struct Throws;

using C = pmem_exp::vector<int>;

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
		path, "VectorTest: insert_iter_iter_iter", PMEMOBJ_MIN_POOL,
		S_IWUSR | S_IRUSR);

	auto r = pop.root();

	{
		try {
			nvobj::transaction::run(pop, [&] {
				r->v = nvobj::make_persistent<C>(100U);
			});
			int a[] = {1, 2, 3, 4, 5};
			const int N = sizeof(a) / sizeof(a[0]);
			C::iterator i = r->v->insert(
				r->v->cbegin() + 10,
				test_support::input_it<const int *>(a),
				test_support::input_it<const int *>(a + N));
			UT_ASSERT(r->v->size() == 100 + N);
			UT_ASSERT(i == r->v->begin() + 10);
			unsigned j;
			for (j = 0; j < 10; ++j)
				UT_ASSERT((*r->v)[j] == 0);
			for (std::size_t k = 0; k < N; ++j, ++k)
				UT_ASSERT((*r->v)[j] == a[k]);
			for (; j < 105; ++j)
				UT_ASSERT((*r->v)[j] == 0);
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
				r->v = nvobj::make_persistent<C>(100U);
			});
			int a[] = {1, 2, 3, 4, 5};
			const int N = sizeof(a) / sizeof(a[0]);
			C::iterator i = r->v->insert(
				r->v->cbegin() + 10,
				test_support::forward_it<const int *>(a),
				test_support::forward_it<const int *>(a + N));
			UT_ASSERT(r->v->size() == 100 + N);
			UT_ASSERT(i == r->v->begin() + 10);
			unsigned j;
			for (j = 0; j < 10; ++j)
				UT_ASSERT((*r->v)[j] == 0);
			for (std::size_t k = 0; k < N; ++j, ++k)
				UT_ASSERT((*r->v)[j] == a[k]);
			for (; j < 105; ++j)
				UT_ASSERT((*r->v)[j] == 0);
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
				r->v = nvobj::make_persistent<C>(100U);
			});
			while (r->v->size() < r->v->capacity())
				r->v->push_back(0); // force reallocation
			size_t sz = r->v->size();
			int a[] = {1, 2, 3, 4, 5};
			const int N = sizeof(a) / sizeof(a[0]);
			C::iterator i = r->v->insert(
				r->v->cbegin() + 10,
				test_support::forward_it<const int *>(a),
				test_support::forward_it<const int *>(a + N));
			UT_ASSERT(r->v->size() == sz + N);
			UT_ASSERT(i == r->v->begin() + 10);
			std::size_t j;
			for (j = 0; j < 10; ++j)
				UT_ASSERT((*r->v)[j] == 0);
			for (std::size_t k = 0; k < N; ++j, ++k)
				UT_ASSERT((*r->v)[j] == a[k]);
			for (; j < r->v->size(); ++j)
				UT_ASSERT((*r->v)[j] == 0);
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
				r->v = nvobj::make_persistent<C>(100U);
			});
			r->v->reserve(128); // force no reallocation
			size_t sz = r->v->size();
			int a[] = {1, 2, 3, 4, 5};
			const int N = sizeof(a) / sizeof(a[0]);
			C::iterator i = r->v->insert(
				r->v->cbegin() + 10,
				test_support::forward_it<const int *>(a),
				test_support::forward_it<const int *>(a + N));
			UT_ASSERT(r->v->size() == sz + N);
			UT_ASSERT(i == r->v->begin() + 10);
			std::size_t j;
			for (j = 0; j < 10; ++j)
				UT_ASSERT((*r->v)[j] == 0);
			for (std::size_t k = 0; k < N; ++j, ++k)
				UT_ASSERT((*r->v)[j] == a[k]);
			for (; j < r->v->size(); ++j)
				UT_ASSERT((*r->v)[j] == 0);
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
