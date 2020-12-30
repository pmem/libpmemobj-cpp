// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

/*
 * concurrent_hash_map.cpp -- pmem::obj::concurrent_hash_map test
 *
 */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <iterator>
#include <thread>
#include <vector>

#include "concurrent_hash_map_layout.hpp"
#include <libpmemobj++/container/concurrent_hash_map.hpp>
#include <libpmemobj++/container/string.hpp>

#define LAYOUT "concurrent_hash_map"

namespace nvobj = pmem::obj;

/**
 * Specialization of pmem::obj::string
 */
namespace std
{
template <>
struct hash<pmem::obj::string> {
	size_t
	operator()(const pmem::obj::string &x) const
	{
		return hash<const char *>()(x.c_str());
	}
};
} /* namespace std */

typedef nvobj::concurrent_hash_map<nvobj::p<long long>, nvobj::p<long long>>
	persistent_map_type;

typedef nvobj::concurrent_hash_map<nvobj::string, nvobj::string>
	persistent_map_type_string;

typedef nvobj::concurrent_hash_map<nvobj::string, nvobj::p<long long>>
	persistent_map_type_mixed;

struct root {
};

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<root>::create(
			path, LAYOUT, PMEMOBJ_MIN_POOL * 20, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	static_assert(std::is_standard_layout<persistent_map_type>::value, "");
	static_assert(sizeof(persistent_map_type::value_type) == 16, "");

	hashmap_test<persistent_map_type, 16>::check_layout(pop);
	hashmap_test<persistent_map_type, 16>::check_layout_different_version(
		pop);

	static_assert(
		std::is_standard_layout<persistent_map_type_string>::value, "");
	static_assert(sizeof(persistent_map_type_string::value_type) == 64, "");

	hashmap_test<persistent_map_type_string, 64>::check_layout(pop);

	static_assert(std::is_standard_layout<persistent_map_type_mixed>::value,
		      "");
	static_assert(sizeof(persistent_map_type_mixed::value_type) == 40, "");

	hashmap_test<persistent_map_type_mixed, 40>::check_layout(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
