// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2020, Intel Corporation */

/*
 * obj_cpp_cond_var_posix.c -- cpp condition variable test
 */

#include "pthread_common.hpp"
#include "unittest.hpp"

#include <libpmemobj++/condition_variable.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj/atomic_base.h>

#include <mutex>
#include <vector>

#define LAYOUT "cpp"

namespace nvobj = pmem::obj;

namespace
{

/* convenience typedef */
typedef void *(*reader_type)(void *);

/* pool root structure */
struct root {
	nvobj::mutex pmutex;
	nvobj::condition_variable cond;
	unsigned counter;
};

/* the number of threads */
const unsigned num_threads = 30;

/* notification limit */
const unsigned limit = 7000;

/* cond wait time in milliseconds */
const std::chrono::milliseconds wait_time(150);

/* arguments for write worker */
struct writer_args {
	nvobj::persistent_ptr<struct root> proot;
	bool notify;
	bool all;
};

/*
 * write_notify -- (internal) bump up the counter up to a limit and notify
 */
void *
write_notify(void *args)
{
	struct writer_args *wargs = static_cast<struct writer_args *>(args);

	std::lock_guard<nvobj::mutex> lock(wargs->proot->pmutex);

	while (wargs->proot->counter < limit)
		wargs->proot->counter++;

	if (wargs->notify) {
		if (wargs->all)
			wargs->proot->cond.notify_all();
		else
			wargs->proot->cond.notify_one();
	}

	return nullptr;
}

/*
 * reader_mutex -- (internal) verify the counter value
 */
void *
reader_mutex(void *arg)
{
	nvobj::persistent_ptr<struct root> *proot =
		static_cast<nvobj::persistent_ptr<struct root> *>(arg);
	(*proot)->pmutex.lock();
	while ((*proot)->counter != limit)
		(*proot)->cond.wait((*proot)->pmutex);

	UT_ASSERTeq((*proot)->counter, limit);
	(*proot)->pmutex.unlock();

	return nullptr;
}

/*
 * reader_mutex_pred -- (internal) verify the counter value
 */
void *
reader_mutex_pred(void *arg)
{
	nvobj::persistent_ptr<struct root> *proot =
		static_cast<nvobj::persistent_ptr<struct root> *>(arg);
	(*proot)->pmutex.lock();
	(*proot)->cond.wait((*proot)->pmutex,
			    [&]() { return (*proot)->counter == limit; });

	UT_ASSERTeq((*proot)->counter, limit);
	(*proot)->pmutex.unlock();

	return nullptr;
}

/*
 * reader_lock -- (internal) verify the counter value
 */
void *
reader_lock(void *arg)
{
	nvobj::persistent_ptr<struct root> *proot =
		static_cast<nvobj::persistent_ptr<struct root> *>(arg);
	std::unique_lock<nvobj::mutex> lock((*proot)->pmutex);
	while ((*proot)->counter != limit)
		(*proot)->cond.wait(lock);

	UT_ASSERTeq((*proot)->counter, limit);
	lock.unlock();

	return nullptr;
}

/*
 * reader_lock_pred -- (internal) verify the counter value
 */
void *
reader_lock_pred(void *arg)
{
	nvobj::persistent_ptr<struct root> *proot =
		static_cast<nvobj::persistent_ptr<struct root> *>(arg);
	std::unique_lock<nvobj::mutex> lock((*proot)->pmutex);
	(*proot)->cond.wait(lock, [&]() { return (*proot)->counter == limit; });

	UT_ASSERTeq((*proot)->counter, limit);
	lock.unlock();

	return nullptr;
}

/*
 * reader_mutex_until -- (internal) verify the counter value or timeout
 */
void *
reader_mutex_until(void *arg)
{
	nvobj::persistent_ptr<struct root> *proot =
		static_cast<nvobj::persistent_ptr<struct root> *>(arg);
	(*proot)->pmutex.lock();
	auto until = std::chrono::system_clock::now();
	until += wait_time;
	auto ret = (*proot)->cond.wait_until((*proot)->pmutex, until);

	auto now = std::chrono::system_clock::now();
	if (ret == std::cv_status::timeout) {
		auto epsilon = std::chrono::milliseconds(10);
		auto diff =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				until - now);
		if (now < until)
			UT_ASSERT(diff < epsilon);
	} else {
		UT_ASSERTeq((*proot)->counter, limit);
	}
	(*proot)->pmutex.unlock();

	return nullptr;
}

/*
 * reader_mutex_until_pred -- (internal) verify the counter value or timeout
 */
