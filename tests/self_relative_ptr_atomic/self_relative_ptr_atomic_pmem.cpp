// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "thread_helpers.hpp"
#include "unittest.hpp"

#include <libpmemobj++/experimental/atomic_self_relative_ptr.hpp>
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
template <typename T>
using atomic_ptr = std::atomic<self_relative_ptr<T>>;

constexpr int ARR_SIZE = 10000;

struct root {
	atomic_ptr<nvobj::p<int>[ARR_SIZE]> parr;
	atomic_ptr<int> ptr;
};

namespace
{

void
test_ptr_transactional(nvobj::pool<root> &pop)
{
	auto r = pop.root();
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->ptr.load() == nullptr);
			nvobj::transaction::snapshot<atomic_ptr<int>>(&r->ptr);
			r->ptr = nvobj::make_persistent<int>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(r->ptr.load() != nullptr);

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<int>(r->ptr.load());
			nvobj::transaction::snapshot<atomic_ptr<int>>(&r->ptr);
			r->ptr.store(nullptr);
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(r->ptr.load() == nullptr);
}

} /* namespace */

static void
test(int argc, char *argv[])
{
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

	test_ptr_transactional(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
