// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_TESTS_CONTAINER_TXABORT
#define LIBPMEMOBJ_CPP_TESTS_CONTAINER_TXABORT

#include "../common/unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;

const int NUMBER_OF_INSERTS = 100;

template <typename T>
void
init_containers(nvobj::pool_base &pop, nvobj::persistent_ptr<T> &c1,
		nvobj::persistent_ptr<T> &c2)
{
	using V = typename T::value_type;

	nvobj::transaction::run(pop, [&] {
		c1 = nvobj::make_persistent<T>();
		c2 = nvobj::make_persistent<T>();
	});

	for (int i = 0; i < NUMBER_OF_INSERTS; i++) {
		c1->insert(V(i, i));
		c2->insert(V(i, i + 1));
	}
}

template <typename T>
void
verify_elements(nvobj::persistent_ptr<T> &container1,
		nvobj::persistent_ptr<T> &container2)
{
	for (int i = 0; i < NUMBER_OF_INSERTS; i++) {
		auto it = container1->find(i);
		auto it2 = container2->find(i);

		UT_ASSERT(it->MAP_VALUE == i);
		UT_ASSERT(it2->MAP_VALUE == i + 1);
	}
}

template <typename T>
void
test_tx_singlethread(nvobj::pool_base &pop,
		     nvobj::persistent_ptr<T> &container1,
		     nvobj::persistent_ptr<T> &container2)
{
	init_containers<T>(pop, container1, container2);

	nvobj::transaction::run(pop, [&] {
		auto container_tmp = nvobj::make_persistent<T>();
		auto container_tmp2 =
			nvobj::make_persistent<T>(std::move(*container_tmp));

		nvobj::delete_persistent<T>(container_tmp);
		nvobj::delete_persistent<T>(container_tmp2);
	});

	try {
		nvobj::transaction::run(pop, [&] {
			container1->swap(*container2);
			nvobj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	verify_elements<T>(container1, container2);

	try {
		nvobj::transaction::run(pop, [&] {
			*container1 = *container2;
			nvobj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	verify_elements<T>(container1, container2);

	try {
		nvobj::transaction::run(pop, [&] {
			container1->clear();
			nvobj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	verify_elements<T>(container1, container2);

	try {
		nvobj::transaction::run(pop, [&] {
			container1->clear();
			*container1 = {{0, 0}};
			nvobj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	verify_elements<T>(container1, container2);

	try {
		nvobj::transaction::run(pop, [&] {
			*container1 = {{0, 0}, {1, 1}};
			nvobj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	verify_elements<T>(container1, container2);

	try {
		nvobj::transaction::run(pop, [&] {
			for (auto &e : *container1) {
				e.MAP_VALUE = 10;
			}
			nvobj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	verify_elements<T>(container1, container2);

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<T>(container1);
			nvobj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	verify_elements<T>(container1, container2);

	auto test_value = 10;
	{
		auto it = container1->find(test_value);

		try {
			nvobj::transaction::run(pop, [&] {
				UT_ASSERT(it->MAP_VALUE == test_value);
				it->MAP_VALUE = 0;
				UT_ASSERT(it->MAP_VALUE == 0);

				nvobj::transaction::abort(0);
			});
		} catch (pmem::manual_tx_abort &) {
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}

	verify_elements<T>(container1, container2);

	{
		auto it = container1->find(test_value);

		UT_ASSERT(it->MAP_VALUE == test_value);
	}

	try {
		nvobj::transaction::run(pop, [&] {
			container1->clear();
			nvobj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(static_cast<int>(container1->size()) == NUMBER_OF_INSERTS);

#if defined(LIBPMEMOBJ_CPP_TESTS_CONCURRENT_MAP)
	try {
		nvobj::transaction::run(pop, [&] {
			container1->free_data();
			nvobj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (...) {
		UT_ASSERT(0);
	}

	verify_elements<T>(container1, container2);
#endif

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<T>(container1);
		nvobj::delete_persistent<T>(container2);
	});
}

#endif
