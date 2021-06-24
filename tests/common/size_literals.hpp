// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#ifndef SIZE_LITERALS_HPP
#define SIZE_LITERALS_HPP

#include <cstddef>

constexpr size_t operator""_Kilo(unsigned long long int x)
{
	return 1024ULL * x;
}

constexpr size_t operator"" _Mega(unsigned long long int x)
{
	return 1024_Kilo * x;
}

constexpr size_t operator"" _Giga(unsigned long long int x)
{
	return 1024_Mega * x;
}

constexpr size_t operator"" _Tera(unsigned long long int x)
{
	return 1024_Giga * x;
}

constexpr size_t operator"" _Peta(unsigned long long int x)
{
	return 1024_Tera * x;
}

constexpr size_t operator"" _Eksa(unsigned long long int x)
{
	return 1024_Peta * x;
}

#endif /* SIZE_LITERALS_HPP */
