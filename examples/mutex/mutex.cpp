// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2021, Intel Corporation */

/*
 * mutex.cpp -- C++ documentation snippets.
 */

#include <iostream>
//! [unique_guard_example]
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <mutex>

void
unique_guard_example()
{
	/* pool root structure */
	struct root {
		pmem::obj::mutex pmutex;
	};

	/* create a pmemobj pool */
	auto pop = pmem::obj::pool<root>::create("poolfile", "layout",
						 PMEMOBJ_MIN_POOL);
	auto proot = pop.root();

	/* typical usage schemes */
	std::lock_guard<pmem::obj::mutex> guard(proot->pmutex);

	std::unique_lock<pmem::obj::mutex> other_guard(proot->pmutex);
}
//! [unique_guard_example]

//! [shared_mutex_example]
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/shared_mutex.hpp>
#include <mutex>

void
shared_mutex_example()
{
	/* pool root structure */
	struct root {
		pmem::obj::shared_mutex pmutex;
	};

	/* create a pmemobj pool */
	auto pop = pmem::obj::pool<root>::create("poolfile", "layout",
						 PMEMOBJ_MIN_POOL);
	auto proot = pop.root();

	/* typical usage schemes */
	proot->pmutex.lock_shared();

	std::unique_lock<pmem::obj::shared_mutex> guard(proot->pmutex);
}
//! [shared_mutex_example]

//! [timed_mutex_example]
#include <chrono>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/timed_mutex.hpp>

void
timed_mutex_example()
{
	/* pool root structure */
	struct root {
		pmem::obj::timed_mutex pmutex;
	};

	/* create a pmemobj pool */
	auto pop = pmem::obj::pool<root>::create("poolfile", "layout",
						 PMEMOBJ_MIN_POOL);
	auto proot = pop.root();

	const auto timeout = std::chrono::milliseconds(100);

	/* typical usage schemes */
	proot->pmutex.try_lock_for(timeout);

	proot->pmutex.try_lock_until(std::chrono::steady_clock::now() +
				     timeout);
}
//! [timed_mutex_example]

//! [cond_var_example]
#include <libpmemobj++/condition_variable.hpp>
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <mutex>
#include <thread>

void
cond_var_example()
{
	/* pool root structure */
	struct root {
		pmem::obj::mutex pmutex;
		pmem::obj::condition_variable cond;
		int counter;
	};

	/* create a pmemobj pool */
	auto pop = pmem::obj::pool<root>::create("poolfile", "layout",
						 PMEMOBJ_MIN_POOL);

	auto proot = pop.root();

	/* run worker to bump up the counter */
	std::thread worker([&] {
		std::unique_lock<pmem::obj::mutex> lock(proot->pmutex);
		while (proot->counter < 1000)
			++proot->counter;
		/* unlock before notifying to avoid blocking on waiting thread
		 */
		lock.unlock();
		/* notify the waiting thread */
		proot->cond.notify_one();
	});

	std::unique_lock<pmem::obj::mutex> lock(proot->pmutex);
	/* wait on condition variable */
	proot->cond.wait(lock, [&] { return proot->counter >= 1000; });

	worker.join();
}
//! [cond_var_example]

int
main()
{
	try {
		unique_guard_example();
		shared_mutex_example();
		timed_mutex_example();
		cond_var_example();
	} catch (const std::exception &e) {
		std::cerr << "Exception " << e.what() << std::endl;
		return -1;
	}
	return 0;
}
