/*
 * Copyright 2019, Intel Corporation
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

#include "helper_classes.hpp"
#include "list_wrapper.hpp"

#include <libpmemobj++/make_persistent.hpp>

#include <vector>

namespace nvobj = pmem::obj;

using c1 = copy_assignable_copy_insertable<int>;
using C1 = container_t<c1>;

using c2 = emplace_constructible_moveable_and_assignable<int>;
using C2 = container_t<c2>;

using c3 = emplace_constructible_and_move_insertable<int>;
using C3 = container_t<c3>;

using c4 = move_assignable;
using C4 = container_t<c4>;

using c5 = copy_insertable;
using C5 = container_t<c5>;

using c6 = move_insertable;
using C6 = container_t<c6>;

struct root {
	nvobj::persistent_ptr<C1> v1;
	nvobj::persistent_ptr<C2> v2;
	nvobj::persistent_ptr<C3> v3;
	nvobj::persistent_ptr<C4> v4;
	nvobj::persistent_ptr<C5> v5;
	nvobj::persistent_ptr<C6> v6;
};

#if defined(VECTOR)
void
test_copy_assignable_copy_insertable(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->v1 = nvobj::make_persistent<C1>(); });

		{
			/*
			 * Test if insert(const_iterator, const value_type &)
			 * works for CopyAssignable and CopyInsertable types.
			 */
			c1 temp(1);
			r->v1->insert(r->v1->cbegin(), temp);
			UT_ASSERTeq(r->v1->const_at(0).value, 1);
		}

		{
			/*
			 * Test if insert(const_iterator, size_type, const
			 * value_type &) works for CopyAssignable and
			 * CopyInsertable types.
			 */
			c1 temp(1);
			r->v1->insert(r->v1->cbegin(), 1, temp);
			UT_ASSERTeq(r->v1->const_at(0).value, 1);
		}

		{
			/*
			 * Test if insert(const_iterator, InputIt, InputIt)
			 * works for EmplaceConstructible, Swappable,
			 * CopyAssignable, CopyConstructible and CopyInsertable
			 * types.
			 */
			c1 temp(1);
			test_support::random_access_it<c1 *> temp_begin(&temp);
			test_support::random_access_it<c1 *> temp_end =
				temp_begin + 1;
			r->v1->insert(r->v1->cbegin(), temp_begin, temp_end);
			UT_ASSERTeq(r->v1->const_at(0).value, 1);
		}

		{
			/*
			 * Test if insert(const_iterator,
			 * std::initializer_list<T>) works for
			 * EmplaceConstructible, Swappable, CopyAssignable,
			 * CopyConstructible and CopyInsertable types.
			 */
			c1 temp(1);
			std::initializer_list<c1> ilist = {temp};
			r->v1->insert(r->v1->cbegin(), ilist);
			UT_ASSERTeq(r->v1->const_at(0).value, 1);
		}

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C1>(r->v1); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}
#endif

#if defined(VECTOR)
void
test_emplace_constructible_moveable_and_assignable(
	nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->v2 = nvobj::make_persistent<C2>(); });

		{
			/*
			 * Test if insert(const_iterator, value_type &&)
			 * works for MoveAssignable and MoveInsertable types.
			 */
			c2 temp(1);
			r->v2->insert(r->v2->cbegin(), std::move(temp));
			UT_ASSERTeq(r->v2->const_at(0).value, 1);
		}

		{
			/*
			 * Test if emplace(const_iterator, Args &&...) works for
			 * MoveAssignable, MoveInsertable and
			 * EmplaceConstructible types.
			 */
			r->v2->emplace(r->v2->cbegin(), 1);
			UT_ASSERTeq(r->v2->const_at(0).value, 1);
		}

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C2>(r->v2); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}
#endif

void
test_emplace_constructible_and_move_insertable(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->v3 = nvobj::make_persistent<C3>(); });

		{
			/*
			 * Test if emplace_back(Args &&...) works for
			 * MoveInsertable and EmplaceConstructible types.
			 */
			r->v3->emplace_back(1);
			UT_ASSERTeq(r->v3->const_at(0).value, 1);
		}

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C3>(r->v3); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

#if defined(VECTOR)
void
test_move_assignable(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->v4 = nvobj::make_persistent<C4>(10U); });

		{
			/*
			 * Test if erase(const_iterator) works for
			 * MoveAssignable types.
			 */
			UT_ASSERTeq(r->v4->size(), 10);
			r->v4->erase(r->v4->cbegin());
			UT_ASSERTeq(r->v4->size(), 9);
		}

		{
			/*
			 * Test if erase(const_iterator, const_iterator) works
			 * for MoveAssignable types.
			 */
			UT_ASSERTeq(r->v4->size(), 9);
			r->v4->erase(r->v4->cbegin(), r->v4->cbegin() + 4);
			UT_ASSERTeq(r->v4->size(), 5);
		}

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C4>(r->v4); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}
#endif

#if defined(VECTOR)
void
test_copy_insertable(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->v5 = nvobj::make_persistent<C5>(); });

		{
			/*
			 * Test if push_back(const value_type &) works for
			 * CopyInsertable types.
			 */
			copy_insertable temp(1);
			r->v5->push_back(temp);
			UT_ASSERTeq(r->v5->const_at(0).value, 1);
		}

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C5>(r->v5); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}
#endif

void
test_move_insertable(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->v6 = nvobj::make_persistent<C6>(); });

		{
			/*
			 * Test if push_back(const value_type &&) works for
			 * MoveInsertable types.
			 */
			move_insertable temp(1);
			r->v6->push_back(std::move(temp));
			UT_ASSERTeq(r->v6->const_at(0).value, 1);
		}

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C6>(r->v6); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
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
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: vector_modifiers_type_requirements",
		PMEMOBJ_MIN_POOL * 2, S_IWUSR | S_IRUSR);

	test_emplace_constructible_and_move_insertable(pop);
#if defined(VECTOR)
	test_copy_assignable_copy_insertable(pop);
	test_emplace_constructible_moveable_and_assignable(pop);
	test_move_assignable(pop);
	test_copy_insertable(pop);
#endif
	test_move_insertable(pop);

	pop.close();

	return 0;
}
