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
#ifndef THROWING_ITERATOR_HPP
#define THROWING_ITERATOR_HPP

#include <iterator>

template <typename T>
struct throwing_it {
  typedef std::bidirectional_iterator_tag iterator_category;
  typedef std::ptrdiff_t difference_type;
  typedef const T value_type;
  typedef const T *pointer;
  typedef const T &reference;

  enum throwing_action {
	TAIncrement,
	TADecrement,
	TADereference,
	TAAssignment,
	TAComparison
  };

  //  Constructors
  throwing_it()
	  : begin_(nullptr),
		end_(nullptr),
		current_(nullptr),
		action_(TADereference),
		index_(0)
  {
  }
  throwing_it(const T *first, const T *last, size_t index = 0,
				   throwing_action action = TADereference)
	  : begin_(first),
		end_(last),
		current_(first),
		action_(action),
		index_(index)
  {
  }
  throwing_it(const throwing_it &rhs)
	  : begin_(rhs.begin_),
		end_(rhs.end_),
		current_(rhs.current_),
		action_(rhs.action_),
		index_(rhs.index_)
  {
  }
  throwing_it &
  operator=(const throwing_it &rhs)
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

  throwing_it &
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

  throwing_it
  operator++(int)
  {
	throwing_it temp = *this;
	++(*this);
	return temp;
  }

  throwing_it &
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

  throwing_it
  operator--(int)
  {
	throwing_it temp = *this;
	--(*this);
	return temp;
  }

  bool
  operator==(const throwing_it &rhs) const
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
  operator!=(const throwing_it &rhs) const
  {
	return !(*this == rhs);
  }

 private:
  const T *begin_;
  const T *end_;
  const T *current_;
  throwing_action action_;
  mutable size_t index_;
};

#endif /* THROWING_ITERATOR_HPP */
