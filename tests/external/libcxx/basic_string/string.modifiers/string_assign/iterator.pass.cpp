//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/experimental/string.hpp>

namespace nvobj = pmem::obj;
namespace pmem_exp = pmem::obj::experimental;
using S = pmem_exp::string;

template <typename T>
struct ThrowingIterator {
	typedef std::bidirectional_iterator_tag iterator_category;
	typedef ptrdiff_t difference_type;
	typedef const T value_type;
	typedef const T *pointer;
	typedef const T &reference;

	enum ThrowingAction {
		TAIncrement,
		TADecrement,
		TADereference,
		TAAssignment,
		TAComparison
	};

	//  Constructors
	ThrowingIterator()
	    : begin_(nullptr),
	      end_(nullptr),
	      current_(nullptr),
	      action_(TADereference),
	      index_(0)
	{
	}
	ThrowingIterator(const T *first, const T *last, size_t index = 0,
			 ThrowingAction action = TADereference)
	    : begin_(first),
	      end_(last),
	      current_(first),
	      action_(action),
	      index_(index)
	{
	}
	ThrowingIterator(const ThrowingIterator &rhs)
	    : begin_(rhs.begin_),
	      end_(rhs.end_),
	      current_(rhs.current_),
	      action_(rhs.action_),
	      index_(rhs.index_)
	{
	}
	ThrowingIterator &
	operator=(const ThrowingIterator &rhs)
	{
		if (action_ == TAAssignment) {
			if (index_ == 0)
				throw std::runtime_error(
					"throw from iterator assignment");
			else
				--index_;
		}
		begin_ = rhs.begin_;
		end_ = rhs.end_;
		current_ = rhs.current_;
		action_ = rhs.action_;
		index_ = rhs.index_;
		return *this;
	}

	//  iterator operations
	reference operator*() const
	{
		if (action_ == TADereference) {
			if (index_ == 0)
				throw std::runtime_error(
					"throw from iterator dereference");
			else
				--index_;
		}
		return *current_;
	}

	ThrowingIterator &
	operator++()
	{
		if (action_ == TAIncrement) {
			if (index_ == 0)
				throw std::runtime_error(
					"throw from iterator increment");
			else
				--index_;
		}
		++current_;
		return *this;
	}

	ThrowingIterator
	operator++(int)
	{
		ThrowingIterator temp = *this;
		++(*this);
		return temp;
	}

	ThrowingIterator &
	operator--()
	{
		if (action_ == TADecrement) {
			if (index_ == 0)
				throw std::runtime_error(
					"throw from iterator decrement");
			else
				--index_;
		}
		--current_;
		return *this;
	}

	ThrowingIterator
	operator--(int)
	{
		ThrowingIterator temp = *this;
		--(*this);
		return temp;
	}

	bool
	operator==(const ThrowingIterator &rhs) const
	{
		if (action_ == TAComparison) {
			if (index_ == 0)
				throw std::runtime_error(
					"throw from iterator comparison");
			else
				--index_;
		}
		bool atEndL = current_ == end_;
		bool atEndR = rhs.current_ == rhs.end_;
		if (atEndL != atEndR)
			return false; // one is at the end (or empty), the other
				      // is not.
		if (atEndL)
			return true; // both are at the end (or empty)
		return current_ == rhs.current_;
	}

	bool
	operator!=(const ThrowingIterator &rhs) const
	{
		return !(*this == rhs);
	}

private:
	const T *begin_;
	const T *end_;
	const T *current_;
	ThrowingAction action_;
	mutable size_t index_;
};

struct root {
	nvobj::persistent_ptr<S> s, s_short, s_long;
	nvobj::persistent_ptr<S> s_arr[8];
	nvobj::persistent_ptr<S> aCopy;
};

template <class S, class It>
void
test(nvobj::pool<struct root> &pop, const S &s1, It first, It last,
     const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<S>(s1); });

	auto &s = *r->s;

	s.assign(first, last);

	UT_ASSERT(s == expected);

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<S>(r->s); });
}

