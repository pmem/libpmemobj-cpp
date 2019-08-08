/*
 * Copyright 2020, Intel Corporation
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

#ifndef PMEMOBJ_CONCURRENT_SKIP_LIST_IMPL_HPP
#define PMEMOBJ_CONCURRENT_SKIP_LIST_IMPL_HPP

#include <array>
#include <atomic>
#include <mutex> // for std::unique_lock
#include <random>

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/enumerable_thread_specific.hpp>
#include <libpmemobj++/detail/life.hpp>
#include <libpmemobj++/detail/persistent_pool_ptr.hpp>
#include <libpmemobj++/detail/template_helpers.hpp>
#include <libpmemobj++/experimental/v.hpp>
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmem
{
namespace detail
{

template <typename T>
inline void
store_with_release(persistent_pool_ptr<T> &dst, persistent_pool_ptr<T> src)
{
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
	ANNOTATE_HAPPENS_BEFORE(&dst);
#endif
	std::atomic_thread_fence(std::memory_order_release);

	dst = src;
}

template <typename T>
inline persistent_pool_ptr<T>
load_with_acquire(const persistent_pool_ptr<T> &ptr)
{
	persistent_pool_ptr<T> ret = ptr;
	std::atomic_thread_fence(std::memory_order_acquire);
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
	ANNOTATE_HAPPENS_AFTER(&ptr);
#endif
	return ret;
}

template <typename Compare>
using is_transparent = typename Compare::is_transparent;

template <typename Compare>
using has_is_transparent = detail::supports<Compare, is_transparent>;

/**
 * Copy assignment implementation for allocator if
 * propagate_on_container_copy_assignment == true_type
 */
template <typename MyAlloc, typename OtherAlloc>
inline void
allocator_copy_assignment(MyAlloc &my_allocator, OtherAlloc &other_allocator,
			  std::true_type)
{
	my_allocator = other_allocator;
}

/**
 * Copy assignment implementation for allocator if
 * propagate_on_container_copy_assignment == false_type
 */
template <typename MyAlloc, typename OtherAlloc>
inline void
allocator_copy_assignment(MyAlloc &, OtherAlloc &, std::false_type)
{ /* NO COPY */
}

/**
 *  Move assignment implementation for allocator if
 * propagate_on_container_move_assignment == true_type.
 */
template <typename MyAlloc, typename OtherAlloc>
inline void
allocator_move_assignment(MyAlloc &my_allocator, OtherAlloc &other_allocator,
			  std::true_type)
{
	my_allocator = std::move(other_allocator);
}

/**
 *  Move assignment implementation for allocator if
 * propagate_on_container_move_assignment == false_type.
 */
template <typename MyAlloc, typename OtherAlloc>
inline void
allocator_move_assignment(MyAlloc &, OtherAlloc &, std::false_type)
{ /* NO MOVE */
}

/**
 * Swap implementation for allocators if propagate_on_container_swap ==
 * true_type.
 */
template <typename MyAlloc, typename OtherAlloc>
inline void
allocator_swap(MyAlloc &my_allocator, OtherAlloc &other_allocator,
	       std::true_type)
{
	std::swap(my_allocator, other_allocator);
}

/**
 * Swap implementation for allocators if propagate_on_container_swap ==
 * false_type.
 */
template <typename MyAlloc, typename OtherAlloc>
inline void
allocator_swap(MyAlloc &, OtherAlloc &, std::false_type)
{ /* NO SWAP */
}

template <typename Value, typename Mutex = pmem::obj::mutex,
	  typename LockType = std::unique_lock<Mutex>>
class skip_list_node {
public:
	using value_type = Value;
	using size_type = std::size_t;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;
	using node_pointer = persistent_pool_ptr<skip_list_node>;
	using mutex_type = Mutex;
	using lock_type = LockType;

	skip_list_node(size_type levels) : height_(levels)
	{
		for (size_type lev = 0; lev < height_; ++lev)
			detail::create<node_pointer>(&get_next(lev), nullptr);

		assert(height() == levels);
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
		/*
		 * Valgrind does not understand std::atomic and reports
		 * false-postives in drd and helgrind tools.
		 */
		for (size_type lev = 0; lev < height_; ++lev) {
			VALGRIND_HG_DISABLE_CHECKING(&get_next(lev),
						     sizeof(node_pointer));
		}
		VALGRIND_HG_DISABLE_CHECKING(&mutex, sizeof(mutex_type));
#endif
	}

	~skip_list_node()
	{
		for (size_type lev = 0; lev < height_; ++lev)
			detail::destroy<node_pointer>(get_next(lev));
	}

	skip_list_node(const skip_list_node &) = delete;

	skip_list_node &operator=(const skip_list_node &) = delete;

	pointer
	get()
	{
		return &val;
	}

	const_pointer
	get() const
	{
		return &val;
	}

	reference
	value()
	{
		return *get();
	}

	node_pointer
	next(size_type level) const
	{
		assert(level < height());
		return load_with_acquire(get_next(level));
	}

	void
	set_next(size_type level, node_pointer next)
	{
		assert(level < height());
		store_with_release(get_next(level), next);
	}

