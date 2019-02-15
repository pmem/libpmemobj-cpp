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

#include "unittest.hpp"

#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/pool.hpp>

#include <cstring>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;

struct X {
	static unsigned count;
	nvobj::p<int> val;

	X() : val(1)
	{
		++count;
	};
	~X()
	{
		--count;
	};
};

unsigned X::count = 0;

using vector_type = pmem_exp::vector<X>;

struct root {
	nvobj::persistent_ptr<vector_type> pptr;
};

/**
 * Test pmem::obj::experimental::vector default destructor.
 *
 * Call default destructor out of transaction scope.
 * Expects vector is empty and no exception is thrown.
 */
void
test_dtor(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	using size_type = vector_type::size_type;
	const size_type size = 100;
	try {
		nvobj::transaction::run(pop, [&] {
			r->pptr = nvobj::make_persistent<vector_type>(size);
		});
		UT_ASSERTeq(r->pptr->size(), X::count);
		UT_ASSERTeq(X::count, size);

		r->pptr->~vector();

		UT_ASSERT(r->pptr->empty());
		UT_ASSERTeq(X::count, 0);
	} catch (std::exception &e) {
		UT_FATALexc(e);
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
	auto pop =
		nvobj::pool<root>::create(path, "VectorTest: vector_dtor",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	test_dtor(pop);

	pop.close();

	return 0;
}
