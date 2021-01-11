// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

/*
 * simplekv_rebuild.hpp -- implementation of simple kv which uses vector to hold
 * values, string as a key and array to hold buckets. This version stores only
 * vectors of keys and values on persistent memory and rebuilds volatile hashmap
 * on restart.
 */

#include <array>
#include <functional>
#include <libpmemobj++/container/array.hpp>
#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/utils.hpp>
#include <stdexcept>
#include <string>
#include <vector>

template <typename Value, std::size_t N>
struct simple_kv_persistent;

/**
 * This class is runtime wrapper for simple_kv_peristent.
 * Value - type of the value stored in hashmap
 * N - number of buckets in hashmap
 */
template <typename Value, std::size_t N>
class simple_kv_runtime {
private:
	using volatile_key_type = std::string;
	using bucket_entry_type = std::pair<volatile_key_type, std::size_t>;
	using bucket_type = std::vector<bucket_entry_type>;
	using bucket_array_type = std::array<bucket_type, N>;

	bucket_array_type buckets;
	simple_kv_persistent<Value, N> *data;

public:
	simple_kv_runtime(simple_kv_persistent<Value, N> *data)
	{
		this->data = data;

		for (std::size_t i = 0; i < data->values.size(); i++) {
			auto volatile_key = std::string(data->keys[i].c_str(),
							data->keys[i].size());

			auto index = std::hash<std::string>{}(volatile_key) % N;
			buckets[index].emplace_back(
				bucket_entry_type{volatile_key, i});
		}
	}

	const Value &
	get(const std::string &key) const
	{
		auto index = std::hash<std::string>{}(key) % N;

		for (const auto &e : buckets[index]) {
			if (e.first == key)
				return data->values[e.second];
		}

		throw std::out_of_range("no entry in simplekv");
	}

	template <typename T>
	void
	put(const std::string &key, const T &val)
	{
		auto index = std::hash<std::string>{}(key) % N;

		/* get pool on which persistent data resides */
		auto pop = pmem::obj::pool_by_vptr(data);

		/* search for element with specified key - if found
		 * transactionally update its value */
		for (const auto &e : buckets[index]) {
			if (e.first == key) {
				pmem::obj::transaction::run(pop, [&] {
					data->values[e.second] = val;
				});

				return;
			}
		}

		/* if there is no element with specified key, insert new value
		 * to the end of values vector and key to keys vector
		 * transactionally */
		pmem::obj::transaction::run(pop, [&] {
			data->values.emplace_back(val);
			data->keys.emplace_back(key);
		});

		buckets[index].emplace_back(key, data->values.size() - 1);
	}
};

/**
 * Class which is stored on persistent memory.
 * Value - type of the value stored in hashmap
 * N - number of buckets in hashmap
 */
template <typename Value, std::size_t N>
struct simple_kv_persistent {
	using key_type = pmem::obj::string;
	using value_vector = pmem::obj::vector<Value>;
	using key_vector = pmem::obj::vector<key_type>;

	/* values and keys are stored in separate vectors to optimize
	 * snapshotting. If they were stored as a pair in single vector
	 * entire pair would have to be snapshotted in case of value update */
	value_vector values;
	key_vector keys;
};
