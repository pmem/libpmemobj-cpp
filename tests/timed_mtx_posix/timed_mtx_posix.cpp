// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2020, Intel Corporation */

/*
 * obj_cpp_mutex_posix.cpp -- cpp mutex test
 */

#include "pthread_common.hpp"
#include "unittest.hpp"

#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/timed_mutex.hpp>
#include <libpmemobj/atomic_base.h>

#include <mutex>

#define LAYOUT "cpp"

namespace nvobj = pmem::obj;

namespace
{

/* pool root structure */
struct root {
	nvobj::timed_mutex pmutex;
	unsigned counter;
};

/* number of ops per thread */
const unsigned num_ops = 200;

/* the number of threads */
const unsigned num_threads = 30;

/* timeout for try_lock_for and try_lock_until methods */
const auto timeout = std::chrono::milliseconds(100);

/* loop trylock_for|until tests */
bool loop = false;

/*
 * increment_pint -- (internal) test the mutex with an std::lock_guard
 */
static void *
increment_pint(void *arg)
{
	auto proot = static_cast<nvobj::persistent_ptr<struct root> *>(arg);

	for (unsigned i = 0; i < num_ops; ++i) {
		std::lock_guard<nvobj::timed_mutex> lock((*proot)->pmutex);
		((*proot)->counter)++;
	}
	return nullptr;
}

/*
 * decrement_pint -- (internal) test the mutex with an std::unique_lock
 */
static void *
decrement_pint(void *arg)
{
	auto proot = static_cast<nvobj::persistent_ptr<struct root> *>(arg);

	std::unique_lock<nvobj::timed_mutex> lock((*proot)->pmutex);
	for (unsigned i = 0; i < num_ops; ++i)
		--((*proot)->counter);

	lock.unlock();
	return nullptr;
}

/*
 * trylock_test -- (internal) test the trylock implementation
 */
static void *
trylock_test(void *arg)
{
	auto proot = static_cast<nvobj::persistent_ptr<struct root> *>(arg);
	for (;;) {
		if ((*proot)->pmutex.try_lock()) {
			((*proot)->counter)++;
			(*proot)->pmutex.unlock();
			break;
		}
	}
	return nullptr;
}

/*
 * trylock_for_test -- (internal) test the try_lock_for implementation
 */
static void *
trylock_for_test(void *arg)
{
	using clk = std::chrono::steady_clock;
	auto proot = static_cast<nvobj::persistent_ptr<struct root> *>(arg);

	do {
		auto t1 = clk::now();
		if ((*proot)->pmutex.try_lock_for(timeout)) {
			((*proot)->counter)++;
			(*proot)->pmutex.unlock();
			break;
		} else {
			auto t2 = clk::now();
			auto t_diff = t2 - t1;
			UT_ASSERT(t_diff >= timeout);
		}
	} while (loop);

	return nullptr;
}

/*
 * trylock_until_test -- (internal) test the try_lock_until implementation
 */
static void *
trylock_until_test(void *arg)
{
	using clk = std::chrono::steady_clock;
	auto proot = static_cast<nvobj::persistent_ptr<struct root> *>(arg);

	do {
		auto t1 = clk::now();
		if ((*proot)->pmutex.try_lock_until(t1 + timeout)) {
			--((*proot)->counter);
			(*proot)->pmutex.unlock();
			break;
		} else {
			auto t2 = clk::now();
			auto t_diff = t2 - t1;
			UT_ASSERT(t_diff >= timeout);
		}
	} while (loop);

	return nullptr;
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
		[](PMEMobjpool *pop, void *ptr, void *arg) -> int {
			PMEMmutex *mtx = static_cast<PMEMmutex *>(ptr);
			pmemobj_memset_persist(pop, mtx, 1, sizeof(*mtx));
			return 0;
		},
		nullptr);

	nvobj::timed_mutex *placed_mtx =
		new (pmemobj_direct(raw_mutex)) nvobj::timed_mutex;
	std::unique_lock<nvobj::timed_mutex> lck(*placed_mtx);
}

/*
 * mutex_test -- (internal) launch worker threads to test the pmutex
 */
template <typename Worker>
void
timed_mtx_test(nvobj::pool<struct root> &pop, Worker function)
{
	pthread_t threads[num_threads];

	auto proot = pop.root();

	for (unsigned i = 0; i < num_threads; ++i)
		ut_pthread_create(&threads[i], nullptr, function, &proot);

	for (unsigned i = 0; i < num_threads; ++i)
		ut_pthread_join(&threads[i], nullptr);
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

	timed_mtx_test(pop, increment_pint);
	UT_ASSERTeq(pop.root()->counter, num_threads * num_ops);

	timed_mtx_test(pop, decrement_pint);
	UT_ASSERTeq(pop.root()->counter, 0);

	timed_mtx_test(pop, trylock_test);
	UT_ASSERTeq(pop.root()->counter, num_threads);

	/* loop the next two tests */
	loop = true;

	timed_mtx_test(pop, trylock_until_test);
	UT_ASSERTeq(pop.root()->counter, 0);

	timed_mtx_test(pop, trylock_for_test);
	UT_ASSERTeq(pop.root()->counter, num_threads);

	loop = false;

	pop.root()->pmutex.lock();

	timed_mtx_test(pop, trylock_until_test);
	UT_ASSERTeq(pop.root()->counter, num_threads);

	timed_mtx_test(pop, trylock_for_test);
	UT_ASSERTeq(pop.root()->counter, num_threads);

	pop.root()->pmutex.unlock();

	/* pmemcheck related persist */
	pmemobj_persist(pop.handle(), &(pop.root()->counter),
			sizeof(pop.root()->counter));

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
