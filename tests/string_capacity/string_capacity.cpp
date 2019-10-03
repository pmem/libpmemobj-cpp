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

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using S = pmem::obj::string;

struct root {
	nvobj::persistent_ptr<S> s, s1, s2;
};

/**
 * Check if access method can be called out of transaction scope
 */
void
check_access_out_of_tx(const S &s)
{
	try {
		s.empty();
		s.size();
		s.length();
		s.max_size();
		s.capacity();
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

/**
 * Checks if string's state is reverted when transaction aborts.
 */
void
assert_tx_abort(pmem::obj::pool<struct root> &pop, S &s,
		std::function<void(void)> f)
{
	bool exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			f();
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

void
verify_string(const S &s, const S &expected)
{
	UT_ASSERT(s == expected);
	UT_ASSERT(s.size() == expected.size());
	UT_ASSERT(s.capacity() == expected.capacity());
}

void
check_tx_abort(pmem::obj::pool<struct root> &pop, const S &expected)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->s = nvobj::make_persistent<S>(expected);
		});

		auto &s = *r->s;

		assert_tx_abort(pop, s, [&] { s.resize(30); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.resize(300); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.resize(30, 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.resize(300, 'a'); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.reserve(); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.reserve(30); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.reserve(300); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.shrink_to_fit(); });
		verify_string(s, expected);

		assert_tx_abort(pop, s, [&] { s.clear(); });
		verify_string(s, expected);

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<S>(r->s); });
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
	auto pop = nvobj::pool<root>::create(
		path, "StringTest", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->s1 = nvobj::make_persistent<S>("0123456789");
		r->s2 = nvobj::make_persistent<S>(
			"0123456789012345678901234567890123456789"
			"0123456789012345678901234567890123456789"
			"0123456789012345678901234567890123456789"
			"0123456789");
	});

	check_access_out_of_tx(*r->s1);
	check_access_out_of_tx(*r->s2);

	check_tx_abort(pop, *r->s1);
	check_tx_abort(pop, *r->s2);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<S>(r->s1);
		nvobj::delete_persistent<S>(r->s2);
	});

	pop.close();

	return 0;
}
