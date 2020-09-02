// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/transaction.hpp>

#include <sstream>

namespace nvobj = pmem::obj;
namespace nvobjexp = nvobj::experimental;

const int TEST_ARR_SIZE = 10;

/*
 * prepare_array -- preallocate and fill a persistent array
 */
template <typename T, template <typename U> class pointer>
pointer<T>
prepare_array(nvobj::pool_base &pop)
{
	int ret;

	PMEMoid oid;
	ret = pmemobj_zalloc(pop.handle(), &oid, sizeof(T) * TEST_ARR_SIZE, 0);
	pointer<T> parr_vsize = oid;

	UT_ASSERTeq(ret, 0);

	T *parray = parr_vsize.get();

	try {
		nvobj::transaction::run(pop, [&] {
			for (int i = 0; i < TEST_ARR_SIZE; ++i) {
				parray[i] = i;
			}
		});
	} catch (...) {
		UT_FATAL("Transactional prepare_array aborted");
	}

	for (int i = 0; i < TEST_ARR_SIZE; ++i) {
		UT_ASSERTeq(parray[i], i);
	}

	return parr_vsize;
}

/*
 * test_arith -- test arithmetic operations on persistent pointers
 */
template <template <typename U> class pointer>
void
test_arith(nvobj::pool_base &pop)
{
	auto parr_vsize = prepare_array<nvobj::p<int>, pointer>(pop);

	/* test prefix postfix operators */
	for (int i = 0; i < TEST_ARR_SIZE; ++i) {
		UT_ASSERTeq(*parr_vsize, i);
		parr_vsize++;
	}

	for (int i = TEST_ARR_SIZE; i > 0; --i) {
		parr_vsize--;
		UT_ASSERTeq(*parr_vsize, i - 1);
	}

	for (int i = 0; i < TEST_ARR_SIZE; ++i) {
		UT_ASSERTeq(*parr_vsize, i);
		++parr_vsize;
	}

	for (int i = TEST_ARR_SIZE; i > 0; --i) {
		--parr_vsize;
		UT_ASSERTeq(*parr_vsize, i - 1);
	}

	/* test addition assignment and subtraction */
	parr_vsize += 2;
	UT_ASSERTeq(*parr_vsize, 2);

	parr_vsize -= 2;
	UT_ASSERTeq(*parr_vsize, 0);

	/* test strange invocations, parameter ignored */
	parr_vsize.operator++(5);
	UT_ASSERTeq(*parr_vsize, 1);

	parr_vsize.operator--(2);
	UT_ASSERTeq(*parr_vsize, 0);

	/* test subtraction and addition */
	for (int i = 0; i < TEST_ARR_SIZE; ++i)
		UT_ASSERTeq(*(parr_vsize + i), i);

	/* using STL one-pas-end style */
	auto parr_end = parr_vsize + TEST_ARR_SIZE;

	for (int i = TEST_ARR_SIZE; i > 0; --i)
		UT_ASSERTeq(*(parr_end - i), TEST_ARR_SIZE - i);

	UT_OUT("%ld", parr_end - parr_vsize);
	UT_ASSERTeq(parr_end - parr_vsize, TEST_ARR_SIZE);

	/* check ostream operator */
	std::stringstream stream;
	stream << parr_vsize;
	UT_OUT("%s", stream.str().c_str());
}

/*
 * test_relational -- test relational operators on persistent pointers
 */
template <template <typename U> class pointer>
void
test_relational(nvobj::pool_base &pop)
{

	auto first_elem = prepare_array<nvobj::p<int>, pointer>(pop);
	pointer<int[10][12]> parray;
	auto last_elem = first_elem + TEST_ARR_SIZE - 1;

	UT_ASSERT(first_elem != last_elem);
	UT_ASSERT(first_elem <= last_elem);
	UT_ASSERT(first_elem < last_elem);
	UT_ASSERT(last_elem > first_elem);
	UT_ASSERT(last_elem >= first_elem);
	UT_ASSERT(first_elem == first_elem);
	UT_ASSERT(first_elem >= first_elem);
	UT_ASSERT(first_elem <= first_elem);

	/* nullptr comparisons */
	UT_ASSERT(first_elem != nullptr);
	UT_ASSERT(nullptr != first_elem);
	UT_ASSERT(!(first_elem == nullptr));
	UT_ASSERT(!(nullptr == first_elem));

	UT_ASSERT(nullptr < first_elem);
	UT_ASSERT(!(first_elem < nullptr));
	UT_ASSERT(nullptr <= first_elem);
	UT_ASSERT(!(first_elem <= nullptr));

	UT_ASSERT(first_elem > nullptr);
	UT_ASSERT(!(nullptr > first_elem));
	UT_ASSERT(first_elem >= nullptr);
	UT_ASSERT(!(nullptr >= first_elem));

	/* pointer to array */
	UT_ASSERT(parray == nullptr);
	UT_ASSERT(nullptr == parray);
	UT_ASSERT(!(parray != nullptr));
	UT_ASSERT(!(nullptr != parray));

	UT_ASSERT(!(nullptr < parray));
	UT_ASSERT(!(parray < nullptr));
	UT_ASSERT(nullptr <= parray);
	UT_ASSERT(parray <= nullptr);

	UT_ASSERT(!(parray > nullptr));
	UT_ASSERT(!(nullptr > parray));
	UT_ASSERT(parray >= nullptr);
	UT_ASSERT(nullptr >= parray);

	auto different_array = prepare_array<nvobj::p<double>, pointer>(pop);

	/* only verify if this compiles */
	UT_ASSERT((first_elem < different_array) || true);
}
