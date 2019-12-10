/*
 * Copyright 2019, Intel Corporation
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

struct v_data1 {
	v_data1() : val(new int(VALUE))
	{
		volatile_object_state = volatile_object_state_type::created;
	}

	~v_data1()
	{
		volatile_object_state = volatile_object_state_type::destroyed;

		delete val;
	}

	int *val;
};

static std::atomic<int> v2_initialized;

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

	pmem::detail::volatile_state v;
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

	*(r->obj_ptr1->v.get<v_data1>()->val) = 1;
	*(r->obj_ptr2->v.get<v_data1>()->val) = 2;

	UT_ASSERT(*(r->obj_ptr1->v.get<v_data1>()->val) == 1);
	UT_ASSERT(*(r->obj_ptr2->v.get<v_data1>()->val) == 2);

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

	UT_ASSERT(*(r->obj_ptr1->v.get<v_data1>()->val) == VALUE);
	UT_ASSERT(volatile_object_state == volatile_object_state_type::created);

	r->obj_ptr1->v.destroy();

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

	UT_ASSERT(r->obj_ptr1->v.get_if_exists<v_data1>() == nullptr);
	UT_ASSERT(volatile_object_state == volatile_object_state_type::none);

	UT_ASSERT(*(r->obj_ptr1->v.get<v_data1>()->val) == VALUE);
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

	UT_ASSERT(*(r->obj_ptr1->v.get<v_data1>()->val) == VALUE);

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

	UT_ASSERT(*(r->obj_ptr1->v.get<v_data1>()->val) == VALUE);
	UT_ASSERT(volatile_object_state == volatile_object_state_type::created);

	try {
		nvobj::transaction::run(pop, [&] {
			r->obj_ptr1->v.destroy();
			nvobj::transaction::abort(0);
		});
		UT_ASSERT(false);
	} catch (pmem::manual_tx_abort &) {

	} catch (...) {
		UT_ASSERT(false);
	}

	UT_ASSERT(*(r->obj_ptr1->v.get<v_data1>()->val) == VALUE);
	UT_ASSERT(volatile_object_state == volatile_object_state_type::created);

	nvobj::transaction::run(pop, [&] {
		r->obj_ptr1->v.destroy();
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
		nvobj::transaction::run(pop,
					[&] { r->obj_ptr1->v.get<v_data1>(); });
		UT_ASSERT(false);
	} catch (pmem::transaction_scope_error &e) {
	} catch (...) {
		UT_ASSERT(false);
	}

	r->obj_ptr1->v.get<v_data1>();
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(*(r->obj_ptr1->v.get_if_exists<v_data1>()
					    ->val) == VALUE);
			UT_ASSERT(*(r->obj_ptr1->v.get<v_data1>()->val) ==
				  VALUE);
		});
	} catch (...) {
		UT_ASSERT(false);
	}

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<pmem_obj>(r->obj_ptr1); });
}

void
test_pool_close(nvobj::pool<root> &pop, const std::string &pool_path)
{
	auto r = pop.root();

	volatile_object_state = volatile_object_state_type::none;

	nvobj::transaction::run(
		pop, [&] { r->obj_ptr1 = nvobj::make_persistent<pmem_obj>(); });

	r->obj_ptr1->v.get<v_data1>();
	UT_ASSERT(volatile_object_state == volatile_object_state_type::created);

	pop.close();
	UT_ASSERT(volatile_object_state ==
		  volatile_object_state_type::destroyed);

	pop = nvobj::pool<root>::open(pool_path, "VolatileStateTest");
	r = pop.root();
	UT_ASSERT(r->obj_ptr1->v.get_if_exists<v_data1>() == nullptr);

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
				r->obj_ptr1->v.get<v_data2>();
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
		(*r->vec_obj_ptr)[i].v.get<v_data2>();

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
		(*r1->vec_obj_ptr)[i].v.get<v_data2>();
		(*r2->vec_obj_ptr)[i].v.get<v_data2>();
	}

	UT_ASSERT(v2_initialized == 2 * NUM_ELEMENTS);

	std::thread test_thread([&]() {
		for (size_t i = 0; i < NUM_ELEMENTS; i++) {
			auto &v_ref = (*r1->vec_obj_ptr)[i].v;

			UT_ASSERT(*(v_ref.get<v_data2>()->val) == VALUE);
		}
	});

	std::thread close_pool_thread([&]() { pop2.close(); });

	test_thread.join();
	close_pool_thread.join();

	UT_ASSERT(v2_initialized == NUM_ELEMENTS);

	nvobj::transaction::run(pop1, [&] {
		pmem::obj::delete_persistent<nvobj::vector<pmem_obj>>(
			r1->vec_obj_ptr);
	});

	UT_ASSERT(v2_initialized == 0);
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

	return 0;
}
