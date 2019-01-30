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

/**
 * Helper classes that represent C++ concepts
 */

#ifndef HELPER_CLASSES_HPP
#define HELPER_CLASSES_HPP

#include "unittest.hpp"

/**
 * default_constructible_only - helper class
 * Instance of that type can be only default constructed
 */

class default_constructible_only {
public:
	static int count;

	default_constructible_only() : _val(1)
	{
		++count;
	}

	~default_constructible_only()
	{
		--count;
	};

	default_constructible_only(const default_constructible_only &) = delete;

	default_constructible_only &
	operator=(default_constructible_only &) = delete;

	bool
	operator==(const default_constructible_only &other) const
	{
		return _val == other._val;
	}

private:
	int _val;
};

int default_constructible_only::count = 0;

/**
 * emplace_constructible - helper class
 * Instance of that type can be constructed in uninitialized storage
 */
template <typename T>
struct emplace_constructible {
	T value;

	emplace_constructible(T val) : value(val)
	{
	}

	emplace_constructible(const emplace_constructible &) = delete;
};

/**
 * emplace_constructible_and_move_insertable - helper class
 * Satisfies requirements:
 * - instance of that type can be constructed in uninitialized storage
 * - rvalue of the type can be copied in uninitialized storage
 */
template <typename T>
struct emplace_constructible_and_move_insertable {
	T value;

	int moved = 0;

	emplace_constructible_and_move_insertable(T val) : value(val)
	{
	}

	emplace_constructible_and_move_insertable(
		emplace_constructible_and_move_insertable &&other)
	    : value(other.value), moved(other.moved + 1)
	{
	}

	/* Since move constructor is user-declared, copy constructor and copy
	 * assignment operator are not implicitly declared by compiler */
};

/**
 * copy_insertable_and_move_insertable - helper class
 * Satisfies requirements:
 * - instance of that type can be copy-constructed in uninitialized storage
 * - rvalue of the type can be copied in uninitialized storage
 */
template <typename T>
struct emplace_constructible_copy_insertable_move_insertable {
	T value;

	int copied = 0;

	int moved = 0;

	emplace_constructible_copy_insertable_move_insertable(T val)
	    : value(val)
	{
	}

	emplace_constructible_copy_insertable_move_insertable(
		const emplace_constructible_copy_insertable_move_insertable
			&other)
	    : value(other.value), copied(other.copied + 1)
	{
	}

	emplace_constructible_copy_insertable_move_insertable(
		emplace_constructible_copy_insertable_move_insertable &&other)
	    : value(other.value), moved(other.moved + 1)
	{
	}
};

/**
 * failing_reference_operator - helper structure
 * Instance of that type cannot use reference operator
 */
struct failing_reference_operator {
	failing_reference_operator() : val(0)
	{
	}

	failing_reference_operator(int i) : val(i)
	{
	}

	~failing_reference_operator()
	{
	}

	failing_reference_operator *operator&() const
	{
		UT_ASSERT(0);
		return nullptr;
	}

	int val;
};

/**
 *  move_only - helper class
 *  Instance of that type can be constructed from an rvalue argument only
 */
struct move_only {
	int value;

	move_only(int val = 1) : value(val)
	{
	}

	move_only(const move_only &) = delete;

	move_only &operator=(const move_only &) = delete;

	move_only &
	operator=(move_only &&other)
	{
		value = other.value;
		other.value = 0;
		return *this;
	}

	move_only(move_only &&other) : value(other.value)
	{
		other.value = 0;
	}

	bool
	operator==(const move_only &other) const
	{
		return value == other.value;
	}
};

#endif /* HELPER_CLASSES_HPP */
