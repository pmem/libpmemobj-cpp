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

#include <libpmemobj++/experimental/string.hpp>
#include <libpmemobj++/make_persistent.hpp>

#include <vector>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;
using S = pmem_exp::string;
using WS = pmem_exp::wstring;

struct root {
	nvobj::persistent_ptr<S> s;
	nvobj::persistent_ptr<WS> ws;
};

template <typename CharT>
void
check_string(nvobj::pool<struct root> &pop,
	     nvobj::persistent_ptr<pmem_exp::basic_string<CharT>> &ptr,
	     size_t count, CharT value)
{
	UT_ASSERTeq(ptr->size(), count);

	for (unsigned i = 0; i < count; ++i)
		UT_ASSERTeq(ptr->const_at(i), value);
}

void
assert_tx_abort(pmem::obj::pool<struct root> &pop, std::function<void(void)> f)
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

/**
 * Test pmem::obj::experimental::string assign() methods and operator=.
 *
 * Checks if string's state is reverted when transaction aborts.
 */
template <std::size_t InitialSize, std::size_t TestSize, typename CharT>
void
test(nvobj::pool<struct root> &pop,
     nvobj::persistent_ptr<pmem_exp::basic_string<CharT>> &ptr)
{
	using string_type = pmem_exp::basic_string<CharT>;

	/* assign() - fill version */
	check_string(pop, ptr, InitialSize, static_cast<CharT>('a'));
	assert_tx_abort(pop, [&] {
		ptr->assign(TestSize, static_cast<CharT>('b'));
		check_string(pop, ptr, TestSize, static_cast<CharT>('b'));
	});
	check_string(pop, ptr, InitialSize, static_cast<CharT>('a'));

	/* assign() - range version */
	assert_tx_abort(pop, [&] {
		std::string v2(TestSize, static_cast<CharT>('b'));
		ptr->assign(v2.begin(), v2.end());
		check_string(pop, ptr, TestSize, static_cast<CharT>('b'));
	});
	check_string(pop, ptr, InitialSize, static_cast<CharT>('a'));

	/* assign() - initializer list version */
	assert_tx_abort(pop, [&] {
		ptr->assign({static_cast<CharT>(2), static_cast<CharT>(2),
			     static_cast<CharT>(2), static_cast<CharT>(2),
			     static_cast<CharT>(2)});
		check_string(pop, ptr, 5, static_cast<CharT>(2));
	});
	check_string(pop, ptr, InitialSize, static_cast<CharT>('a'));

	/* assign() - r value reference to other string */
	assert_tx_abort(pop, [&] {
		auto v2 = nvobj::make_persistent<string_type>(
			TestSize, static_cast<CharT>('b'));
		ptr->assign(std::move(*v2));
		check_string(pop, ptr, TestSize, static_cast<CharT>('b'));
		nvobj::delete_persistent<string_type>(v2);
	});
	check_string(pop, ptr, InitialSize, static_cast<CharT>('a'));

	/* assign() - l value reference to other string */
	assert_tx_abort(pop, [&] {
		auto v2 = nvobj::make_persistent<string_type>(
			TestSize, static_cast<CharT>('b'));
		ptr->assign(*v2);
		check_string(pop, ptr, TestSize, static_cast<CharT>('b'));
		nvobj::delete_persistent<string_type>(v2);
	});
	check_string(pop, ptr, InitialSize, static_cast<CharT>('a'));

	/* assign() - c-style string */
	assert_tx_abort(pop, [&] {
		auto cstring = new CharT[TestSize + 1];
		std::fill(cstring, cstring + TestSize, static_cast<CharT>('b'));
		cstring[TestSize] = static_cast<CharT>('\0');

		ptr->assign(cstring);
		check_string(pop, ptr, TestSize, static_cast<CharT>('b'));

		delete[] cstring;
	});
	check_string(pop, ptr, InitialSize, static_cast<CharT>('a'));

	/* assign() - c-style string, count */
	assert_tx_abort(pop, [&] {
		auto cstring = new CharT[TestSize + 11];
		std::fill(cstring, cstring + TestSize + 11,
			  static_cast<CharT>('b'));
		cstring[TestSize + 10] = static_cast<CharT>('\0');

		ptr->assign(cstring, TestSize);
		check_string(pop, ptr, TestSize, static_cast<CharT>('b'));

		delete[] cstring;
	});
	check_string(pop, ptr, InitialSize, static_cast<CharT>('a'));

	/* assign() - c-style string, count */
	assert_tx_abort(pop, [&] {
		auto v2 = nvobj::make_persistent<string_type>(
			TestSize + 20, static_cast<CharT>('b'));
		ptr->assign(*v2, 20, TestSize);
		check_string(pop, ptr, TestSize, static_cast<CharT>('b'));
		nvobj::delete_persistent<string_type>(v2);
	});

	check_string(pop, ptr, InitialSize, static_cast<CharT>('a'));

	/* assign() - c-style string, count */
	assert_tx_abort(pop, [&] {
		auto v2 = nvobj::make_persistent<string_type>(
			TestSize + 20, static_cast<CharT>('b'));
		ptr->assign(*v2, 20);
		check_string(pop, ptr, TestSize, static_cast<CharT>('b'));
		nvobj::delete_persistent<string_type>(v2);
	});
	check_string(pop, ptr, InitialSize, static_cast<CharT>('a'));

	/* copy assignment operator */
	assert_tx_abort(pop, [&] {
		auto cstring = new CharT[TestSize + 1];
		std::fill(cstring, cstring + TestSize + 1,
			  static_cast<CharT>('b'));
		cstring[TestSize] = static_cast<CharT>('\0');

		*ptr = cstring;
		check_string(pop, ptr, TestSize, static_cast<CharT>('b'));

		delete[] cstring;
	});
	check_string(pop, ptr, InitialSize, static_cast<CharT>('a'));

	/* assignment operator for cstyle string */
	assert_tx_abort(pop, [&] {
		auto v2 = nvobj::make_persistent<string_type>(
			TestSize, static_cast<CharT>('b'));
		*ptr = *v2;
		check_string(pop, ptr, TestSize, static_cast<CharT>('b'));
		nvobj::delete_persistent<string_type>(v2);
	});
	check_string(pop, ptr, InitialSize, static_cast<CharT>('a'));

	/* move assignment operator */
	assert_tx_abort(pop, [&] {
		auto v2 = nvobj::make_persistent<string_type>(
			TestSize, static_cast<CharT>('b'));
		*ptr = std::move(*v2);
		check_string(pop, ptr, TestSize, static_cast<CharT>('b'));
		UT_ASSERT(v2->empty());
		nvobj::delete_persistent<string_type>(v2);
	});
	check_string(pop, ptr, InitialSize, static_cast<CharT>('a'));

	/* initializer list assignment operator */
	assert_tx_abort(pop, [&] {
		*ptr = {static_cast<CharT>(2), static_cast<CharT>(2),
			static_cast<CharT>(2), static_cast<CharT>(2),
			static_cast<CharT>(2)};
		check_string(pop, ptr, 5, static_cast<CharT>(2));
		nvobj::transaction::abort(EINVAL);
	});
	check_string(pop, ptr, InitialSize, static_cast<CharT>('a'));
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
		nvobj::pool<root>::create(path, "VectorTest: assign_txabort",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->s = nvobj::make_persistent<S>(10U, 'a');
			r->ws = nvobj::make_persistent<WS>(10U, L'a');
		});

		test<10, 20, char>(pop, r->s);
		test<10, 11, char>(pop, r->s);
		test<10, 9, char>(pop, r->s);
		test<10, 5, char>(pop, r->s);
		test<10, 100, char>(pop, r->s);

		test<10, 11, wchar_t>(pop, r->ws);
		test<10, 100, wchar_t>(pop, r->ws);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<S>(r->s);
			nvobj::delete_persistent<WS>(r->ws);
			r->s = nvobj::make_persistent<S>(100U, 'a');
			r->ws = nvobj::make_persistent<WS>(100U, L'a');
		});

		test<100, 10, char>(pop, r->s);
		test<100, 101, char>(pop, r->s);
		test<100, 150, char>(pop, r->s);
		test<100, 99, char>(pop, r->s);
		test<100, 70, char>(pop, r->s);

		test<100, 10, wchar_t>(pop, r->ws);
		test<100, 101, wchar_t>(pop, r->ws);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<S>(r->s);
			nvobj::delete_persistent<WS>(r->ws);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
