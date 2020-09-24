// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_TESTS_TRANSACTION
#define LIBPMEMOBJ_CPP_TESTS_TRANSACTION

#include <libpmemobj++/transaction.hpp>

namespace
{
int counter = 0;
}

/*
 * XXX The Microsoft compiler does not follow the ISO SD-6: SG10 Feature
 * Test Recommendations. "_MSC_VER" is a workaround.
 */
#if _MSC_VER < 1900
#ifndef __cpp_lib_uncaught_exceptions
#define __cpp_lib_uncaught_exceptions 201411
namespace std
{

int
uncaught_exceptions() noexcept
{
	return ::counter;
}

} /* namespace std */
#endif /* __cpp_lib_uncaught_exceptions */
#endif /* _MSC_VER */
#endif /* LIBPMEMOBJ_CPP_TESTS_TRANSACTION */
