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

#include <libpmemobj++/experimental/slice.hpp>
#include <libpmemobj++/experimental/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <iostream>

namespace pmemobj_exp = pmem::obj::experimental;

using string_type = pmemobj_exp::string;

struct root {
	pmem::obj::persistent_ptr<string_type> short_str;
	pmem::obj::persistent_ptr<string_type> long_str;
};

static constexpr char short_c_str_ctor[] = "0987654321";
static constexpr char long_c_str_ctor[] =
	"0987654321098765432109876543210987654321"
	"0987654321098765432109876543210987654321"
	"0987654321098765432109876543210987654321"
	"0987654321";

static constexpr char short_c_str[] = "1234567890";
static constexpr char long_c_str[] = "1234567890123456789012345678901234567890"
				     "1234567890123456789012345678901234567890"
				     "1234567890123456789012345678901234567890"
				     "1234567890";

void
test_string_snapshot(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->short_str = pmem::obj::make_persistent<string_type>(
				short_c_str_ctor);
			r->long_str = pmem::obj::make_persistent<string_type>(
				long_c_str_ctor);

			UT_ASSERTeq(r->short_str->size(),
				    strlen(short_c_str_ctor));
			UT_ASSERTeq(r->long_str->size(),
				    strlen(long_c_str_ctor));
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	using T = typename string_type::traits_type;

	try {
		pmem::obj::transaction::run(pop, [&] {
			auto data = r->short_str->data();
			strcpy(data, short_c_str);

			UT_ASSERTeq(T::compare(r->short_str->cdata(),
					       short_c_str,
					       r->short_str->size()),
				    0);
			UT_ASSERTeq(T::length(r->short_str->cdata()),
				    T::length(short_c_str));
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			auto data = r->long_str->data();
			strcpy(data, long_c_str);

			UT_ASSERTeq(T::compare(r->long_str->cdata(), long_c_str,
					       r->long_str->size()),
				    0);
			UT_ASSERTeq(T::length(r->long_str->cdata()),
				    T::length(long_c_str));
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<string_type>(r->short_str);
			pmem::obj::delete_persistent<string_type>(r->long_str);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

int
main(int argc, char *argv[])
{
	START();

	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " file-name " << std::endl;
		return 1;
	}

	auto path = argv[1];
	auto pop = pmem::obj::pool<root>::create(
		path, "StringTest", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	test_string_snapshot(pop);

	pop.close();

	return 0;
}
