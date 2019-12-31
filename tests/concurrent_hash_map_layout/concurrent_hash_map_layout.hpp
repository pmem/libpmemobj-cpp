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
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

namespace nvobj = pmem::obj;

static constexpr std::size_t HASHMAP_SIZE = 1312;
static constexpr std::size_t BUCKET_SIZE = 80;

static constexpr std::size_t CACHELINE_SIZE = 64;

/*
 * Test is implemented in inherited class to get access to protected variables.
 */
template <typename MapType, std::size_t value_size>
struct hashmap_test : public MapType {
	static constexpr std::size_t NODE_SIZE = 72 + value_size;

	using persistent_map_type = MapType;
	using hash_map_base = typename MapType::hash_map_base;

	template <typename T>
	static void
	check_layout_hashmap_base(T &t)
	{
		ASSERT_ALIGNED_BEGIN(T, t);
		ASSERT_ALIGNED_FIELD(T, t, my_pool_uuid);
		ASSERT_ALIGNED_FIELD(T, t, layout_features);
		ASSERT_ALIGNED_FIELD(T, t, my_mask_reserved);
		ASSERT_ALIGNED_FIELD(T, t, my_mask);
		ASSERT_ALIGNED_FIELD(T, t, padding1);
		ASSERT_OFFSET_CHECKPOINT(T, CACHELINE_SIZE);
		ASSERT_ALIGNED_FIELD(T, t, my_table);
		ASSERT_ALIGNED_FIELD(T, t, my_size);
		ASSERT_ALIGNED_FIELD(T, t, padding2);
		ASSERT_OFFSET_CHECKPOINT(T, 16 * CACHELINE_SIZE);
		ASSERT_ALIGNED_FIELD(T, t, reserved);
		ASSERT_OFFSET_CHECKPOINT(T, 17 * CACHELINE_SIZE);
		ASSERT_ALIGNED_FIELD(T, t, my_segment_enable_mutex);
		ASSERT_OFFSET_CHECKPOINT(T, 18 * CACHELINE_SIZE);
		ASSERT_ALIGNED_FIELD(T, t, my_embedded_segment);
		ASSERT_ALIGNED_CHECK(T);
		static_assert(sizeof(T) == HASHMAP_SIZE, "");
		static_assert(std::is_standard_layout<T>::value, "");
	}

	static void
	check_layout(nvobj::pool_base &pop)
	{

		pmem::obj::persistent_ptr<hashmap_test> map;
		pmem::obj::persistent_ptr<hash_map_base> map_base;
		pmem::obj::persistent_ptr<typename hashmap_test::bucket> bucket;
		pmem::obj::persistent_ptr<typename hashmap_test::node> node;

		pmem::obj::transaction::run(pop, [&] {
			map = nvobj::make_persistent<hashmap_test>();

			bucket = nvobj::make_persistent<typename hashmap_test::bucket>();

			node = nvobj::make_persistent<typename hashmap_test::node>(
				nullptr);
		});

		/* hash_map_base and peristent_map_type should be the same */
		check_layout_hashmap_base(*map_base);
		check_layout_hashmap_base(*map);

		static_assert(std::is_standard_layout<
				     typename persistent_map_type::bucket>::value,
			      "");

		ASSERT_ALIGNED_BEGIN(typename persistent_map_type::bucket, *bucket);
		ASSERT_ALIGNED_FIELD(typename persistent_map_type::bucket, *bucket,
				     mutex);
		ASSERT_ALIGNED_FIELD(typename persistent_map_type::bucket, *bucket,
				     rehashed);
		ASSERT_ALIGNED_FIELD(typename persistent_map_type::bucket, *bucket,
				     node_list);
		ASSERT_ALIGNED_CHECK(typename persistent_map_type::bucket);
		static_assert(
			sizeof(typename persistent_map_type::bucket) == BUCKET_SIZE, "");

		static_assert(std::is_standard_layout<typename persistent_map_type::node>::value,
		 "");

		ASSERT_ALIGNED_BEGIN(typename persistent_map_type::node, *node);
		ASSERT_ALIGNED_FIELD(typename persistent_map_type::node, *node, next);
		ASSERT_ALIGNED_FIELD(typename persistent_map_type::node,*node , mutex);
		ASSERT_ALIGNED_FIELD(typename persistent_map_type::node,*node ,item);
		ASSERT_ALIGNED_CHECK(typename persistent_map_type::node);
		static_assert(sizeof(typename persistent_map_type::node) == NODE_SIZE,
			      "");
		
		pmem::obj::transaction::run(pop, [&] {
			nvobj::delete_persistent<hashmap_test>(map);
			nvobj::delete_persistent<typename hashmap_test::bucket>(bucket);
			nvobj::delete_persistent<typename hashmap_test::node>(node);
		});
	}

	static void
	check_layout_different_version(nvobj::pool_base &pop)
	{
		pmem::obj::persistent_ptr<hashmap_test> map;
		pmem::obj::transaction::run(pop, [&] {
			map = nvobj::make_persistent<hashmap_test>();
		});

		map->layout_features.incompat = static_cast<uint32_t>(-1);

		try {
			map->runtime_initialize(true);
			UT_ASSERT(0);
		} catch (pmem::layout_error &) {
		} catch (...) {
			UT_ASSERT(0);
		}

		try {
			map->runtime_initialize(false);
			UT_ASSERT(0);
		} catch (pmem::layout_error &) {
		} catch (...) {
			UT_ASSERT(0);
		}

		pmem::obj::transaction::run(pop, [&] {
			nvobj::delete_persistent<hashmap_test>(map);
		});
	}
};
