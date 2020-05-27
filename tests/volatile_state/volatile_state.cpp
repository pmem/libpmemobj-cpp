// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/detail/volatile_state.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <atomic>
#include <thread>

namespace nvobj = pmem::obj;

static constexpr int VALUE = 10;

enum class volatile_object_state_type { none, created, destroyed };

static volatile_object_state_type volatile_object_state;

using v_state = pmem::detail::volatile_state;

struct v_data1 {
	v_data1() : val(new int(VALUE))
	{
		volatile_object_state = volatile_object_state_type::created;
	}

	v_data1(const v_data1 &rhs) = delete;

	~v_data1()
	{
		volatile_object_state = volatile_object_state_type::destroyed;

		delete val;
	}

	int *val;
};

static std::atomic<size_t> v2_initialized;

struct v_data2 {
	v_data2() : val(new int(VALUE))
	{
		v2_initialized++;
	}

	~v_data2()
	{
		v2_initialized--;
	}

	std::unique_ptr<int> val;
};

struct pmem_obj {
	char data[100];

	~pmem_obj()
	{
		try {
			v_state::destroy(pmemobj_oid(this));
		} catch (const pmem::transaction_scope_error &e) {
			UT_ASSERT(false);
		}
	}
};

struct root {
	nvobj::persistent_ptr<pmem_obj> obj_ptr1;
	nvobj::persistent_ptr<pmem_obj> obj_ptr2;

	nvobj::persistent_ptr<nvobj::vector<pmem_obj>> vec_obj_ptr;
};

void
test_volatile_basic(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	volatile_object_state = volatile_object_state_type::none;

	nvobj::transaction::run(pop, [&] {
		r->obj_ptr1 = nvobj::make_persistent<pmem_obj>();
		r->obj_ptr2 = nvobj::make_persistent<pmem_obj>();
	});

	*(v_state::get<v_data1>(r->obj_ptr1.raw())->val) = 1;
	*(v_state::get<v_data1>(r->obj_ptr2.raw())->val) = 2;

	UT_ASSERT(*(v_state::get<v_data1>(r->obj_ptr1.raw()))->val == 1);
	UT_ASSERT(*(v_state::get<v_data1>(r->obj_ptr2.raw()))->val == 2);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<pmem_obj>(r->obj_ptr1);
		nvobj::delete_persistent<pmem_obj>(r->obj_ptr2);
	});
}

void
test_volatile_state_lifecycle(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	volatile_object_state = volatile_object_state_type::none;

	nvobj::transaction::run(
		pop, [&] { r->obj_ptr1 = nvobj::make_persistent<pmem_obj>(); });

	UT_ASSERT(volatile_object_state == volatile_object_state_type::none);

	UT_ASSERT(*(v_state::get<v_data1>(r->obj_ptr1.raw()))->val == VALUE);
	UT_ASSERT(volatile_object_state == volatile_object_state_type::created);

	v_state::destroy(r->obj_ptr1.raw());

	UT_ASSERT(volatile_object_state ==
		  volatile_object_state_type::destroyed);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<pmem_obj>(r->obj_ptr1); });
}

void
test_volatile_state_lifecycle_tx(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	volatile_object_state = volatile_object_state_type::none;

	nvobj::transaction::run(
		pop, [&] { r->obj_ptr1 = nvobj::make_persistent<pmem_obj>(); });

	UT_ASSERT(volatile_object_state == volatile_object_state_type::none);

	UT_ASSERT(v_state::get_if_exists<v_data1>(r->obj_ptr1.raw()) ==
		  nullptr);
	UT_ASSERT(volatile_object_state == volatile_object_state_type::none);

	UT_ASSERT(*(v_state::get<v_data1>(r->obj_ptr1.raw()))->val == VALUE);
	UT_ASSERT(volatile_object_state == volatile_object_state_type::created);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<pmem_obj>(r->obj_ptr1); });

	UT_ASSERT(volatile_object_state ==
		  volatile_object_state_type::destroyed);
}

void
test_volatile_state_lifecycle_tx_abort(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	volatile_object_state = volatile_object_state_type::none;

	nvobj::transaction::run(
		pop, [&] { r->obj_ptr1 = nvobj::make_persistent<pmem_obj>(); });

	UT_ASSERT(*(v_state::get<v_data1>(r->obj_ptr1.raw()))->val == VALUE);

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<pmem_obj>(r->obj_ptr1);
			nvobj::transaction::abort(0);
		});
		UT_ASSERT(false);
	} catch (pmem::manual_tx_abort &) {

	} catch (...) {
		UT_ASSERT(false);
	}

	UT_ASSERT(*(v_state::get_if_exists<v_data1>(r->obj_ptr1.raw()))->val ==
		  VALUE);
	UT_ASSERT(volatile_object_state == volatile_object_state_type::created);

	try {
		nvobj::transaction::run(pop, [&] {
			v_state::destroy(r->obj_ptr1.raw());
			nvobj::transaction::abort(0);
		});
		UT_ASSERT(false);
	} catch (pmem::manual_tx_abort &) {

	} catch (...) {
		UT_ASSERT(false);
	}

	UT_ASSERT(*(v_state::get_if_exists<v_data1>(r->obj_ptr1.raw()))->val ==
		  VALUE);
	UT_ASSERT(volatile_object_state == volatile_object_state_type::created);

	nvobj::transaction::run(pop, [&] {
		v_state::destroy(r->obj_ptr1.raw());
		nvobj::delete_persistent<pmem_obj>(r->obj_ptr1);
	});

	UT_ASSERT(volatile_object_state ==
		  volatile_object_state_type::destroyed);
}

