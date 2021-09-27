// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/**
 * @file
 * Atomic specialization for persistent-aware self_relative_ptr.
 */

#ifndef LIBPMEMOBJ_CPP_ATOMIC_PERSISTENT_AWARE_PTR_HPP
#define LIBPMEMOBJ_CPP_ATOMIC_PERSISTENT_AWARE_PTR_HPP

#include <libpmemobj++/detail/atomic_backoff.hpp>
#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/self_relative_ptr_base_impl.hpp>
#include <libpmemobj++/experimental/atomic_self_relative_ptr.hpp>
#include <libpmemobj++/experimental/self_relative_ptr.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/utils.hpp>

#include <atomic>

namespace pmem
{
namespace obj
{
namespace experimental
{

/**
 * Atomic specialization of a persistent ptr (self_relative_ptr) that manages
 * its persistence by itself.
 *
 * In a multi-threaded scenario, the persistence of this ptr is guaranteed when
 * it is visible to (or read by) other threads. Performance-wise, two different
 * options are provided: Read-optimized and Write-optimized. See corresponding
 * store/load functions for details.
 */
template <typename T, typename ReadOptimized>
struct atomic_persistent_aware_ptr {
private:
	using ptr_type = pmem::detail::self_relative_ptr_base_impl<
		std::atomic<std::ptrdiff_t>>;
	using accessor = pmem::detail::self_relative_accessor<
		std::atomic<std::ptrdiff_t>>;

	static constexpr uintptr_t IS_DIRTY = 1;

public:
	using this_type = atomic_persistent_aware_ptr;
	using value_type = pmem::obj::experimental::self_relative_ptr<T>;
	using difference_type = typename value_type::difference_type;

	constexpr atomic_persistent_aware_ptr() noexcept = default;

	/**
	 * Constructors
	 */
	atomic_persistent_aware_ptr(value_type value) : ptr()
	{
		store(value);
	}
	atomic_persistent_aware_ptr(const atomic_persistent_aware_ptr &) =
		delete;

	/**
	 * Read-optimized store does the flush in store function, and clear the
	 * dirty marker after flush.
	 *
	 * @param[in] desired the self_relative_ptr (no dirty flag) to be stored
	 *
	 */
	template <typename OPT = ReadOptimized>
	typename std::enable_if<std::is_same<OPT, std::true_type>::value>::type
	store(value_type desired,
	      std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto dirty_desired = mark_dirty(desired);
		ptr.store(dirty_desired, order);
		pool_by_vptr(this).persist(&ptr, sizeof(ptr));
		ptr.compare_exchange_strong(dirty_desired, clear_dirty(desired),
					    order);
	}

	/**
	 * Write-optimized store updates the ptr with the dirty flag, relies on
	 * consequent load to do the flush.
	 *
	 * @param[in] desired the self_relative_ptr (no dirty flag) to be stored
	 *
	 */
	template <typename OPT = ReadOptimized>
	typename std::enable_if<!std::is_same<OPT, std::true_type>::value>::type
	store(value_type desired,
	      std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		ptr.store(mark_dirty(desired), order);
	}

	/**
	 * Read-optimized load retries upon dirty ptr, relies on the store
	 * function to clear the dirty before continue.
	 * But for correctness, just flush the dirty ptr and return the clear
	 * ptr for now.
	 *
	 * @return the self_relative_ptr (no dirty flag)
	 */
	template <typename OPT = ReadOptimized>
	typename std::enable_if<std::is_same<OPT, std::true_type>::value,
				value_type>::type
	load(std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto val = ptr.load(order);
		if (is_dirty(val)) {
			pool_by_vptr(this).persist(&ptr, sizeof(ptr));
		}
		return clear_dirty(val);
	}

	/**
	 * Write-optimized load flushes the ptr with the dirty flag, clears the
	 * flag using CAS after flush.
	 * If CAS failed, simply return the old clear ptr, rely on later load to
	 * clear the dirty flag.
	 *
	 * @return the self_relative_ptr (no dirty flag)
	 */
	template <typename OPT = ReadOptimized>
	typename std::enable_if<!std::is_same<OPT, std::true_type>::value,
				value_type>::type
	load(std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto val = ptr.load(order);
		if (is_dirty(val)) {
			pool_by_vptr(this).persist(&ptr, sizeof(ptr));
			auto clear_val = clear_dirty(val);
			ptr.compare_exchange_strong(val, clear_val, order);
			return clear_val;
		}
		return clear_dirty(val);
	}

	bool
	is_lock_free() const noexcept
	{
		return ptr.is_lock_free();
	}

	/*
	 * Operators
	 */
	operator value_type() const noexcept
	{
		return load();
	}

	value_type
	operator=(value_type desired) noexcept
	{
		store(desired);
		return desired;
	}

private:
	value_type
	mark_dirty(value_type ptr) const
	{
		auto dirty_ptr =
			reinterpret_cast<uintptr_t>(ptr.get()) | IS_DIRTY;
		return value_type{reinterpret_cast<T *>(dirty_ptr)};
	}

	value_type
	clear_dirty(value_type ptr) const
	{
		auto clear_ptr =
			reinterpret_cast<uintptr_t>(ptr.get()) & ~IS_DIRTY;
		return value_type{reinterpret_cast<T *>(clear_ptr)};
	}

	bool
	is_dirty(value_type ptr) const
	{
		return reinterpret_cast<uintptr_t>(ptr.get()) & IS_DIRTY;
	}

	std::atomic<self_relative_ptr<T>> ptr;
};
} // namespace experimental
} // namespace obj
} // namespace pmem

#endif // LIBPMEMOBJ_CPP_ATOMIC_PERSISTENT_AWARE_PTR_HPP
