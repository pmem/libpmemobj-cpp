//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2020, Intel Corporation
//
// Modified to test pmem::obj containers
//

// <string>

// template<class _Tp>
//   basic_string(const _Tp& __t, size_type __pos, size_type __n,
//                const allocator_type& __a = allocator_type());
//
//  Mostly we're testing string_view here

// #include <utility>
// #include <vector>

#include "unittest.hpp"

#include <algorithm>
#include <stdexcept>

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/string_view.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;

struct root {
	nvobj::persistent_ptr<nvobj::basic_string<char, std::char_traits<char>>>
		s;
};

template <class S, class SV>
void
test(SV sv, std::size_t pos, std::size_t n, pmem::obj::pool<root> &pop)
{
	typedef typename S::traits_type T;
	typedef typename S::size_type Size;
	if (pos <= sv.size()) {
		nvobj::transaction::run(pop, [&] {
			auto s2_ptr = nvobj::make_persistent<S>(
				sv, static_cast<Size>(pos),
				static_cast<Size>(n));
			UT_ASSERT(pos <= sv.size());
			std::size_t rlen = (std::min)(sv.size() - pos, n);
			UT_ASSERT(s2_ptr->size() == rlen);
			UT_ASSERT(T::compare(s2_ptr->data(), sv.data() + pos,
					     rlen) == 0);
			UT_ASSERT(s2_ptr->capacity() >= s2_ptr->size());
			nvobj::delete_persistent<S>(s2_ptr);
		});
	}
#ifndef TEST_HAS_NO_EXCEPTIONS
	else {
		try {
			nvobj::transaction::run(pop, [&] {
				auto s2 = nvobj::make_persistent<S>(
					sv, static_cast<Size>(pos),
					static_cast<Size>(n));
				(void)s2;
				UT_ASSERT(false);
			});
		} catch (std::out_of_range &) {
			UT_ASSERT(pos > sv.size());
		}
	}
#endif
}