	void
	set_nexts(const node_pointer *new_nexts, size_type h)
	{
		assert(h == height());
		node_pointer *nexts = get_nexts();
		// Adding the whole array to TX log, instead of adding each
		// element separatly fron the assignment operator of
		// persistent_pool_ptr.
		detail::conditional_add_to_tx(nexts, h);
		for (size_t l = 0; l < h; ++l)
			nexts[l] = new_nexts[l];

		/*
		 * From the performance perspective the following way is more
		 * optimal: std::memcpy(nexts, new_nexts, h *
		 * sizeof(node_pointer)); It avoids calling
		 * detail::conditional_add_to_tx() from the assignment operator
		 * of persistent_pool_ptr. But build is failed because it is not
		 * allowed to use memcpy for the object with non-trivial
		 * copy-assignment or copy-initialization.
		 */
	}

	/** @return number of layers */
	size_type
	height() const
	{
		return height_;
	}

	lock_type
	acquire()
	{
		return lock_type(mutex);
	}

private:
	node_pointer *
	get_nexts()
	{
		return reinterpret_cast<node_pointer *>(this + 1);
	}

	node_pointer &
	get_next(size_type level)
	{
		node_pointer *arr = get_nexts();
		return arr[level];
	}

	const node_pointer &
	get_next(size_type level) const
	{
		const node_pointer *arr =
			reinterpret_cast<const node_pointer *>(this + 1);
		return arr[level];
	}

	mutex_type mutex;
	union {
		value_type val;
	};
	size_type height_;
};

template <typename NodeType, bool is_const>
class skip_list_iterator {
	using node_type = NodeType;
	using node_ptr = typename std::conditional<is_const, const node_type *,
						   node_type *>::type;
	friend class skip_list_iterator<node_type, true>;

public:
	using value_type = typename node_type::value_type;
	using iterator_category = std::forward_iterator_tag;
	using difference_type = ptrdiff_t;
	using reference =
		typename std::conditional<is_const,
					  typename node_type::const_reference,
					  typename node_type::reference>::type;
	using pointer = typename std::conditional<is_const, const value_type *,
						  value_type *>::type;

	skip_list_iterator() : pool_uuid(0), node(nullptr)
	{
	}

	/** Copy constructor. */
	skip_list_iterator(const skip_list_iterator &other) = default;

	/** Copy constructor for const iterator from non-const iterator */
	template <typename U = void,
		  typename = typename std::enable_if<is_const, U>::type>
	skip_list_iterator(const skip_list_iterator<node_type, false> &other)
	    : pool_uuid(other.pool_uuid), node(other.node)
	{
	}

	reference operator*() const
	{
		return *(node->get());
	}

	pointer operator->() const
	{
		return &**this;
	}

	skip_list_iterator &
	operator++()
	{
		assert(node != nullptr);
		node = node->next(0).get(pool_uuid);
		return *this;
	}

	skip_list_iterator
	operator++(int)
	{
		skip_list_iterator tmp = *this;
		++*this;
		return tmp;
	}

private:
	skip_list_iterator(uint64_t pool_uuid, node_type *n)
	    : pool_uuid(pool_uuid), node(n)
	{
	}

	template <typename T = void,
		  typename = typename std::enable_if<is_const, T>::type>
	skip_list_iterator(uint64_t pool_uuid, const node_type *n)
	    : pool_uuid(pool_uuid), node(n)
	{
	}

	uint64_t pool_uuid;

	node_ptr node;

	template <typename Traits>
	friend class concurrent_skip_list;

	template <typename T, bool M, bool U>
	friend bool operator==(const skip_list_iterator<T, M> &lhs,
			       const skip_list_iterator<T, U> &rhs);

	template <typename T, bool M, bool U>
	friend bool operator!=(const skip_list_iterator<T, M> &lhs,
			       const skip_list_iterator<T, U> &rhs);
};

template <typename T, bool M, bool U>
bool
operator==(const skip_list_iterator<T, M> &lhs,
	   const skip_list_iterator<T, U> &rhs)
{
	return lhs.node == rhs.node;
}

template <typename T, bool M, bool U>
bool
operator!=(const skip_list_iterator<T, M> &lhs,
	   const skip_list_iterator<T, U> &rhs)
{
	return lhs.node != rhs.node;
}

template <size_t MAX_LEVEL>
class geometric_level_generator {
public:
	static constexpr size_t max_level = MAX_LEVEL;

	size_t
	operator()()
	{
		static thread_local std::mt19937_64 engine(
			static_cast<size_t>(time(0)));
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
		VALGRIND_HG_DISABLE_CHECKING(&engine, sizeof(engine));
		VALGRIND_HG_DISABLE_CHECKING(&distribution,
					     sizeof(distribution));
#endif
		return (distribution(engine) % MAX_LEVEL) + 1;
	}

private:
	std::geometric_distribution<size_t> distribution;
};

