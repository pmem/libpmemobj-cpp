// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2020, Intel Corporation */

/*
 * obj_cpp_mutex.cpp -- cpp mutex test
 */

#include "unittest.hpp"

#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj/atomic_base.h>

#include <mutex>
#include <thread>

#define LAYOUT "cpp"

namespace nvobj = pmem::obj;

namespace
{

/* pool root structure */
struct root {
	nvobj::mutex pmutex;
	unsigned counter;
};

/* number of ops per thread */
const unsigned num_ops = 200;

/* the number of threads */
const unsigned num_threads = 30;

/*
 * increment_pint -- (internal) test the mutex with an std::lock_guard
 */
void
increment_pint(nvobj::persistent_ptr<struct root> proot)
{
	for (unsigned i = 0; i < num_ops; ++i) {
		std::lock_guard<nvobj::mutex> lock(proot->pmutex);
		(proot->counter)++;
	}
}

/*
 * decrement_pint -- (internal) test the mutex with an std::unique_lock
 */
void
decrement_pint(nvobj::persistent_ptr<struct root> proot)
{
	std::unique_lock<nvobj::mutex> lock(proot->pmutex);
	for (unsigned i = 0; i < num_ops; ++i)
		--(proot->counter);

	lock.unlock();
}

/*
 * trylock_test -- (internal) test the trylock implementation
 */
void
trylock_test(nvobj::persistent_ptr<struct root> proot)
{
	for (;;) {
		if (proot->pmutex.try_lock()) {
			(proot->counter)++;
			proot->pmutex.unlock();
			return;
		}
	}
}

/*
 * mutex_zero_test -- (internal) test the zeroing constructor
 */
void
mutex_zero_test(nvobj::pool<struct root> &pop)
{
	PMEMoid raw_mutex;

	pmemobj_alloc(
		pop.handle(), &raw_mutex, sizeof(PMEMmutex), 1,
		[](PMEMobjpool *pop, void *ptr, void *) -> int {
			PMEMmutex *mtx = static_cast<PMEMmutex *>(ptr);
			pmemobj_memset_persist(pop, mtx, 1, sizeof(*mtx));
			return 0;
		},
		nullptr);

	nvobj::mutex *placed_mtx = new (pmemobj_direct(raw_mutex)) nvobj::mutex;
	std::unique_lock<nvobj::mutex> lck(*placed_mtx);
}

/*
 * mutex_test -- (internal) launch worker threads to test the pmutex
 */
template <typename Worker>
void
mutex_test(nvobj::pool<struct root> &pop, const Worker &function)
{
	std::thread threads[num_threads];

	nvobj::persistent_ptr<struct root> proot = pop.root();

	for (unsigned i = 0; i < num_threads; ++i)
		threads[i] = std::thread(function, proot);

	for (unsigned i = 0; i < num_threads; ++i)
		threads[i].join();
}

void
test_stack()
{
	/* mutex is not allowed outside of pmem */
	try {
		nvobj::mutex stack_mutex;
		UT_ASSERT(0);
	} catch (pmem::lock_error &le) {
	} catch (...) {
		UT_ASSERT(0);
	}
}

void
test_error_handling(nvobj::pool<root> &pop)
{
	nvobj::persistent_ptr<root> proot = pop.root();

	proot->pmutex.lock();

	/* try_locking already taken lock fails with false */
	UT_ASSERT(proot->pmutex.try_lock() == false);

	proot->pmutex.unlock();
}
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<struct root> pop;

	try {
		pop = nvobj::pool<struct root>::create(
			path, LAYOUT, PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	mutex_zero_test(pop);

	mutex_test(pop, increment_pint);
	UT_ASSERTeq(pop.root()->counter, num_threads * num_ops);

	mutex_test(pop, decrement_pint);
	UT_ASSERTeq(pop.root()->counter, 0);

	mutex_test(pop, trylock_test);
	UT_ASSERTeq(pop.root()->counter, num_threads);

	/* pmemcheck related persist */
	pmemobj_persist(pop.handle(), &(pop.root()->counter),
			sizeof(pop.root()->counter));

	test_stack();
	test_error_handling(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
