//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Copyright 2020, Intel Corporation
//
// Modified to test pmem::obj containers
//

#ifndef LIBPMEMOBJ_CPP_TESTS_DEFAULT_ONLY_H
#define LIBPMEMOBJ_CPP_TESTS_DEFAULT_ONLY_H

#include <libpmemobj++/persistent_ptr.hpp>

#include <cassert>

class DefaultOnly {
	pmem::obj::p<int> data_;

	DefaultOnly(const DefaultOnly &);
	DefaultOnly &operator=(const DefaultOnly &);

public:
	static int count;

	DefaultOnly() : data_(-1)
	{
		++count;
	}
	~DefaultOnly()
	{
		data_ = 0;
		--count;
	}

	friend bool
	operator==(const DefaultOnly &x, const DefaultOnly &y)
	{
		return x.data_ == y.data_;
	}
	friend bool
	operator<(const DefaultOnly &x, const DefaultOnly &y)
	{
		return x.data_ < y.data_;
	}
};

int DefaultOnly::count = 0;

#endif // LIBPMEMOBJ_CPP_TESTS_DEFAULT_ONLY_H