template <typename Traits>
class concurrent_skip_list {
protected:
	using traits_type = Traits;
	using allocator_type = typename traits_type::allocator_type;
	using allocator_traits_type = std::allocator_traits<allocator_type>;
	using key_compare = typename traits_type::compare_type;
	using key_type = typename traits_type::key_type;
	using value_type = typename traits_type::value_type;
	using list_node_type = skip_list_node<value_type>;

	using iterator = skip_list_iterator<list_node_type, false>;
	using const_iterator = skip_list_iterator<list_node_type, true>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = typename allocator_traits_type::pointer;
	using const_pointer = typename allocator_traits_type::const_pointer;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using random_level_generator_type =
		typename traits_type::random_level_generator_type;
	using node_allocator_type = typename std::allocator_traits<
		allocator_type>::template rebind_alloc<uint8_t>;
	using node_allocator_traits = typename std::allocator_traits<
		allocator_type>::template rebind_traits<uint8_t>;
	using node_ptr = list_node_type *;
	using const_node_ptr = const list_node_type *;
	using persistent_node_ptr = persistent_pool_ptr<list_node_type>;

	static constexpr size_type MAX_LEVEL = traits_type::MAX_LEVEL;

	using prev_array_type = std::array<node_ptr, MAX_LEVEL>;
	using next_array_type = std::array<persistent_node_ptr, MAX_LEVEL>;
	using node_lock_type = typename list_node_type::lock_type;
	using lock_array = std::array<node_lock_type, MAX_LEVEL>;

public:
	static constexpr bool allow_multimapping =
		traits_type::allow_multimapping;

	/**
	 * Default constructor. Construct empty skip list.
	 */
	concurrent_skip_list()
	{
		init();
	}

	explicit concurrent_skip_list(
		const key_compare &comp,
		const allocator_type &alloc = allocator_type())
	    : _node_allocator(alloc), _compare(comp)
	{
		init();
	}

	template <class InputIt>
	concurrent_skip_list(InputIt first, InputIt last,
			     const key_compare &comp = key_compare(),
			     const allocator_type &alloc = allocator_type())
	    : _node_allocator(alloc), _compare(comp)
	{
		init();
		internal_copy(first, last);
	}

	/** Copy constructor */
	concurrent_skip_list(const concurrent_skip_list &other)
	    : _node_allocator(other._node_allocator),
	      _compare(other._compare),
	      _rnd_generator(other._rnd_generator)
	{
		init();
		internal_copy(other);
		assert(my_size.get_ro() == other.my_size.get_ro());
	}

	concurrent_skip_list(const concurrent_skip_list &other,
			     const allocator_type &alloc)
	    : _node_allocator(alloc),
	      _compare(other._compare),
	      _rnd_generator(other._rnd_generator)
	{
		init();
		internal_copy(other);
		assert(my_size.get_ro() == other.my_size.get_ro());
	}

	concurrent_skip_list(concurrent_skip_list &&other)
	    : _node_allocator(std::move(other._node_allocator)),
	      _compare(other._compare),
	      _rnd_generator(other._rnd_generator)
	{
		init();
		internal_move(std::move(other));
	}

	concurrent_skip_list(concurrent_skip_list &&other,
			     const allocator_type &alloc)
	    : _node_allocator(alloc),
	      _compare(other._compare),
	      _rnd_generator(other._rnd_generator)
	{
		init();
		if (alloc == other.get_allocator()) {
			internal_move(std::move(other));
		} else {
			init();
			internal_copy(std::make_move_iterator(other.begin()),
				      std::make_move_iterator(other.end()));
		}
	}

	/**
	 * Intialize persistent concurrent map after process restart.
	 * MUST be called everytime after process restart.
	 * Not thread safe.
	 *
	 * @throw pmem::layout_error if map was created using incompatible
	 * version of libpmemobj-cpp
	 */
	void
	runtime_initialize(bool graceful_shutdown = false)
	{
		if (!graceful_shutdown) {
			auto actual_size =
				std::distance(this->begin(), this->end());
			assert(actual_size >= 0);
			this->restore_size(size_type(actual_size));
		} else {
			assert(this->size() ==
			       size_type(std::distance(this->begin(),
						       this->end())));
		}

		cleanup_node_tls();
	}

	~concurrent_skip_list()
	{
		clear();
		delete_dummy_head();
	}

	concurrent_skip_list &
	operator=(const concurrent_skip_list &other)
	{
		if (this == &other)
			return *this;

		obj::pool_base pop = get_pool_base();
		obj::transaction::run(pop, [&] {
			using pocca_t = typename node_allocator_traits::
				propagate_on_container_copy_assignment;
			clear();
			allocator_copy_assignment(_node_allocator,
						  other._node_allocator,
						  pocca_t());
			_compare = other._compare;
			_rnd_generator = const_cast<pmem::obj::experimental::v<
				random_level_generator_type> &>(
				other._rnd_generator);

			internal_copy(other);
		});
		return *this;
	}

