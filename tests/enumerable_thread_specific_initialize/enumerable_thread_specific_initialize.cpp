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

#include <set>

namespace nvobj = pmem::obj;

using test_t = nvobj::p<size_t>;
using container_type = pmem::detail::enumerable_thread_specific<test_t>;

struct root {
	nvobj::persistent_ptr<container_type> pptr;
};

void
create_and_fill(nvobj::pool<struct root> &pop, size_t concurrency)
{
	auto &tls = pop.root()->pptr;

	UT_ASSERT(tls == nullptr);

	nvobj::transaction::run(
		pop, [&] { tls = nvobj::make_persistent<container_type>(); });
	parallel_exec_with_sync(concurrency, [&](size_t thread_index) {
		tls->local() = thread_index;
		pop.persist(tls->local());
	});
	UT_ASSERT(tls->size() <= concurrency);
}

void
check_and_delete(nvobj::pool<struct root> &pop, size_t concurrency)
{
	auto &tls = pop.root()->pptr;

	std::set<size_t> checker;
	tls->initialize([&checker](test_t &e) {
		UT_ASSERT(checker.emplace(e).second);
	});
	UT_ASSERT(checker.size() <= concurrency);
	UT_ASSERT(tls->empty());

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_type>(tls);
		tls = nullptr;
	});
}

void
check_with_tx_and_delete(nvobj::pool<struct root> &pop, size_t concurrency)
{
	auto &tls = pop.root()->pptr;

	std::set<size_t> checker;
	nvobj::transaction::run(pop, [&] {
		tls->initialize([&checker](test_t &e) {
			UT_ASSERT(checker.emplace(e).second);
		});
	});

	UT_ASSERT(checker.size() <= concurrency);
	UT_ASSERT(tls->empty());

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_type>(tls);
		tls = nullptr;
	});
}

void
check_with_tx_abort_and_delete(nvobj::pool<struct root> &pop,
			       size_t concurrency)
{
	auto &tls = pop.root()->pptr;

	std::set<size_t> checker;

	try {
		nvobj::transaction::run(pop, [&] {
			tls->initialize([&checker](test_t &e) {
				UT_ASSERT(checker.emplace(e).second);
			});

			nvobj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(checker.size() <= concurrency);
	UT_ASSERT(!tls->empty());
	UT_ASSERT(tls->size() <= concurrency);

	for (auto &e : *tls)
		e = 0;

	parallel_exec_with_sync(concurrency,
				[&](size_t thread_index) { tls->local()++; });

	for (auto &e : *tls) {
		UT_ASSERTeq(e, 1);
	}

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_type>(tls);
		tls = nullptr;
	});
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
	auto layout = "TLSTest: enumerable_thread_specific_initialize";
	auto pop = nvobj::pool<root>::create(path, layout, PMEMOBJ_MIN_POOL,
					     S_IWUSR | S_IRUSR);
	// Adding more concurrency will increase DRD test time
	size_t concurrency = 16;

	{
		create_and_fill(pop, concurrency);

		pop.close();
		pop = nvobj::pool<root>::open(path, layout);

		check_and_delete(pop, concurrency);
	}
	{
		create_and_fill(pop, concurrency);

		pop.close();
		pop = nvobj::pool<root>::open(path, layout);

		check_with_tx_and_delete(pop, concurrency);
	}
	{
		create_and_fill(pop, concurrency);

		pop.close();
		pop = nvobj::pool<root>::open(path, layout);

		check_with_tx_abort_and_delete(pop, concurrency);
	}

	pop.close();
	return 0;
}