void *
reader_mutex_until_pred(void *arg)
{
	nvobj::persistent_ptr<struct root> *proot =
		static_cast<nvobj::persistent_ptr<struct root> *>(arg);
	(*proot)->pmutex.lock();
	auto until = std::chrono::system_clock::now();
	until += wait_time;
	auto ret = (*proot)->cond.wait_until((*proot)->pmutex, until, [&]() {
		return (*proot)->counter == limit;
	});

	auto now = std::chrono::system_clock::now();
	if (ret == false) {
		auto epsilon = std::chrono::milliseconds(10);
		auto diff =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				until - now);
		if (now < until)
			UT_ASSERT(diff < epsilon);
	} else {
		UT_ASSERTeq((*proot)->counter, limit);
	}
	(*proot)->pmutex.unlock();

	return nullptr;
}

/*
 * reader_lock_until -- (internal) verify the counter value or timeout
 */
void *
reader_lock_until(void *arg)
{
	nvobj::persistent_ptr<struct root> *proot =
		static_cast<nvobj::persistent_ptr<struct root> *>(arg);
	std::unique_lock<nvobj::mutex> lock((*proot)->pmutex);
	auto until = std::chrono::system_clock::now();
	until += wait_time;
	auto ret = (*proot)->cond.wait_until(lock, until);

	auto now = std::chrono::system_clock::now();
	if (ret == std::cv_status::timeout) {
		auto epsilon = std::chrono::milliseconds(10);
		auto diff =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				until - now);
		if (now < until)
			UT_ASSERT(diff < epsilon);
	} else {
		UT_ASSERTeq((*proot)->counter, limit);
	}
	lock.unlock();

	return nullptr;
}

/*
 * reader_lock_until_pred -- (internal) verify the counter value or timeout
 */
void *
reader_lock_until_pred(void *arg)
{
	nvobj::persistent_ptr<struct root> *proot =
		static_cast<nvobj::persistent_ptr<struct root> *>(arg);
	std::unique_lock<nvobj::mutex> lock((*proot)->pmutex);
	auto until = std::chrono::system_clock::now();
	until += wait_time;
	auto ret = (*proot)->cond.wait_until(
		lock, until, [&]() { return (*proot)->counter == limit; });

	auto now = std::chrono::system_clock::now();
	if (ret == false) {
		auto epsilon = std::chrono::milliseconds(10);
		auto diff =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				until - now);
		if (now < until)
			UT_ASSERT(diff < epsilon);
	} else {
		UT_ASSERTeq((*proot)->counter, limit);
	}
	lock.unlock();

	return nullptr;
}

/*
 * reader_mutex_for -- (internal) verify the counter value or timeout
 */
void *
reader_mutex_for(void *arg)
{
	nvobj::persistent_ptr<struct root> *proot =
		static_cast<nvobj::persistent_ptr<struct root> *>(arg);
	(*proot)->pmutex.lock();
	auto until = std::chrono::system_clock::now();
	until += wait_time;
	auto ret = (*proot)->cond.wait_for((*proot)->pmutex, wait_time);

	auto now = std::chrono::system_clock::now();
	if (ret == std::cv_status::timeout) {
		auto epsilon = std::chrono::milliseconds(10);
		auto diff =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				until - now);
		if (now < until)
			UT_ASSERT(diff < epsilon);
	} else {
		UT_ASSERTeq((*proot)->counter, limit);
	}
	(*proot)->pmutex.unlock();

	return nullptr;
}

/*
 * reader_mutex_for_pred -- (internal) verify the counter value or timeout
 */
void *
reader_mutex_for_pred(void *arg)
{
	nvobj::persistent_ptr<struct root> *proot =
		static_cast<nvobj::persistent_ptr<struct root> *>(arg);
	(*proot)->pmutex.lock();
	auto until = std::chrono::system_clock::now();
	until += wait_time;
	auto ret = (*proot)->cond.wait_for((*proot)->pmutex, wait_time, [&]() {
		return (*proot)->counter == limit;
	});

	auto now = std::chrono::system_clock::now();
	if (ret == false) {
		auto epsilon = std::chrono::milliseconds(10);
		auto diff =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				until - now);
		if (now < until)
			UT_ASSERT(diff < epsilon);
	} else {
		UT_ASSERTeq((*proot)->counter, limit);
	}
	(*proot)->pmutex.unlock();

	return nullptr;
}

/*
 * reader_lock_for -- (internal) verify the counter value or timeout
 */
