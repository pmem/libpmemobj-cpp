//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2018-2019, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "helper_classes.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using vector_type = pmem::obj::vector<int>;
using vector_type2 = pmem::obj::vector<emplace_constructible<int>>;
using vector_type3 =
	pmem::obj::vector<emplace_constructible_and_move_insertable<int>>;

struct root {
	nvobj::persistent_ptr<vector_type> test1;
	nvobj::persistent_ptr<vector_type2> test2;
	nvobj::persistent_ptr<vector_type3> test3;
};

template <class C, class Iterator>
void
basic_test(nvobj::pool<struct root> &pop, Iterator first, Iterator last)
{
	auto r = pop.root();

	/* construct */
	try {
		nvobj::transaction::run(pop, [&] {
			r->test1 = nvobj::make_persistent<C>(first, last);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	/* validate */
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERTeq(r->test1->size(),
				    static_cast<std::size_t>(
					    std::distance(first, last)));

			for (typename C::const_iterator i = r->test1->begin(),
							e = r->test1->end();
			     i != e; ++i, ++first)
				UT_ASSERTeq(*i, *first);

			nvobj::delete_persistent<C>(r->test1);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

/**
 * Test pmem::obj::vector range constructor
 *
 * Constructs container with elements within [first, last) range pointed by
 * iterators of following categories: Input, Forward, Bidirectional, Random
 * access Validates container's size and its elements. Expects no exception is
 * thrown.
 */
static void
basic_test_cases(nvobj::pool<struct root> &pop)
{
	int a[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 7, 6, 5, 4, 3, 1, 0};
	int *an = a + sizeof(a) / sizeof(a[0]);
	basic_test<vector_type>(pop, test_support::input_it<const int *>(a),
				test_support::input_it<const int *>(an));
	basic_test<vector_type>(pop, test_support::forward_it<const int *>(a),
				test_support::forward_it<const int *>(an));
	basic_test<vector_type>(
		pop, test_support::bidirectional_it<const int *>(a),
		test_support::bidirectional_it<const int *>(an));
	basic_test<vector_type>(
		pop, test_support::random_access_it<const int *>(a),
		test_support::random_access_it<const int *>(an));
}

/**
 * Test pmem::obj::vector range constructor
 *
 * Constructs container with elements within [first, last) range pointed by
 * iterators
 * TEST_1 - Checks if elements are emplace-constructed from given range, in the
 * same order
 *
 * TEST_2 - additionaly to TEST_1 checks if elements within [first, last) range
 * are not moved if iterator does not meet forward iterator requirements
 */
static void
emplaceable_concept_tests(nvobj::pool<struct root> &pop)
{
	int arr1[] = {42};
	int arr2[] = {1, 101, 42};

	auto r = pop.root();
	/* TEST_1 */
	{
		using It = test_support::forward_it<int *>;
		/* construct */
		try {
			nvobj::transaction::run(pop, [&] {
				r->test2 = nvobj::make_persistent<vector_type2>(
					It(arr1), It(std::end(arr1)));
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
		/* validate */
		try {
			UT_ASSERTeq((*r->test2)[0].value, 42);
			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<vector_type2>(
					r->test2);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}

		/* construct */
		try {
			nvobj::transaction::run(pop, [&] {
				r->test2 = nvobj::make_persistent<vector_type2>(
					It(arr2), It(std::end(arr2)));
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
		/* validate */
		try {
			UT_ASSERTeq((*r->test2)[0].value, 1);
			UT_ASSERTeq((*r->test2)[1].value, 101);
			UT_ASSERTeq((*r->test2)[2].value, 42);

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<vector_type2>(
					r->test2);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}
	/* TEST_2 */
	{
		using It = test_support::input_it<int *>;
		/* construct */
		try {
			nvobj::transaction::run(pop, [&] {
				r->test3 = nvobj::make_persistent<vector_type3>(
					It(arr1), It(std::end(arr1)));
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
		/* validate */
		try {
			UT_ASSERTeq((*r->test3)[0].value, 42);
			UT_ASSERTeq((*r->test3)[0].moved, 0);

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<vector_type3>(
					r->test3);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}

		/* construct */
		try {
			nvobj::transaction::run(pop, [&] {
				r->test3 = nvobj::make_persistent<vector_type3>(
					It(arr2), It(std::end(arr2)));
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
		/* validate */
		try {
			UT_ASSERTeq((*r->test3)[0].value, 1);
			UT_ASSERTeq((*r->test3)[1].value, 101);
			UT_ASSERTeq((*r->test3)[2].value, 42);
			UT_ASSERTeq((*r->test3)[2].moved, 0);

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<vector_type3>(
					r->test3);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
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
		path, "VectorTest: construct_iter_iter", PMEMOBJ_MIN_POOL,
		S_IWUSR | S_IRUSR);

	basic_test_cases(pop);
	emplaceable_concept_tests(pop);

	pop.close();

	return 0;
}
