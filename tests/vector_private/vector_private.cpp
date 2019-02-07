/*
 * Copyright 2018-2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "unittest.hpp"

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <cstring>
#include <vector>

/**
 * Ugly way to test private methods, but safe since vector is header-only
 * container
 */
#define private public
#include <libpmemobj++/experimental/vector.hpp>

namespace nvobj = pmem::obj;
namespace pmem_exp = pmem::obj::experimental;
namespace test = test_support;

using element_type = int;
using vector_type = pmem_exp::vector<element_type>;

#define TEST_CAPACITY 666
#define TEST_SIZE_1 66
#define TEST_VAL_1 6
#define TEST_SIZE_2 33
#define TEST_VAL_2 3
#define TEST_SIZE_3 11
#define TEST_SIZE_OOM (PMEMOBJ_MIN_POOL / sizeof(element_type) + 1)

struct root {
	nvobj::persistent_ptr<vector_type> v_pptr;
};

/**
 * Test alloc pmem::obj::experimental::vector private function.
 *
 * First case: allocate memory for TEST_CAPACITY elements of given type (int),
 * check if _capacity had changed. Expect no exception is thrown.
 *
 * Second case: allocate memory for more than max_size() elements of given type
 * (int). Expect std::length_error exception is thrown.
 *
 * Third case: allocate memory for more than PMEMOBJ_MIN_POOL/sizeof(int)
 * and less than max_size() bytes. Expect transaction_alloc_error exception is
 * thrown.
 *
 * Fourth case: allocate memory for zero elements. Expect no memory is
 * allocated.
 */
void
test_vector_private_alloc(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	/* first case */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v_pptr = nvobj::make_persistent<vector_type>();
			r->v_pptr->alloc(TEST_CAPACITY);
		});

		UT_ASSERTeq(r->v_pptr->_capacity, TEST_CAPACITY);

		nvobj::delete_persistent_atomic<vector_type>(r->v_pptr);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	/* second case */
	bool exception_thrown = false;

	try {
		nvobj::transaction::run(pop, [&] {
			r->v_pptr = nvobj::make_persistent<vector_type>();
			r->v_pptr->alloc(r->v_pptr->max_size() + 1);
		});
		UT_ASSERT(0);
	} catch (std::length_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);

	/* third case */
	exception_thrown = false;

	try {
		nvobj::transaction::run(pop, [&] {
			r->v_pptr = nvobj::make_persistent<vector_type>();
			r->v_pptr->alloc(TEST_SIZE_OOM);
			UT_ASSERT(0);
		});
	} catch (pmem::transaction_alloc_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);

	/* fourth case */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v_pptr = nvobj::make_persistent<vector_type>();
			r->v_pptr->alloc(0);
		});
		UT_ASSERT(r->v_pptr->capacity() == 0);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

/**
 * Test dealloc pmem::obj::experimental::vector private function.
 *
 * Allocate memory for TEST_CAPACITY elements of given type (int)
 * and call dealloc. Expect _capacity changed to 0 and no exception is thrown.
 */
void
test_vector_private_dealloc(nvobj::pool<struct root> &pop) try {
	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->v_pptr = nvobj::make_persistent<vector_type>();

		r->v_pptr->alloc(TEST_CAPACITY);
		UT_ASSERT(r->v_pptr->_data != nullptr);
		r->v_pptr->dealloc();
	});

	UT_ASSERTeq(r->v_pptr->_capacity, 0);

	nvobj::delete_persistent_atomic<vector_type>(r->v_pptr);
} catch (std::exception &e) {
	UT_FATALexc(e);
}

