// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2018, Intel Corporation */

/*
 * mutex.cpp -- C++ documentation snippets.
 */

//! [unique_guard_example]
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <mutex>

namespace nvobj = pmem::obj;

void
unique_guard_example()
{
	// pool root structure
	struct root {
		nvobj::mutex pmutex;
	};

	// create a pmemobj pool
	auto pop = nvobj::pool<root>::create("poolfile", "layout",
					     PMEMOBJ_MIN_POOL);
	auto proot = pop.root();

	// typical usage schemes
	std::lock_guard<nvobj::mutex> guard(proot->pmutex);

	std::unique_lock<nvobj::mutex> other_guard(proot->pmutex);
}
//! [unique_guard_example]

//! [shared_mutex_example]
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/shared_mutex.hpp>
#include <mutex>

namespace nvobj = pmem::obj;

void
shared_mutex_example()
{
	// pool root structure
	struct root {
		nvobj::shared_mutex pmutex;
	};

	// create a pmemobj pool
	auto pop = nvobj::pool<root>::create("poolfile", "layout",
					     PMEMOBJ_MIN_POOL);
	auto proot = pop.root();

	// typical usage schemes
	proot->pmutex.lock_shared();

	std::unique_lock<nvobj::shared_mutex> guard(proot->pmutex);
}
//! [shared_mutex_example]

//! [timed_mutex_example]
#include <chrono>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/timed_mutex.hpp>

namespace nvobj = pmem::obj;

void
timed_mutex_example()
{
	// pool root structure
	struct root {
		nvobj::timed_mutex pmutex;
	};

	// create a pmemobj pool
	auto pop = nvobj::pool<root>::create("poolfile", "layout",
					     PMEMOBJ_MIN_POOL);
	auto proot = pop.root();

	const auto timeout = std::chrono::milliseconds(100);

	// typical usage schemes
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

namespace nvobj = pmem::obj;

void
cond_var_example()
{
	// pool root structure
	struct root {
		nvobj::mutex pmutex;
		nvobj::condition_variable cond;
		int counter;
	};

	// create a pmemobj pool
	auto pop = nvobj::pool<root>::create("poolfile", "layout",
					     PMEMOBJ_MIN_POOL);

	auto proot = pop.root();

	// run worker to bump up the counter
	std::thread worker([&] {
		std::unique_lock<nvobj::mutex> lock(proot->pmutex);
		while (proot->counter < 1000)
			++proot->counter;
		// unlock before notifying to avoid blocking on waiting thread
		lock.unlock();
		// notify the waiting thread
		proot->cond.notify_one();
	});

	std::unique_lock<nvobj::mutex> lock(proot->pmutex);
	// wait on condition variable
	proot->cond.wait(lock, [&] { return proot->counter >= 1000; });

	worker.join();
}
//! [cond_var_example]
