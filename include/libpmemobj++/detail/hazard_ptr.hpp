// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/**
 * @file
 * Hazard pointer implementation.
 */

#ifndef LIBPMEMOBJ_CPP_HAZARD_POINTER_HPP
#define LIBPMEMOBJ_CPP_HAZARD_POINTER_HPP

#include <atomic>
#include <list>
#include <mutex>

namespace pmem
{
namespace detail
{

template <typename AtomicPointer>
class hazard_ptr_accessor {
public:
	hazard_ptr_accessor(AtomicPointer &target)
	{
		acquire(target);
	}

	~hazard_ptr_accessor()
	{
		holder().record_it->store(nullptr, std::memory_order_release);
	}

	hazard_ptr_accessor &
	operator=(AtomicPointer &target)
	{
		acquire(target);
	}

	hazard_ptr_accessor(const hazard_ptr_accessor &) = delete;
	hazard_ptr_accessor(hazard_ptr_accessor &&) = default;

	const AtomicPointer &
	get()
	{
		return *holder().record_it;
	}

	template <typename F>
	void static foreach (F &&f)
	{
		std::unique_lock<std::mutex> lock(holder().mtx());
		for (auto &e : holder().records())
			f(e);
	}

private:
	/* Acquires a pointers and registers it as a hazard. */
	void
	acquire(AtomicPointer &target)
	{
		/* The loop is necessary to avoid a following case:
		 *
		 * thread1                      |            thread2
		 * auto ptr = target.load();    |
		 *                              | auto old_t = target.load();
		 *                              | target.store(nullptr);
		 *                              | if(!hazards.contains(target))
		 *                              |     delete old_t;
		 * hazards.insert(ptr);         |
		 */
		while (true) {
			auto ptr = target.load(std::memory_order_acquire);
			holder().record_it->store(ptr,
						  std::memory_order_relaxed);
			if (ptr == target.load(std::memory_order_acquire))
				return ptr;
		}
	}

	/* Registers and deregisters hazard pointer record in a global list. */
	struct record_holder {
		record_holder()
		{
			std::unique_lock<std::mutex> lock(mtx());
			records().emplace_front();
			record_it = records.front();
		}

		~record_holder()
		{
			std::unique_lock<std::mutex> lock(mtx());
			records().erase(record_it);
		}

		static std::list<AtomicPointer> &
		records()
		{
			static std::list<AtomicPointer> r;
			return r;
		}

		static std::mutex &
		mtx()
		{
			static std::mutex m;
			return m;
		}

		typename std::list<AtomicPointer>::iterator record_it;
	};

	/* Returns thread-local reference to record holder. Record is valid for
	 * the entire lifetime of a thread. */
	static record_holder &
	holder()
	{
		thread_local record_holder h;
		return h;
	}
};

template <typename AtomicPtr>
class hazard_ptr {
public:
	template <typename... Args>
	hazard_ptr(Args &&... args) : ptr(std::forward<Args>(args)...)
	{
	}

	hazard_ptr_accessor<AtomicPtr>
	access()
	{
		return hazard_ptr_accessor<AtomicPtr>(ptr);
	}

private:
	AtomicPtr ptr;
};

} /* namespace detail */
} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_HAZARD_POINTER_HPP */
