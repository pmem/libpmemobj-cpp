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

#include <libpmemobj++/experimental/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmem_exp = pmem::obj::experimental;

struct NonSwappable {
	NonSwappable()
	{
	}

private:
	NonSwappable(NonSwappable const &);
	NonSwappable &operator=(NonSwappable const &);
};

using pmem_exp::swap;

template <class Tp>
decltype(swap(std::declval<Tp>(), std::declval<Tp>())) can_swap_imp(int);

template <class Tp>
std::false_type can_swap_imp(...);

template <class Tp>
struct can_swap : std::is_same<decltype(can_swap_imp<Tp>(0)), void> {
};

struct Testcase1 {
	typedef double T;
	typedef pmem_exp::array<T, 3> C;
	C c1 = {{1, 2, 3.5}};
	C c2 = {{4, 5, 6.5}};

	void
	run()
	{
		swap(c1, c2);
		UT_ASSERT(c1.size() == 3);
		UT_ASSERT(c1[0] == 4);
		UT_ASSERT(c1[1] == 5);
		UT_ASSERT(c1[2] == 6.5);
		UT_ASSERT(c2.size() == 3);
		UT_ASSERT(c2[0] == 1);
		UT_ASSERT(c2[1] == 2);
		UT_ASSERT(c2[2] == 3.5);
	}
};

struct Testcase2 {
	typedef double T;
	typedef pmem_exp::array<T, 0> C;
	C c1 = {{}};
	C c2 = {{}};

	void
	run()
	{
		swap(c1, c2);
		UT_ASSERT(c1.size() == 0);
		UT_ASSERT(c2.size() == 0);
	}
};

struct Testcase3 {
	typedef NonSwappable T;
	typedef pmem_exp::array<T, 0> C0;
	static_assert(can_swap<C0 &>::value, "");
	C0 l = {{}};
	C0 r = {{}};

	void
	run()
	{
		swap(l, r);
	}
};

struct root {
	pmem::obj::persistent_ptr<Testcase1> r1;
	pmem::obj::persistent_ptr<Testcase2> r2;
	pmem::obj::persistent_ptr<Testcase3> r3;
};

void
run(pmem::obj::pool<root> &pop)
{
	try {
		pmem::obj::transaction::run(pop, [&] {
			pop.root()->r1 =
				pmem::obj::make_persistent<Testcase1>();
			pop.root()->r2 =
				pmem::obj::make_persistent<Testcase2>();
			pop.root()->r3 =
				pmem::obj::make_persistent<Testcase3>();
		});

		pmem::obj::transaction::run(pop, [&] {
			pop.root()->r1->run();
			pop.root()->r2->run();
			pop.root()->r3->run();
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

int
main(int argc, char *argv[])
{
	START();

	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<root> pop;
	try {
		pop = pmem::obj::pool<root>::create(
			path, "swap.pass", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}

	run(pop);

	return 0;
}
