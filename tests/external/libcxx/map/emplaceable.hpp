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

#ifndef LIBPMEMOBJ_CPP_TESTS_EMPLACEABLE_H
#define LIBPMEMOBJ_CPP_TESTS_EMPLACEABLE_H

#include <functional>

class Emplaceable {
	Emplaceable(const Emplaceable &);
	Emplaceable &operator=(const Emplaceable &);

	int int_;
	double double_;

public:
	Emplaceable() : int_(0), double_(0)
	{
	}
	Emplaceable(int i, double d) : int_(i), double_(d)
	{
	}
	Emplaceable(Emplaceable &&x) : int_(x.int_), double_(x.double_)
	{
		x.int_ = 0;
		x.double_ = 0;
	}
	Emplaceable &
	operator=(Emplaceable &&x)
	{
		int_ = x.int_;
		x.int_ = 0;
		double_ = x.double_;
		x.double_ = 0;
		return *this;
	}

	bool
	operator==(const Emplaceable &x) const
	{
		return int_ == x.int_ && double_ == x.double_;
	}
	bool
	operator<(const Emplaceable &x) const
	{
		return int_ < x.int_ || (int_ == x.int_ && double_ < x.double_);
	}

	int
	get() const
	{
		return int_;
	}
};

namespace std
{

template <>
struct hash<Emplaceable> {
	typedef Emplaceable argument_type;
	typedef std::size_t result_type;

	std::size_t
	operator()(const Emplaceable &x) const
	{
		return static_cast<std::size_t>(x.get());
	}
};

}

#endif // LIBPMEMOBJ_CPP_TESTS_EMPLACEABLE_H