void
test_inside_tx(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	volatile_object_state = volatile_object_state_type::none;

	nvobj::transaction::run(
		pop, [&] { r->obj_ptr1 = nvobj::make_persistent<pmem_obj>(); });

	/* Creating volatile state inside tx causes exception */
	try {
		nvobj::transaction::run(
			pop, [&] { v_state::get<v_data1>(r->obj_ptr1.raw()); });
		UT_ASSERT(false);
	} catch (pmem::transaction_scope_error &) {
	} catch (...) {
		UT_ASSERT(false);
	}

	v_state::get<v_data1>(r->obj_ptr1.raw());
	UT_ASSERT(volatile_object_state == volatile_object_state_type::created);

	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(*(v_state::get_if_exists<v_data1>(
					    r->obj_ptr1.raw()))
					   ->val == VALUE);
			UT_ASSERT(*(v_state::get<v_data1>(r->obj_ptr1.raw()))
					   ->val == VALUE);
		});
	} catch (...) {
		UT_ASSERT(false);
	}

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<pmem_obj>(r->obj_ptr1); });

	UT_ASSERT(volatile_object_state ==
		  volatile_object_state_type::destroyed);
}

void
test_pool_close(nvobj::pool<root> &pop, const std::string &pool_path)
{
	auto r = pop.root();

	volatile_object_state = volatile_object_state_type::none;

	nvobj::transaction::run(
		pop, [&] { r->obj_ptr1 = nvobj::make_persistent<pmem_obj>(); });

	v_state::get<v_data1>(r->obj_ptr1.raw());
	UT_ASSERT(volatile_object_state == volatile_object_state_type::created);

	pop.close();
	UT_ASSERT(volatile_object_state ==
		  volatile_object_state_type::destroyed);

	pop = nvobj::pool<root>::open(pool_path, "VolatileStateTest");
	r = pop.root();
	UT_ASSERT(v_state::get_if_exists<v_data1>(r->obj_ptr1.raw()) ==
		  nullptr);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<pmem_obj>(r->obj_ptr1); });
}

void
test_mt_same_element(nvobj::pool<root> &pop, size_t concurrency)
{
	std::vector<std::thread> threads;
	threads.reserve(concurrency);

	auto r = pop.root();

	nvobj::transaction::run(
		pop, [&] { r->obj_ptr1 = nvobj::make_persistent<pmem_obj>(); });

	v2_initialized = 0;

	for (size_t i = 0; i < concurrency; ++i) {
		threads.emplace_back(
			[&](size_t thread_id) {
				v_state::get<v_data2>(r->obj_ptr1.raw());
			},
			i);
	}

	for (auto &t : threads) {
		t.join();
	}

	UT_ASSERT(v2_initialized == 1);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<pmem_obj>(r->obj_ptr1); });

	UT_ASSERT(v2_initialized == 0);
}

void
test_vector_of_elements(nvobj::pool<root> &pop)
{
	constexpr size_t NUM_ELEMENTS = 10;

	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->vec_obj_ptr =
			nvobj::make_persistent<nvobj::vector<pmem_obj>>(
				NUM_ELEMENTS);
	});

	v2_initialized = 0;

	for (size_t i = 0; i < NUM_ELEMENTS; i++)
		v_state::get<v_data2>(pmemobj_oid(&(*r->vec_obj_ptr)[i]));

	UT_ASSERT(v2_initialized == NUM_ELEMENTS);

	nvobj::transaction::run(pop, [&] {
		pmem::obj::delete_persistent<nvobj::vector<pmem_obj>>(
			r->vec_obj_ptr);
	});

	UT_ASSERT(v2_initialized == 0);
}

void
test_multiple_pool(nvobj::pool<root> &pop1, const std::string &path)
{
	constexpr size_t NUM_ELEMENTS = 10;

	auto pop2 = nvobj::pool<root>::create(path + "2", "VolatileStateTest2",
					      PMEMOBJ_MIN_POOL * 2,
					      S_IWUSR | S_IRUSR);

	auto r1 = pop1.root();
	auto r2 = pop2.root();

	nvobj::transaction::run(pop1, [&] {
		r1->vec_obj_ptr =
			nvobj::make_persistent<nvobj::vector<pmem_obj>>(
				NUM_ELEMENTS);
	});

	nvobj::transaction::run(pop2, [&] {
		r2->vec_obj_ptr =
			nvobj::make_persistent<nvobj::vector<pmem_obj>>(
				NUM_ELEMENTS);
	});

	v2_initialized = 0;

	for (size_t i = 0; i < NUM_ELEMENTS; i++) {
		auto oid1 = pmemobj_oid(&(*r1->vec_obj_ptr)[i]);
		auto oid2 = pmemobj_oid(&(*r2->vec_obj_ptr)[i]);

		v_state::get<v_data2>(oid1);
		v_state::get<v_data2>(oid2);
	}

	UT_ASSERT(v2_initialized == 2 * NUM_ELEMENTS);

	pop2.close();

	UT_ASSERT(v2_initialized == NUM_ELEMENTS);

	nvobj::transaction::run(pop1, [&] {
		pmem::obj::delete_persistent<nvobj::vector<pmem_obj>>(
			r1->vec_obj_ptr);
	});

	UT_ASSERT(v2_initialized == 0);
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}
	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(path, "VolatileStateTest",
					     PMEMOBJ_MIN_POOL * 2,
					     S_IWUSR | S_IRUSR);

	test_volatile_basic(pop);
	test_inside_tx(pop);
	test_pool_close(pop, path);
	test_volatile_state_lifecycle(pop);
	test_volatile_state_lifecycle_tx(pop);
	test_volatile_state_lifecycle_tx_abort(pop);
	test_mt_same_element(pop, 8);
	test_vector_of_elements(pop);
	test_multiple_pool(pop, path);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