	concurrent_skip_list &
	operator=(concurrent_skip_list &&other)
	{
		if (this == &other)
			return *this;

		obj::pool_base pop = get_pool_base();
		obj::transaction::run(pop, [&] {
			using pocma_t = typename node_allocator_traits::
				propagate_on_container_move_assignment;
			clear();
			if (pocma_t::value ||
			    _node_allocator == other._node_allocator) {
				delete_dummy_head();
				allocator_move_assignment(_node_allocator,
							  other._node_allocator,
							  pocma_t());
				_compare = other._compare;
				_rnd_generator = other._rnd_generator;
				internal_move(std::move(other));
			} else {
				internal_copy(
					std::make_move_iterator(other.begin()),
					std::make_move_iterator(other.end()));
			}
		});
		return *this;
	}

	concurrent_skip_list &
	operator=(std::initializer_list<value_type> il)
	{
		obj::pool_base pop = get_pool_base();
		obj::transaction::run(pop, [&] {
			clear();
			insert(il.begin(), il.end());
		});
		return *this;
	}

	std::pair<iterator, bool>
	insert(const value_type &value)
	{
		return internal_insert(value.first, value);
	}

	template <typename P,
		  typename std::enable_if<
			  std::is_constructible<value_type, P &&>::value>::type>
	std::pair<iterator, bool>
	insert(P &&value)
	{
		return emplace(std::forward<P>(value));
	}

	std::pair<iterator, bool>
	insert(value_type &&value)
	{
		return internal_insert(value.first, std::move(value));
	}

	iterator
	insert(const_iterator, const_reference value)
	{
		// Ignore hint
		return insert(value).first;
	}

	template <typename P,
		  typename std::enable_if<
			  std::is_constructible<value_type, P &&>::value>::type>
	iterator
	insert(const_iterator hint, P &&value)
	{
		return emplace_hint(hint, std::forward<P>(value));
	}

	template <typename InputIterator>
	void
	insert(InputIterator first, InputIterator last)
	{
		for (InputIterator it = first; it != last; ++it)
			insert(*it);
	}

	void
	insert(std::initializer_list<value_type> init)
	{
		insert(init.begin(), init.end());
	}

	template <typename... Args>
	std::pair<iterator, bool>
	emplace(Args &&... args)
	{
		return internal_emplace(std::forward<Args>(args)...);
	}

	template <typename... Args>
	iterator
	emplace_hint(const_iterator, Args &&... args)
	{
		// Ignore hint
		return emplace(std::forward<Args>(args)...).first;
	}

	template <typename... Args>
	std::pair<iterator, bool>
	try_emplace(const key_type &k, Args &&... args)
	{
		return internal_try_emplace(k, std::forward<Args>(args)...);
	}

	template <typename... Args>
	std::pair<iterator, bool>
	try_emplace(key_type &&k, Args &&... args)
	{
		return internal_try_emplace(std::move(k),
					    std::forward<Args>(args)...);
	}

	template <typename K, typename... Args>
	typename std::enable_if<
		has_is_transparent<key_compare>::value &&
			std::is_constructible<key_type, K &&>::value,
		std::pair<iterator, bool>>::type
	try_emplace(K &&k, Args &&... args)
	{
		return internal_try_emplace(std::forward<K>(k),
					    std::forward<Args>(args)...);
	}

	iterator
	unsafe_erase(iterator pos)
	{
		obj::pool_base pop = get_pool_base();
		obj::transaction::manual tx(pop);

		std::pair<persistent_node_ptr, persistent_node_ptr>
			extract_result = internal_extract(pos);

		if (extract_result.first) { // node was extracted
			delete_node(extract_result.first);
			obj::transaction::commit();
			return iterator(pool_uuid,
					extract_result.second.get(pool_uuid));
		}
		return end();
	}

	iterator
	unsafe_erase(const_iterator pos)
	{
		return unsafe_erase(get_iterator(pos));
	}

	iterator
	unsafe_erase(const_iterator first, const_iterator last)
	{
		obj::pool_base pop = get_pool_base();
		obj::transaction::manual tx(pop);
		while (first != last) {
			first = unsafe_erase(first);
		}
		obj::transaction::commit();
		return get_iterator(first);
	}

	size_type
	unsafe_erase(const key_type &key)
	{
		std::pair<iterator, iterator> range = equal_range(key);
		size_type sz = static_cast<size_type>(
			std::distance(range.first, range.second));
		unsafe_erase(range.first, range.second);
		return sz;
	}

	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	size_type
	unsafe_erase(const K &key)
	{
		std::pair<iterator, iterator> range = equal_range(key);
		size_type sz = static_cast<size_type>(
			std::distance(range.first, range.second));
		unsafe_erase(range.first, range.second);
		return sz;
	}

	iterator
	lower_bound(const key_type &key)
	{
		return internal_get_bound(key, _compare);
	}

	const_iterator
	lower_bound(const key_type &key) const
	{
		return internal_get_bound(key, _compare);
	}

	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	iterator
	lower_bound(const K &key)
	{
		return internal_get_bound(key, _compare);
	}

	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	const_iterator
	lower_bound(const K &key) const
	{
		return internal_get_bound(key, _compare);
	}

	iterator
	upper_bound(const key_type &key)
	{
		return internal_get_bound(key, not_greater_compare(_compare));
	}

