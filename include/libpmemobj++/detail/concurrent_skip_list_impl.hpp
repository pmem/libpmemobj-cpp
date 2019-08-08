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

#ifndef PMEMOBJ_CONCURRENT_SKIP_LIST_IMPL_HPP
#define PMEMOBJ_CONCURRENT_SKIP_LIST_IMPL_HPP

#include <array>
#include <atomic>
#include <mutex> // for std::unique_lock
#include <random>

#include <libpmemobj++/detail/common.hpp>
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

using namespace pmem::obj;
using namespace pmem::obj::experimental;

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

template <typename Value>
class skip_list_node {
public:
	using value_type = Value;
	using size_type = std::size_t;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;
	using node_pointer = persistent_pool_ptr<skip_list_node>;
#if 1
	using mutex_type = pmem::obj::mutex;
	using lock_type = std::unique_lock<mutex_type>;
#else
	using mutex_type = v<tbb::spin_mutex>;
	using lock_type = std::unique_lock<tbb::spin_mutex>;
#endif

	skip_list_node() : my_height(1)
	{
		assert(height() == 1);
		new (&my_next(0)) node_pointer(nullptr);
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
		VALGRIND_HG_DISABLE_CHECKING(&my_next(0), sizeof(node_pointer));
		VALGRIND_HG_DISABLE_CHECKING(&my_fullyLinked,
					     sizeof(my_fullyLinked));
#endif
	}

	skip_list_node(size_type levels) : my_height(levels)
	{
		for (size_type lev = 0; lev < my_height; ++lev)
			new (&my_next(lev)) node_pointer(nullptr);
		assert(height() == levels);
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
		for (size_type lev = 0; lev < my_height; ++lev) {
			VALGRIND_HG_DISABLE_CHECKING(&my_next(lev),
						     sizeof(node_pointer));
		}
		VALGRIND_HG_DISABLE_CHECKING(&my_fullyLinked,
					     sizeof(my_fullyLinked));
#endif
	}

	~skip_list_node()
	{
		for (size_type lev = 0; lev < my_height; ++lev)
			my_next(lev).~persistent_pool_ptr();
	}

	skip_list_node(const skip_list_node &) = delete;

	skip_list_node &operator=(const skip_list_node &) = delete;

	pointer
	get()
	{
		return reinterpret_cast<pointer>(&my_val);
	}

	const_pointer
	get() const
	{
		return reinterpret_cast<const_pointer>(&my_val);
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
		return load_with_acquire(my_next(level));
	}

	void
	set_next(size_type level, node_pointer next)
	{
		assert(level < height());
		store_with_release(my_next(level), next);
	}

	void
	set_next(pool_base &pop, size_type level, node_pointer next)
	{
		this->set_next(level, next);
		node_pointer &ptr = my_next(level);
		// TODO: Can we use just flush (without barrier)
		pop.persist(&ptr, sizeof(ptr));
	}

	/** @return number of layers */
	size_type
	height() const
	{
		return my_height;
	}

	bool
	fully_linked() const
	{
		return my_fullyLinked.get_ro().load(std::memory_order_acquire);
	}

	void
	mark_linked(pool_base &pop)
	{
		my_fullyLinked.get_rw().store(true, std::memory_order_release);
		pop.persist(my_fullyLinked);
	}

	lock_type
	acquire()
	{
		return lock_type(my_mutex);
	}

private:
	using aligned_storage_type =
		typename std::aligned_storage<sizeof(value_type),
					      alignof(value_type)>::type;

	node_pointer &
	my_next(size_type level)
	{
		node_pointer *arr = reinterpret_cast<node_pointer *>(this + 1);
		return arr[level];
	}

	const node_pointer &
	my_next(size_type level) const
	{
		const node_pointer *arr =
			reinterpret_cast<const node_pointer *>(this + 1);
		return arr[level];
	}

	mutex_type my_mutex;
	aligned_storage_type my_val;
	size_type my_height;
	p<std::atomic_bool> my_fullyLinked;
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

	skip_list_iterator() : my_pool_uuid(0), my_node_ptr(nullptr)
	{
	}

	template <typename T = void,
		  typename = typename std::enable_if<is_const, T>::type>
	skip_list_iterator(const skip_list_iterator<node_type, false> &other)
	    : my_pool_uuid(other.my_pool_uuid), my_node_ptr(other.my_node_ptr)
	{
	}

	skip_list_iterator(const skip_list_iterator &other) = default;

	reference operator*() const
	{
		return *(my_node_ptr->get());
	}

	pointer operator->() const
	{
		return &**this;
	}

