//===----------------------------------------------------------------------===//
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

#include "unittest.hpp"

#include "libpmemobj++/experimental/vector.hpp"
#include "libpmemobj++/make_persistent.hpp"
#include "libpmemobj++/pool.hpp"
#include "libpmemobj++/transaction.hpp"

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;

struct root {
	nvobj::persistent_ptr<pmem_exp::vector<int>> v1;
	nvobj::persistent_ptr<pmem_exp::vector<int>> v2;
};

void
test_contiguous(const pmem_exp::vector<int> &c)
{
	for (size_t i = 0; i < c.size(); ++i)
		UT_ASSERT(*(c.begin() +
			    static_cast<typename pmem_exp::vector<
				    int>::difference_type>(i)) ==
			  *(std::addressof(*c.begin()) + i));
}

int
main(int argc, char *argv[])
{
	START();

	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
		return 1;
	}

	auto path = argv[1];
	auto pop =
		nvobj::pool<root>::create(path, "VectorTest: contiguous.pass",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->v1 = nvobj::make_persistent<pmem_exp::vector<int>>();
			r->v2 = nvobj::make_persistent<pmem_exp::vector<int>>(
				3U, 5);
		});
		test_contiguous(*r->v1);
		test_contiguous(*r->v2);
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl
			  << std::strerror(nvobj::transaction::error())
			  << std::endl;
		UT_ASSERT(0);
	}

	pop.close();

	return 0;
}
