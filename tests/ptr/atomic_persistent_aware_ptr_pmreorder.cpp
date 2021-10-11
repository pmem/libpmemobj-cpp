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

/* data (for self_relative_ptr) has to be aligned to 2B */
static int TEST_DATA = 0xACDC;
static std::function<void(void)> sync_func;

/* mock prepared to check negative case (simulate wrong implementation) */
template <typename T>
class mock_atomic_self_relative_ptr {
public:
	using value_type = pmem::obj::experimental::self_relative_ptr<T>;

	void
	store(value_type val)
	{
		ptr.store(val);
		/* sync other thread to stat reading now */
		sync_func();
		/* wait for other thread to finish reading */
		sync_func();
		pmem::obj::pool_by_vptr(this).persist(&ptr, sizeof(ptr));
	}

	value_type
	load()
	{
		return ptr.load();
	}

	mock_atomic_self_relative_ptr &
	operator=(value_type const &r)
	{
		ptr = r;
		return *this;
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

template <typename PtrA, typename PtrR>
void
insert_and_read(nvobj::pool<root> &pop, PtrA &ptr_atomic, PtrR &ptr_read)
{
	parallel_xexec(
		2,
		[&](size_t thread_id, std::function<void(void)> syncthreads) {
			syncthreads();

			if (thread_id == 0) {
				VALGRIND_PMC_EMIT_LOG("PMREORDER_MARKER.BEGIN");
			}
			syncthreads();

			if (thread_id == 0) {
				/* save globalally for mock class to use */
				sync_func = syncthreads;

				/* insert test data into atomic ptr's */
				ptr_atomic.store(
					reinterpret_cast<int *>(TEST_DATA));
			} else {
				/* read test data into self relative ptr's */
				if (std::is_same<PtrA,
						 mock_atomic_self_relative_ptr<
							 int>>::value) {
					/* negative case have to be sync'ed (see
					 * comments in mock class) */
					syncthreads();
					ptr_read = ptr_atomic.load();
					syncthreads();
				} else {
					ptr_read = ptr_atomic.load();
				}
				pop.persist(&ptr_read, sizeof(ptr_read));
			}

			syncthreads();
			if (thread_id == 0) {
				VALGRIND_PMC_EMIT_LOG("PMREORDER_MARKER.END");
			}
		});
}

template <typename PtrL, typename PtrR>
void
check_consistency(nvobj::pool<root> &pop, PtrL &ptr_load, PtrR &ptr_read)
{
	/* no matter how it's reordered we either don't have the read_*
	 * set up yet, or it's properly set to loaded value */
	UT_ASSERT(ptr_read == nullptr ||
		  ptr_load.load().get() == ptr_read.get());
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
				pop.root()->ptr_neg = nullptr;
				pop.root()->read_r = nullptr;
				pop.root()->read_w = nullptr;
				pop.root()->read_neg = nullptr;
			});

		} else if (argv[1][0] == 'i') {
			/* store and load in parallel */
			pop = nvobj::pool<root>::open(path, LAYOUT);
			auto r = pop.root();
			insert_and_read(pop, r->ptr_r, r->read_r);
			insert_and_read(pop, r->ptr_w, r->read_w);

		} else if (argv[1][0] == 'o') {
			/* re-open at the end, for consistency check */
			pop = nvobj::pool<root>::open(path, LAYOUT);
			auto r = pop.root();
			check_consistency(pop, r->ptr_r, r->read_r);
			check_consistency(pop, r->ptr_w, r->read_w);

		} else if (argv[1][0] == 'm') {
			/* mock store and load in parallel */
			pop = nvobj::pool<root>::open(path, LAYOUT);
			auto r = pop.root();
			insert_and_read(pop, r->ptr_neg, r->read_neg);

		} else if (argv[1][0] == 'n') {
			/* re-open at the end, for negative consistency check */
			pop = nvobj::pool<root>::open(path, LAYOUT);
			auto r = pop.root();
			check_consistency(pop, r->ptr_neg, r->read_neg);
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
