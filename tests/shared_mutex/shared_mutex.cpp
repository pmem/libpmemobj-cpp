/*
 * Copyright 2016-2019, Intel Corporation
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

/*
 * obj_cpp_shared_mutex.cpp -- cpp shared mutex test
 */

#include "unittest.hpp"

#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/shared_mutex.hpp>
#include <libpmemobj/atomic_base.h>

#include <mutex>
#include <thread>

#define LAYOUT "cpp"

namespace nvobj = pmem::obj;

namespace
{

/* pool root structure */
struct root {
	nvobj::shared_mutex pmutex;
	unsigned counter;
};

/* number of ops per thread */
const unsigned num_ops = 200;

/* the number of threads */
const unsigned num_threads = 30;

/*
 * writer -- (internal) bump up the counter by 2
 */
void
writer(nvobj::persistent_ptr<root> proot)
{
	for (unsigned i = 0; i < num_ops; ++i) {
		std::lock_guard<nvobj::shared_mutex> lock(proot->pmutex);
		++(proot->counter);
		++(proot->counter);
	}
}

/*
 * reader -- (internal) verify if the counter is even
 */
void
reader(nvobj::persistent_ptr<root> proot)
{
	for (unsigned i = 0; i < num_ops; ++i) {
		proot->pmutex.lock_shared();
		UT_ASSERTeq(proot->counter % 2, 0);
		proot->pmutex.unlock_shared();
	}
}

/*
 * writer_trylock -- (internal) trylock bump the counter by 2
 */
void
writer_trylock(nvobj::persistent_ptr<root> proot)
{
	for (;;) {
		if (proot->pmutex.try_lock()) {
			--(proot->counter);
			--(proot->counter);
			proot->pmutex.unlock();
			return;
		}
	}
}

/*
 * reader_trylock -- (internal) trylock verify that the counter is even
 */
void
reader_trylock(nvobj::persistent_ptr<root> proot)
{
	for (;;) {
		if (proot->pmutex.try_lock_shared()) {
			UT_ASSERTeq(proot->counter % 2, 0);
			proot->pmutex.unlock_shared();
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
		pop.handle(), &raw_mutex, sizeof(PMEMrwlock), 1,
		[](PMEMobjpool *pop, void *ptr, void *arg) -> int {
			PMEMrwlock *mtx = static_cast<PMEMrwlock *>(ptr);
			pmemobj_memset_persist(pop, mtx, 1, sizeof(*mtx));
			return 0;
		},
		nullptr);

	nvobj::shared_mutex *placed_mtx =
		new (pmemobj_direct(raw_mutex)) nvobj::shared_mutex;
	std::unique_lock<nvobj::shared_mutex> lck(*placed_mtx);
}

/*
 * mutex_test -- (internal) launch worker threads to test the pshared_mutex
 */
template <typename Worker>
void
mutex_test(nvobj::pool<root> &pop, const Worker &writer, const Worker &reader)
{
	const auto total_threads = num_threads * 2u;
	std::thread threads[total_threads];

	nvobj::persistent_ptr<root> proot = pop.root();

	for (unsigned i = 0; i < total_threads; i += 2) {
		threads[i] = std::thread(writer, proot);
		threads[i + 1] = std::thread(reader, proot);
	}

	for (unsigned i = 0; i < total_threads; ++i)
		threads[i].join();
}

void
test_stack()
{
	/* shared_mutex is not allowed outside of pmem */
	try {
		nvobj::shared_mutex stack_mutex;
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

	/* pmemobj doesn't implement this on Windows */
#if !defined(_WIN32)
	/* double wrlock should fail with an exception */
	try {
		proot->pmutex.lock();
	} catch (pmem::lock_error &le) {
	} catch (...) {
		UT_ASSERT(0);
	}

	/*
	 * rdlock should fail with an exception when wrlock is already taken
	 * by the same thread
	 */
	try {
		proot->pmutex.lock_shared();
	} catch (pmem::lock_error &le) {
	} catch (...) {
		UT_ASSERT(0);
	}
#endif

	/* but try_locking fails with false */
	UT_ASSERT(proot->pmutex.try_lock() == false);
	UT_ASSERT(proot->pmutex.try_lock_shared() == false);

	proot->pmutex.unlock();
}
}

int
main(int argc, char *argv[])
{
	START();

	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<root>::create(path, LAYOUT, PMEMOBJ_MIN_POOL,
						S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	mutex_zero_test(pop);

	auto expected = num_threads * num_ops * 2;
	mutex_test(pop, writer, reader);
	UT_ASSERTeq(pop.root()->counter, expected);

	/* trylocks are not tested as exhaustively */
	expected -= num_threads * 2;
	mutex_test(pop, writer_trylock, reader_trylock);
	UT_ASSERTeq(pop.root()->counter, expected);

	/* pmemcheck related persist */
	pmemobj_persist(pop.handle(), &(pop.root()->counter),
			sizeof(pop.root()->counter));

	test_stack();
	test_error_handling(pop);

	pop.close();

	return 0;
}
