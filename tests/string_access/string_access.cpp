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
	nvobj::persistent_ptr<S> s1;
	nvobj::persistent_ptr<S> s2;
};

/* Check if access method can be called out of transaction scope */
void
check_access_out_of_tx(S &s)
{
	try {
		s[0];
		s.at(0);
		s.begin();
		s.c_str();
		s.data();
		s.end();
		s.rbegin();
		s.rend();
		s.front();
		s.back();

		s.const_at(0);
		s.cbegin();
		s.cdata();
		s.cend();
		s.crbegin();
		s.crend();
		s.cfront();
		s.cback();

		static_cast<const S &>(s)[0];
		static_cast<const S &>(s).at(0);
		static_cast<const S &>(s).begin();
		static_cast<const S &>(s).data();
		static_cast<const S &>(s).end();
		static_cast<const S &>(s).rbegin();
		static_cast<const S &>(s).rend();
		static_cast<const S &>(s).front();
		static_cast<const S &>(s).back();
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

/*
 * Check if access methods, iterators and dereference operator add
 * elements to transaction. Expect no pmemcheck errors.
 */
void
check_add_to_tx(nvobj::pool<struct root> &pop, S &s)
{
	try {
		nvobj::transaction::run(pop, [&] { s[0] = '1'; });
		nvobj::transaction::run(pop, [&] { s.at(0) = '2'; });
		nvobj::transaction::run(pop, [&] {
			auto p = s.data();
			for (unsigned i = 0; i < s.size(); ++i)
				*(p + i) = '0';
		});
		nvobj::transaction::run(pop, [&] { *s.begin() = '3'; });
		nvobj::transaction::run(pop, [&] { *(s.end() - 1) = '4'; });
		nvobj::transaction::run(pop, [&] { *s.rbegin() = '5'; });
		nvobj::transaction::run(pop, [&] { *(s.rend() - 1) = '6'; });
		nvobj::transaction::run(pop, [&] { s.front() = '7'; });
		nvobj::transaction::run(pop, [&] { s.back() = '8'; });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

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
check_tx_abort(pmem::obj::pool<struct root> &pop, S &s)
{
	try {
		assert_tx_abort(pop, s, [&] { s[0] = '5'; });
		UT_ASSERT(s[0] == '0');

		assert_tx_abort(pop, s, [&] { s.at(0) = '5'; });
		UT_ASSERT(s.at(0) == '0');

		assert_tx_abort(pop, s, [&] { *s.begin() = '5'; });
		UT_ASSERT(*s.begin() == '0');

		assert_tx_abort(pop, s, [&] { *(s.end() - 1) = '5'; });
		UT_ASSERT(*(s.end() - 1) == '9');

		assert_tx_abort(pop, s, [&] { *s.rbegin() = '5'; });
		UT_ASSERT(*s.rbegin() == '9');

		assert_tx_abort(pop, s, [&] { *(s.rend() - 1) = '5'; });
		UT_ASSERT(*(s.rend() - 1) == '0');

		assert_tx_abort(pop, s, [&] { s.front() = '5'; });
		UT_ASSERT(s[0] == '0');

		assert_tx_abort(pop, s, [&] { s.back() = '5'; });
		UT_ASSERT(s[s.size() - 1] == '9');
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
	check_add_to_tx(pop, *r->s1);
	check_add_to_tx(pop, *r->s2);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<S>(r->s1);
		nvobj::delete_persistent<S>(r->s2);
		r->s1 = nvobj::make_persistent<S>("0123456789");
		r->s2 = nvobj::make_persistent<S>(
			"0123456789012345678901234567890123456789"
			"0123456789012345678901234567890123456789"
			"0123456789012345678901234567890123456789"
			"0123456789");
	});

	check_tx_abort(pop, *r->s1);
	check_tx_abort(pop, *r->s2);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<S>(r->s1);
		nvobj::delete_persistent<S>(r->s2);
	});

	pop.close();

	return 0;
}
