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

// <map>

// class map

// map();

#include <libpmemobj++/experimental/concurrent_map.hpp>

#include "unittest.hpp"

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

struct X {
#ifdef XXX // Error: incomplete type/forward declaration of struct X
	nvobjex::concurrent_map<int, X> m;
	nvobjex::concurrent_map<int, X>::iterator i;
	nvobjex::concurrent_map<int, X>::const_iterator ci;
	nvobjex::concurrent_map<int, X>::reverse_iterator ri;
	nvobjex::concurrent_map<int, X>::const_reverse_iterator cri;
#endif
};

int
main(int, char **)
{

	return 0;
}
