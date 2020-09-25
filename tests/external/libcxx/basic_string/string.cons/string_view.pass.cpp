//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <string>

// explicit basic_string(basic_string_view<CharT, traits> sv, const Allocator& a
// = Allocator());

#include "unittest.hpp"

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/string_view.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;

struct root {
	nvobj::persistent_ptr<pmem::obj::string> s1;
};

#ifdef XXX // XXX: implement test_allocator class
template <class charT>
void
test(std::basic_string_view<charT> sv)
{
	typedef std::basic_string<charT, std::char_traits<charT>,
				  test_allocator<charT>>
		S;
	typedef typename S::traits_type T;
	typedef typename S::allocator_type A;
	{
		S s2(sv);
		LIBCPP_UT_ASSERT(s2.__invariants());
		UT_ASSERT(s2.size() == sv.size());
		UT_ASSERT(T::compare(s2.data(), sv.data(), sv.size()) == 0);
		UT_ASSERT(s2.get_allocator() == A());
		UT_ASSERT(s2.capacity() >= s2.size());
	}
	{
		S s2;
		s2 = sv;
		LIBCPP_UT_ASSERT(s2.__invariants());
		UT_ASSERT(s2.size() == sv.size());
		UT_ASSERT(T::compare(s2.data(), sv.data(), sv.size()) == 0);
		UT_ASSERT(s2.get_allocator() == A());
		UT_ASSERT(s2.capacity() >= s2.size());
	}
}
#endif

template <class charT>
void
test(nvobj::basic_string_view<charT> sv, pmem::obj::pool<root> &pop)
{
	typedef nvobj::basic_string<charT, std::char_traits<charT>> S;
	typedef typename S::traits_type T;

	nvobj::persistent_ptr<S> s2;
	nvobj::transaction::run(pop,
				[&] { s2 = nvobj::make_persistent<S>(sv); });
	{
		UT_ASSERT(s2->size() == sv.size());
		UT_ASSERT(T::compare(s2->data(), sv.data(), sv.size()) == 0);
		UT_ASSERT(s2->capacity() >= s2->size());
	}
	{
		*s2 = sv;
		UT_ASSERT(s2->size() == sv.size());
		UT_ASSERT(T::compare(s2->data(), sv.data(), sv.size()) == 0);
		UT_ASSERT(s2->capacity() >= s2->size());
	}
	nvobj::transaction::run(pop, [&] { nvobj::delete_persistent<S>(s2); });
}

void
run(pmem::obj::pool<root> &pop)
{
	{
		typedef nvobj::basic_string_view<char, std::char_traits<char>>
			SV;

		test(SV(""), pop);
		test(SV("1"), pop);
		test(SV("1234567980"), pop);
		test(SV("123456798012345679801234567980123456798012345679801234567980"),
		     pop);
	}
#ifdef XXX // XXX: implement test_allocator class
	{
		typedef min_allocator<char> A;
		typedef std::basic_string_view<char, std::char_traits<char>> SV;

		test(SV(""));
		test(SV(""), A());

		test(SV("1"));
		test(SV("1"), A());

		test(SV("1234567980"));
		test(SV("1234567980"), A());

		test(SV("123456798012345679801234567980123456798012345679801234567980"));
		test(SV("123456798012345679801234567980123456798012345679801234567980"),
		     A());
	}
#endif
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<root> pop;

	try {
		pop = pmem::obj::pool<root>::create(path, "string_view",
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
