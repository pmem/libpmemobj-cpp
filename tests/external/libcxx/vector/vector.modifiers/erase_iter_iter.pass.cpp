//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019-2020, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

#include <iterator>

namespace nvobj = pmem::obj;

struct Throws;

using C = container_t<int>;
using C2 = container_t<Throws>;
using C3 = container_t<C>;
using std::distance;
using std::next;

struct root {
	nvobj::persistent_ptr<C> l1;
	nvobj::persistent_ptr<C2> v;
	nvobj::persistent_ptr<C3> outer;
};

struct Throws {
	Throws() : v_(0)
	{
	}
	Throws(int v) : v_(v)
	{
	}
	Throws(const Throws &rhs) : v_(rhs.v_)
	{
		if (sThrows)
			throw 1;
	}
	Throws(Throws &&rhs) : v_(rhs.v_)
	{
		if (sThrows)
			throw 1;
	}
	Throws &
	operator=(const Throws &rhs)
	{
		v_ = rhs.v_;
		return *this;
	}
	Throws &
	operator=(Throws &&rhs)
	{
		v_ = rhs.v_;
		return *this;
	}
	int v_;
	static bool sThrows;
};

bool Throws::sThrows = false;

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: erase_iter_iter", PMEMOBJ_MIN_POOL * 4,
		S_IWUSR | S_IRUSR);

	auto r = pop.root();
	{
		int a1[] = {1, 2, 3};
		{
			try {
				nvobj::transaction::run(pop, [&] {
					r->l1 = nvobj::make_persistent<C>(
						a1, a1 + 3);
				});
				C::iterator i = r->l1->erase(r->l1->cbegin(),
							     r->l1->cbegin());
				UT_ASSERT(r->l1->size() == 3);
				UT_ASSERT(distance(r->l1->cbegin(),
						   r->l1->cend()) == 3);
				UT_ASSERT(i == r->l1->begin());
				nvobj::transaction::run(pop, [&] {
					nvobj::delete_persistent<C>(r->l1);
				});
			} catch (std::exception &e) {
				UT_FATALexc(e);
			}
		}
		{
			try {
				nvobj::transaction::run(pop, [&] {
					r->l1 = nvobj::make_persistent<C>(
						a1, a1 + 3);
				});
				C::iterator i = r->l1->erase(
					r->l1->cbegin(), next(r->l1->cbegin()));
				UT_ASSERTeq(r->l1->size(), 2);
				UT_ASSERT(distance(r->l1->cbegin(),
						   r->l1->cend()) == 2);
				UT_ASSERT(i == r->l1->begin());

				nvobj::transaction::run(pop, [&] {
					nvobj::persistent_ptr<C> tmp =
						nvobj::make_persistent<C>(
							a1 + 1, a1 + 3);
					UT_ASSERT(*r->l1 == *tmp);
					nvobj::delete_persistent<C>(tmp);
				});
				nvobj::transaction::run(pop, [&] {
					nvobj::delete_persistent<C>(r->l1);
				});
			} catch (std::exception &e) {
				UT_FATALexc(e);
			}
		}
		{
			try {
				nvobj::transaction::run(pop, [&] {
					r->l1 = nvobj::make_persistent<C>(
						a1, a1 + 3);
				});
				C::iterator i =
					r->l1->erase(r->l1->cbegin(),
						     next(r->l1->cbegin(), 2));
				UT_ASSERT(r->l1->size() == 1);
				UT_ASSERT(distance(r->l1->cbegin(),
						   r->l1->cend()) == 1);
				UT_ASSERT(i == r->l1->begin());

				nvobj::transaction::run(pop, [&] {
					nvobj::persistent_ptr<C> tmp =
						nvobj::make_persistent<C>(
							a1 + 2, a1 + 3);
					UT_ASSERT(*r->l1 == *tmp);
					nvobj::delete_persistent<C>(tmp);
				});
				nvobj::transaction::run(pop, [&] {
					nvobj::delete_persistent<C>(r->l1);
				});
			} catch (std::exception &e) {
				UT_FATALexc(e);
			}
		}
		{
			try {
				nvobj::transaction::run(pop, [&] {
					r->l1 = nvobj::make_persistent<C>(
						a1, a1 + 3);
				});
				C::iterator i =
					r->l1->erase(r->l1->cbegin(),
						     next(r->l1->cbegin(), 3));
				UT_ASSERT(r->l1->size() == 0);
				UT_ASSERT(distance(r->l1->cbegin(),
						   r->l1->cend()) == 0);
				UT_ASSERT(i == r->l1->begin());
				nvobj::transaction::run(pop, [&] {
					nvobj::delete_persistent<C>(r->l1);
				});
			} catch (std::exception &e) {
				UT_FATALexc(e);
			}
		}
		{
			try {
				nvobj::persistent_ptr<C> tmp;
				nvobj::transaction::run(pop, [&] {
					tmp = nvobj::make_persistent<C>(1U);
					r->outer = nvobj::make_persistent<C3>(
						2U, *tmp);
				});
				r->outer->erase(r->outer->begin(),
						r->outer->begin());
				UT_ASSERT(r->outer->size() == 2);
				UT_ASSERT((*r->outer)[0].size() == 1);
				UT_ASSERT((*r->outer)[1].size() == 1);
				nvobj::transaction::run(pop, [&] {
					nvobj::delete_persistent<C>(tmp);
					nvobj::delete_persistent<C3>(r->outer);
				});
			} catch (std::exception &e) {
				UT_FATALexc(e);
			}
		}
	}
	{
		Throws arr[] = {1, 2, 3};
		try {
			nvobj::transaction::run(pop, [&] {
				r->v = nvobj::make_persistent<C2>(arr, arr + 3);
			});
			Throws::sThrows = true;
			r->v->erase(r->v->begin(), --r->v->end());
			UT_ASSERT(r->v->size() == 1);
			r->v->erase(r->v->begin(), r->v->end());
			UT_ASSERT(r->v->size() == 0);
			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C2>(r->v);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
