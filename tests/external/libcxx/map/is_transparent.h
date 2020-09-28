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
// testing transparent
//

#ifndef LIBPMEMOBJ_CPP_TESTS_TRANSPARENT_H
#define LIBPMEMOBJ_CPP_TESTS_TRANSPARENT_H

/* Windows has a max macro which collides with std::numeric_limits::max */
#if defined(max) && defined(_WIN32)
#undef max
#endif

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/p.hpp>
#include <limits>

#include "private_constructor.h"
#include "unittest.hpp"

struct C2Int { // comparable to int
	C2Int() : i_(0)
	{
	}
	C2Int(int i) : i_(i)
	{
	}
	int
	get() const
	{
		return i_;
	}

private:
	pmem::obj::p<int> i_;
};

bool
operator<(int rhs, const C2Int &lhs)
{
	return rhs < lhs.get();
}
bool
operator<(const C2Int &rhs, const C2Int &lhs)
{
	return rhs.get() < lhs.get();
}
bool
operator<(const C2Int &rhs, int lhs)
{
	return rhs.get() < lhs;
}

class Moveable {
	Moveable &operator=(const Moveable &);

	pmem::obj::p<int> int_;
	pmem::obj::p<double> double_;

public:
	Moveable() : int_(0), double_(0)
	{
	}
	Moveable(int i, double d) : int_(i), double_(d)
	{
	}
	Moveable(Moveable &&x) : int_(x.int_), double_(x.double_)
	{
		x.int_ = -1;
		x.double_ = -1;
	}
	Moveable(const Moveable &) = default;
	Moveable &
	operator=(Moveable &&x)
	{
		int_ = x.int_;
		x.int_ = -1;
		double_ = x.double_;
		x.double_ = -1;
		return *this;
	}

	bool
	operator==(const Moveable &x) const
	{
		return int_ == x.int_ && double_ == x.double_;
	}
	bool
	operator<(const Moveable &x) const
	{
		return int_ < x.int_ || (int_ == x.int_ && double_ < x.double_);
	}

	int
	get() const
	{
		return int_;
	}
	bool
	moved() const
	{
		return int_ == -1;
	}
};

class MoveableWrapper {
	Moveable mv_;

public:
	MoveableWrapper(Moveable &&mv) : mv_(std::move(mv))
	{
	}

	Moveable
	get() const
	{
		return mv_;
	}
};

bool
operator<(const MoveableWrapper &lhs, const MoveableWrapper &rhs)
{
	return lhs.get() < rhs.get();
}
bool
operator<(const MoveableWrapper &lhs, Moveable rhs)
{
	return lhs.get() < rhs;
}
bool
operator<(Moveable lhs, const MoveableWrapper &rhs)
{
	return lhs < rhs.get();
}

struct transparent_less {
	template <class T, class U>
	constexpr auto
	operator()(T &&t, U &&u) const
		noexcept(noexcept(std::forward<T>(t) < std::forward<U>(u)))
			-> decltype(std::forward<T>(t) < std::forward<U>(u))
	{
		return std::forward<T>(t) < std::forward<U>(u);
	}
	using is_transparent = void; // correct
};

struct transparent_less_not_referenceable {
	template <class T, class U>
	constexpr auto
	operator()(T &&t, U &&u) const
		noexcept(noexcept(std::forward<T>(t) < std::forward<U>(u)))
			-> decltype(std::forward<T>(t) < std::forward<U>(u))
	{
		return std::forward<T>(t) < std::forward<U>(u);
	}
	using is_transparent =
		void() const &; // it's a type; a weird one, but a type
};

struct heterogenous_bytes_view {
	heterogenous_bytes_view(const int *value)
	{
		v = (unsigned)(*value + (std::numeric_limits<int>::max)() + 1);
	}

	heterogenous_bytes_view(const C2Int *value)
	{
		v = (unsigned)(value->get() +
			       (std::numeric_limits<int>::max)() + 1);
	}

	heterogenous_bytes_view(const Moveable *value)
	{
		v = (unsigned)(value->get() +
			       (std::numeric_limits<int>::max)() + 1);
	}

	heterogenous_bytes_view(const MoveableWrapper *value)
	{
		v = (unsigned)(value->get().get() +
			       (std::numeric_limits<int>::max)() + 1);
	}

	heterogenous_bytes_view(const PrivateConstructor *value)
	{
		v = (unsigned)(value->get() +
			       (std::numeric_limits<int>::max)() + 1);
	}

	size_t
	size() const
	{
		return sizeof(int);
	}

	char operator[](std::size_t p) const
	{
		return reinterpret_cast<const char *>(&v)[size() - p - 1];
	}

	unsigned v;

	using is_transparent = void;
};

#endif // LIBPMEMOBJ_CPP_TESTS_TRANSPARENT_H