/**
 * Test construct and construct_range pmem::obj::experimental::vector private
 * functions.
 *
 * First case: allocate memory for TEST_CAPACITY elements of given type (int)
 * and call construct overload for count-value arguments. Check if TEST_SIZE_1
 * elements in underlying array were constructed with TEST_VAL_1 value, starting
 * at position TEST_SIZE_2.
 *
 * Second case: using allocated memory in previous test case, construct
 * additional TEST_SIZE_2 elements at the beginning of underlying array. Note
 * that construct function requires that memory for elements to be created must
 * be snapshotted. Since this memory area is uninitialized yet, we must use
 * Valgrind annotations and mark this area as added to transactions and flushed.
 * Compare values in underlying array with expected values.
 *
 * Third case: using allocated memory in previous test case call construct_range
 * overload for InputIterator-InputIterator arguments. Check if first
 * TEST_SIZE_1 elements in underlying array were constructed with values pointed
 * by argument's iterators.
 *
 * Fourth case: using allocated memory in previous test case, construct
 * additional TEST_SIZE_2 at the end of underlying array by calling
 * construct_range overload for InputIterator-InputIterator arguments. Note that
 * construct_range function requires that memory for elements to be created must
 * be snapshotted. Since this memory area is uninitialized yet, we must use
 * Valgrind annotations and mark this area as added to transaction and flushed.
 * Compare values in underlying array with expected values.
 */
void
test_vector_grow(nvobj::pool<struct root> &pop) try {
	auto r = pop.root();

	/* first case */
	nvobj::transaction::run(pop, [&] {
		r->v_pptr = nvobj::make_persistent<vector_type>();
		r->v_pptr->alloc(TEST_CAPACITY);

		UT_ASSERT(r->v_pptr->_data != nullptr);
		UT_ASSERTeq(r->v_pptr->_capacity, TEST_CAPACITY);

		r->v_pptr->construct(TEST_SIZE_2, TEST_SIZE_1, TEST_VAL_1);
	});
	UT_ASSERTeq(r->v_pptr->_size, TEST_SIZE_1);

	auto ptr = r->v_pptr->_data.get() + TEST_SIZE_2;
	for (unsigned i = 0; i < TEST_SIZE_1; ++i)
		UT_ASSERTeq(*ptr++, TEST_VAL_1);

	/* second case */
	nvobj::transaction::run(pop, [&] {
		UT_ASSERT(r->v_pptr->_capacity >=
			  r->v_pptr->_size + TEST_SIZE_2);

		auto *addr = r->v_pptr->_data.get();
		auto sz = sizeof(*addr) * TEST_SIZE_2;

		if (On_pmemcheck)
			VALGRIND_ADD_TO_TX(addr, sz);

		r->v_pptr->construct(0, TEST_SIZE_2, TEST_VAL_2);

		if (On_pmemcheck) {
			VALGRIND_SET_CLEAN(addr, sz);
			VALGRIND_REMOVE_FROM_TX(addr, sz);
		}
	});
	UT_ASSERTeq(r->v_pptr->_size, TEST_SIZE_1 + TEST_SIZE_2);

	ptr = r->v_pptr->_data.get();
	unsigned j = 0;
	for (; j < TEST_SIZE_2; ++j)
		UT_ASSERTeq(*ptr++, TEST_VAL_2);
	for (; j < TEST_SIZE_1; ++j)
		UT_ASSERTeq(*ptr++, TEST_VAL_1);

	nvobj::transaction::run(pop, [&] {
		r->v_pptr->dealloc();
		nvobj::delete_persistent<vector_type>(r->v_pptr);
	});

	/* third case */
	std::vector<element_type> v(TEST_SIZE_1, TEST_VAL_1);
	v.insert(v.end(), TEST_SIZE_2, TEST_VAL_2);

	auto first = test::input_it<element_type *>(v.data());
	auto middle = test::input_it<element_type *>(v.data() + TEST_SIZE_1);
	auto last = test::input_it<element_type *>(v.data() + v.size());

	nvobj::transaction::run(pop, [&] {
		r->v_pptr = nvobj::make_persistent<vector_type>();
		r->v_pptr->alloc(TEST_CAPACITY);

		UT_ASSERT(r->v_pptr->_data != nullptr);
		UT_ASSERTeq(r->v_pptr->_capacity, TEST_CAPACITY);

		r->v_pptr->construct_range(r->v_pptr->_size, first, middle);
	});
	UT_ASSERTeq(r->v_pptr->_size, TEST_SIZE_1);

	ptr = r->v_pptr->_data.get();
	for (unsigned i = 0; i < TEST_SIZE_1; ++i)
		UT_ASSERTeq(*ptr++, v[i]);

	/* fourth case */
	nvobj::transaction::run(pop, [&] {
		UT_ASSERT(r->v_pptr->_capacity >=
			  r->v_pptr->_size + TEST_SIZE_2);

		auto *addr = r->v_pptr->_data.get() +
			static_cast<std::size_t>(r->v_pptr->_size);
		auto sz = sizeof(element_type) * TEST_SIZE_2;

		if (On_pmemcheck)
			VALGRIND_ADD_TO_TX(addr, sz);

		r->v_pptr->construct_range(r->v_pptr->_size, middle, last);

		if (On_pmemcheck) {
			VALGRIND_SET_CLEAN(addr, sz);
			VALGRIND_REMOVE_FROM_TX(addr, sz);
		}
	});
	UT_ASSERTeq(r->v_pptr->_size, TEST_SIZE_1 + TEST_SIZE_2);

	ptr = r->v_pptr->_data.get();
	j = 0;
	for (; j < TEST_SIZE_1; ++j)
		UT_ASSERTeq(*ptr++, v[j]);
	for (; j < TEST_SIZE_2; ++j)
		UT_ASSERTeq(*ptr++, v[j]);

	nvobj::transaction::run(pop, [&] {
		r->v_pptr->dealloc();
		nvobj::delete_persistent<vector_type>(r->v_pptr);
	});

} catch (std::exception &e) {
	UT_FATALexc(e);
}

