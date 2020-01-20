/*
 * Copyright 2019-2020, Intel Corporation
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

#include <libpmemobj++/detail/enumerable_thread_specific.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using test_t = int;

using container_type = pmem::detail::enumerable_thread_specific<test_t>;

struct root {
	nvobj::persistent_ptr<container_type> pptr;
};

void
test(nvobj::pool<struct root> &pop, size_t batch_size)
{
	const size_t num_batches = 3;

	auto tls = pop.root()->pptr;

	UT_ASSERT(tls != nullptr);
	UT_ASSERT(tls->size() == 0);
	UT_ASSERT(tls->empty());

	for (size_t i = 0; i < num_batches; i++)
		parallel_exec(batch_size,
			      [&](size_t thread_index) { tls->local(); });

	/* There was at most batch_size threads at any given time. */
	UT_ASSERT(tls->size() <= batch_size);

	tls->clear();
	UT_ASSERT(tls->size() == 0);
	UT_ASSERT(tls->empty());
}

void
test_with_spin(nvobj::pool<struct root> &pop, size_t batch_size)
{
	auto tls = pop.root()->pptr;

	UT_ASSERT(tls != nullptr);
	UT_ASSERT(tls->size() == 0);
	UT_ASSERT(tls->empty());

	parallel_exec_with_sync(batch_size, [&](size_t thread_index) {
		tls->local() = thread_index;
	});

	/*
	 * tls->size() will be equal to max number of threads that have used
	 * tls at any given time. This test assumes that batch_size is >=
	 * than any previously used number of threads
	 */
	UT_ASSERTeq(tls->size(), batch_size);

	tls->clear();
	UT_ASSERT(tls->size() == 0);
	UT_ASSERT(tls->empty());
}

void
test_clear_abort(nvobj::pool<struct root> &pop, size_t batch_size)
{
	auto tls = pop.root()->pptr;

	UT_ASSERT(tls != nullptr);
	UT_ASSERT(tls->size() == 0);
	UT_ASSERT(tls->empty());

	parallel_exec_with_sync(batch_size,
				[&](size_t thread_index) { tls->local() = 2; });

	/*
	 * tls->size() will be equal to max number of threads that have used
	 * tls at any given time. This test assumes that batch_size is >=
	 * than any previously used number of threads
	 */
	UT_ASSERTeq(tls->size(), batch_size);

	try {
		nvobj::transaction::run(pop, [&] {
			tls->clear();

			nvobj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(tls->size(), batch_size);

	for (auto &e : *tls)
		UT_ASSERTeq(e, 2);
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

		test(pop, 8);
		test(pop, 10);

		test_with_spin(pop, 12);
		test_with_spin(pop, 16);

		test_clear_abort(pop, 16);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<container_type>(r->pptr);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
