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

#include <libpmemobj++/experimental/concurrent_hash_map.hpp>

#define LAYOUT "concurrent_hash_map"

namespace nvobj = pmem::obj;

typedef nvobj::experimental::concurrent_hash_map<nvobj::p<long long>,
						 nvobj::p<long long>>
	persistent_map_type;

struct root {
};

static_assert(sizeof(persistent_map_type::value_type) == 16, "");

static constexpr std::size_t HASHMAP_SIZE = 1344;
static constexpr std::size_t BUCKET_SIZE = 96;
static constexpr std::size_t NODE_SIZE =
	72 + sizeof(persistent_map_type::value_type);

static constexpr std::size_t CACHELINE_SIZE = 64;

/*
 * Test is implemented in inherited class to get access to protected variables.
 */
struct hashmap_test : public persistent_map_type {
	template <typename T>
	static void
	check_layout_hashmap_base(T &t)
	{
		ASSERT_ALIGNED_BEGIN(T, t);
		ASSERT_ALIGNED_FIELD(T, t, my_pool_uuid);
		ASSERT_ALIGNED_FIELD(T, t, version);
		ASSERT_ALIGNED_FIELD(T, t, my_mask_reserved);
		ASSERT_ALIGNED_FIELD(T, t, my_mask);
		ASSERT_ALIGNED_FIELD(T, t, padding1);
		ASSERT_OFFSET_CHECKPOINT(T, CACHELINE_SIZE);
		ASSERT_ALIGNED_FIELD(T, t, my_table);
		ASSERT_ALIGNED_FIELD(T, t, my_size);
		ASSERT_ALIGNED_FIELD(T, t, padding2);
		ASSERT_OFFSET_CHECKPOINT(T, 16 * CACHELINE_SIZE);
		ASSERT_ALIGNED_FIELD(T, t, my_embedded_segment);
		ASSERT_ALIGNED_FIELD(T, t, my_segment_enable_mutex);
		ASSERT_ALIGNED_FIELD(T, t, reserved);
		ASSERT_ALIGNED_CHECK(T);
		static_assert(sizeof(T) == HASHMAP_SIZE, "");
		static_assert(std::is_standard_layout<T>::value, "");
	}

	static void
	check_layout(nvobj::pool<root> &pop)
	{
		pmem::obj::persistent_ptr<hashmap_test> map;
		pmem::obj::persistent_ptr<
			pmem::obj::experimental::internal::hash_map_base>
			map_base;
		pmem::obj::persistent_ptr<hashmap_test::bucket> bucket;
		pmem::obj::persistent_ptr<hashmap_test::node> node;

		pmem::obj::transaction::run(pop, [&] {
			map = nvobj::make_persistent<hashmap_test>();

			bucket = nvobj::make_persistent<hashmap_test::bucket>();

			node = nvobj::make_persistent<hashmap_test::node>(
				nullptr);
		});

		/* hash_map_base and peristent_map_type should be the same */
		check_layout_hashmap_base(*map_base);
		check_layout_hashmap_base(*map);

		static_assert(std::is_standard_layout<
				      persistent_map_type::bucket>::value,
			      "");

		ASSERT_ALIGNED_BEGIN(persistent_map_type::bucket, *bucket);
		ASSERT_ALIGNED_FIELD(persistent_map_type::bucket, *bucket,
				     mutex);
		ASSERT_ALIGNED_FIELD(persistent_map_type::bucket, *bucket,
				     rehashed);
		ASSERT_ALIGNED_FIELD(persistent_map_type::bucket, *bucket,
				     node_list);
		ASSERT_ALIGNED_FIELD(persistent_map_type::bucket, *bucket,
				     reserved);
		ASSERT_ALIGNED_CHECK(persistent_map_type::bucket);
		static_assert(
			sizeof(persistent_map_type::bucket) == BUCKET_SIZE, "");

		// XXX: node is not standard layout because following is not
		// fulfilled: Has all non-static data members and bit-fields
		// declared in the same class (either all in the derived or all
		// in some base)

		// static_assert(std::is_standard_layout<persistent_map_type::node>::value,
		// "");

		// ASSERT_ALIGNED_BEGIN(persistent_map_type::node);
		// ASSERT_ALIGNED_FIELD(persistent_map_type::node, next);
		// ASSERT_ALIGNED_FIELD(persistent_map_type::node, mutex);
		// ASSERT_ALIGNED_FIELD(persistent_map_type::node, item);
		// ASSERT_ALIGNED_FIELD(persistent_map_type::node, reserved);
		// ASSERT_ALIGNED_CHECK(persistent_map_type::node);
		static_assert(sizeof(persistent_map_type::node) == NODE_SIZE,
			      "");
	}
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

	hashmap_test::check_layout(pop);

	pop.close();

	return 0;
}