/**
 * Test _shrink pmem::obj::experimental::vector private function.
 *
 * Allocate memory for TEST_CAPACITY elements of given type (int)
 * and fill it with TEST_VAL_1 value. Call _shrink with TEST_SIZE_1
 * argument and compare values in underlying array with expected results.
 */
void
test_vector_shrink(nvobj::pool<struct root> &pop) try {
	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->v_pptr = nvobj::make_persistent<vector_type>();
		r->v_pptr->alloc(TEST_CAPACITY);

		UT_ASSERT(r->v_pptr->_data != nullptr);

		r->v_pptr->construct(r->v_pptr->_size, TEST_CAPACITY,
				     TEST_VAL_1);
		r->v_pptr->shrink(TEST_SIZE_1);
	});
	UT_ASSERTeq(r->v_pptr->_size, TEST_SIZE_1);

	auto ptr = r->v_pptr->_data.get();
	unsigned j = 0;
	for (; j < TEST_SIZE_1; ++j)
		UT_ASSERTeq(*ptr++, TEST_VAL_1);
	for (; j < TEST_VAL_1; ++j)
		UT_ASSERTeq(*ptr++, 0);

	nvobj::transaction::run(pop, [&] {
		r->v_pptr->dealloc();
		nvobj::delete_persistent<vector_type>(r->v_pptr);
	});

} catch (std::exception &e) {
	UT_FATALexc(e);
}

void
test_vector_realloc(nvobj::pool<struct root> &pop) try {
	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->v_pptr = nvobj::make_persistent<vector_type>(
			static_cast<std::size_t>(TEST_SIZE_1), TEST_VAL_1);
	});

	nvobj::transaction::run(
		pop, [&] { r->v_pptr->realloc(TEST_SIZE_1 + TEST_SIZE_2); });
	UT_ASSERTeq(r->v_pptr->_capacity, TEST_SIZE_1 + TEST_SIZE_2);
	UT_ASSERTeq(r->v_pptr->_size, TEST_SIZE_1);

	unsigned j = 0;
	for (; j < TEST_SIZE_1; ++j)
		UT_ASSERTeq(r->v_pptr->_data[j], TEST_VAL_1);

	nvobj::transaction::run(pop, [&] {
		r->v_pptr->dealloc();
		nvobj::delete_persistent<vector_type>(r->v_pptr);
	});

} catch (std::exception &e) {
	UT_FATALexc(e);
}

