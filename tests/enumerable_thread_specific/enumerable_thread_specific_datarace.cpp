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

#include "unittest.hpp"

#include <libpmemobj++/experimental/enumerable_thread_specific.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/shared_mutex.hpp>

#include <unordered_map>

/**
 * wrapper around std::unordered_map to count elements and test if
 * storage.size() equal map.size() to avoid dataraces
 */
template <typename Key, typename T, typename Hash>
class Map {
public:
	using reference = T &;
	using map_type = typename std::unordered_map<Key, T, Hash>;
	using iterator = typename map_type::iterator;
	using const_iterator = typename map_type::const_iterator;

	Map()
	{
	}

	const_iterator
	find(const Key &key) const
	{
		return _map.find(key);
	}

	const_iterator
	cend()
	{
		return _map.cend();
	}

	void
	clear()
	{
		counter = 0;
		_map.clear();
	}

	reference operator[](const Key &key)
	{
		++counter;
		return _map[key];
	}

	static std::size_t counter;

private:
	std::unordered_map<Key, T, Hash> _map;
};

namespace nvobj = pmem::obj;

using test_type = std::size_t;
using key_type = std::thread::id;
using map_type = Map<key_type, size_t, std::hash<key_type>>;
using container_type = nvobj::experimental::enumerable_thread_specific<
	test_type, Map, pmem::obj::shared_mutex>;

struct root {
	nvobj::persistent_ptr<container_type> pptr;
};

template <>
std::size_t map_type::counter = 0;

template <typename Function>
void
parallel_exec(size_t concurrency, Function f)
{
	std::vector<std::thread> threads;
	threads.reserve(concurrency);

	for (size_t i = 0; i < concurrency; ++i) {
		threads.emplace_back(f, i);
	}

	for (auto &t : threads) {
		t.join();
	}
}

void
test(nvobj::pool<struct root> &pop)
{
	// Adding more concurrency will increase DRD test time
	const size_t concurrency = 32;

	auto tls = pop.root()->pptr;

	UT_ASSERT(tls != nullptr);

	parallel_exec(concurrency, [&](size_t thread_index) { tls->local(); });

	tls->initialize([](test_type &) {});
	UT_ASSERT(tls->empty() == true);
	/* map after initialize method must be empty */
	/* counter == 0 means that map.clear() was called */
	UT_ASSERT(map_type::counter == 0);
}

int
main(int argc, char *argv[])
{
	START();

	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
		return 1;
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "TLSTest: enumerable_thread_specific_datarace",
		PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->pptr = nvobj::make_persistent<container_type>();
		});

		test(pop);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<container_type>(r->pptr);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
