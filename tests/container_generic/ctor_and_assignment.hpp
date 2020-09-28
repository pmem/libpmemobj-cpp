// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_TESTS_CTOR_AND_ASSIGNMENT
#define LIBPMEMOBJ_CPP_TESTS_CTOR_AND_ASSIGNMENT

#include "../common/unittest.hpp"

namespace nvobj = pmem::obj;

/**
 * Wrapper around PMDK allocator
 * @throw std::bad_alloc on allocation failure.
 */
template <typename T, typename U, typename... Args>
void
tx_alloc_wrapper(nvobj::pool_base &pop, nvobj::persistent_ptr<U> &ptr,
		 Args &&... args)
{
	try {
		nvobj::transaction::manual tx(pop);
		ptr = nvobj::make_persistent<T>(std::forward<Args>(args)...);
		nvobj::transaction::commit();
	} catch (...) {
		throw std::bad_alloc();
	}
}

template <typename T>
void
verify_elements(T &container, size_t elements)
{
	UT_ASSERT(container.size() == elements);

	for (int i = 0; i < static_cast<int>(elements); i++) {
		UT_ASSERT(container.count(i) == 1);
	}
}

template <typename T>
void
ctor_test(nvobj::pool_base &pop, nvobj::persistent_ptr<T> &container1,
	  nvobj::persistent_ptr<T> &container2)
{
	using V = typename T::value_type;

	tx_alloc_wrapper<T>(pop, container1);
	UT_ASSERT(container1->empty());
	UT_ASSERT(container1->size() == size_t(0));

	for (int i = 0; i < 300; i++) {
		auto ret = container1->insert(V(i, i));
		UT_ASSERT(ret.second == true);
		UT_ASSERT(ret.first->MAP_KEY == i);
		UT_ASSERT(ret.first->MAP_VALUE == i);
	}

	tx_alloc_wrapper<T>(pop, container2, container1->begin(),
			    container1->end());

	UT_ASSERT(!container2->empty());
	UT_ASSERT(container1->size() == container2->size());

	verify_elements<T>(*container2, 300);

	pmem::detail::destroy<T>(*container2);
	tx_alloc_wrapper<T>(pop, container2, *container1);

	UT_ASSERT(container1->size() == container2->size());

	verify_elements<T>(*container2, 300);

	pmem::detail::destroy<T>(*container2);
	tx_alloc_wrapper<T>(pop, container2, std::move(*container1));

	verify_elements<T>(*container2, 300);

	pmem::detail::destroy<T>(*container2);
	tx_alloc_wrapper<T>(pop, container2,
			    std::initializer_list<V>{V(0, 0), V(1, 1)});

	verify_elements<T>(*container2, 2);

	pmem::detail::destroy<T>(*container1);
	pmem::detail::destroy<T>(*container2);
}

template <typename T>
void
assignment_test(nvobj::pool_base &pop, nvobj::persistent_ptr<T> &container1,
		nvobj::persistent_ptr<T> &container2)
{
	using V = typename T::value_type;

	tx_alloc_wrapper<T>(pop, container1);
	tx_alloc_wrapper<T>(pop, container2);

	UT_ASSERT(container1->empty());

	for (int i = 0; i < 50; i++) {
		auto ret = container1->insert(V(i, i));
		UT_ASSERT(ret.second == true);
	}

	verify_elements<T>(*container1, 50);

	for (int i = 0; i < 300; i++) {
		auto ret = container2->insert(V(i, i));
		UT_ASSERT(ret.second == true);
	}

	*container1 = *container2;

	verify_elements<T>(*container1, 300);

	for (int i = 300; i < 350; i++) {
		auto ret = container1->insert(V(i, i));
		UT_ASSERT(ret.second == true);
	}

	verify_elements<T>(*container1, 350);
	verify_elements<T>(*container2, 300);

	container2->clear();

	*container1 = *container2;

	UT_ASSERT(container1->size() == 0);
	UT_ASSERT(std::distance(container1->begin(), container1->end()) == 0);
	UT_ASSERT(container2->size() == 0);
	UT_ASSERT(std::distance(container2->begin(), container2->end()) == 0);

	for (int i = 0; i < 350; i++) {
		UT_ASSERT(container1->count(i) == 0);
		UT_ASSERT(container2->count(i) == 0);
	}

	for (int i = 0; i < 100; i++) {
		auto ret = container1->insert(V(i, i));
		UT_ASSERT(ret.second == true);
	}

	verify_elements<T>(*container1, 100);

	*container2 = std::move(*container1);

	verify_elements<T>(*container2, 100);

	pmem::detail::destroy<T>(*container1);
	pmem::detail::destroy<T>(*container2);
}

#endif
