// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * concurrent_map.cpp -- pmem::obj::concurrent_map test
 *
 */

#include "thread_helpers.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <iterator>
#include <thread>
#include <vector>

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/experimental/concurrent_map.hpp>

#include <libpmemobj.h>

#define LAYOUT "concurrent_map"

namespace nvobj = pmem::obj;

namespace
{

const size_t NUMBER_ITEMS_INSERT = 50;

const size_t init_concurrency = 8;

const size_t TOTAL_ITEMS = NUMBER_ITEMS_INSERT * init_concurrency;

const int mt_insert_key = 55;

struct hetero_less {
	using is_transparent = void;
	template <typename T1, typename T2>
	bool
	operator()(const T1 &lhs, const T2 &rhs) const
	{
		return lhs < rhs;
	}
};

typedef nvobj::experimental::concurrent_map<nvobj::string, nvobj::string,
					    hetero_less>
	persistent_map_type_string;

struct root {
	nvobj::persistent_ptr<persistent_map_type_string> c;
	nvobj::p<bool> reader_status;
};

std::string
gen_key(persistent_map_type_string &, int i)
{
	return std::to_string(i);
}

void
gdb_sync1()
{
}
void
gdb_sync2()
{
}
void
gdb_sync3()
{
}
void
gdb_sync_exit()
{
}

static int loop_sync_1 = 1;
static int loop_sync_2 = 1;

int
num_allocs(nvobj::pool<root> &pop)
{
	auto oid = pmemobj_first(pop.handle());
	int num = 0;

	while (!OID_IS_NULL(oid)) {
		num++;
		oid = pmemobj_next(oid);
	}

	return num;
}

struct test_case {
	virtual void
	insert(nvobj::pool<root> &pop,
	       nvobj::persistent_ptr<persistent_map_type_string> &map) = 0;

	virtual void
	check(nvobj::pool<root> &pop,
	      nvobj::persistent_ptr<persistent_map_type_string> &map) = 0;

	virtual ~test_case()
	{
	}
};

struct test_case_0 : public test_case {
	void
	insert(nvobj::pool<root> &pop,
	       nvobj::persistent_ptr<persistent_map_type_string> &map) override
	{
		UT_ASSERT(map != nullptr);

		map->runtime_initialize();

		/* prepare concurrent_map */
		parallel_exec(init_concurrency, [&](size_t thread_id) {
			int begin = thread_id * NUMBER_ITEMS_INSERT;
			int end = begin + int(NUMBER_ITEMS_INSERT);
			for (int i = begin; i < end; ++i) {
				auto ret = map->emplace(gen_key(*map, i * 10),
							gen_key(*map, i * 10));
				UT_ASSERT(ret.second == true);

				UT_ASSERTeq(map->count(gen_key(*map, i * 10)),
					    1);
			}
		});

		UT_ASSERT(map->size() == TOTAL_ITEMS);

		UT_ASSERTeq(map->count(gen_key(*map, mt_insert_key)), 0);

		std::pair<persistent_map_type_string::iterator, bool> r1 = {
			{}, false};
		std::pair<persistent_map_type_string::iterator, bool> r2 = {
			{}, false};
		pop.root()->reader_status = false;
		pop.persist(pop.root()->reader_status);

		/* two threads trying to insert the same element and one reader
		 */
		parallel_xexec(
			3,
			[&](size_t thread_id,
			    std::function<void(void)> syncthreads) {
				syncthreads();

				if (thread_id == 0) {
					gdb_sync1();

					r1 = map->emplace(
						gen_key(*map, mt_insert_key),
						gen_key(*map, mt_insert_key));
				} else if (thread_id == 1) {
					while (loop_sync_1) {
						gdb_sync2();
					};

					r2 = map->emplace(
						gen_key(*map, mt_insert_key),
						gen_key(*map, mt_insert_key));
				} else {
					while (loop_sync_2) {
						gdb_sync3();
					};

					pop.root()->reader_status =
						map->count(gen_key(
							*map, mt_insert_key)) ==
						1;
					pop.persist(pop.root()->reader_status);
				}

				gdb_sync_exit();
			});
	}