	skip_list_iterator &
	operator++()
	{
		assert(my_node_ptr != nullptr);
		my_node_ptr = my_node_ptr->next(0).get(my_pool_uuid);
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
	// TODO: investigate iterator logic and constructors
	skip_list_iterator(uint64_t pool_uuid, node_type *n)
	    : my_pool_uuid(pool_uuid), my_node_ptr(n)
	{
	}

	template <typename T = void,
		  typename = typename std::enable_if<is_const, T>::type>
	skip_list_iterator(uint64_t pool_uuid, const node_type *n)
	    : my_pool_uuid(pool_uuid), my_node_ptr(n)
	{
	}

	uint64_t my_pool_uuid;

	node_ptr my_node_ptr;

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
	return lhs.my_node_ptr == rhs.my_node_ptr;
}

template <typename T, bool M, bool U>
bool
operator!=(const skip_list_iterator<T, M> &lhs,
	   const skip_list_iterator<T, U> &rhs)
{
	return lhs.my_node_ptr != rhs.my_node_ptr;
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
	using lock_array =
		std::array<typename list_node_type::lock_type, MAX_LEVEL>;

public:
	static bool const allow_multimapping = traits_type::allow_multimapping;

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

		pool_base pop = get_pool_base();
		transaction::run(pop, [&] {
			using pocca_t = typename node_allocator_traits::
				propagate_on_container_copy_assignment;
			clear();
			allocator_copy_assignment(_node_allocator,
						  other._node_allocator,
						  pocca_t());
			_compare = other._compare;
			_rnd_generator =
				const_cast<v<random_level_generator_type> &>(
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

		pool_base pop = get_pool_base();
		transaction::run(pop, [&] {
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
		pool_base pop = get_pool_base();
		transaction::run(pop, [&] {
			clear();
			insert(il.begin(), il.end());
		});
		return *this;
	}

	std::pair<iterator, bool>
	insert(const value_type &value)
	{
		return internal_insert(value);
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
		return internal_insert(std::move(value));
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
		if (std::is_constructible<value_type, P &&>::value == true)
			return emplace_hint(hint, std::forward<P>(value));
		return end();
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
		return internal_insert(std::forward<Args>(args)...);
	}

	template <typename... Args>
	iterator
	emplace_hint(const_iterator, Args &&... args)
	{
		// Ignore hint
		return emplace(std::forward<Args>(args)...).first;
	}

	iterator
	unsafe_erase(const_iterator pos)
	{ // TODO: iterator or const_iterator as an arg
		std::pair<iterator, persistent_node_ptr> extract_result =
			internal_extract(pos);
		if (extract_result.second) { // node was extracted
			delete_node(extract_result.second);
			// internal_extract returns the previous iterator - move
			// iterator forward
			return ++extract_result.first;
		}
		return end();
	}

	iterator
	unsafe_erase(const_iterator first, const_iterator last)
	{
		while (first != last) {
			first = unsafe_erase(first);
		}
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
	clear() noexcept
	{
		assert(dummy_head(my_pool_uuid)->height() > 0);
		pool_base pop = get_pool_base();

		persistent_node_ptr current = dummy_head(my_pool_uuid)->next(0);
		transaction::manual tx(pop);
		while (current) {
			assert(current(my_pool_uuid)->height() > 0);
			persistent_node_ptr next =
				current(my_pool_uuid)->next(0);
			delete_node(current);
			current = next;
		}

		my_size.get_rw() = 0;
		node_ptr head = dummy_head.get(my_pool_uuid);
		for (size_type i = 0; i < head->height(); ++i) {
			head->set_next(i, nullptr);
		}
		transaction::commit();
	}

	iterator
	begin()
	{
		return iterator(my_pool_uuid,
				dummy_head.get(my_pool_uuid)
					->next(0)
					.get(my_pool_uuid));
	}

	const_iterator
	begin() const
	{
		return const_iterator(my_pool_uuid,
				      dummy_head.get(my_pool_uuid)
					      ->next(0)
					      .get(my_pool_uuid));
	}

	const_iterator
	cbegin() const
	{
		return const_iterator(my_pool_uuid,
				      dummy_head.get(my_pool_uuid)
					      ->next(0)
					      .get(my_pool_uuid));
	}

	// TODO: implement reverse_iterator methods
	// reverse_iterator rbegin() {}

	// const_reverse_iterator rbegin() const {}

	// const_reverse_iterator crbegin() const {}

	iterator
	end()
	{
		return iterator(my_pool_uuid, nullptr);
	}

	const_iterator
	end() const
	{
		return const_iterator(my_pool_uuid, nullptr);
	}

	const_iterator
	cend() const
	{
		return const_iterator(my_pool_uuid, nullptr);
	}

	// TODO: implement reverse_iterator methods
	// reverse_iterator rend() {}

	// const_reverse_iterator rend() const {}

	// const_reverse_iterator crend() const {}

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

	// TODO: implement max_size
	// size_type max_size() const {}

	allocator_type
	get_allocator() const
	{
		return _node_allocator;
	}

	void
	swap(concurrent_skip_list &other)
	{
		pool_base pop = get_pool_base();
		transaction::run(pop, [&] {
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
		my_pool_uuid = pmemobj_oid(this).pool_uuid_lo;
		my_size.get_rw() = 0;
		create_dummy_head();
	}

	void
	internal_move(concurrent_skip_list &&other)
	{
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
			return std::distance(range.first, range.second);
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
		pointer_type curr = next.get(my_pool_uuid);

		while (curr && cmp(get_key(curr), key)) {
			prev = curr;
			assert(level < prev->height());
			next = prev->next(level);
			curr = next.get(my_pool_uuid);
		}

		return next;
	}

	template <typename comparator>
	void
	fill_prev_next_arrays(prev_array_type &prev_nodes,
			      next_array_type &next_nodes, const key_type &key,
			      const comparator &cmp)
	{
		node_ptr prev = dummy_head.get(my_pool_uuid);
		prev_nodes.fill(prev);
		next_nodes.fill(nullptr);

		for (size_type h = prev->height(); h > 0; --h) {
			persistent_node_ptr next =
				internal_find_position(h - 1, prev, key, cmp);
			prev_nodes[h - 1] = prev;
			next_nodes[h - 1] = next;
		}
	}

	template <typename... Args>
	std::pair<iterator, bool>
	internal_insert(Args &&... args)
	{
		persistent_node_ptr new_node =
			create_node(std::forward<Args>(args)...);
		std::pair<iterator, bool> insert_result =
			internal_insert_node(new_node);
		if (!insert_result.second) {
			delete_node(new_node);
		}
		return insert_result;
	}

	std::pair<iterator, bool>
	internal_insert_node(persistent_node_ptr new_node)
	{
		prev_array_type prev_nodes;
		next_array_type next_nodes;
		assert(dummy_head(my_pool_uuid)->height() >=
		       new_node(my_pool_uuid)->height());
		const key_type &new_key = get_key(new_node.get(my_pool_uuid));

		do {
			if (allow_multimapping) {
				fill_prev_next_arrays(
					prev_nodes, next_nodes, new_key,
					[&](const key_type &lhs,
					    const key_type &rhs) {
						return !_compare(rhs, lhs);
					});
			} else {
				fill_prev_next_arrays(prev_nodes, next_nodes,
						      new_key, _compare);
			}

			node_ptr next = next_nodes[0].get(my_pool_uuid);
			if (next && !allow_multimapping &&
			    !_compare(new_key, get_key(next))) {
				// TODO: do we really need to wait?
				while (!next->fully_linked()) {
					// TODO: atomic backoff
				}

				return std::pair<iterator, bool>(
					iterator(my_pool_uuid, next), false);
			}

		} while (!try_insert_node(new_node, prev_nodes, next_nodes));

		assert(new_node);
		return std::pair<iterator, bool>(
			iterator(my_pool_uuid, new_node.get(my_pool_uuid)),
			true);
	}

	bool
	try_insert_node(persistent_node_ptr new_node,
			prev_array_type &prev_nodes,
			next_array_type &next_nodes)
	{
		assert(dummy_head(my_pool_uuid)->height() >=
		       new_node(my_pool_uuid)->height());
		node_ptr n = new_node.get(my_pool_uuid);
		lock_array locks;

		if (!try_lock_nodes(n->height(), prev_nodes, next_nodes,
				    locks)) {
			return false;
		}
		pool_base pop = get_pool_base();
		for (size_type level = 0; level < n->height(); ++level) {
			assert(prev_nodes[level]->height() > level);
			assert(prev_nodes[level]->next(level) ==
			       next_nodes[level]);
			n->set_next(pop, level, next_nodes[level]);
			prev_nodes[level]->set_next(pop, level, new_node);
		}

		n->mark_linked(pop);

		// TODO: remove from persistent TLS

		++(my_size.get_rw());
		pop.persist(my_size);

		return true;
	}

	bool
	try_lock_nodes(size_type height, prev_array_type &prevs,
		       next_array_type &nexts, lock_array &locks)
	{
		for (size_type l = 0; l < height; ++l) {
			if (l == 0 || prevs[l] != prevs[l - 1])
				locks[l] = prevs[l]->acquire();

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
		const_node_ptr prev = dummy_head.get(my_pool_uuid);
		assert(prev->height() > 0);
		persistent_node_ptr next = nullptr;

		for (size_type h = prev->height(); h > 0; --h) {
			next = internal_find_position(h - 1, prev, key, cmp);
		}

		return const_iterator(my_pool_uuid, next.get(my_pool_uuid));
	}

	template <typename K, typename comparator>
	iterator
	internal_get_bound(const K &key, const comparator &cmp)
	{
		node_ptr prev = dummy_head.get(my_pool_uuid);
		assert(prev->height() > 0);
		persistent_node_ptr next = nullptr;

		for (size_type h = prev->height(); h > 0; --h) {
			next = internal_find_position(h - 1, prev, key, cmp);
		}

		return iterator(my_pool_uuid, next.get(my_pool_uuid));
	}

	// Returns an iterator points to previous node and node_ptr to
	// extracted_node
	std::pair<iterator, persistent_node_ptr>
	internal_extract(const_iterator it)
	{
		key_type key = traits_type::get_key(*it);
		assert(dummy_head(my_pool_uuid)->height() > 0);

		prev_array_type prev_nodes;
		next_array_type next_nodes;

		fill_prev_next_arrays(prev_nodes, next_nodes, key, _compare);

		node_ptr erase_node = next_nodes[0].get(my_pool_uuid);

		if (erase_node && !_compare(key, get_key(erase_node))) {
			for (size_type level = 0; level < erase_node->height();
			     ++level) {
				assert(prev_nodes[level]->height() > level);
				assert(next_nodes[level] == next_nodes[0]);
				prev_nodes[level]->set_next(
					level, erase_node->next(level));
			}
			--my_size.get_rw();
			return std::pair<iterator, persistent_node_ptr>(
				iterator(my_pool_uuid, prev_nodes[0]),
				next_nodes[0]);
		}
		return std::pair<iterator, persistent_node_ptr>(end(), nullptr);
	}

private:
	/**
	 * Get the persistent memory pool where hashmap resides.
	 * @returns pmem::obj::pool_base object.
	 */
	pool_base
	get_pool_base() const
	{
		PMEMobjpool *pop = pmemobj_pool_by_ptr(this);
		return pool_base(pop);
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
		clear();
		try {
			for (auto it = first; it != last; ++it)
				insert(*it);
		} catch (...) {
			clear();
			delete_dummy_head();
			throw;
		}
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

		size_type sz = calc_node_size(levels);
		pool_base pop = get_pool_base();
		transaction::manual tx(pop);
		// TODO: we need assign result of allocation to persistent TLS
		persistent_node_ptr node =
			node_allocator_traits::allocate(_node_allocator, sz)
				.raw();
		node_ptr new_node = node.get(my_pool_uuid);
		try {
			node_allocator_traits::construct(_node_allocator,
							 new_node, levels);

		} catch (...) {
			deallocate_node(node, sz);
			throw;
		}

		try {
			node_allocator_traits::construct(
				_node_allocator, new_node->get(),
				std::forward<Args>(args)...);
		} catch (...) {
			node_allocator_traits::destroy(_node_allocator,
						       new_node);
			deallocate_node(node, sz);
			throw;
		}
		transaction::commit();

		return node;
	}

	/**
	 * Creates dummy node.
	 * Always called from ctor.
	 */
	void
	create_dummy_head()
	{
		size_type sz = calc_node_size(MAX_LEVEL);

		dummy_head =
			(node_allocator_traits::allocate(_node_allocator, sz))
				.raw();
		// TODO: investigate linkage fail in debug without this
		// workaround
		auto max_level = MAX_LEVEL;

		try {
			node_allocator_traits::construct(
				_node_allocator, dummy_head.get(my_pool_uuid),
				max_level);
		} catch (...) {
			deallocate_node(dummy_head, sz);
			throw;
		}
	}

	template <bool is_dummy = false>
	void
	delete_node(persistent_node_ptr &node)
	{
		node_ptr n = node.get(my_pool_uuid);
		size_type sz = calc_node_size(n->height());
		pool_base pop = get_pool_base();

		transaction::manual tx(pop);
		// Destroy value
		if (!is_dummy)
			node_allocator_traits::destroy(_node_allocator,
						       n->get());
		// Destroy node
		node_allocator_traits::destroy(_node_allocator, n);
		// Deallocate memory
		deallocate_node(node, sz);
		node = nullptr;
		transaction::commit();
	}

	void
	deallocate_node(persistent_node_ptr &node, size_type sz)
	{
		persistent_ptr<uint8_t> tmp =
			node.get_persistent_ptr(my_pool_uuid).raw();
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
		return iterator(my_pool_uuid,
				const_cast<typename iterator::node_ptr>(
					it.my_node_ptr));
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

	uint64_t my_pool_uuid;
	node_allocator_type _node_allocator;
	key_compare _compare;
	v<random_level_generator_type> _rnd_generator;
	persistent_node_ptr dummy_head;

	p<std::atomic<size_type>> my_size;
}; // class concurrent_skip_list

} // namespace detail
} // namespace pmem

#endif // PMEMOBJ_CONCURRENT_SKIP_LIST_IMPL_HPP
