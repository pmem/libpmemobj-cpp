// -*- C++ -*-
//===---------------------------- test_macros.h ---------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2018, Intel Corporation
//
// Modified to test pmem::obj containers
//

#ifndef SUPPORT_TEST_MACROS_HPP
#define SUPPORT_TEST_MACROS_HPP

#define TEST_ALIGNOF(...) __alignof(__VA_ARGS__)
#define TEST_ALIGNAS(...) __attribute__((__aligned__(__VA_ARGS__)))
#define TEST_CONSTEXPR
#define TEST_CONSTEXPR_CXX14
#define TEST_NOEXCEPT throw()
#define TEST_NOEXCEPT_FALSE
#define TEST_NOEXCEPT_COND(...)
#define TEST_THROW_SPEC(...) throw(__VA_ARGS__)

#endif // SUPPORT_TEST_MACROS_HPP
