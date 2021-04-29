// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_TAGGED_PTR
#define LIBPMEMOBJ_CPP_TAGGED_PTR

#include <cassert>

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/experimental/atomic_self_relative_ptr.hpp>
#include <libpmemobj++/experimental/self_relative_ptr.hpp>
#include <libpmemobj++/persistent_ptr.hpp>

namespace pmem
{
namespace detail
{

template <typename P1, typename P2, typename PointerType>
struct tagged_ptr_impl {
	tagged_ptr_impl() = default;
	tagged_ptr_impl(const tagged_ptr_impl &rhs) = default;

	tagged_ptr_impl(std::nullptr_t) : ptr(nullptr)
	{
		assert(!(bool)*this);
	}

	tagged_ptr_impl(const PointerType &ptr) : ptr(ptr)
	{
	}

	tagged_ptr_impl(const obj::persistent_ptr<P1> &ptr)
	    : ptr(add_tag(ptr.get()))
	{
		assert(get<P1>() == ptr.get());
	}

	tagged_ptr_impl(const obj::persistent_ptr<P2> &ptr) : ptr(ptr.get())
	{
		assert(get<P2>() == ptr.get());
	}

	tagged_ptr_impl &operator=(const tagged_ptr_impl &rhs) = default;

	tagged_ptr_impl &operator=(std::nullptr_t)
	{
		ptr = nullptr;
		assert(!(bool)*this);

		return *this;
	}
	tagged_ptr_impl &
	operator=(const obj::persistent_ptr<P1> &rhs)
	{
		ptr = add_tag(rhs.get());
		assert(get<P1>() == rhs.get());

		return *this;
	}
	tagged_ptr_impl &
	operator=(const obj::persistent_ptr<P2> &rhs)
	{
		ptr = rhs.get();
		assert(get<P2>() == rhs.get());

		return *this;
	}

	bool
	operator==(const tagged_ptr_impl &rhs) const
	{
		return ptr.to_byte_pointer() == rhs.ptr.to_byte_pointer();
	}
	bool
	operator!=(const tagged_ptr_impl &rhs) const
	{
		return !(*this == rhs);
	}

	bool
	operator==(const P1 *rhs) const
	{
		return is_tagged() && get<P1>() == rhs;
	}

	bool
	operator!=(const P2 *rhs) const
	{
		return !(*this == rhs);
	}

	void
	swap(tagged_ptr_impl &rhs)
	{
		ptr.swap(rhs.ptr);
	}

	template <typename T>
	typename std::enable_if<std::is_same<T, P1>::value, bool>::type
	is() const
	{
		return is_tagged();
	}

	template <typename T>
	typename std::enable_if<!std::is_same<T, P1>::value, bool>::type
	is() const
	{
		return !is_tagged();
	}

	template <typename T>
	typename std::enable_if<std::is_same<T, P1>::value, T *>::type
	get() const
	{
		assert(is_tagged());
		return static_cast<P1 *>(remove_tag(ptr.to_void_pointer()));
	}

	template <typename T>
	typename std::enable_if<!std::is_same<T, P1>::value, T *>::type
	get() const
	{
		assert(!is_tagged());
		return static_cast<P2 *>(ptr.to_void_pointer());
	}

	P2 *operator->() const
	{
		return get<P2>();
	}

	explicit operator bool() const noexcept
	{
		return remove_tag(ptr.to_void_pointer()) != nullptr;
	}

private:
	static constexpr uintptr_t IS_TAGGED = 1;
	void *
	add_tag(P1 *ptr) const
	{
		auto tagged =
			reinterpret_cast<uintptr_t>(ptr) | uintptr_t(IS_TAGGED);
		return reinterpret_cast<P1 *>(tagged);
	}

	void *
	remove_tag(void *ptr) const
	{
		auto untagged = reinterpret_cast<uintptr_t>(ptr) &
			~uintptr_t(IS_TAGGED);
		return reinterpret_cast<void *>(untagged);
	}

	bool
	is_tagged() const
	{
		auto value = reinterpret_cast<uintptr_t>(ptr.to_void_pointer());
		return value & uintptr_t(IS_TAGGED);
	}

	PointerType ptr;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
	friend std::atomic<tagged_ptr_impl<
		P1, P2, obj::experimental::self_relative_ptr<void>>>;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
};

template <typename P1, typename P2>
using tagged_ptr =
	tagged_ptr_impl<P1, P2, obj::experimental::self_relative_ptr<void>>;

} /* namespace detail */
} /* namespace pmem */

namespace std
{

template <typename P1, typename P2>
struct atomic<pmem::detail::tagged_ptr<P1, P2>> {
private:
	using ptr_type = pmem::detail::tagged_ptr_impl<
		P1, P2,
		atomic<pmem::obj::experimental::self_relative_ptr<void>>>;
	using value_type = pmem::detail::tagged_ptr<P1, P2>;

public:
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
		LIBPMEMOBJ_CPP_ANNOTATE_HAPPENS_BEFORE(order, &ptr.ptr);
		ptr.ptr.store(desired.ptr, order);
	}

	void
	store_with_snapshot(value_type desired,
			    std::memory_order order = std::memory_order_seq_cst)
	{
		LIBPMEMOBJ_CPP_ANNOTATE_HAPPENS_BEFORE(order, &ptr.ptr);
		pmem::obj::transaction::snapshot(&ptr.ptr);
		ptr.ptr.store(desired.ptr, order);
	}

	void
	store_with_snapshot_release(value_type desired)
	{
		store_with_snapshot(desired, std::memory_order_release);
	}

	value_type
	load(std::memory_order order = std::memory_order_seq_cst) const noexcept
	{
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
		VALGRIND_HG_DISABLE_CHECKING(&ptr.ptr, sizeof(ptr.ptr));
#endif
		auto ret = this->ptr.ptr.load(order);
		LIBPMEMOBJ_CPP_ANNOTATE_HAPPENS_AFTER(order, &ptr.ptr);
		return value_type(ret);
	}

	value_type
	load_acquire() const noexcept
	{
		return load(std::memory_order_acquire);
	}

	void
	swap(atomic<pmem::detail::tagged_ptr<P1, P2>> &rhs)
	{
		auto tmp = rhs.load();
		rhs.store_with_snapshot(this->load());
		this->store_with_snapshot(tmp);
	}

private:
	ptr_type ptr;
};

} /* namespace std */

#endif /* LIBPMEMOBJ_CPP_TAGGED_PTR */
