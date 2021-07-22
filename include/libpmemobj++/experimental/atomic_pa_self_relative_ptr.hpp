// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_ATOMIC_SELF_RELATIVE_PTR_HPP
#define LIBPMEMOBJ_CPP_ATOMIC_SELF_RELATIVE_PTR_HPP

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/self_relative_ptr_base_impl.hpp>
#include <libpmemobj++/experimental/pa_self_relative_ptr.hpp>
#include <libpmemobj++/transaction.hpp>

#include <atomic>

namespace std
{
/**
 * Atomic specialization for pa_self_relative_ptr
 *
 * Doesn't automatically add itself to the transaction.
 * The user is responsible for persisting the data.
 */
template <typename T>
struct atomic<pmem::obj::experimental::pa_self_relative_ptr<T>> {
private:
	using ptr_type = pmem::detail::self_relative_ptr_base_impl<
		std::atomic<std::ptrdiff_t>>;
	using accessor = pmem::detail::self_relative_accessor<
		std::atomic<std::ptrdiff_t>>;

public:
	using this_type = atomic;
	using value_type = pmem::obj::experimental::pa_self_relative_ptr<T>;
	using difference_type = typename value_type::difference_type;

	/*
	 * Constructors
	 */

	constexpr atomic() noexcept = default;
	atomic(value_type value) : ptr()
	{
		store(value);
	}
	atomic(const atomic &) = delete;

	void
	store(value_type desired,
	      std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto offset = accessor::pointer_to_offset(ptr, desired.get());
		LIBPMEMOBJ_CPP_ANNOTATE_HAPPENS_BEFORE(order, &ptr);
		accessor::get_offset(ptr).store((offset & desired.flush_set_mask()), order);
	}

	value_type
	load(std::memory_order order = std::memory_order_seq_cst) const noexcept
	{
		auto offset = accessor::get_offset(ptr).load(order);
		LIBPMEMOBJ_CPP_ANNOTATE_HAPPENS_AFTER(order, &ptr);
		auto pointer = accessor::offset_to_pointer<T>(offset | ~(value_type::flush_set_mask(offset)), ptr);
		return value_type{pointer, value_type::flush_needed(offset)};
	}

	value_type
	exchange(value_type desired,
		 std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto new_offset =
			accessor::pointer_to_offset(ptr, desired.get());
		auto old_offset =
			accessor::get_offset(ptr).exchange(new_offset & desired.flush_set_mask(), order);
		return value_type{
			accessor::offset_to_pointer<T>(old_offset | ~(value_type::flush_set_mask(old_offset)), ptr), value_type::flush_needed(old_offset)};
	}

	bool
	compare_exchange_weak(value_type &expected,
			      value_type desired,
			      std::memory_order success,
			      std::memory_order failure) noexcept
	{
		auto expected_offset =
			accessor::pointer_to_offset(ptr, expected.get());
		auto expected_actual = expected_offset & expected.flush_set_mask();
		auto desired_offset =
			accessor::pointer_to_offset(ptr, desired.get());
		auto desired_actual = desired_offset & desired.flush_set_mask();
		bool result = accessor::get_offset(ptr).compare_exchange_weak(
			expected_actual, desired_actual, success, failure);
		if (!result) {
			expected = value_type{accessor::offset_to_pointer<T>(
				expected_actual | ~(value_type::flush_set_mask(expected_actual)), ptr),
					      value_type::flush_needed(expected_actual)};
		}
		return result;
	}

	bool
	compare_exchange_weak(
		value_type &expected, value_type desired,
		std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto expected_offset =
			accessor::pointer_to_offset(ptr, expected.get());
		auto expected_actual = expected_offset & expected.flush_set_mask();
		auto desired_offset =
			accessor::pointer_to_offset(ptr, desired.get());
		auto desired_actual = desired_offset & desired.flush_set_mask();
		bool result = accessor::get_offset(ptr).compare_exchange_weak(
			expected_actual, desired_actual, order);
		if (!result) {
			expected = value_type{accessor::offset_to_pointer<T>(
				expected_actual | ~(value_type::flush_set_mask(expected_actual)), ptr),
					      value_type::flush_needed(expected_actual)};
		}
		return result;
	}

