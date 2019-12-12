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

#include <thread>
#include <vector>

#include <libpmemobj++/detail/pool_data.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <libpmemobj/pool_base.h>

namespace nvobj = pmem::obj;

struct root {
	nvobj::p<int> val;
};

/*
 * pool_close -- (internal) test pool close
 */
void
pool_cleanup(nvobj::pool<root> &pop1, nvobj::pool<root> &pop2)
{
	const size_t concurrency = 16;

	std::vector<std::thread> threads;
	threads.reserve(concurrency);

	auto r1 = pop1.root();
	auto r2 = pop2.root();

	size_t counter1 = 0, counter2 = 0;

	for (unsigned i = 0; i < concurrency / 2; i++)
		threads.emplace_back([&] {
			auto oid = r1.raw();
			auto pop = pmemobj_pool_by_oid(oid);
			auto *user_data =
				static_cast<pmem::detail::pool_data *>(
					pmemobj_get_user_data(pop));

			user_data->set_cleanup([&] { counter1++; });
		});

	for (unsigned i = 0; i < concurrency / 2; i++)
		threads.emplace_back([&] {
			auto oid = r2.raw();
			auto pop = pmemobj_pool_by_oid(oid);
			auto *user_data =
				static_cast<pmem::detail::pool_data *>(
					pmemobj_get_user_data(pop));

			user_data->set_cleanup([&] { counter2++; });
		});

	for (auto &t : threads)
		t.join();

	try {
		pop1.close();
	} catch (std::logic_error &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(counter1, 1);
	UT_ASSERTeq(counter2, 0);

	try {
		pop2.close();
	} catch (std::logic_error &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(counter1, 1);
	UT_ASSERTeq(counter2, 1);
}

int
main(int argc, char *argv[])
{
	START();

	if (argc < 3) {
		std::cerr << "usage: " << argv[0] << " file-name1 file-name2"
			  << std::endl;
		return 1;
	}

	auto pop1 = nvobj::pool<root>::create(argv[1], "pool_callbacks test",
					      PMEMOBJ_MIN_POOL * 2,
					      S_IWUSR | S_IRUSR);

	auto pop2 = nvobj::pool<root>::create(argv[2], "pool_callbacks test",
					      PMEMOBJ_MIN_POOL * 2,
					      S_IWUSR | S_IRUSR);

	pool_cleanup(pop1, pop2);

	return 0;
}