void
check_memcheck_uninitialized_range(element_type *start, element_type *end)
{
#if LIBPMEMOBJ_CPP_VG_MEMCHECK_ENABLED
	if (!On_memcheck)
		return;

	auto cache_start = start;
	VALGRIND_DISABLE_ERROR_REPORTING;

	while (start < end && VALGRIND_CHECK_MEM_IS_DEFINED(start, 1))
		start++;

	VALGRIND_ENABLE_ERROR_REPORTING;

	UT_ASSERTeq(start, end);

	VALGRIND_MAKE_MEM_DEFINED(cache_start,
				  static_cast<std::size_t>(end - cache_start) *
					  sizeof(element_type));
#endif /* #if LIBPMEMOBJ_CPP_VG_MEMCHECK_ENABLED */
}

void
test_vector_insert_gap(nvobj::pool<struct root> &pop) try {
	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->v_pptr = nvobj::make_persistent<vector_type>(
			static_cast<std::size_t>(TEST_SIZE_1), TEST_VAL_1);
		/*
		 * insert gap with realocation from:
		 * 11...1
		 * ^66x 1
		 * to:
		 * 11...1xx...x11..1
		 * 33x 1 + 11x uninitialized empty element slots + 33x 1
		 */
		r->v_pptr->insert_gap(TEST_SIZE_2, TEST_SIZE_3);
	});
	UT_ASSERTeq(
		r->v_pptr->_capacity,
		r->v_pptr->get_recommended_capacity(TEST_SIZE_1 + TEST_SIZE_3));
	UT_ASSERTeq(r->v_pptr->_size, TEST_SIZE_1);

	unsigned j = 0;
	for (; j < TEST_SIZE_2; ++j)
		UT_ASSERTeq(r->v_pptr->_data[j], TEST_VAL_1);

	/*
	 * Check if elements from index 34 to 44 are uninitialized
	 * and ignore memcheck errors.
	 */
	check_memcheck_uninitialized_range(
		&r->v_pptr->_data[j],
		&r->v_pptr->_data[TEST_SIZE_2 + TEST_SIZE_3]);

	for (j = TEST_SIZE_2 + TEST_SIZE_3; j < TEST_SIZE_1 + TEST_SIZE_3; ++j)
		UT_ASSERTeq(r->v_pptr->_data[j], TEST_VAL_1);

	nvobj::transaction::run(pop, [&] {
		r->v_pptr->dealloc();
		nvobj::delete_persistent<vector_type>(r->v_pptr);
	});

	nvobj::transaction::run(pop, [&] {
		r->v_pptr = nvobj::make_persistent<vector_type>(
			static_cast<std::size_t>(TEST_SIZE_1), TEST_VAL_1);

		r->v_pptr->reserve(TEST_SIZE_1 + TEST_SIZE_2);

		UT_ASSERTeq(r->v_pptr->_capacity, TEST_SIZE_1 + TEST_SIZE_2);
		UT_ASSERTeq(r->v_pptr->_size, TEST_SIZE_1);
	});

	nvobj::transaction::run(pop, [&] {
		/*
		 * insert gap from:
		 * 11...1xx...x
		 * ^66x 1 and 33 uninitialized empty slots
		 * to:
		 * xx...x11...1
		 * 33x uninitialized empty element slots and 36x 1
		 */
		r->v_pptr->insert_gap(0, TEST_SIZE_2);
	});

	UT_ASSERTeq(r->v_pptr->_capacity, TEST_SIZE_1 + TEST_SIZE_2);
	UT_ASSERTeq(r->v_pptr->_size, TEST_SIZE_1);

	j = TEST_SIZE_2;
	for (; j < TEST_SIZE_1 + TEST_SIZE_2; ++j)
		UT_ASSERTeq(r->v_pptr->_data[j], TEST_VAL_1);

	nvobj::transaction::run(pop, [&] {
		r->v_pptr->dealloc();
		nvobj::delete_persistent<vector_type>(r->v_pptr);
	});

} catch (std::exception &e) {
	UT_FATALexc(e);
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
		nvobj::pool<root>::create(path, "VectorTest: vector_private",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	test_vector_private_alloc(pop);
	test_vector_private_dealloc(pop);
	test_vector_grow(pop);
	test_vector_shrink(pop);
	test_vector_realloc(pop);
	test_vector_insert_gap(pop);

	pop.close();

	return 0;
}
