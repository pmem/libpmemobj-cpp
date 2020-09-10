// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2020, Intel Corporation */

/**
 * @file
 * Convenience extensions for the resides on pmem property template.
 */

#ifndef LIBPMEMOBJ_CPP_PEXT_HPP
#define LIBPMEMOBJ_CPP_PEXT_HPP

#include <iostream>
#include <libpmemobj++/p.hpp>
#include <limits>

namespace pmem
{

namespace obj
{

/**
 * Ostream operator overload.
 */
template <typename T>
std::ostream &
operator<<(std::ostream &os, const p<T> &pp)
{
	return os << pp.get_ro();
}

/**
 * Istream operator overload.
 */
template <typename T>
std::istream &
operator>>(std::istream &is, p<T> &pp)
{
	is >> pp.get_rw();
	return is;
}

/**
 * Prefix increment operator overload.
 */
template <typename T>
p<T> &
operator++(p<T> &pp)
{
	++(pp.get_rw());
	return pp;
}

/**
 * Prefix decrement operator overload.
 */
template <typename T>
p<T> &
operator--(p<T> &pp)
{
	--(pp.get_rw());
	return pp;
}

/**
 * Postfix increment operator overload.
 */
template <typename T>
p<T>
operator++(p<T> &pp, int)
{
	p<T> temp = pp;
	++pp;
	return temp;
}

/**
 * Postfix decrement operator overload.
 */
template <typename T>
p<T>
operator--(p<T> &pp, int)
{
	p<T> temp = pp;
	--pp;
	return temp;
}

/**
 * Addition assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator+=(p<T> &lhs, const p<Y> &rhs)
{
	lhs.get_rw() += rhs.get_ro();
	return lhs;
}

/**
 * Addition assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator+=(p<T> &lhs, const Y &rhs)
{
	lhs.get_rw() += rhs;
	return lhs;
}

/**
 * Subtraction assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator-=(p<T> &lhs, const p<Y> &rhs)
{
	lhs.get_rw() -= rhs.get_ro();
	return lhs;
}

/**
 * Subtraction assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator-=(p<T> &lhs, const Y &rhs)
{
	lhs.get_rw() -= rhs;
	return lhs;
}

/**
 * Multiplication assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator*=(p<T> &lhs, const p<Y> &rhs)
{
	lhs.get_rw() *= rhs.get_ro();
	return lhs;
}

/**
 * Multiplication assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator*=(p<T> &lhs, const Y &rhs)
{
	lhs.get_rw() *= rhs;
	return lhs;
}

/**
 * Division assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator/=(p<T> &lhs, const p<Y> &rhs)
{
	lhs.get_rw() /= rhs.get_ro();
	return lhs;
}

/**
 * Division assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator/=(p<T> &lhs, const Y &rhs)
{
	lhs.get_rw() /= rhs;
	return lhs;
}

/**
 * Modulo assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator%=(p<T> &lhs, const p<Y> &rhs)
{
	lhs.get_rw() %= rhs.get_ro();
	return lhs;
}

/**
 * Modulo assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator%=(p<T> &lhs, const Y &rhs)
{
	lhs.get_rw() %= rhs;
	return lhs;
}

/**
 * Bitwise AND assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator&=(p<T> &lhs, const p<Y> &rhs)
{
	lhs.get_rw() &= rhs.get_ro();
	return lhs;
}

/**
 * Bitwise AND assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator&=(p<T> &lhs, const Y &rhs)
{
	lhs.get_rw() &= rhs;
	return lhs;
}

/**
 * Bitwise OR assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator|=(p<T> &lhs, const p<Y> &rhs)
{
	lhs.get_rw() |= rhs.get_ro();
	return lhs;
}

/**
 * Bitwise OR assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator|=(p<T> &lhs, const Y &rhs)
{
	lhs.get_rw() |= rhs;
	return lhs;
}

/**
 * Bitwise XOR assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator^=(p<T> &lhs, const p<Y> &rhs)
{
	lhs.get_rw() ^= rhs.get_ro();
	return lhs;
}

/**
 * Bitwise XOR assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator^=(p<T> &lhs, const Y &rhs)
{
	lhs.get_rw() ^= rhs;
	return lhs;
}

/**
 * Bitwise left shift assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator<<=(p<T> &lhs, const p<Y> &rhs)
{
	lhs.get_rw() = lhs.get_ro() << rhs.get_ro();
	return lhs;
}

/**
 * Bitwise left shift assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator<<=(p<T> &lhs, const Y &rhs)
{
	lhs.get_rw() = lhs.get_ro() << rhs;
	return lhs;
}

/**
 * Bitwise right shift assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator>>=(p<T> &lhs, const p<Y> &rhs)
{
	lhs.get_rw() = lhs.get_ro() >> rhs.get_ro();
	return lhs;
}

/**
 * Bitwise right shift assignment operator overload.
 */
template <typename T, typename Y>
p<T> &
operator>>=(p<T> &lhs, const Y &rhs)
{
	lhs.get_rw() = lhs.get_ro() >> rhs;
	return lhs;
}

} /* namespace obj */

} /* namespace pmem */

namespace std
{
/**
 * Specialization of std::numeric_limits for p<T>
 */
template <typename T>
struct numeric_limits<pmem::obj::p<T>> : public numeric_limits<T> {

	static constexpr bool is_specialized = true;
};

/**
 * Specialization of std::less for p<T>
 */
template <typename T>
struct less<pmem::obj::p<T>> {
	size_t
	operator()(const pmem::obj::p<T> &lhs, const pmem::obj::p<T> &rhs) const
	{
		return lhs.get_ro() < rhs.get_ro();
	}
};

} /* namespace std */

#endif /* LIBPMEMOBJ_CPP_PEXT_HPP */
