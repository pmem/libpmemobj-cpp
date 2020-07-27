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

// ~map() // implied noexcept;

// UNSUPPORTED: c++98, c++03

#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using CM = container_t<int, double>;

struct root {
	nvobj::persistent_ptr<CM> s;
};

class MoveOnly {
	MoveOnly(const MoveOnly &);
	MoveOnly &operator=(const MoveOnly &);

	int data_;

public:
	MoveOnly(int data = 1) : data_(data)
	{
	}
	MoveOnly(MoveOnly &&x) : data_(x.data_)
	{
		x.data_ = 0;
	}
	MoveOnly &
	operator=(MoveOnly &&x)
	{
		data_ = x.data_;
		x.data_ = 0;
		return *this;
	}

	int
	get() const
	{
		return data_;
	}

	bool
	operator==(const MoveOnly &x) const
	{
		return data_ == x.data_;
	}
	bool
	operator<(const MoveOnly &x) const
	{
		return data_ < x.data_;
	}
	MoveOnly
	operator+(const MoveOnly &x) const
	{
		return MoveOnly{data_ + x.data_};
	}
	MoveOnly operator*(const MoveOnly &x) const
	{
		return MoveOnly{data_ * x.data_};
	}
};

template <class T>
struct some_comp {
	typedef T value_type;
	~some_comp() noexcept(false);
	bool
	operator()(const T &, const T &) const noexcept
	{
		return false;
	}
};

int
run(pmem::obj::pool<root> &pop)
{
	{
		typedef nvobjex::concurrent_map<MoveOnly, MoveOnly> C;
		static_assert(std::is_nothrow_destructible<C>::value, "");
	}
#ifdef XXX // XXX: Implemenet test_allocator and other_allocator class
	typedef std::pair<const MoveOnly, MoveOnly> V;
	{
		typedef nvobjex::concurrent_map<MoveOnly, MoveOnly,
						std::less<MoveOnly>,
						test_allocator<V>>
			C;
		static_assert(std::is_nothrow_destructible<C>::value, "");
	}
	{
		typedef nvobjex::concurrent_map<MoveOnly, MoveOnly,
						std::less<MoveOnly>,
						other_allocator<V>>
			C;
		static_assert(std::is_nothrow_destructible<C>::value, "");
	}
#endif
	{
		typedef container_t<MoveOnly, MoveOnly, some_comp<MoveOnly>> C;
		static_assert(!std::is_nothrow_destructible<C>::value, "");
	}

	return 0;
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<root> pop;
	try {
		pop = pmem::obj::pool<root>::create(path, "dtor_noexcept.pass",
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}
	try {
		run(pop);
		pop.close();
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
