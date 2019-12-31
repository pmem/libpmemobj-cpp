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
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/slice.hpp>
#include <libpmemobj++/transaction.hpp>

#include <iostream>

using string_type = pmem::obj::string;

struct pmem_string_struct {
	pmem_string_struct() : str("abcdefgh"), other("abc")
	{
	}

	string_type str;
	string_type other;
};

struct root {
	pmem::obj::persistent_ptr<pmem_string_struct> p_storage;
};

/*
 * this function verifies that f() throws pool_error exception.
 */
void
assert_pool_exception(std::function<void(void)> f)
{
	bool exception_thrown = false;
	try {
		f();
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(exception_thrown);
}

/*
 * this function verifies that f() throws transaction_scope_error exception.
 */
void
assert_tx_exception(std::function<void(void)> f)
{
	bool exception_thrown = false;
	try {
		f();
		UT_ASSERT(0);
	} catch (pmem::transaction_scope_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(exception_thrown);
}

void
test_ctor_exceptions_nopmem(pmem::obj::pool<struct root> &pop)
try {
	auto r = pop.root();

	pmem::obj::transaction::run(pop, [&] {
		r->p_storage = pmem::obj::make_persistent<pmem_string_struct>();
	});

	pmem::obj::transaction::run(pop, [&] {
		assert_pool_exception([&] { string_type str; });

		assert_pool_exception([&] { string_type str(2, 'a'); });

		assert_pool_exception(
			[&] { string_type str(r->p_storage->str, 2, 2); });

		assert_pool_exception(
			[&] { string_type str(r->p_storage->str, 2); });

		assert_pool_exception([&] { string_type str("abc", 1); });

		assert_pool_exception([&] { string_type str("abc"); });

		assert_pool_exception(
			[&] { string_type str(std::move(r->p_storage->str)); });

		assert_pool_exception([&] {
			string_type str({'a', 'b', 'c'});
		});

		assert_pool_exception([&] {
			std::string s("abc");
			string_type str(s);
		});

		assert_pool_exception([&] {
			std::string s("abc");
			string_type str(s, 0, 3);
		});
	});

	pmem::obj::transaction::run(pop, [&] {
		pmem::obj::delete_persistent<pmem_string_struct>(r->p_storage);
	});
} catch (std::exception &e) {
	UT_FATALexc(e);
}

void
test_ctor_exceptions_notx(pmem::obj::pool<struct root> &pop)
try {
	auto r = pop.root();

	pmem::obj::transaction::run(pop, [&] {
		r->p_storage = pmem::obj::make_persistent<pmem_string_struct>();
	});

	pmem::obj::transaction::run(pop,
				    [&] { r->p_storage->str.~string_type(); });

	assert_tx_exception([&] { new (&(r->p_storage->str)) string_type(); });

	pmem::obj::transaction::run(pop,
				    [&] { r->p_storage->str.~string_type(); });

	assert_tx_exception(
		[&] { new (&(r->p_storage->str)) string_type(2, 'a'); });

	pmem::obj::transaction::run(pop,
				    [&] { r->p_storage->str.~string_type(); });

	assert_tx_exception([&] {
		new (&(r->p_storage->str))
			string_type(r->p_storage->other, 2, 2);
	});

	pmem::obj::transaction::run(pop,
				    [&] { r->p_storage->str.~string_type(); });

	assert_tx_exception([&] {
		new (&(r->p_storage->str)) string_type(r->p_storage->other, 2);
	});

	pmem::obj::transaction::run(pop,
				    [&] { r->p_storage->str.~string_type(); });

	assert_tx_exception(
		[&] { new (&(r->p_storage->str)) string_type("abc", 1); });

	pmem::obj::transaction::run(pop,
				    [&] { r->p_storage->str.~string_type(); });

	assert_tx_exception(
		[&] { new (&(r->p_storage->str)) string_type("abc"); });

	pmem::obj::transaction::run(pop,
				    [&] { r->p_storage->str.~string_type(); });

	assert_tx_exception([&] {
		new (&(r->p_storage->str))
			string_type(std::move(r->p_storage->other));
	});

	pmem::obj::transaction::run(pop,
				    [&] { r->p_storage->str.~string_type(); });

	assert_tx_exception([&] {
		new (&(r->p_storage->str)) string_type({'a', 'b', 'c'});
	});

	pmem::obj::transaction::run(pop,
				    [&] { r->p_storage->str.~string_type(); });

	assert_tx_exception([&] {
		std::string s("abc");
		new (&(r->p_storage->str)) string_type(s);
	});

	pmem::obj::transaction::run(pop,
				    [&] { r->p_storage->str.~string_type(); });

	assert_tx_exception([&] {
		std::string s("abc");
		new (&(r->p_storage->str)) string_type(s, 0, 3);
	});

	pmem::obj::transaction::run(pop,
				    [&] { r->p_storage->str.~string_type(); });

	pmem::obj::transaction::run(pop, [&] {
		pmem::obj::delete_persistent<pmem_string_struct>(r->p_storage);
	});
} catch (std::exception &e) {
	UT_FATALexc(e);
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

	test_ctor_exceptions_nopmem(pop);
	test_ctor_exceptions_notx(pop);

	pop.close();

	return 0;
}
