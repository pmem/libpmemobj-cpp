/*
 * Copyright 2018-2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

int
main(int argc, char *argv[])
{
	START();

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

	return 0;
}
