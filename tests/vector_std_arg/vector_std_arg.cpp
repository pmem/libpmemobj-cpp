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

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>

#include <vector>

namespace nvobj = pmem::obj;

using pmem_vec = container_t<int>;
using std_vec = std::vector<int>;

struct root {
	nvobj::persistent_ptr<pmem_vec> pptr;
};

void
check_vector(const pmem_vec &v1, const std_vec &v2)
{
	UT_ASSERTeq(v1.size(), v2.size());

	for (unsigned i = 0; i < v1.size(); ++i)
		UT_ASSERTeq(v1[i], v2[i]);
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
		path, "VectorTest", PMEMOBJ_MIN_POOL * 2, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		std_vec stdvector(10U, 1);

		nvobj::transaction::run(pop, [&] {
			r->pptr = nvobj::make_persistent<pmem_vec>(stdvector);
		});

		pmem_vec &pvector = *(r->pptr);

		check_vector(pvector, stdvector);

		stdvector.assign(20U, 2);
		pvector.assign(stdvector);

		check_vector(pvector, stdvector);

		stdvector = std_vec(30U, 3);
		pvector = stdvector;

		check_vector(pvector, stdvector);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<pmem_vec>(r->pptr);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
