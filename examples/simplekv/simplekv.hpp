/*
 * Copyright 2019, Intel Corporation
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
 * simplekv.hpp -- implementation of simple kv which uses vector to hold
 * values, string as a key and array to hold buckets
 */

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

/**
 * Value - type of the value stored in hashmap
 * N - number of buckets in hashmap
 */
template <typename Value, std::size_t N>
class simple_kv {
private:
	using key_type = pmem::obj::string;
	using bucket_type = pmem::obj::vector<std::pair<key_type, std::size_t>>;
	using bucket_array_type = pmem::obj::array<bucket_type, N>;
	using value_vector = pmem::obj::vector<Value>;

	bucket_array_type buckets;
	value_vector values;

public:
	simple_kv() = default;

	const Value &
	get(const std::string &key) const
	{
		auto index = std::hash<std::string>{}(key) % N;

		for (const auto &e : buckets[index]) {
			if (e.first == key)
				return values[e.second];
		}

		throw std::out_of_range("no entry in simplekv");
	}

	void
	put(const std::string &key, const Value &val)
	{
		auto index = std::hash<std::string>{}(key) % N;

		/* get pool on which this simple_kv resides */
		auto pop = pmem::obj::pool_by_vptr(this);

		/* search for element with specified key - if found
		 * transactionally update its value */
		for (const auto &e : buckets[index]) {
			if (e.first == key) {
				pmem::obj::transaction::run(
					pop, [&] { values[e.second] = val; });

				return;
			}
		}

		/* if there is no element with specified key, insert new value
		 * to the end of values vector and put reference in proper
		 * bucket transactionally */
		pmem::obj::transaction::run(pop, [&] {
			values.emplace_back(val);
			buckets[index].emplace_back(key, values.size() - 1);
		});
	}
};
