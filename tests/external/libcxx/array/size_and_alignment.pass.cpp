//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2018-2020, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"
#include <cstddef>

#include <libpmemobj++/container/array.hpp>

template <class T, size_t Size>
struct MyArray {
	T elems[Size];
};

template <class T, size_t Size>
void
test()
{
	typedef T CArrayT[Size];
	typedef pmem::obj::array<T, Size> ArrayT;
	typedef MyArray<T, Size> MyArrayT;
	static_assert(sizeof(ArrayT) == sizeof(CArrayT), "");
	static_assert(sizeof(ArrayT) == sizeof(MyArrayT), "");
	static_assert(alignof(ArrayT) == alignof(MyArrayT), "");
}

template <class T>
void
test_zero_sized()
{
	typedef pmem::obj::array<T, 0> ArrayT;
	static_assert(sizeof(ArrayT) == sizeof(T), "");
}

template <class T>
void
test_type()
{
	test<T, 1>();
	test<T, 42>();
	test_zero_sized<T>();
}

// static_assert(sizeof(void*) == 4, "");

int
main(int argc, char *argv[])
{
	return run_test([&] {
		test_type<char>();
		test_type<int>();
		test_type<double>();
		test_type<long double>();
		test_type<std::max_align_t>();
	});
}
