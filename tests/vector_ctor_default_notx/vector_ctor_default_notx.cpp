/*
 * Copyright 2018, Intel Corporation
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

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/life.hpp>
#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <cstring>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;
using vector_type = pmem_exp::vector<int>;

/**
 * Test pmem::obj::experimental::vector default constructor.
 *
 * Call default constructor out of transaction scope. Expect
 * pmem:transaction_error exception is thrown.
 */
void
test_default_ctor(nvobj::pool<struct root> &pop)
{
	bool exception_thrown = false;
	try {
		nvobj::persistent_ptr<vector_type> pptr_v = nullptr;
		nvobj::transaction::run(pop, [&] {
			pptr_v = pmemobj_tx_alloc(
				sizeof(vector_type),
				pmem::detail::type_num<vector_type>());
			if (pptr_v == nullptr)
				UT_ASSERT(0);
		});
		pmem::detail::create<vector_type>(&*pptr_v);
	} catch (pmem::transaction_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}
	UT_ASSERT(exception_thrown);
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
		path, "VectorTest: vector_ctor_default", PMEMOBJ_MIN_POOL,
		S_IWUSR | S_IRUSR);

	test_default_ctor(pop);

	pop.close();

	return 0;
}
