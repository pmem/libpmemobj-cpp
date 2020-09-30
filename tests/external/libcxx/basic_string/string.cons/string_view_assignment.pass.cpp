//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <string>

// basic_string<charT,traits,Allocator>& operator=(basic_string_view<charT,
// traits> sv);

#include <utility>
#include <vector>

#include "unittest.hpp"

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/string_view.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;

typedef nvobj::string S;
typedef nvobj::string_view SV;

struct root {
	nvobj::persistent_ptr<S> s1;
};

template <class S, class SV>
void
test(S &s1, const SV &sv)
{
	typedef typename S::traits_type T;
	s1 = sv;
	UT_ASSERT(s1.size() == sv.size());
	UT_ASSERT(T::compare(s1.data(), sv.data(), s1.size()) == 0);
	UT_ASSERT(s1.capacity() >= s1.size());
}

void
run(pmem::obj::pool<root> &pop)
{
	std::vector<std::pair<const char *, const char *>> vec = {
		{nullptr, ""},
		{"1", ""},
		{nullptr, "1"},
		{"1", "2"},
		{"1", "2"},
		{nullptr,
		 "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"},
		{"123456789",
		 "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"},
		{"1234567890123456789012345678901234567890123456789012345678901234567890",
		 "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"},
		{"1234567890123456789012345678901234567890123456789012345678901234567890"
		 "1234567890123456789012345678901234567890123456789012345678901234567890",
		 "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"}};
	{
		for (auto &str_sv_pair : vec) {
			nvobj::transaction::run(pop, [&] {
				auto ns =
					(str_sv_pair.first
						 ? nvobj::make_persistent<S>(
							   str_sv_pair.first)
						 : nvobj::make_persistent<S>());
				test(*ns, SV(str_sv_pair.second));
				nvobj::delete_persistent<S>(ns);
			});
		}
	}
#ifdef XXX // XXX: implement min_allocator class
	{
		typedef std::basic_string<char, std::char_traits<char>,
					  min_allocator<char>>
			S;
		typedef std::string_view SV;
		test(S(), SV(""));
		test(S("1"), SV(""));
		test(S(), SV("1"));
		test(S("1"), SV("2"));
		test(S("1"), SV("2"));

		test(S(),
		     SV("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
		test(S("123456789"),
		     SV("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
		test(S("1234567890123456789012345678901234567890123456789012345678901234567890"),
		     SV("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
		test(S("1234567890123456789012345678901234567890123456789012345678901234567890"
		       "1234567890123456789012345678901234567890123456789012345678901234567890"),
		     SV("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
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
		pop = pmem::obj::pool<root>::create(
			path, "string_view_assignment", PMEMOBJ_MIN_POOL,
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