	const_iterator
	upper_bound(const key_type &key) const
	{
		return internal_get_bound(key, not_greater_compare(_compare));
	}

	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	iterator
	upper_bound(const K &key)
	{
		return internal_get_bound(key, not_greater_compare(_compare));
	}

	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	const_iterator
	upper_bound(const K &key) const
	{
		return internal_get_bound(key, not_greater_compare(_compare));
	}

	iterator
	find(const key_type &key)
	{
		return internal_find(key);
	}

	const_iterator
	find(const key_type &key) const
	{
		return internal_find(key);
	}

	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	iterator
	find(const K &key)
	{
		return internal_find(key);
	}

	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	const_iterator
	find(const K &key) const
	{
		return internal_find(key);
	}

	size_type
	count(const key_type &key) const
	{
		return internal_count(key);
	}

	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	size_type
	count(const K &key) const
	{
		return internal_count(key);
	}

	bool
	contains(const key_type &key) const
	{
		return find(key) != end();
	}

	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	bool
	contains(const K &key) const
	{
		return find(key) != end();
	}

	void
	clear()
	{
		assert(dummy_head(pool_uuid)->height() > 0);
		obj::pool_base pop = get_pool_base();

		persistent_node_ptr current = dummy_head(pool_uuid)->next(0);
		obj::transaction::manual tx(pop);
		while (current) {
			assert(current(pool_uuid)->height() > 0);
			persistent_node_ptr next = current(pool_uuid)->next(0);
			delete_node(current);
			current = next;
		}

		my_size.get_rw() = 0;
		node_ptr head = dummy_head.get(pool_uuid);
		for (size_type i = 0; i < head->height(); ++i) {
			head->set_next(i, nullptr);
		}
		obj::transaction::commit();
	}

	iterator
	begin()
	{
		return iterator(
			pool_uuid,
			dummy_head.get(pool_uuid)->next(0).get(pool_uuid));
	}

	const_iterator
	begin() const
	{
		return const_iterator(
			pool_uuid,
			dummy_head.get(pool_uuid)->next(0).get(pool_uuid));
	}

	const_iterator
	cbegin() const
	{
		return const_iterator(
			pool_uuid,
			dummy_head.get(pool_uuid)->next(0).get(pool_uuid));
	}

	iterator
	end()
	{
		return iterator(pool_uuid, nullptr);
	}

	const_iterator
	end() const
	{
		return const_iterator(pool_uuid, nullptr);
	}

	const_iterator
	cend() const
	{
		return const_iterator(pool_uuid, nullptr);
	}

	size_type
	size() const
	{
		return my_size.get_ro().load(std::memory_order_relaxed);
	}

	size_type
	max_size() const
	{
		return _node_allocator.max_size();
	}

	bool
	empty() const
	{
		return 0 == size();
	}

	allocator_type
	get_allocator() const
	{
		return _node_allocator;
	}

	void
	swap(concurrent_skip_list &other)
	{
		obj::pool_base pop = get_pool_base();
		obj::transaction::run(pop, [&] {
			using pocs_t = typename node_allocator_traits::
				propagate_on_container_swap;
			allocator_swap(_node_allocator, other._node_allocator,
				       pocs_t());
			std::swap(_compare, other._compare);
			std::swap(_rnd_generator, other._rnd_generator);
			std::swap(dummy_head, other.dummy_head);

			size_type tmp = my_size.get_ro();
			my_size.get_rw().store(other.my_size.get_ro());
			other.my_size.get_rw().store(tmp);
		});
	}

	std::pair<iterator, iterator>
	equal_range(const key_type &key)
	{
		return std::pair<iterator, iterator>(lower_bound(key),
						     upper_bound(key));
	}

	std::pair<const_iterator, const_iterator>
	equal_range(const key_type &key) const
	{
		return std::pair<const_iterator, const_iterator>(
			lower_bound(key), upper_bound(key));
	}

	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	std::pair<iterator, iterator>
	equal_range(const K &key)
	{
		return std::pair<iterator, iterator>(lower_bound(key),
						     upper_bound(key));
	}

	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	std::pair<const_iterator, const_iterator>
	equal_range(const K &key) const
	{
		return std::pair<const_iterator, const_iterator>(
			lower_bound(key), upper_bound(key));
	}

	key_compare
	key_comp() const
	{
		return _compare;
	}

private:
	void
	init()
	{
		pool_uuid = pmemobj_oid(this).pool_uuid_lo;
		my_size.get_rw() = 0;
		create_dummy_head();
	}

	void
	restore_size(size_type actual_size)
	{
		my_size.get_rw().store(actual_size, std::memory_order_relaxed);
		obj::pool_base pop = get_pool_base();
		pop.persist(my_size);
	}

	void
	internal_move(concurrent_skip_list &&other)
	{
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);
		dummy_head = other.dummy_head;
		other.dummy_head = nullptr;
		other.create_dummy_head();

