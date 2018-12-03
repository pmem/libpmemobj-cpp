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

/*
 * pmemobj_check_cow -- checks if pmemobj supports COW on pool opening
 *			which can be triggered using PMEMOBJ_COW env variable
 */

#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <iostream>
#include <string>

struct root {
	int foo = 0;
};

void
init(const std::string &path)
{
	auto pop = pmem::obj::pool<root>::create(
		path, "COW_CHECK", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();
	r->foo = 0;
	r.persist();

	pop.close();
}

void
open_and_write(const std::string &path)
{
	auto pop = pmem::obj::pool<root>::open(path, "COW_CHECK");

	auto r = pop.root();
	r->foo = 1;
	r.persist();

	pop.close();
}

bool
check_cow_support(const std::string &path)
{
	auto pop = pmem::obj::pool<root>::open(path, "COW_CHECK");

	bool cow_supported = pop.root()->foo == 0;

	pop.close();

	return cow_supported;
}

/*
 * return value is:
 * - 0 when COW is supported
 * - 1 when error occures during this program execution
 * - 2 when COW is not supported
 */
int
main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " "
			  << " filename\n";
		return 1;
	}

	std::string path = argv[1];
	bool supported = false;

	try {
		init(path);
		open_and_write(path);
		supported = check_cow_support(path);
	} catch (pmem::pool_error &pe) {
		std::cerr << pe.what();
		return 1;
	}

	remove(path.c_str());

	return supported ? 0 : 2;
}
