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

namespace nvobj = pmem::obj;

using test_t = int;

#if LIBPMEMOBJ_CPP_USE_TBB

#include "enumerable_thread_specific_tbb_traits.hpp"

using container_type = nvobj::experimental::enumerable_thread_specific<
	test_t, tbb::concurrent_unordered_map, null_rw_mutex>;

#else

using container_type = nvobj::experimental::enumerable_thread_specific<test_t>;

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
	UT_ASSERT(tls->size() == 0);
	UT_ASSERT(tls->empty());

	parallel_exec(concurrency, [&](size_t thread_index) { tls->local(); });

	UT_ASSERT(tls->size() == concurrency);

	tls->clear();
	UT_ASSERT(tls->size() == 0);
	UT_ASSERT(tls->empty());
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
		path, "TLSTest: enumerable_thread_specific_size",
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