	void
	check(nvobj::pool<root> &pop,
	      nvobj::persistent_ptr<persistent_map_type_string> &map) override
	{
		UT_ASSERT(map != nullptr);

		/* Reader did not read the new node */
		UT_ASSERT(!pop.root()->reader_status);

		auto initial_nodes_num = num_allocs(pop);

		map->runtime_initialize();

		auto cleared_nodes_num = num_allocs(pop);

		/* both writer threads allocated node in tls, both should be
		 * cleared */
		UT_ASSERTeq(cleared_nodes_num, initial_nodes_num - 2);

		UT_ASSERT(map->size() == TOTAL_ITEMS);

		UT_ASSERTeq(map->count(gen_key(*map, mt_insert_key)), 0);
	}
};

struct test_case_1_2 : public test_case {
	void
	insert(nvobj::pool<root> &pop,
	       nvobj::persistent_ptr<persistent_map_type_string> &map) override
	{
		UT_ASSERT(map != nullptr);

		map->runtime_initialize();

		/* prepare concurrent_map */
		parallel_exec(init_concurrency, [&](size_t thread_id) {
			int begin = thread_id * NUMBER_ITEMS_INSERT;
			int end = begin + int(NUMBER_ITEMS_INSERT);
			for (int i = begin; i < end; ++i) {
				auto ret = map->emplace(gen_key(*map, i * 10),
							gen_key(*map, i * 10));
				UT_ASSERT(ret.second == true);

				UT_ASSERTeq(map->count(gen_key(*map, i * 10)),
					    1);
			}
		});

		UT_ASSERT(map->size() == TOTAL_ITEMS);

		UT_ASSERTeq(map->count(gen_key(*map, mt_insert_key)), 0);

		std::pair<persistent_map_type_string::iterator, bool> r1 = {
			{}, false};
		std::pair<persistent_map_type_string::iterator, bool> r2 = {
			{}, false};
		pop.root()->reader_status = false;
		pop.persist(pop.root()->reader_status);

		/* two threads trying to insert the same element and one reader
		 */
		parallel_xexec(
			3,
			[&](size_t thread_id,
			    std::function<void(void)> syncthreads) {
				syncthreads();

				if (thread_id == 0) {
					gdb_sync1();

					r1 = map->emplace(
						gen_key(*map, mt_insert_key),
						gen_key(*map, mt_insert_key));
				} else if (thread_id == 1) {
					while (loop_sync_1) {
						gdb_sync2();
					};

					r2 = map->emplace(
						gen_key(*map, mt_insert_key),
						gen_key(*map, mt_insert_key));
				} else {
					while (loop_sync_2) {
						gdb_sync3();
					};

					pop.root()->reader_status =
						map->count(gen_key(
							*map, mt_insert_key)) ==
						1;
					pop.persist(pop.root()->reader_status);
				}

				gdb_sync_exit();
			});
	}

	void
	check(nvobj::pool<root> &pop,
	      nvobj::persistent_ptr<persistent_map_type_string> &map) override
	{
		UT_ASSERT(map != nullptr);

		/* Reader did read the new node */
		UT_ASSERT(pop.root()->reader_status);

		auto initial_nodes_num = num_allocs(pop);

		map->runtime_initialize();

		auto cleared_nodes_num = num_allocs(pop);

		/* only one thread allocated nodes which was not inserted */
		UT_ASSERTeq(cleared_nodes_num, initial_nodes_num - 1);

		UT_ASSERT(map->size() == TOTAL_ITEMS + 1);

		UT_ASSERTeq(map->count(gen_key(*map, mt_insert_key)), 1);
	}
};
}

static void
test(int argc, char *argv[])
{
	if (argc < 4) {
		UT_FATAL("usage: %s mode[i/c] test_case file-name", argv[0]);
	}

	auto path = std::string(argv[3]);
	auto mode = std::string(argv[1]);
	auto t_case = std::stoul(argv[2]);

	std::vector<std::unique_ptr<test_case>> cases;
	cases.emplace_back(new test_case_0);
	cases.emplace_back(new test_case_1_2);
	cases.emplace_back(new test_case_1_2);

	nvobj::pool<root> pop;

	if (mode == "i") {
		try {
			pop = nvobj::pool<root>::create(path, LAYOUT,
							PMEMOBJ_MIN_POOL * 20,
							S_IWUSR | S_IRUSR);
			nvobj::transaction::run(pop, [&] {
				pop.root()->c = nvobj::make_persistent<
					persistent_map_type_string>();
			});
		} catch (pmem::pool_error &pe) {
			UT_FATAL("!pool::create: %s %s", pe.what(),
				 path.c_str());
		}

		cases[t_case]->insert(pop, pop.root()->c);

		/* gdb should have crashed the program by now */
		UT_ASSERT(false);
	} else {
		pop = nvobj::pool<root>::open(path, LAYOUT);
		cases[t_case]->check(pop, pop.root()->c);

		pop.close();
	}
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
