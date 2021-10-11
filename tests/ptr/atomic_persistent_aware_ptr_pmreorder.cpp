// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/*
 * atomic_persistent_aware_ptr_pmreorder.cpp.cpp -- self_relative_ptr test under
 * pmreorder
 */

#include "thread_helpers.hpp"
#include "unittest.hpp"

#include <libpmemobj++/experimental/atomic_persistent_aware_ptr.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#define LAYOUT "pmreorder"

/* data (for self_relative_ptr) has to set the least significant byte to 0 */
static int TEST_DATA_R = 0xABBA;
static int TEST_DATA_W = 0xACDC;

/* mock prepared to check negative case (simulate wrong implementation) */
template <typename T>
class mock_atomic_self_relative_ptr {
public:
	using value_type = pmem::obj::experimental::self_relative_ptr<T>;

	void
	store(value_type val)
	{
		ptr.store(val);
		pmem::obj::pool_by_vptr(this).persist(&ptr, sizeof(ptr));
	}

	value_type
	load()
	{
		return ptr.load();
	}

private:
	std::atomic<pmem::obj::experimental::self_relative_ptr<T>> ptr;
};

namespace nvobj = pmem::obj;
template <typename T>
using self_relative_ptr = nvobj::experimental::self_relative_ptr<T>;
template <typename T, typename ReadOptimized>
using atomic_ptr =
	nvobj::experimental::atomic_persistent_aware_ptr<T, ReadOptimized>;

struct root {
	/* read-optimized */
	atomic_ptr<int, std::true_type> ptr_r;
	/* write-optimized */
	atomic_ptr<int, std::false_type> ptr_w;
	/* mock for negative case */
	mock_atomic_self_relative_ptr<int> ptr_neg;

	self_relative_ptr<int> read_r;
	self_relative_ptr<int> read_w;
	self_relative_ptr<int> read_neg;
};

void
insert_and_read(nvobj::pool<root> &pop)
{
	parallel_xexec(
		2,
		[&](size_t thread_id, std::function<void(void)> syncthreads) {
			syncthreads();
			auto r = pop.root();

			if (thread_id == 0) {
				VALGRIND_PMC_EMIT_LOG("PMREORDER_MARKER.BEGIN");
				/* insert test data into atomic ptr's */
				r->ptr_r.store(
					reinterpret_cast<int *>(TEST_DATA_R));
				r->ptr_w.store(
					reinterpret_cast<int *>(TEST_DATA_W));
				VALGRIND_PMC_EMIT_LOG("PMREORDER_MARKER.END");
			} else {
				/* read test data into self relative ptr's */
				r->read_w = r->ptr_w.load();
				pop.persist(r->read_w.to_persistent_ptr());
				r->read_r = r->ptr_r.load();
				pop.persist(r->read_r.to_persistent_ptr());
			}
		});
}

void
insert_and_read_mock(nvobj::pool<root> &pop)
{
	parallel_xexec(
		2,
		[&](size_t thread_id, std::function<void(void)> syncthreads) {
			syncthreads();
			auto r = pop.root();

			if (thread_id == 0) {
				VALGRIND_PMC_EMIT_LOG("PMREORDER_MARKER.BEGIN");
				/* insert test data into mock atomic ptr */
				r->ptr_neg.store(
					reinterpret_cast<int *>(TEST_DATA_R));
				VALGRIND_PMC_EMIT_LOG("PMREORDER_MARKER.END");
			} else {
				/* read test data into self relative ptr */
				r->read_neg = r->ptr_neg.load();
				pop.persist(r->read_neg.to_persistent_ptr());
			}
		});
}

void
check_consistency(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	/* we use pmreorder ReorderAccumulative engine
	 * This means we either don't have the "read_* set up or it's properly
	 * set to loaded value */
	UT_ASSERT(r->read_w == nullptr ||
		  r->ptr_w.load().get() == r->read_w.get());
	UT_ASSERT(r->read_r == nullptr ||
		  r->ptr_r.load().get() == r->read_r.get());
}

void
check_consistency_mock(nvobj::pool<root> &pop)
{
	auto r = pop.root();
	UT_ASSERT(r->ptr_neg.load().get() == r->read_neg.get());
}

static void
test(int argc, char *argv[])
{
	if (argc != 3 || strchr("ciomn", argv[1][0]) == nullptr)
		UT_FATAL("usage: %s <c|i|o|m|n> file-name", argv[0]);

	const char *path = argv[2];

	nvobj::pool<root> pop;

	try {
		if (argv[1][0] == 'c') {
			/* just create and init with nulls */
			pop = nvobj::pool<root>::create(path, LAYOUT,
							PMEMOBJ_MIN_POOL * 20,
							S_IWUSR | S_IRUSR);

			pmem::obj::transaction::run(pop, [&] {
				pop.root()->ptr_r = nullptr;
				pop.root()->ptr_w = nullptr;
				pop.root()->read_r = nullptr;
				pop.root()->read_w = nullptr;
				pop.root()->read_neg = nullptr;
			});

		} else if (argv[1][0] == 'i') {
			/* store and load in parallel */
			pop = nvobj::pool<root>::open(path, LAYOUT);
			insert_and_read(pop);

		} else if (argv[1][0] == 'o') {
			/* re-open at the end, for consistency check */
			pop = nvobj::pool<root>::open(path, LAYOUT);
			check_consistency(pop);

		} else if (argv[1][0] == 'm') {
			/* mock store and load in parallel */
			pop = nvobj::pool<root>::open(path, LAYOUT);
			insert_and_read_mock(pop);

		} else if (argv[1][0] == 'n') {
			/* re-open at the end, for negative consistency check */
			pop = nvobj::pool<root>::open(path, LAYOUT);
			check_consistency_mock(pop);
		}
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