	bool
	compare_exchange_strong(value_type &expected, value_type desired,
				std::memory_order success,
				std::memory_order failure) noexcept
	{
		auto expected_offset =
			accessor::pointer_to_offset(ptr, expected.get());
		auto expected_actual = expected_offset & expected.flush_set_mask();
		auto desired_offset =
			accessor::pointer_to_offset(ptr, desired.get());
		auto desired_actual = desired_offset & desired.flush_set_mask();
		bool result = accessor::get_offset(ptr).compare_exchange_strong(
			expected_actual, desired_actual, success, failure);
		if (!result) {
			expected = value_type{accessor::offset_to_pointer<T>(
				expected_actual | ~(value_type::flush_set_mask(expected_actual)), ptr),
					      value_type::flush_needed(expected_actual)};
		}
		return result;
	}

	bool
	compare_exchange_strong(
		value_type &expected, value_type desired,
		std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto expected_offset =
			accessor::pointer_to_offset(ptr, expected.get());
		auto expected_actual = expected_offset & expected.flush_set_mask();
		auto desired_offset =
			accessor::pointer_to_offset(ptr, desired.get());
		auto desired_actual = desired_offset & desired.flush_set_mask();
		bool result = accessor::get_offset(ptr).compare_exchange_strong(
			expected_actual, desired_actual, order);
		if (!result) {
			expected = value_type{accessor::offset_to_pointer<T>(
				expected_actual | ~(value_type::flush_set_mask(expected_actual)), ptr),
					      value_type::flush_needed(expected_actual)};
		}
		return result;
	}

	value_type
	fetch_add(difference_type val,
		  std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto offset = accessor::get_offset(ptr).fetch_add(
			val * static_cast<difference_type>(sizeof(T)), order);
		return value_type{accessor::offset_to_pointer<T>(offset | ~(value_type::flush_set_mask(offset)), ptr), value_type::flush_needed(offset)};
	}

	value_type
	fetch_sub(difference_type val,
		  std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto offset = accessor::get_offset(ptr).fetch_sub(
			val * static_cast<difference_type>(sizeof(T)), order);
		return value_type{accessor::offset_to_pointer<T>(offset | ~(value_type::flush_set_mask(offset)), ptr), value_type::flush_needed(offset)};
	}

	bool
	is_lock_free() const noexcept
	{
		return accessor::get_offset(ptr).is_lock_free();
	}

	/*
	 * Operators
	 */

	operator value_type() const noexcept
	{
		return load();
	}

	atomic &operator=(const atomic &) = delete;
	atomic &operator=(const atomic &) volatile = delete;

	value_type
	operator=(value_type desired) noexcept
	{
		store(desired);
		return desired;
	}

	value_type
	operator++() noexcept
	{
		return this->fetch_add(1) + 1;
	}

	value_type
	operator++(int) noexcept
	{
		return this->fetch_add(1);
	}

	value_type
	operator--() noexcept
	{
		return this->fetch_sub(1) - 1;
	}

	value_type
	operator--(int) noexcept
	{
		return this->fetch_sub(1);
	}

	value_type
	operator+=(difference_type diff) noexcept
	{
		return this->fetch_add(diff) + diff;
	}

	value_type
	operator-=(difference_type diff) noexcept
	{
		return this->fetch_sub(diff) - diff;
	}

private:
	ptr_type ptr;
};

} /* namespace std */

namespace pmem
{

namespace detail
{

/**
 * can_do_snapshot atomic specialization for pa_self_relative_ptr. Not thread safe.
 *
 * Use in a single threaded environment only.
 */
template <typename T>
struct can_do_snapshot<std::atomic<obj::experimental::pa_self_relative_ptr<T>>> {
	using snapshot_type = obj::experimental::pa_self_relative_ptr<T>;
	static constexpr bool value = sizeof(std::atomic<snapshot_type>) ==
				      sizeof(typename snapshot_type::offset_type);
	static_assert(value,
		      "std::atomic<pa_self_relative_ptr> should be the same size");
};

} /* namespace detail */

} /* namespace pmem */

#endif