void *
reader_lock_for(void *arg)
{
	nvobj::persistent_ptr<struct root> *proot =
		static_cast<nvobj::persistent_ptr<struct root> *>(arg);
	std::unique_lock<nvobj::mutex> lock((*proot)->pmutex);
	auto until = std::chrono::system_clock::now();
	until += wait_time;
	auto ret = (*proot)->cond.wait_for(lock, wait_time);

	auto now = std::chrono::system_clock::now();
	if (ret == std::cv_status::timeout) {
		auto epsilon = std::chrono::milliseconds(10);
		auto diff =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				until - now);
		if (now < until)
			UT_ASSERT(diff < epsilon);
	} else {
		UT_ASSERTeq((*proot)->counter, limit);
	}
	lock.unlock();

	return nullptr;
}

/*
 * reader_lock_for_pred -- (internal) verify the counter value or timeout
 */
void *
reader_lock_for_pred(void *arg)
{
	nvobj::persistent_ptr<struct root> *proot =
		static_cast<nvobj::persistent_ptr<struct root> *>(arg);
	std::unique_lock<nvobj::mutex> lock((*proot)->pmutex);
	auto until = std::chrono::system_clock::now();
	until += wait_time;
	auto ret = (*proot)->cond.wait_for(
		lock, wait_time, [&]() { return (*proot)->counter == limit; });

	auto now = std::chrono::system_clock::now();
	if (ret == false) {
		auto epsilon = std::chrono::milliseconds(10);
		auto diff =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				until - now);
		if (now < until)
			UT_ASSERT(diff < epsilon);
	} else {
		UT_ASSERTeq((*proot)->counter, limit);
	}
	lock.unlock();

	return nullptr;
}

/*
 * cond_zero_test -- (internal) test the zeroing constructor
 */
void
cond_zero_test(nvobj::pool<struct root> &pop)
{
	PMEMoid raw_cnd;

	pmemobj_alloc(
		pop.handle(), &raw_cnd, sizeof(PMEMcond), 1,
		[](PMEMobjpool *pop, void *ptr, void *arg) -> int {
			PMEMcond *mtx = static_cast<PMEMcond *>(ptr);
			pmemobj_memset_persist(pop, mtx, 1, sizeof(*mtx));
			return 0;
		},
		nullptr);

	nvobj::condition_variable *placed_mtx =
		new (pmemobj_direct(raw_cnd)) nvobj::condition_variable;
	std::unique_lock<nvobj::mutex> lock(pop.root()->pmutex);
	placed_mtx->wait_for(lock, wait_time, []() { return false; });
}

/*
 * mutex_test -- (internal) launch worker threads to test the pshared_mutex
 */
template <typename Reader, typename Writer>
void
mutex_test(nvobj::pool<struct root> &pop, bool notify, bool notify_all,
	   Reader writer, Writer reader)
{
	const auto total_threads = num_threads * 2u;
	pthread_t threads[total_threads];

	nvobj::persistent_ptr<struct root> proot = pop.root();
	struct writer_args wargs = {proot, notify, notify_all};

	for (unsigned i = 0; i < total_threads; i += 2) {
		ut_pthread_create(&threads[i], nullptr, reader, &proot);
		ut_pthread_create(&threads[i + 1], nullptr, writer, &wargs);
	}

	for (unsigned i = 0; i < total_threads; ++i)
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

	cond_zero_test(pop);

	std::vector<reader_type> notify_functions(
		{reader_mutex, reader_mutex_pred, reader_lock, reader_lock_pred,
		 reader_mutex_until, reader_mutex_until_pred, reader_lock_until,
		 reader_lock_until_pred, reader_mutex_for,
		 reader_mutex_for_pred, reader_lock_for, reader_lock_for_pred});

	for (auto func : notify_functions) {
		unsigned reset_value = 42;

		mutex_test(pop, true, false, write_notify, func);
		pop.root()->counter = reset_value;

		mutex_test(pop, true, true, write_notify, func);
		pop.root()->counter = reset_value;
	}

	std::vector<reader_type> not_notify_functions(
		{reader_mutex_until, reader_mutex_until_pred, reader_lock_until,
		 reader_lock_until_pred, reader_mutex_for,
		 reader_mutex_for_pred, reader_lock_for, reader_lock_for_pred});

	for (auto func : not_notify_functions) {
		unsigned reset_value = 42;

		mutex_test(pop, false, false, write_notify, func);
		pop.root()->counter = reset_value;

		mutex_test(pop, false, true, write_notify, func);
		pop.root()->counter = reset_value;
	}

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
