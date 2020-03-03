// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "unittest.hpp"

#include <thread>
#include <vector>

#include <libpmemobj++/detail/pool_data.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <libpmemobj/pool_base.h>

namespace nvobj = pmem::obj;

struct root {
	nvobj::p<int> val;
};

/*
 * pool_close -- (internal) test pool close
 */
void
pool_cleanup(nvobj::pool<root> &pop1, nvobj::pool<root> &pop2)
{
	const size_t concurrency = 16;

	std::vector<std::thread> threads;
	threads.reserve(concurrency);

	auto r1 = pop1.root();
	auto r2 = pop2.root();

	size_t counter1 = 0, counter2 = 0;

	for (unsigned i = 0; i < concurrency / 2; i++)
		threads.emplace_back([&] {
			auto oid = r1.raw();
			auto pop = pmemobj_pool_by_oid(oid);
			auto *user_data =
				static_cast<pmem::detail::pool_data *>(
					pmemobj_get_user_data(pop));

			user_data->set_cleanup([&] { counter1++; });
		});

	for (unsigned i = 0; i < concurrency / 2; i++)
		threads.emplace_back([&] {
			auto oid = r2.raw();
			auto pop = pmemobj_pool_by_oid(oid);
			auto *user_data =
				static_cast<pmem::detail::pool_data *>(
					pmemobj_get_user_data(pop));

			user_data->set_cleanup([&] { counter2++; });
		});

	for (auto &t : threads)
		t.join();

	try {
		pop1.close();
	} catch (std::logic_error &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(counter1, 1);
	UT_ASSERTeq(counter2, 0);

	try {
		pop2.close();
	} catch (std::logic_error &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(counter1, 1);
	UT_ASSERTeq(counter2, 1);
}

static void
test(int argc, char *argv[])
{
	if (argc < 3) {
		UT_FATAL("usage: %s file-name1 file-name2", argv[0]);
	}

	auto pop1 = nvobj::pool<root>::create(argv[1], "pool_callbacks test",
					      PMEMOBJ_MIN_POOL * 2,
					      S_IWUSR | S_IRUSR);

	auto pop2 = nvobj::pool<root>::create(argv[2], "pool_callbacks test",
					      PMEMOBJ_MIN_POOL * 2,
					      S_IWUSR | S_IRUSR);

	pool_cleanup(pop1, pop2);
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