		my_size.get_rw().store(other.my_size.get_ro());
		other.my_size.get_rw().store(0);
	}

	static const_reference
	get_val(const_node_ptr n)
	{
		assert(n);
		return *(n->get());
	}

	static reference
	get_val(node_ptr n)
	{
		assert(n);
		return *(n->get());
	}

	static const key_type &
	get_key(const_node_ptr n)
	{
		assert(n);
		return traits_type::get_key(get_val(n));
	}

	template <typename K>
	iterator
	internal_find(const K &key)
	{
		iterator it = lower_bound(key);
		return (it == end() || _compare(key, traits_type::get_key(*it)))
			? end()
			: it;
	}

	template <typename K>
	const_iterator
	internal_find(const K &key) const
	{
		const_iterator it = lower_bound(key);
		return (it == end() || _compare(key, traits_type::get_key(*it)))
			? end()
			: it;
	}

	template <typename K>
	size_type
	internal_count(const K &key) const
	{
		if (allow_multimapping) {
			std::pair<const_iterator, const_iterator> range =
				equal_range(key);
			return static_cast<size_type>(
				std::distance(range.first, range.second));
		}
		return (find(key) == end()) ? size_type(0) : size_type(1);
	}

	/**
	 * Finds position on the @param level using @param cmp
	 * @param level - on which level search prev node
	 * @param prev - pointer to the start node to search
	 * @param key - key to search
	 * @param cmp - callable object to compare two objects
	 *  (_compare member is default comparator)
	 * @returns pointer to the node which is not satisfy the comparison with
	 * @param key
	 */
	template <typename K, typename pointer_type, typename comparator>
	persistent_node_ptr
	internal_find_position(size_type level, pointer_type &prev,
			       const K &key, comparator cmp) const
	{
		assert(level < prev->height());
		persistent_node_ptr next = prev->next(level);
		pointer_type curr = next.get(pool_uuid);

		while (curr && cmp(get_key(curr), key)) {
			prev = curr;
			assert(level < prev->height());
			next = prev->next(level);
			curr = next.get(pool_uuid);
		}

		return next;
	}

	template <typename K, typename comparator>
	void
	fill_prev_next_arrays(prev_array_type &prev_nodes,
			      next_array_type &next_nodes, const K &key,
			      const comparator &cmp)
	{
		node_ptr prev = dummy_head.get(pool_uuid);
		prev_nodes.fill(prev);
		next_nodes.fill(nullptr);

		for (size_type h = prev->height(); h > 0; --h) {
			persistent_node_ptr next =
				internal_find_position(h - 1, prev, key, cmp);
			prev_nodes[h - 1] = prev;
			next_nodes[h - 1] = next;
		}
	}

	template <typename K, typename... Args>
	std::pair<iterator, bool>
	internal_try_emplace(K &&key, Args &&... args)
	{
		persistent_node_ptr new_node = nullptr;

		size_type height = random_level();

		std::pair<iterator, bool> insert_result = internal_insert_node(
			key, height, new_node, std::piecewise_construct,
			std::forward_as_tuple(std::forward<K>(key)),
			std::forward_as_tuple(std::forward<Args>(args)...));

		assert(new_node == nullptr);

		return insert_result;
	}

	template <typename... Args>
	std::pair<iterator, bool>
	internal_emplace(Args &&... args)
	{
		persistent_node_ptr &new_node = get_tls_node_ptr();

		obj::pool_base pop = get_pool_base();
		{ // tx scope
			obj::transaction::manual tx(pop);
			new_node = create_node(std::forward<Args>(args)...);
			obj::transaction::commit();
		}

		node_ptr n = new_node.get(pool_uuid);
		std::pair<iterator, bool> insert_result =
			internal_insert_node(get_key(n), n->height(), new_node,
					     std::forward<Args>(args)...);
		if (!insert_result.second) {
			delete_node(new_node);
		}
		return insert_result;
	}

	template <typename K, typename... Args>
	std::pair<iterator, bool>
	internal_insert(const K &key, Args &&... args)
	{
		persistent_node_ptr new_node = nullptr;

		size_type height = random_level();

		std::pair<iterator, bool> insert_result = internal_insert_node(
			key, height, new_node, std::forward<Args>(args)...);

		assert(new_node == nullptr);

		return insert_result;
	}

	template <typename K, typename... Args>
	std::pair<iterator, bool>
	internal_insert_node(const K &key, size_type height,
			     persistent_node_ptr &new_node, Args &&... args)
	{
		prev_array_type prev_nodes;
		next_array_type next_nodes;
		node_ptr n = nullptr;

		do {
			if (allow_multimapping) {
				fill_prev_next_arrays(
					prev_nodes, next_nodes, key,
					[&](const key_type &lhs,
					    const key_type &rhs) {
						return !_compare(rhs, lhs);
					});
			} else {
				fill_prev_next_arrays(prev_nodes, next_nodes,
						      key, _compare);
			}

			node_ptr next = next_nodes[0].get(pool_uuid);
			if (next && !allow_multimapping &&
			    !_compare(key, get_key(next))) {

				return std::pair<iterator, bool>(
					iterator(pool_uuid, next), false);
			}

		} while ((n = try_insert_node(
				  new_node, prev_nodes, next_nodes, height,
				  std::forward<Args>(args)...)) == nullptr);

		assert(n);
		return std::pair<iterator, bool>(iterator(pool_uuid, n), true);
	}

	/**
	 * Try to insert new node to the skip list.
	 * @returns pointer to the new node if it was inserted. Otherwise,
	 * returns nullptr.
	 */
	template <typename... Args>
	node_ptr
	try_insert_node(persistent_node_ptr &new_node,
			prev_array_type &prev_nodes,
			next_array_type &next_nodes, size_type height,
			Args &&... args)
	{
		assert(dummy_head(pool_uuid)->height() >= height);
		lock_array locks;
		if (!try_lock_nodes(height, prev_nodes, next_nodes, locks)) {
			return nullptr;
		}

		node_ptr n = nullptr;

		obj::pool_base pop = get_pool_base();
		{ // tx scope
			obj::transaction::manual tx(pop);

			if (new_node) {

			} else {
				new_node = create_node(
					height, std::forward<Args>(args)...);
			}

			n = new_node.get(pool_uuid);
			n->set_nexts(next_nodes.data(), height);
			/*
			 * We need to hold lock to the new node until changes
			 * are committed to persistent domain. Otherwise, the
			 * new node would be visible to concurrent inserts
			 * before it is persisted.
			 */
			node_lock_type new_node_lock = n->acquire();

			for (size_type level = 0; level < height; ++level) {
				assert(prev_nodes[level]->height() > level);
				assert(prev_nodes[level]->next(level) ==
				       next_nodes[level]);
				prev_nodes[level]->set_next(level, new_node);
			}

			new_node = nullptr;
			obj::transaction::commit();
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
			VALGRIND_HG_ENABLE_CHECKING(
				new_node_lock.mutex(),
				sizeof(typename node_lock_type::mutex_type));
#endif
		}

		++(my_size.get_rw());
		pop.persist(my_size);

		assert(n);
		return n;
	}

	/**
	 * Used only inside asserts.
	 * Checks that prev_array is filled with correct values.
	 */
	bool
	check_prev_array(const prev_array_type &prevs, size_type height)
	{
		for (size_type l = 1; l < height; ++l) {
			if (prevs[l] == dummy_head.get(pool_uuid)) {
				continue;
			}

			assert(prevs[l - 1] != dummy_head.get(pool_uuid));
			assert(!_compare(get_key(prevs[l - 1]),
					 get_key(prevs[l])));
		}

		return true;
	}

	bool
	try_lock_nodes(size_type height, prev_array_type &prevs,
		       next_array_type &nexts, lock_array &locks)
	{
		assert(check_prev_array(prevs, height));

		for (size_type l = 0; l < height; ++l) {
			if (l == 0 || prevs[l] != prevs[l - 1]) {
				locks[l] = prevs[l]->acquire();
			}

			persistent_node_ptr next = prevs[l]->next(l);
			if (next != nexts[l])
				return false;
		}

		return true;
	}

	template <typename K, typename comparator>
	const_iterator
	internal_get_bound(const K &key, const comparator &cmp) const
	{
		const_node_ptr prev = dummy_head.get(pool_uuid);
		assert(prev->height() > 0);
		persistent_node_ptr next = nullptr;

		for (size_type h = prev->height(); h > 0; --h) {
			next = internal_find_position(h - 1, prev, key, cmp);
		}

		return const_iterator(pool_uuid, next.get(pool_uuid));
	}

	template <typename K, typename comparator>
	iterator
	internal_get_bound(const K &key, const comparator &cmp)
	{
		node_ptr prev = dummy_head.get(pool_uuid);
		assert(prev->height() > 0);
		persistent_node_ptr next = nullptr;

		for (size_type h = prev->height(); h > 0; --h) {
			next = internal_find_position(h - 1, prev, key, cmp);
		}

		return iterator(pool_uuid, next.get(pool_uuid));
	}

	/**
	 * @returns a pointer to extracted node and a pointer to next node
	 */
	std::pair<persistent_node_ptr, persistent_node_ptr>
	internal_extract(const_iterator it)
	{
		if (it != end()) {
			assert(pmemobj_tx_stage() == TX_STAGE_WORK);
			const key_type &key = traits_type::get_key(*it);
			assert(dummy_head(pool_uuid)->height() > 0);

			prev_array_type prev_nodes;
			next_array_type next_nodes;

			fill_prev_next_arrays(prev_nodes, next_nodes, key,
					      _compare);

			node_ptr erase_node = next_nodes[0].get(pool_uuid);
			assert(erase_node != nullptr);

			if (!_compare(key, get_key(erase_node))) {
				for (size_type level = 0;
				     level < erase_node->height(); ++level) {
					assert(prev_nodes[level]->height() >
					       level);
					assert(next_nodes[level].get(
						       pool_uuid) ==
					       erase_node);
					prev_nodes[level]->set_next(
						level, erase_node->next(level));
				}
				--my_size.get_rw();
				return std::pair<persistent_node_ptr,
						 persistent_node_ptr>(
					next_nodes[0], erase_node->next(0));
			}
		}
		return std::pair<persistent_node_ptr, persistent_node_ptr>(
			nullptr, nullptr);
	}

	/**
	 * Get the persistent memory pool where hashmap resides.
	 * @returns pmem::obj::pool_base object.
	 */
	obj::pool_base
	get_pool_base() const
	{
		PMEMobjpool *pop = pmemobj_pool_by_ptr(this);
		return obj::pool_base(pop);
	}

	void
	internal_copy(const concurrent_skip_list &other)
	{
		internal_copy(other.begin(), other.end());
	}

	template <typename Iterator>
	void
	internal_copy(Iterator first, Iterator last)
	{
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);

		prev_array_type prev_nodes;
		prev_nodes.fill(dummy_head.get(pool_uuid));
		size_type sz = 0;

		for (; first != last; ++first, ++sz) {
			persistent_node_ptr new_node = create_node(*first);
			node_ptr n = new_node.get(pool_uuid);
			for (size_type level = 0; level < n->height();
			     ++level) {
				prev_nodes[level]->set_next(level, new_node);
				prev_nodes[level] = n;
			}
		}
		my_size.get_rw().store(sz);
	}

	/** Generate random level */
	size_type
	random_level()
	{
		return _rnd_generator.get()();
	}

	static size_type
	calc_node_size(size_type height)
	{
		return sizeof(list_node_type) +
			height * sizeof(typename list_node_type::node_pointer);
	}

	/** Creates new node */
	template <typename... Args>
	persistent_node_ptr
	create_node(Args &&... args)
	{
		size_type levels = random_level();

		return create_node(levels, std::forward<Args>(args)...);
	}

	/** Creates new node */
	template <typename... Args>
	persistent_node_ptr
	create_node(size_type levels, Args &&... args)
	{
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);

		persistent_node_ptr node = creates_dummy_node(levels);

		node_ptr new_node = node.get(pool_uuid);

		node_allocator_traits::construct(_node_allocator,
						 new_node->get(),
						 std::forward<Args>(args)...);

		return node;
	}

	/**
	 * Creates dummy head.
	 * Always called from ctor.
	 */
	void
	create_dummy_head()
	{
		dummy_head = creates_dummy_node(MAX_LEVEL);
	}

	/**
	 * Creates new node, value_type should be constructed separately.
	 */
	persistent_node_ptr
	creates_dummy_node(size_type height)
	{
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);
		size_type sz = calc_node_size(height);

		persistent_node_ptr n =
			node_allocator_traits::allocate(_node_allocator, sz)
				.raw();

		node_allocator_traits::construct(_node_allocator,
						 n.get(pool_uuid), height);

		return n;
	}

	template <bool is_dummy = false>
	void
	delete_node(persistent_node_ptr &node)
	{
		node_ptr n = node.get(pool_uuid);
		size_type sz = calc_node_size(n->height());
		obj::pool_base pop = get_pool_base();

		obj::transaction::manual tx(pop);
		// Destroy value
		if (!is_dummy)
			node_allocator_traits::destroy(_node_allocator,
						       n->get());
		// Destroy node
		node_allocator_traits::destroy(_node_allocator, n);
		// Deallocate memory
		deallocate_node(node, sz);
		node = nullptr;
		obj::transaction::commit();
	}

	void
	deallocate_node(persistent_node_ptr &node, size_type sz)
	{
		obj::persistent_ptr<uint8_t> tmp =
			node.get_persistent_ptr(pool_uuid).raw();
		node_allocator_traits::deallocate(_node_allocator, tmp, sz);
	}

	void
	delete_dummy_head()
	{
		delete_node<true>(dummy_head);
	}

	iterator
	get_iterator(const_iterator it)
	{
		return iterator(
			pool_uuid,
			const_cast<typename iterator::node_ptr>(it.node));
	}

	struct alignas(64) node_tls_entry {
		persistent_node_ptr ptr;
	};

	persistent_node_ptr &
	get_tls_node_ptr()
	{
		return new_node_tls.local().ptr;
	}

	void
	cleanup_node_tls()
	{
		new_node_tls.initialize([this](node_tls_entry &tls_entry) {
			persistent_node_ptr &node = tls_entry.ptr;
			if (node) {
				internal_insert_node(
					get_key(node.get(pool_uuid)),
					node.get(pool_uuid)->height(), node);
			}
			if (node) {
				delete_node(node);
			}
		});
	}

	struct not_greater_compare {
		const key_compare &my_less_compare;

		not_greater_compare(const key_compare &less_compare)
		    : my_less_compare(less_compare)
		{
		}

		template <typename K1, typename K2>
		bool
		operator()(const K1 &first, const K2 &second) const
		{
			return !my_less_compare(second, first);
		}
	};

	uint64_t pool_uuid;
	node_allocator_type _node_allocator;
	key_compare _compare;
	pmem::obj::experimental::v<random_level_generator_type> _rnd_generator;
	persistent_node_ptr dummy_head;

	enumerable_thread_specific<node_tls_entry> new_node_tls;

	obj::p<std::atomic<size_type>> my_size;
}; // class concurrent_skip_list

} // namespace detail
} // namespace pmem

#endif // PMEMOBJ_CONCURRENT_SKIP_LIST_IMPL_HPP
