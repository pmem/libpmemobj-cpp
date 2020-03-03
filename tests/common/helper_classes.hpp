// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019, Intel Corporation */

/**
 * Helper classes that represent C++ concepts
 */

#ifndef HELPER_CLASSES_HPP
#define HELPER_CLASSES_HPP

#include "unittest.hpp"

#include <utility>

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
	operator=(const default_constructible_only &) = delete;

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
 * copy_assignable_copy_insertable
 * Instance of that type satisfies requirements of CopyAssignable and
 * CopyInsertable concepts.
 */
template <typename T>
struct copy_assignable_copy_insertable {
	T value;
	int copied = 0;
	int copied_assigned = 0;

	/* emplace ctor is needed to create first object */
	copy_assignable_copy_insertable(const T &val) : value(val){};

	copy_assignable_copy_insertable(
		const copy_assignable_copy_insertable &other)
	    : value(other.value), copied(other.copied + 1){};

	copy_assignable_copy_insertable &
	operator=(const copy_assignable_copy_insertable &other)
	{
		copied = other.copied;
		copied_assigned = other.copied_assigned + 1;
		value = other.value;
		return *this;
	}
};

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
 * emplace_constructible_copy_insertable_and_move_insertable - helper class
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
 *  emplace_constructible_moveable_and_assignable - helper class
 *  Satisfies requirements:
 *  - instance of that type can be constructed in uninitialized storage
 *  - instance of the type can be constructed from an rvalue argument
 *  - instance of the type can be copy-assigned from an lvalue expression
 */
template <typename T>
struct emplace_constructible_moveable_and_assignable {
	T value;
	int moved = 0;
	int assigned = 0;

	emplace_constructible_moveable_and_assignable(T val) : value(val)
	{
	}

	emplace_constructible_moveable_and_assignable(
		emplace_constructible_moveable_and_assignable &&other)
	    : value(std::move(other.value)), moved(other.moved + 1)
	{
	}

	emplace_constructible_moveable_and_assignable &
	operator=(emplace_constructible_moveable_and_assignable &&other)
	{
		moved = other.moved;
		assigned = other.assigned + 1;
		value = std::move(other.value);
		return *this;
	}

	emplace_constructible_moveable_and_assignable &
	operator=(T val)
	{
		value = std::move(val);
		++assigned;
		return *this;
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

/**
 *  move_assignable - helper class
 *  Instance of that type satisfies MoveAssignable concept requirements.
 */
struct move_assignable {
	int value;

	/* emplace ctor is needed to create first object */
	move_assignable(int val = 0) : value(val)
	{
	}

	move_assignable &
	operator=(move_assignable &&other)
	{
		value = other.value;
		other.value = 0;
		return *this;
	}
};

/**
 *  copy_insertable - helper class
 *  Instance of that type satisfies CopyInsertable concept requirements.
 */
struct copy_insertable {
	int value;

	/* emplace ctor is needed to create first object */
	copy_insertable(int val) : value(val)
	{
	}

	copy_insertable(const copy_insertable &other) : value(other.value)
	{
	}
};

/**
 *  move_insertable - helper class
 *  Instance of that type satisfies MoveInsertable concept requirements.
 */
struct move_insertable {
	int value;

	/* emplace ctor is needed to create first object */
	move_insertable(int val) : value(val)
	{
	}

	move_insertable(const copy_insertable &&other) : value(other.value)
	{
	}
};

struct CompoundType {
	int counter = 0;

	/* If counter holds this value it means object was
	 * initialized */
	static constexpr int INITIALIZED = 999999999;

	CompoundType(int c)
	{
		UT_ASSERT(counter != INITIALIZED);
		counter = c;
	}

	CompoundType()
	{
		UT_ASSERT(counter != INITIALIZED);
		counter = INITIALIZED;
	}

	CompoundType(CompoundType &&rhs)
	{
		UT_ASSERT(counter != INITIALIZED);
		counter = rhs.counter;
	}

	CompoundType(const CompoundType &rhs)
	{
		UT_ASSERT(counter != INITIALIZED);
		counter = rhs.counter;
	}

	~CompoundType()
	{
		counter = 0;
	}

	CompoundType &
	operator=(CompoundType &&rhs)
	{
		UT_ASSERT(counter == INITIALIZED);
		counter = rhs.counter;
		return *this;
	}

	CompoundType &
	operator=(const CompoundType &rhs)
	{
		UT_ASSERT(counter == INITIALIZED);
		counter = rhs.counter;
		return *this;
	}

	bool
	operator==(const CompoundType &rhs)
	{
		return counter == rhs.counter;
	}
};

#endif /* HELPER_CLASSES_HPP */
