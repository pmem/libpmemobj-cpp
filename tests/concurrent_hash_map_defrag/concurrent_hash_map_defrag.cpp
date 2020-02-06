// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * concurrent_hash_map_defrag.cpp -- pmem::obj::concurrent_hash_map test
 *
 */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <iterator>
#include <vector>

#include <libpmemobj++/container/concurrent_hash_map.hpp>
#include <libpmemobj++/container/string.hpp>

#define LAYOUT "concurrent_hash_map"

namespace nvobj = pmem::obj;

namespace
{

class key_equal {
public:
	template <typename M, typename U>
	bool
	operator()(const M &lhs, const U &rhs) const
	{
		return lhs == rhs;
	}
};

class string_hasher {
	/* hash multiplier used by fibonacci hashing */
	static const size_t hash_multiplier = 11400714819323198485ULL;

public:
	using transparent_key_equal = key_equal;

	size_t
	operator()(const pmem::obj::string &str) const
	{
		return hash(str.c_str(), str.size());
	}

private:
	size_t
	hash(const char *str, size_t size) const
	{
		size_t h = 0;
		for (size_t i = 0; i < size; ++i) {
			h = static_cast<size_t>(str[i]) ^ (h * hash_multiplier);
		}
		return h;
	}
};

typedef nvobj::concurrent_hash_map<pmem::obj::string, pmem::obj::string,
				   string_hasher>
	persistent_map_type;

struct root {
	nvobj::persistent_ptr<persistent_map_type> cons;
};

/*
 * insert_and_erase_test_str -- (internal) test insert and erase operations
 * pmem::obj::concurrent_hash_map<pmem::obj::string, pmem::obj::string>
 */
void
insert_defrag_lookup_test(nvobj::pool<root> &pop)
{
	const size_t NUMBER_ITEMS_INSERT = 10000;
	const size_t NUMBER_HOLES = NUMBER_ITEMS_INSERT / 10;

	auto map = pop.root()->cons;

	UT_ASSERT(map != nullptr);

	map->runtime_initialize();

	pmem::obj::persistent_ptr<char> holes[NUMBER_HOLES];
	pmem::obj::persistent_ptr<persistent_map_type::value_type>
		ptr[NUMBER_ITEMS_INSERT];

	pmem::obj::transaction::run(pop, [&] {
		std::string str = " ";
		for (int i = 0; i < static_cast<int>(NUMBER_ITEMS_INSERT);
		     i++) {
			ptr[i] = pmem::obj::make_persistent<
				persistent_map_type::value_type>(str, str);
			str.append(std::to_string(i));
		}
	});

	for (int i = 0; i < static_cast<int>(NUMBER_ITEMS_INSERT); ++i) {
		map->insert(*(ptr[i]));
		if (i % 10 == 0) {
			pmem::obj::transaction::run(pop, [&] {
				holes[i / 10] =
					pmem::obj::make_persistent<char>(4096);
			});
		}
	}

	for (int i = 0; i < static_cast<int>(NUMBER_ITEMS_INSERT); ++i) {
		if (i % 10 == 0) {
			map->erase(ptr[i]->first);
			pmem::obj::transaction::run(pop, [&] {
				pmem::obj::delete_persistent<char>(
					holes[i / 10]);
			});
		}
	}

	size_t active = pop.ctl_get<size_t>("stats.heap.run_active");
	size_t allocated = pop.ctl_get<size_t>("stats.heap.run_allocated");
	float r1 = (float)active / (float)allocated;

	struct pobj_defrag_result result = map->defragment();

	/* this is to trigger global recycling */
	pop.defrag(NULL, 0);

	UT_ASSERT(result.total > 0);
	UT_ASSERT(result.relocated > 0);
	UT_ASSERT(result.total >= result.relocated);

	active = pop.ctl_get<size_t>("stats.heap.run_active");
	allocated = pop.ctl_get<size_t>("stats.heap.run_allocated");
	float r2 = (float)active / (float)allocated;

	UT_ASSERT(r2 < r1);

	for (int i = 0; i < static_cast<int>(NUMBER_ITEMS_INSERT); ++i) {
		if (i % 10 == 0)
			continue;
		persistent_map_type::accessor acc;
		bool res = map->find(acc, ptr[i]->first);

		if (res) {
			UT_ASSERT(acc->first == (ptr[i])->first);
			UT_ASSERT(acc->second == (ptr[i])->second);
		} else {
			UT_ASSERT(false);
		}
	}

	pmem::obj::transaction::run(pop, [&] {
		for (int i = 0; i < static_cast<int>(NUMBER_ITEMS_INSERT);
		     i++) {
			pmem::obj::delete_persistent<
				persistent_map_type::value_type>(ptr[i]);
		}
	});
}
}

static void
test(int argc, char *argv[])
{
	if (argc < 1) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	const char *path = argv[1];
	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<root>::create(path, LAYOUT,
						200 * PMEMOBJ_MIN_POOL,
						S_IWUSR | S_IRUSR);
		pmem::obj::transaction::run(pop, [&] {
			pop.root()->cons =
				nvobj::make_persistent<persistent_map_type>();
		});
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	insert_defrag_lookup_test(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
