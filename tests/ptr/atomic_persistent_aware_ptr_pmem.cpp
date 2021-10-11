// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "thread_helpers.hpp"
#include "unittest.hpp"

#include <libpmemobj++/experimental/atomic_persistent_aware_ptr.hpp>
#include <libpmemobj++/experimental/self_relative_ptr.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/persistent_ptr_base.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#define LAYOUT "cpp"

namespace nvobj = pmem::obj;

template <typename T>
using self_relative_ptr = pmem::obj::experimental::self_relative_ptr<T>;
template <typename T, typename ReadOptimized>
using atomic_ptr =
	pmem::obj::experimental::atomic_persistent_aware_ptr<T, ReadOptimized>;

constexpr int ARR_SIZE = 10000;

template <typename ReadOptimized>
struct root {
	atomic_ptr<nvobj::p<int>[ARR_SIZE], ReadOptimized> parr;
	atomic_ptr<int, ReadOptimized> ptr;
};

namespace
{

template <typename ReadOptimized>
void
test_ptr_allocation(nvobj::pool<root<ReadOptimized>> &pop)
{
	auto r = pop.root();
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->ptr.load() == nullptr);
			r->ptr = nvobj::make_persistent<int>();
		});
	} catch (...) {
		ASSERT_UNREACHABLE;
	}

	UT_ASSERT(r->ptr.load() != nullptr);

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<int>(r->ptr.load());
			r->ptr.store(nullptr);
		});
	} catch (...) {
		ASSERT_UNREACHABLE;
	}

	UT_ASSERT(r->ptr.load() == nullptr);
}

template <typename ReadOptimized>
void
test_ptr_visibility(nvobj::pool<root<ReadOptimized>> &pop)
{
	auto r = pop.root();
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->ptr.load() == nullptr);
			r->ptr = nvobj::make_persistent<int>();
		});
	} catch (...) {
		ASSERT_UNREACHABLE;
	}

	UT_ASSERT(r->ptr.load() != nullptr);

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<int>(r->ptr.load());
			r->ptr.store(nullptr);
		});
	} catch (...) {
		ASSERT_UNREACHABLE;
	}

	UT_ASSERT(r->ptr.load() == nullptr);
}

} /* namespace */

inline const char *const
BoolToString(bool b)
{
	return b ? "_ropt" : "_wopt";
}

template <typename ReadOptimized>
static void
test(char *path)
{
	std::string path_str(path);
	path_str += BoolToString(ReadOptimized::value);

	nvobj::pool<root<ReadOptimized>> pop;

	try {
		pop = nvobj::pool<root<ReadOptimized>>::create(
			path_str, LAYOUT, PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path_str.c_str());
	}

	test_ptr_allocation(pop);
	test_ptr_visibility(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);
	auto ret_writeopt = run_test([&] { test<std::false_type>(argv[1]); });
	auto ret_readopt = run_test([&] { test<std::true_type>(argv[1]); });
	return (ret_writeopt && ret_readopt);
}