void
run(pmem::obj::pool<root> &pop)
{
	{
		typedef nvobj::basic_string_view<char, std::char_traits<char>>
			SV;
		typedef nvobj::basic_string<char, std::char_traits<char>> S;

		test<S, SV>(SV(), 0, 0, pop);
		test<S, SV>(SV(), 0, 1, pop);
		test<S, SV>(SV(), 1, 0, pop);
		test<S, SV>(SV(), 1, 1, pop);
		test<S, SV>(SV(), 1, 2, pop);
		test<S, SV>(SV("1"), 0, 0, pop);
		test<S, SV>(SV("1"), 0, 1, pop);
		test<S, SV>(
			SV("1234567890123456789012345678901234567890123456789012345678901234567890"),
			50, 0, pop);
		test<S, SV>(
			SV("1234567890123456789012345678901234567890123456789012345678901234567890"),
			50, 1, pop);
		test<S, SV>(
			SV("1234567890123456789012345678901234567890123456789012345678901234567890"),
			50, 10, pop);
		test<S, SV>(
			SV("1234567890123456789012345678901234567890123456789012345678901234567890"),
			50, 100, pop);

		test<S, SV>(SV(), 0, 0, pop);
		test<S, SV>(SV(), 0, 1, pop);
		test<S, SV>(SV(), 1, 0, pop);
		test<S, SV>(SV(), 1, 1, pop);
		test<S, SV>(SV(), 1, 2, pop);
		test<S, SV>(SV("1"), 0, 0, pop);
		test<S, SV>(SV("1"), 0, 1, pop);
		test<S, SV>(
			SV("1234567890123456789012345678901234567890123456789012345678901234567890"),
			50, 0, pop);
		test<S, SV>(
			SV("1234567890123456789012345678901234567890123456789012345678901234567890"),
			50, 1, pop);
		test<S, SV>(
			SV("1234567890123456789012345678901234567890123456789012345678901234567890"),
			50, 10, pop);
		test<S, SV>(
			SV("1234567890123456789012345678901234567890123456789012345678901234567890"),
			50, 100, pop);
	}

#ifdef XXX // XXX: Implement min_allocator class
	{
		typedef min_allocator<char> A;
		typedef std::basic_string_view<char, std::char_traits<char>> SV;
		typedef std::basic_string<char, std::char_traits<char>, A> S;

		test<S, SV>(SV(), 0, 0);
		test<S, SV>(SV(), 0, 1);
		test<S, SV>(SV(), 1, 0);
		test<S, SV>(SV(), 1, 1);
		test<S, SV>(SV(), 1, 2);
		test<S, SV>(SV("1"), 0, 0);
		test<S, SV>(SV("1"), 0, 1);
		test<S, SV>(
			SV("1234567890123456789012345678901234567890123456789012345678901234567890"),
			50, 0);
		test<S, SV>(
			SV("1234567890123456789012345678901234567890123456789012345678901234567890"),
			50, 1);
		test<S, SV>(
			SV("1234567890123456789012345678901234567890123456789012345678901234567890"),
			50, 10);
		test<S, SV>(
			SV("1234567890123456789012345678901234567890123456789012345678901234567890"),
			50, 100);

		test<S, SV>(SV(), 0, 0, A());
		test<S, SV>(SV(), 0, 1, A());
		test<S, SV>(SV(), 1, 0, A());
		test<S, SV>(SV(), 1, 1, A());
		test<S, SV>(SV(), 1, 2, A());
		test<S, SV>(SV("1"), 0, 0, A());
		test<S, SV>(SV("1"), 0, 1, A());
		test<S, SV>(
			SV("1234567890123456789012345678901234567890123456789012345678901234567890"),
			50, 0, A());
		test<S, SV>(
			SV("1234567890123456789012345678901234567890123456789012345678901234567890"),
			50, 1, A());
		test<S, SV>(
			SV("1234567890123456789012345678901234567890123456789012345678901234567890"),
			50, 10, A());
		test<S, SV>(
			SV("1234567890123456789012345678901234567890123456789012345678901234567890"),
			50, 100, A());
	}
#endif

	{
		typedef nvobj::string S;
		typedef nvobj::string_view SV;

		pmem::obj::persistent_ptr<S> s;
		nvobj::transaction::run(pop, [&] {
			s = nvobj::make_persistent<S>();
			*s = "ABCD";
		});

		SV sv = "EFGH";
		char arr[] = "IJKL";

		nvobj::transaction::run(pop, [&] {
			auto s1 = nvobj::make_persistent<S>(
				"CDEF", static_cast<S::size_type>(4));
			UT_ASSERT(*s1 == "CDEF");
			nvobj::delete_persistent<S>(s1);
		});

		nvobj::transaction::run(pop, [&] {
			auto s2 = nvobj::make_persistent<S>(
				"QRST", static_cast<S::size_type>(0),
				static_cast<S::size_type>(
					static_cast<S::size_type>(
						3))); // calls
						      // ctor(string("QRST",
						      // pos, len)
			UT_ASSERT(*s2 == "QRS");
			nvobj::delete_persistent<S>(s2);
		});

		nvobj::transaction::run(pop, [&] {
			auto s3 = nvobj::make_persistent<S>(
				sv, static_cast<S::size_type>(0),
				std::string::npos); // calls ctor(T, pos, npos)
			UT_ASSERT(*s3 == sv);
			nvobj::delete_persistent<S>(s3);
		});

		nvobj::transaction::run(pop, [&] {
			auto s4 = nvobj::make_persistent<S>(
				sv, static_cast<S::size_type>(0),
				static_cast<S::size_type>(
					3)); // calls ctor(T, pos, len)
			UT_ASSERT(*s4 == "EFG");
			nvobj::delete_persistent<S>(s4);
		});

		nvobj::transaction::run(pop, [&] {
			auto s5 = nvobj::make_persistent<S>(
				arr, static_cast<S::size_type>(0),
				static_cast<S::size_type>(
					2)); // calls ctor(const char *, len)
			UT_ASSERT(*s5 == "IJ");
			nvobj::delete_persistent<S>(s5);
		});

		nvobj::transaction::run(pop, [&] {
			auto s6 = nvobj::make_persistent<S>(
				arr,
				static_cast<S::size_type>(
					0)); // calls ctor(const char *, len)
			UT_ASSERT(*s6 == "");
			nvobj::delete_persistent<S>(s6);
		});

		nvobj::transaction::run(pop, [&] {
			auto s7 = nvobj::make_persistent<S>(
				s->data(),
				static_cast<S::size_type>(
					2)); // calls ctor(const char *, len)
			UT_ASSERT(*s7 == "AB");
			nvobj::delete_persistent<S>(s7);
		});
		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<S>(s); });
	}
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<root> pop;

	try {
		pop = pmem::obj::pool<root>::create(path, "T_size_size",
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}

	run(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
