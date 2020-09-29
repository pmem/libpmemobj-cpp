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

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/string_view.hpp>

void
foo(const pmem::obj::string &s)
{
}

int
main()
{
	pmem::obj::string_view sv = "ABCDE";
	foo(sv); // requires implicit conversion from string_view to string
}
