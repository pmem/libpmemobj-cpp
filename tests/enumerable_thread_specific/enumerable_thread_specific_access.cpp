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

#include <set>
#include <thread>
#include <vector>

namespace nvobj = pmem::obj;
namespace pexp = pmem::obj::experimental;

#if LIBPMEMOBJ_CPP_USE_TBB

#include "tbb/concurrent_unordered_map.h"
#include "tbb/null_rw_mutex.h"

class tbb_map_traits {
public:
	using K = std::thread::id;
	using map_type = tbb::concurrent_unordered_map<K, size_t, std::hash<K>>;
	using map_rw_mutex = tbb::null_rw_mutex;
	using map_scoped_lock = tbb::null_rw_mutex::scoped_lock;
};
using container_type = pexp::enumerable_thread_specific<size_t, tbb_map_traits>;

#else

using container_type = pexp::enumerable_thread_specific<size_t>;

#endif

struct root {
	nvobj::persistent_ptr<container_type> pptr;
};

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
	const size_t concurrency = 16;

	auto tls = pop.root()->pptr;

	UT_ASSERT(tls != nullptr);

	{
		std::vector<size_t> checker(concurrency, 0);
		parallel_exec(concurrency, [&](size_t thread_index) {
			tls->local() = thread_index;
			bool exists;
			for (size_t i = 0; i < 100; ++i) {
				checker[tls->local(exists)]++;
				UT_ASSERT(exists);
			}
		});

		UT_ASSERT(tls->size() == concurrency);

		for (auto &e : checker) {
			UT_ASSERT(e == 100);
		}
	}
	{
		tls->local() = 99;
		bool exists;
		UT_ASSERT(tls->size() == concurrency + 1);
		UT_ASSERT(tls->local(exists) == 99);
		UT_ASSERT(exists);
	}
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
		path, "TLSTest: enumerable_thread_specific_access",
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
