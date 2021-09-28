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
			for (detail::atomic_backoff backoff(true);;) {
				val = ptr.load(order);
				if (!is_dirty(val))
					break;
			}
		}
		return val;
	}

	/**
	 * Write-optimized load flushes the ptr with the dirty flag, clears the
	 * flag using CAS after flush.
	 *
	 * @return the self_relative_ptr (no dirty flag)
	 */
	template <typename OPT = ReadOptimized>
	typename std::enable_if<!std::is_same<OPT, std::true_type>::value,
				value_type>::type
	load(std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto val = ptr.load(order);
		while (is_dirty(val)) {
			pool_by_vptr(this).persist(&ptr, sizeof(ptr));
			auto clean_val = clear_dirty(val);
			if (ptr.compare_exchange_strong(val, clean_val, order))
				return clean_val;
		}
		return val;
	}

	/**
	 * Not commonly used.
	 *
	 * @param[in] desired the self_relative_ptr (no dirty flag) to be
	 * exchanged in.
	 *
	 * @return the old self_relative_ptr (no dirty flag) before exchange.
	 *
	 */
	value_type
	exchange(value_type desired,
		 std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto dirty_desired = mark_dirty(desired);
		auto ret = ptr.exchange(dirty_desired, order);
		pool_by_vptr(this).persist(&ptr, sizeof(ptr));
		ptr.compare_exchange_strong(dirty_desired,
					    clear_dirty(dirty_desired), order);
		return clear_dirty(ret);
	}

	/**
	 * Not commonly used.
	 *
	 * @param[out] expected the self_relative_ptr (no dirty flag) expected
	 * to be stored, return the actual one if CAS failed.
	 * @param[in] desired the self_relative_ptr (no dirty flag) to be
	 * exchanged in.
	 *
	 * @return true when CAS succeeds, false otherwise.
	 *
	 */
	bool
	compare_exchange_weak(value_type &expected, value_type desired,
			      std::memory_order success,
			      std::memory_order failure) noexcept
	{
		bool ret;
		auto dirty_desired = mark_dirty(desired);
		auto vptr = expected.get();
		if ((ret = ptr.compare_exchange_weak(expected, dirty_desired,
						     success, failure))) {
			/**
			 * if the first CAS is a success, we continue to flush
			 * it and use another CAS to clear the dirty flag after
			 * flush, just like what store() does.
			 */
			pool_by_vptr(this).persist(&ptr, sizeof(ptr));
			if ((ret = ptr.compare_exchange_weak(
				     dirty_desired, clear_dirty(dirty_desired),
				     success, failure))) {
				return ret;
			}
			expected = dirty_desired;
			return ret;
		}
		if (clear_dirty(expected).get() == vptr) {
			/**
			 * if the first CAS failed, expected will have the
			 * actual ptr stored in it. We then check if the failure
			 * is due the dirty flag. If so, it should to not be
			 * considered as a failure. So we CAS again with the
			 * actual ptr (should be with dirty flag this time).
			 *
			 */
			if ((ret = ptr.compare_exchange_weak(
				     expected, dirty_desired, success,
				     failure))) {
				/**
				 * If this CAS is success, we continue to flush
				 * the new ptr (with dirty), and then use CAS to
				 * clear the dirty flag.
				 */
				pool_by_vptr(this).persist(&ptr, sizeof(ptr));
				if (!ptr.compare_exchange_strong(
					    dirty_desired,
					    clear_dirty(dirty_desired), success,
					    failure)) {
					/**
					 * If this CAS failed, it means another
					 * thread just changed the ptr right
					 * after we flushed it. So this should
					 * be considered a failure. We store the
					 * new ptr in expected and return
					 * failure.
					 */
					expected = clear_dirty(dirty_desired);
					ret = false;
				}
				return ret;
			}
			/**
			 * When reach here, it means another thread updates the
			 * ptr after we checked that the failure is due to the
			 * dirty flag. So we just return failure and expected
			 * already has the actual ptr stored in it.
			 */
		}
		/**
		 * When reach here, expected has already the actual ptr stored
		 * in it, return as a failed CAS.
		 */
		return false;
	}

	bool
	compare_exchange_weak(
		value_type &expected, value_type desired,
		std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		return ptr.compare_exchange_weak(expected, desired, order);
	}

	bool
	compare_exchange_strong(value_type &expected, value_type desired,
				std::memory_order success,
				std::memory_order failure) noexcept
	{
		return ptr.compare_exchange_strong(expected, desired, success,
						   failure);
	}

	bool
	compare_exchange_strong(
		value_type &expected, value_type desired,
		std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		return ptr.compare_exchange_strong(expected, desired, order);
	}

	value_type
	fetch_add(difference_type val,
		  std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		return ptr.fetch_add(val, order);
	}

	value_type
	fetch_sub(difference_type val,
		  std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		return ptr.fetch_sub(val, order);
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

	value_type
	operator++() noexcept
	{
		try {
			return this->fetch_add(1) + 1;
		} catch (...) {
			/* This should never happen during normal program
			 * execution */
			std::terminate();
		}
	}

	value_type
	operator++(int) noexcept
	{
		return this->fetch_add(1);
	}

	value_type
	operator--() noexcept
	{
		try {
			return this->fetch_sub(1) - 1;
		} catch (...) {
			/* This should never happen during normal program
			 * execution */
			std::terminate();
		}
	}

	value_type
	operator--(int) noexcept
	{
		return this->fetch_sub(1);
	}

	value_type
	operator+=(difference_type diff) noexcept
	{
		try {
			return this->fetch_add(diff) + diff;
		} catch (...) {
			/* This should never happen during normal program
			 * execution */
			std::terminate();
		}
	}

	value_type
	operator-=(difference_type diff) noexcept
	{
		try {
			return this->fetch_sub(diff) - diff;
		} catch (...) {
			/* This should never happen during normal program
			 * execution */
			std::terminate();
		}
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

namespace pmem
{

namespace detail
{

/**
 * pmem::detail::can_do_snapshot atomic specialization for persistent-aware
 * self_relative_ptr. Not thread safe.
 *
 * Use in a single-threaded environment only.
 */
template <typename T, typename ReadOptimized>
struct can_do_snapshot<
	obj::experimental::atomic_persistent_aware_ptr<T, ReadOptimized>> {
	using snapshot_type =
		std::atomic<obj::experimental::self_relative_ptr<T>>;
	static constexpr bool value =
		sizeof(obj::experimental::atomic_persistent_aware_ptr<
			T, ReadOptimized>) == sizeof(snapshot_type);
	static_assert(value,
		      "atomic_persistent_aware_ptr should be the same size");
};

} /* namespace detail */

} /* namespace pmem */

#endif // LIBPMEMOBJ_CPP_ATOMIC_PERSISTENT_AWARE_PTR_HPP
