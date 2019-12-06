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

namespace nvobj = pmem::obj;

using container = container_t<int>;

struct root {
	nvobj::persistent_ptr<container> pptr;
};

void
check_if_empty(nvobj::persistent_ptr<container> &c)
{
	UT_ASSERT(c->size() == 0);
	UT_ASSERT(c->capacity() == 0);
}

/**
 * Testing container methods with parameters = 0
 */
void
zero_test(nvobj::pool<struct root> &pop)
{
	auto &c = pop.root()->pptr;

	/* ctor test */
	nvobj::transaction::run(
		pop, [&] { c = nvobj::make_persistent<container>(0U); });

	auto list = std::initializer_list<int>({});

	/* assign test */
	// calling on empty container to check when segment=vector
	c->assign(0, 0);
	check_if_empty(c);
	// for size() == 1, to call shrink inside next assign
	c->assign(1, 0);
	c->assign(list.begin(), list.end());
	c->free_data();
	check_if_empty(c);

	/* insert test */
	c->insert(c->cbegin(), 0, 0);
	check_if_empty(c);

	/* resize test */
	c->resize(0);
	check_if_empty(c);

	/* shrink_to_fit test */
	c->shrink_to_fit();
	check_if_empty(c);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<container>(c); });
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
		path, "VectorTest: modifiers_exceptions_oom",
		PMEMOBJ_MIN_POOL * 2, S_IWUSR | S_IRUSR);

	try {
		zero_test(pop);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
