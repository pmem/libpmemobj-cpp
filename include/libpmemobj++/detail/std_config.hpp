/*
 * Copyright 2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * Macros for handling different C++ standards
 */

#ifndef PMEMOBJ_STD_CONFIG_HPP
#define PMEMOBJ_STD_CONFIG_HPP

#include <type_traits>

#if __cplusplus <= 201103L && !defined(PMEMOBJ_CXX_STANDARD)
#define PMEMOBJ_CXX_STANDARD 11
#elif __cplusplus <= 201402L && !defined(PMEMOBJ_CXX_STANDARD)
#define PMEMOBJ_CXX_STANDARD 14
#elif __cplusplus <= 201703L && !defined(PMEMOBJ_CXX_STANDARD)
#define PMEMOBJ_CXX_STANDARD 17
#elif !defined(PMEMOBJ_CXX_STANDARD)
#define PMEMOBJ_CXX_STANDARD 20
#endif

/* Macros for detecting C++17 features not implemented by all compilers */
#define PMEMOBJ_IS_AGGREGATE __cpp_lib_is_aggregate

/*
 * Following macros can be used to mark expressions as constexpr only since
 * specified C++ version.
 *
 * For example PMEMOBJ_CONSTEXPR_CXX14 will be defined as constexpr only
 * for compilers that support C++14 and later.
 */
#if PMEMOBJ_CXX_STANDARD >= 11
#define PMEMOBJ_CONSTEXPR_CXX11 constexpr
#else
#define PMEMOBJ_CONSTEXPR_CXX11
#endif

#if PMEMOBJ_CXX_STANDARD >= 14
#define PMEMOBJ_CONSTEXPR_CXX14 constexpr
#else
#define PMEMOBJ_CONSTEXPR_CXX14
#endif

#if PMEMOBJ_CXX_STANDARD >= 17
#define PMEMOBJ_CONSTEXPR_CXX17 constexpr
#else
#define PMEMOBJ_CONSTEXPR_CXX17
#endif

#endif /* PMEMOBJ_STD_CONFIG_HPP */