template <class S, class It>
void
test_exceptions(nvobj::pool<struct root> &pop, const S &s1, It first, It last)
{
	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->s = nvobj::make_persistent<S>(s1);
		r->aCopy = nvobj::make_persistent<S>(*r->s);
	});

	try {
		r->s->assign(first, last);
		UT_ASSERT(false);
	} catch (std::runtime_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(*r->s == *r->aCopy);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<S>(r->s);
		nvobj::delete_persistent<S>(r->aCopy);
	});
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
		path, "string_test", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();
	{
		auto &s_arr = r->s_arr;
		const char *s =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
		try {
			nvobj::transaction::run(pop, [&] {
				s_arr[0] = nvobj::make_persistent<S>();
				s_arr[1] = nvobj::make_persistent<S>("A");
				s_arr[2] =
					nvobj::make_persistent<S>("ABCDEFGHIJ");
				s_arr[3] = nvobj::make_persistent<S>(
					"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
				s_arr[4] = nvobj::make_persistent<S>("12345");
				s_arr[5] =
					nvobj::make_persistent<S>("1234567890");
				s_arr[6] = nvobj::make_persistent<S>(
					"12345678901234567890");
				s_arr[7] = nvobj::make_persistent<S>(
					"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
			});

			test(pop, *s_arr[0], s, s, *s_arr[0]);
			test(pop, *s_arr[0], s, s + 1, *s_arr[1]);
			test(pop, *s_arr[0], s, s + 10, *s_arr[2]);
			test(pop, *s_arr[0], s, s + 52, *s_arr[3]);
			test(pop, *s_arr[0], s, s + 78, *s_arr[7]);

			test(pop, *s_arr[4], s, s, *s_arr[0]);
			test(pop, *s_arr[4], s, s + 1, *s_arr[1]);
			test(pop, *s_arr[4], s, s + 10, *s_arr[2]);
			test(pop, *s_arr[4], s, s + 52, *s_arr[3]);
			test(pop, *s_arr[4], s, s + 78, *s_arr[7]);

			test(pop, *s_arr[5], s, s, *s_arr[0]);
			test(pop, *s_arr[5], s, s + 1, *s_arr[1]);
			test(pop, *s_arr[5], s, s + 10, *s_arr[2]);
			test(pop, *s_arr[5], s, s + 52, *s_arr[3]);
			test(pop, *s_arr[5], s, s + 78, *s_arr[7]);

			test(pop, *s_arr[6], s, s, *s_arr[0]);
			test(pop, *s_arr[6], s, s + 1, *s_arr[1]);
			test(pop, *s_arr[6], s, s + 10, *s_arr[2]);
			test(pop, *s_arr[6], s, s + 52, *s_arr[3]);
			test(pop, *s_arr[6], s, s + 78, *s_arr[7]);

			using It = test_support::input_it<const char *>;

			test(pop, *s_arr[0], It(s), It(s), *s_arr[0]);
			test(pop, *s_arr[0], It(s), It(s + 1), *s_arr[1]);
			test(pop, *s_arr[0], It(s), It(s + 10), *s_arr[2]);
			test(pop, *s_arr[0], It(s), It(s + 52), *s_arr[3]);
			test(pop, *s_arr[0], It(s), It(s + 78), *s_arr[7]);

			test(pop, *s_arr[4], It(s), It(s), *s_arr[0]);
			test(pop, *s_arr[4], It(s), It(s + 1), *s_arr[1]);
			test(pop, *s_arr[4], It(s), It(s + 10), *s_arr[2]);
			test(pop, *s_arr[4], It(s), It(s + 52), *s_arr[3]);
			test(pop, *s_arr[4], It(s), It(s + 78), *s_arr[7]);

			test(pop, *s_arr[5], It(s), It(s), *s_arr[0]);
			test(pop, *s_arr[5], It(s), It(s + 1), *s_arr[1]);
			test(pop, *s_arr[5], It(s), It(s + 10), *s_arr[2]);
			test(pop, *s_arr[5], It(s), It(s + 52), *s_arr[3]);
			test(pop, *s_arr[5], It(s), It(s + 78), *s_arr[7]);

			test(pop, *s_arr[6], It(s), It(s), *s_arr[0]);
			test(pop, *s_arr[6], It(s), It(s + 1), *s_arr[1]);
			test(pop, *s_arr[6], It(s), It(s + 10), *s_arr[2]);
			test(pop, *s_arr[6], It(s), It(s + 52), *s_arr[3]);
			test(pop, *s_arr[6], It(s), It(s + 78), *s_arr[7]);

			nvobj::transaction::run(pop, [&] {
				for (unsigned i = 0; i < 8; ++i) {
					nvobj::delete_persistent<S>(s_arr[i]);
				}
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}
	{ // test assigning to self
		try {
			nvobj::transaction::run(pop, [&] {
				r->s_short = nvobj::make_persistent<S>("123/");
				r->s_long = nvobj::make_persistent<S>(
					"Lorem ipsum dolor sit amet, consectetur/");
			});

			auto &s_short = *r->s_short;
			auto &s_long = *r->s_long;

			s_short.assign(s_short.begin(), s_short.end());
			UT_ASSERT(s_short == "123/");
			s_short.assign(s_short.begin() + 2, s_short.end());
			UT_ASSERT(s_short == "3/");

			s_long.assign(s_long.begin(), s_long.end());
			UT_ASSERT(s_long ==
				  "Lorem ipsum dolor sit amet, consectetur/");

			s_long.assign(s_long.begin() + 30, s_long.end());
			UT_ASSERT(s_long == "nsectetur/");

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<S>(r->s_short);
				nvobj::delete_persistent<S>(r->s_long);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}
	{ // test assigning a different type
		const int8_t p[] = "ABCD";
		try {
			nvobj::transaction::run(pop, [&] {
				r->s = nvobj::make_persistent<S>();
			});

			auto &s = *r->s;

			s.assign(p, p + 4);
			UT_ASSERT(s == "ABCD");

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<S>(r->s);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}
	{
		typedef ThrowingIterator<char> TIter;
		typedef test_support::forward_it<TIter> IIter;
		const char *s =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

		nvobj::transaction::run(pop, [&] {
			r->s_arr[0] = nvobj::make_persistent<S>();
		});

		test_exceptions(pop, *r->s_arr[0],
				IIter(TIter(s, s + 10, 4, TIter::TAIncrement)),
				IIter());
		test_exceptions(
			pop, *r->s_arr[0],
			IIter(TIter(s, s + 10, 5, TIter::TADereference)),
			IIter());
		test_exceptions(pop, *r->s_arr[0],
				IIter(TIter(s, s + 10, 6, TIter::TAComparison)),
				IIter());

		test_exceptions(pop, *r->s_arr[0],
				TIter(s, s + 10, 4, TIter::TAIncrement),
				TIter());
		test_exceptions(pop, *r->s_arr[0],
				TIter(s, s + 10, 5, TIter::TADereference),
				TIter());
		test_exceptions(pop, *r->s_arr[0],
				TIter(s, s + 10, 6, TIter::TAComparison),
				TIter());

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<S>(r->s_arr[0]); });
	}

	pop.close();

	return 0;
}
