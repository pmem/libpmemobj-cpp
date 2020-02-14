//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019-2020, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <iterator>
#include <libpmemobj++/container/vector.hpp>
#include <type_traits>

template <typename T>
void
test()
{
	using C = container_t<T>;

	static_assert(std::is_same<typename C::value_type, T>::value, "");

	static_assert(std::is_signed<typename C::difference_type>::value, "");
	static_assert(std::is_unsigned<typename C::size_type>::value, "");

	static_assert(
		std::is_same<
			typename C::difference_type,
			typename std::iterator_traits<
				typename C::iterator>::difference_type>::value,
		"");
	static_assert(std::is_same<typename C::difference_type,
				   typename std::iterator_traits<
					   typename C::const_iterator>::
					   difference_type>::value,
		      "");

	static_assert(
		std::is_same<typename std::iterator_traits<
				     typename C::iterator>::iterator_category,
			     std::random_access_iterator_tag>::value,
		"");
	static_assert(
		std::is_same<
			typename std::iterator_traits<
				typename C::const_iterator>::iterator_category,
			std::random_access_iterator_tag>::value,
		"");
	static_assert(
		std::is_same<
			typename C::reverse_iterator,
			std::reverse_iterator<typename C::iterator>>::value,
		"");
	static_assert(std::is_same<typename C::const_reverse_iterator,
				   std::reverse_iterator<
					   typename C::const_iterator>>::value,
		      "");
#if defined(VECTOR)
	static_assert(
		(std::is_same<typename C::const_iterator, const T *>::value),
		"");
#endif
}

static void
test()
{
	test<int>();
	test<int *>();

	using C = pmem::obj::vector<int>;
	static_assert(std::is_same<C::reference, int &>::value, "");
	static_assert(std::is_same<C::const_reference, const int &>::value, "");
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(); });
}
