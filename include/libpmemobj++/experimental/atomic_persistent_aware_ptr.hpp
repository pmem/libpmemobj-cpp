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
 * versions of this struct are provided:
 *  - Read-optimized - data is flushed along the write operation. If more reads
 *		are expected it's probably better to use this scenario.
 *  - Write-optimized - data is lazily flushed with a read operation. In this
 *		approach data storing is expected to be faster, but data is
 *guranteed to be flushed only after consequent read.
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

	/**
	 * Store constructor.
	 *
	 * @param value to be stored in the atomic_persistent_aware_ptr.
	 */
	atomic_persistent_aware_ptr(value_type value) : ptr()
	{
		store(value);
	}

	/**
	 * Deleted copy constructor.
	 */
	atomic_persistent_aware_ptr(const atomic_persistent_aware_ptr &) =
		delete;

	/**
	 * Read-optimized store does the flush already in the store function.
	 *
	 * @param[in] desired the self_relative_ptr to be stored.
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
		/* Flushing is not necessary for correctness, it's enough that
		 * dirty_desired is persistent */
#if LIBPMEMOBJ_CPP_VG_PMEMCHECK_ENABLED
		VALGRIND_PMC_DO_FLUSH(&ptr, sizeof(ptr));
#endif
	}

	/**
	 * Write-optimized store relies on a consequent load to do the flush.
	 *
	 * @param[in] desired the self_relative_ptr to be stored.
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
	 * Read-optimized load. It relies on a store function to flush the data.
	 *
	 * @return the value_type.
	 */
	template <typename OPT = ReadOptimized>
	typename std::enable_if<std::is_same<OPT, std::true_type>::value,
				value_type>::type
	load(std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		/* This load relies on the store function to clear the dirty
		 * flag. For correctness though, it flushes the dirty ptr and
		 * returns the clear ptr for now. */
		auto val = ptr.load(order);
		if (is_dirty(val)) {
			pool_by_vptr(this).persist(&ptr, sizeof(ptr));
		}
		return clear_dirty(val);
	}

	/**
	 * Write-optimized load flushes the data.
	 *
	 * @return the value_type.
	 */
	template <typename OPT = ReadOptimized>
	typename std::enable_if<!std::is_same<OPT, std::true_type>::value,
				value_type>::type
	load(std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		/* It flushes the ptr with the dirty flag - clears the flag
		 * using CAS after flush. If CAS failed it simply returns the
		 * old clear ptr and relies on a later load to clear the dirty
		 * flag. */
		auto val = ptr.load(order);
		if (is_dirty(val)) {
			pool_by_vptr(this).persist(&ptr, sizeof(ptr));
			auto clear_val = clear_dirty(val);
			ptr.compare_exchange_strong(val, clear_val, order);
#if LIBPMEMOBJ_CPP_VG_PMEMCHECK_ENABLED
			VALGRIND_PMC_DO_FLUSH(&ptr, sizeof(ptr));
#endif
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

	/**
	 * Returns the value of the atomic_persistent_aware_ptr.
	 */
	operator value_type() const noexcept
	{
		return load();
	}

	/**
	 * Assignment operator.
	 *
	 * @param desired value to be stored in the atomic_persistent_aware_ptr.
	 * @return assigned value.
	 */
	value_type
	operator=(value_type desired) noexcept
	{
		store(desired);
		return desired;
	}

	/**
	 * Deleted assignment operator.
	 */
	atomic_persistent_aware_ptr &
	operator=(const atomic_persistent_aware_ptr &) = delete;

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
