// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#ifndef PMEMOBJ_CONCURRENT_SKIP_LIST_IMPL_HPP
#define PMEMOBJ_CONCURRENT_SKIP_LIST_IMPL_HPP

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdlib>
#include <mutex> /* for std::unique_lock */
#include <random>
#include <type_traits>

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/enumerable_thread_specific.hpp>
#include <libpmemobj++/detail/life.hpp>
#include <libpmemobj++/detail/persistent_pool_ptr.hpp>
#include <libpmemobj++/detail/template_helpers.hpp>
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmem
{
namespace detail
{

#ifndef NDEBUG
inline void
try_insert_node_finish_marker()
{
}
#endif

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
		 * Valgrind does not understand atomic semantic and reports
		 * false-postives in drd and helgrind tools.
		 */
		for (size_type lev = 0; lev < height_; ++lev) {
			VALGRIND_HG_DISABLE_CHECKING(&get_next(lev),
						     sizeof(node_pointer));
		}
#endif
	}

	skip_list_node(size_type levels, const node_pointer *new_nexts)
	    : height_(levels)
	{
		for (size_type lev = 0; lev < height_; ++lev)
			detail::create<node_pointer>(&get_next(lev),
						     new_nexts[lev]);

		assert(height() == levels);
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
		/*
		 * Valgrind does not understand atomic semantic and reports
		 * false-postives in drd and helgrind tools.
		 */
		for (size_type lev = 0; lev < height_; ++lev) {
			VALGRIND_HG_DISABLE_CHECKING(&get_next(lev),
						     sizeof(node_pointer));
		}
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
	get() noexcept
	{
		return &val;
	}

	const_pointer
	get() const noexcept
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
	set_next(obj::pool_base pop, size_type level, node_pointer next)
	{
		set_next(level, next);
		pop.persist(&get_next(level), sizeof(node_pointer));
	}

	void
	set_nexts(obj::pool_base pop, const node_pointer *new_nexts,
		  size_type h)
	{
		assert(h == height());
		node_pointer *nexts = get_nexts();

		std::copy(new_nexts, new_nexts + h, nexts);
		pop.persist(nexts, sizeof(node_pointer) * h);
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
	using difference_type = std::ptrdiff_t;
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
		return node->get();
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

struct default_random_generator {
	using gen_type = std::mt19937_64;
	using result_type = typename gen_type::result_type;

	size_t
	operator()()
	{
		static thread_local gen_type engine(
			static_cast<size_t>(time(0)));

		return engine();
	}

	static constexpr result_type
	min()
	{
		return gen_type::min();
	}

	static constexpr result_type
	max()
	{
		return gen_type::max();
	}
};

template <typename RndGenerator, size_t MAX_LEVEL>
class geometric_level_generator {
public:
	using rnd_generator_type = RndGenerator;

	static constexpr size_t max_level = MAX_LEVEL;

	size_t
	operator()()
	{
		static rnd_generator_type gen;
		static std::geometric_distribution<size_t> d;

		return (d(gen) % MAX_LEVEL) + 1;
	}
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

	static constexpr size_type MAX_LEVEL = traits_type::max_level;

	using random_level_generator_type = geometric_level_generator<
		typename traits_type::random_generator_type, MAX_LEVEL>;
	using node_allocator_type = typename std::allocator_traits<
		allocator_type>::template rebind_alloc<uint8_t>;
	using node_allocator_traits = typename std::allocator_traits<
		allocator_type>::template rebind_traits<uint8_t>;
	using node_ptr = list_node_type *;
	using const_node_ptr = const list_node_type *;
	using persistent_node_ptr = persistent_pool_ptr<list_node_type>;

	using prev_array_type = std::array<node_ptr, MAX_LEVEL>;
	using next_array_type = std::array<persistent_node_ptr, MAX_LEVEL>;
	using node_lock_type = typename list_node_type::lock_type;
	using lock_array = std::array<node_lock_type, MAX_LEVEL>;

public:
	static constexpr bool allow_multimapping =
		traits_type::allow_multimapping;

	/**
	 * Default constructor. Construct empty skip list.
	 *
	 * @pre must be called in transaction scope.
	 * @throw pmem::pool_error if an object is not in persistent memory.
	 * @throw pmem::transaction_scope_error if constructor wasn't called in
	 * transaction.
	 */
	concurrent_skip_list()
	{
		check_tx_stage_work();
		init();
	}

	/**
	 * Constructs an empty container.
	 *
	 * @param[in] comp comparison function object to use for all comparisons
	 * of keys.
	 * @param[in] alloc allocator to use for all memory allocations of this
	 * container.
	 *
	 * @pre must be called in transaction scope.
	 *
	 * @throw pmem::pool_error if an object is not in persistent memory.
	 * @throw pmem::transaction_scope_error if constructor wasn't called in
	 * transaction.
	 * @throw pmem::transaction_alloc_error when allocating memory for
	 * inserted elements in transaction failed.
	 */
	explicit concurrent_skip_list(
		const key_compare &comp,
		const allocator_type &alloc = allocator_type())
	    : _node_allocator(alloc), _compare(comp)
	{
		check_tx_stage_work();
		init();
	}

	/**
	 * Constructs the container with the contents of the range [first,
	 * last). If multiple elements in the range have keys that compare
	 * equivalent, the first element is inserted.
	 *
	 * @param[in] first first iterator of inserted range.
	 * @param[in] last last iterator of inserted range.
	 * @param[in] comp comparison function object to use for all comparisons
	 * of keys.
	 * @param[in] alloc allocator to use for all memory allocations of this
	 * container.
	 *
	 * InputIt must meet the requirements of LegacyInputIterator.
	 *
	 * @pre must be called in transaction scope.
	 *
	 * @throw pmem::pool_error if an object is not in persistent memory.
	 * @throw pmem::transaction_scope_error if constructor wasn't called in
	 * transaction.
	 * @throw pmem::transaction_alloc_error when allocating memory for
	 * inserted elements in transaction failed.
	 * @throw rethrows element constructor exception.
	 */
	template <class InputIt>
	concurrent_skip_list(InputIt first, InputIt last,
			     const key_compare &comp = key_compare(),
			     const allocator_type &alloc = allocator_type())
	    : _node_allocator(alloc), _compare(comp)
	{
		check_tx_stage_work();
		init();
		internal_copy(first, last);
	}

	/**
	 * Copy constructor. Constructs the container with the copy of the
	 * contents of other.
	 *
	 * @param[in] other reference to the concurrent_skip_list to be copied.
	 *
	 * @pre must be called in transaction scope.
	 *
	 * @post size() == other.size()
	 *
	 * @throw pmem::pool_error if an object is not in persistent memory.
	 * @throw pmem::transaction_alloc_error when allocating memory for
	 * copied elements in transaction failed.
	 * @throw pmem::transaction_scope_error if constructor wasn't called in
	 * transaction.
	 * @throw rethrows element constructor exception.
	 */
	concurrent_skip_list(const concurrent_skip_list &other)
	    : _node_allocator(node_allocator_traits::
				      select_on_container_copy_construction(
					      other._node_allocator)),
	      _compare(other._compare),
	      _rnd_generator(other._rnd_generator)
	{
		check_tx_stage_work();
		init();
		internal_copy(other);
		assert(_size == other._size);
	}

	/**
	 * Copy constructor. Constructs the container with the copy of the
	 * contents of other.
	 *
	 * @param[in] other reference to the concurrent_skip_list to be copied.
	 * @param[in] alloc allocator to use for all memory allocations of this
	 * container.
	 *
	 * @pre must be called in transaction scope.
	 *
	 * @post size() == other.size()
	 *
	 * @throw pmem::pool_error if an object is not in persistent memory.
	 * @throw pmem::transaction_alloc_error when allocating memory for
	 * copied elements in transaction failed.
	 * @throw pmem::transaction_scope_error if constructor wasn't called in
	 * transaction.
	 * @throw rethrows element constructor exception.
	 */
	concurrent_skip_list(const concurrent_skip_list &other,
			     const allocator_type &alloc)
	    : _node_allocator(alloc),
	      _compare(other._compare),
	      _rnd_generator(other._rnd_generator)
	{
		check_tx_stage_work();
		init();
		internal_copy(other);
		assert(_size == other._size);
	}

	/**
	 * Move constructor. Constructs the container with the contents of other
	 * using move semantics. Allocator is obtained by move-construction from
	 * the allocator belonging to other
	 *
	 * @param[in] other reference to the concurrent_skip_list to be copied.
	 *
	 * @pre must be called in transaction scope.
	 *
	 * @post size() == other.size()
	 *
	 * @throw pmem::pool_error if an object is not in persistent memory.
	 * @throw pmem::transaction_alloc_error when allocating memory for
	 * copied elements in transaction failed.
	 * @throw pmem::transaction_scope_error if constructor wasn't called in
	 * transaction.
	 * @throw rethrows element constructor exception.
	 */
	concurrent_skip_list(concurrent_skip_list &&other)
	    : _node_allocator(std::move(other._node_allocator)),
	      _compare(other._compare),
	      _rnd_generator(other._rnd_generator)
	{
		check_tx_stage_work();
		init();
		internal_move(std::move(other));
	}

	/**
	 * Move constructor. Constructs the container with the contents of other
	 * using move semantics.
	 *
	 * @param[in] other reference to the concurrent_skip_list to be copied.
	 * @param[in] alloc allocator to use for all memory allocations of this
	 * container.
	 *
	 * @pre must be called in transaction scope.
	 *
	 * @post size() == other.size()
	 *
	 * @throw pmem::pool_error if an object is not in persistent memory.
	 * @throw pmem::transaction_alloc_error when allocating memory for
	 * copied elements in transaction failed.
	 * @throw pmem::transaction_scope_error if constructor wasn't called in
	 * transaction.
	 * @throw rethrows element constructor exception.
	 */
	concurrent_skip_list(concurrent_skip_list &&other,
			     const allocator_type &alloc)
	    : _node_allocator(alloc),
	      _compare(other._compare),
	      _rnd_generator(other._rnd_generator)
	{
		check_tx_stage_work();
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
	 * Intialize concurrent_skip_list after process restart.
	 * MUST be called everytime after process restart.
	 * Not thread safe.
	 *
	 */
	void
	runtime_initialize()
	{
#if LIBPMEMOBJ_CPP_VG_PMEMCHECK_ENABLED
		/*
		 * We do not need to add _size to TX.
		 * On restart we restore the size using data from TLS.
		 */
		VALGRIND_PMC_REMOVE_PMEM_MAPPING(&_size, sizeof(_size));
#endif

		tls_restore();

		assert(this->size() ==
		       size_type(std::distance(this->begin(), this->end())));
	}

	/**
	 * Destructor.
	 */
	~concurrent_skip_list()
	{
		auto pop = get_pool_base();
		obj::transaction::run(pop, [&] {
			clear();
			delete_dummy_head();
		});
	}

	/**
	 * Copy assignment operator. Replaces the contents with a copy of the
	 * contents of other transactionally. If
	 * std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value
	 * is true, the target allocator is replaced by a copy of the source
	 * allocator.
	 *
	 * @post size() == other.size()
	 *
	 * @throw pmem::transaction_alloc_error when allocating new memory
	 * failed.
	 * @throw pmem::transaction_free_error when freeing old existing
	 * elements failed.
	 * @throw rethrows constructor exception.
	 */
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
			_rnd_generator = other._rnd_generator;

			internal_copy(other);
		});
		return *this;
	}

	/**
	 * Move assignment operator. Replaces the contents with those of other
	 * using move semantics (i.e. the data in other is moved from other into
	 * this container). other is in a valid but unspecified state
	 * afterwards. If
	 * std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value
	 * is true, the target allocator is replaced by a copy of the source
	 * allocator. If it is false and the source and the target allocators do
	 * not compare equal, the target cannot take ownership of the source
	 * memory and must move-assign each element individually, allocating
	 * additional memory using its own allocator as needed. In any case, all
	 * elements originally present in *this are either destroyed or replaced
	 * by elementwise move-assignment.
	 *
	 * @throw pmem::transaction_alloc_error when allocating new memory
	 * failed.
	 * @throw pmem::transaction_free_error when freeing old existing
	 * elements failed.
	 * @throw rethrows constructor exception.
	 */
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

	/**
	 * Replaces the contents with those identified by initializer list il.
	 *
	 * @param[in] il initializer list to use as data source
	 *
	 * @throw pmem::transaction_alloc_error when allocating new memory
	 * failed.
	 * @throw pmem::transaction_free_error when freeing old existing
	 * elements failed.
	 * @throw rethrows constructor exception.
	 */
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

	/**
	 * Inserts value. No iterators or references are invalidated.
	 *
	 * @param[in] value element value to insert.
	 *
	 * @return a pair consisting of an iterator to the inserted element (or
	 * to the element that prevented the insertion) and a bool denoting
	 * whether the insertion took place.
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 * @throw pmem::transaction_alloc_error when allocating new memory
	 * failed.
	 * @throw rethrows constructor exception.
	 */
	std::pair<iterator, bool>
	insert(const value_type &value)
	{
		return internal_insert(value.first, value);
	}

	/**
	 * Inserts value. No iterators or references are invalidated.
	 * This overload is equivalent to emplace(std::forward<P>(value)) and
	 * only participates in overload resolution if
	 * std::is_constructible<value_type, P&&>::value == true.
	 *
	 * @param[in] value element value to insert.
	 *
	 * @return a pair consisting of an iterator to the inserted element (or
	 * to the element that prevented the insertion) and a bool denoting
	 * whether the insertion took place.
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 * @throw pmem::transaction_alloc_error when allocating new memory
	 * failed.
	 * @throw rethrows constructor exception.
	 */
	template <typename P,
		  typename std::enable_if<
			  std::is_constructible<value_type, P &&>::value>::type>
	std::pair<iterator, bool>
	insert(P &&value)
	{
		return emplace(std::forward<P>(value));
	}

	/**
	 * Inserts value using move semantic. No iterators or references are
	 * invalidated.
	 *
	 * @param[in] value element value to insert.
	 *
	 * @return a pair consisting of an iterator to the inserted element (or
	 * to the element that prevented the insertion) and a bool denoting
	 * whether the insertion took place.
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 * @throw pmem::transaction_alloc_error when allocating new memory
	 * failed.
	 * @throw rethrows constructor exception.
	 */
	std::pair<iterator, bool>
	insert(value_type &&value)
	{
		return internal_insert(value.first, std::move(value));
	}

	/**
	 * Inserts value in the position as close as possible, just prior to
	 * hint. No iterators or references are invalidated.
	 *
	 * @param[in] hint iterator to the position before which the new element
	 * will be inserted.
	 * @param[in] value element value to insert.
	 *
	 * @return an iterator to the inserted element, or to the element that
	 * prevented the insertion.
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 * @throw pmem::transaction_alloc_error when allocating new memory
	 * failed.
	 * @throw rethrows constructor exception.
	 */
	iterator
	insert(const_iterator hint, const_reference value)
	{
		/* Ignore hint */
		return insert(value).first;
	}

	/**
	 * Inserts value in the position as close as possible, just prior to
	 * hint. No iterators or references are invalidated.
	 * This overload is equivalent to emplace_hint(hint,
	 * std::forward<P>(value)) and only participates in overload resolution
	 * if std::is_constructible<value_type, P&&>::value == true.
	 *
	 * @param[in] hint iterator to the position before which the new element
	 * will be inserted.
	 * @param[in] value element value to insert.
	 *
	 * @return an iterator to the inserted element, or to the element that
	 * prevented the insertion.
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 * @throw pmem::transaction_alloc_error when allocating new memory
	 * failed.
	 * @throw rethrows constructor exception.
	 */
	template <typename P,
		  typename std::enable_if<
			  std::is_constructible<value_type, P &&>::value>::type>
	iterator
	insert(const_iterator hint, P &&value)
	{
		return emplace_hint(hint, std::forward<P>(value));
	}

	/**
	 * Inserts elements from range [first, last). If multiple elements in
	 * the range have keys that compare equivalent, the first one is
	 * inserted.
	 *
	 * @param[in] first first iterator of inserted range.
	 * @param[in] last last iterator of inserted range.
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 * @throw pmem::transaction_alloc_error when allocating new memory
	 * failed.
	 * @throw rethrows constructor exception.
	 */
	template <typename InputIterator>
	void
	insert(InputIterator first, InputIterator last)
	{
		for (InputIterator it = first; it != last; ++it)
			insert(*it);
	}

	/**
	 * Inserts elements from initializer list ilist. If multiple elements in
	 * the range have keys that compare equivalent, the first one is
	 * inserted.
	 *
	 * @param[in] ilist first initializer list to insert the values from.
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 * @throw pmem::transaction_alloc_error when allocating new memory
	 * failed.
	 * @throw rethrows constructor exception.
	 */
	void
	insert(std::initializer_list<value_type> ilist)
	{
		insert(ilist.begin(), ilist.end());
	}

	/**
	 * Inserts a new element into the container constructed in-place with
	 * the given args if there is no element with the key in the container.
	 *
	 * Careful use of emplace allows the new element to be constructed while
	 * avoiding unnecessary copy or move operations. The constructor of the
	 * new element (i.e. std::pair<const Key, T>) is called with exactly the
	 * same arguments as supplied to emplace, forwarded via
	 * std::forward<Args>(args).... The element may be constructed even if
	 * there already is an element with the key in the container, in which
	 * case the newly constructed element will be destroyed immediately.
	 *
	 * No iterators or references are invalidated.
	 *
	 * @param[in] args arguments to forward to the constructor of the
	 * element
	 *
	 * @return a pair consisting of an iterator to the inserted element (or
	 * to the element that prevented the insertion) and a bool denoting
	 * whether the insertion took place.
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 * @throw pmem::transaction_alloc_error when allocating new memory
	 * failed.
	 * @throw rethrows constructor exception.
	 */
	template <typename... Args>
	std::pair<iterator, bool>
	emplace(Args &&... args)
	{
		return internal_emplace(std::forward<Args>(args)...);
	}

	/**
	 * Inserts a new element to the container as close as possible to the
	 * position just before hint. The element is constructed in-place, i.e.
	 * no copy or move operations are performed.
	 *
	 * The constructor of the element type (value_type, that is,
	 * std::pair<const Key, T>) is called with exactly the same arguments as
	 * supplied to the function, forwarded with std::forward<Args>(args)...
	 *
	 * No iterators or references are invalidated.
	 *
	 * @param[in] hint iterator to the position before which the new element
	 * will be inserted.
	 * @param[in] args arguments to forward to the constructor of the
	 * element.
	 *
	 * @return Returns an iterator to the newly inserted element.
	 *
	 * If the insertion failed because the element already exists, returns
	 * an iterator to the already existing element with the equivalent key.
	 *
	 * @return an iterator to the inserted element, or to the element that
	 * prevented the insertion.
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 * @throw pmem::transaction_alloc_error when allocating new memory
	 * failed.
	 * @throw rethrows constructor exception.
	 */
	template <typename... Args>
	iterator
	emplace_hint(const_iterator hint, Args &&... args)
	{
		/* Ignore hint */
		return emplace(std::forward<Args>(args)...).first;
	}

	/**
	 * If a key equivalent to k already exists in the container, does
	 * nothing. Otherwise, behaves like emplace except that the element is
	 * constructed as value_type(std::piecewise_construct,
	 * std::forward_as_tuple(k),
	 * std::forward_as_tuple(std::forward<Args>(args)...))
	 *
	 * No iterators or references are invalidated.
	 *
	 * @param[in] k the key used both to look up and to insert if not found.
	 * @param[in] args arguments to forward to the constructor of the
	 * element.
	 *
	 * @return a pair consisting of an iterator to the inserted element (or
	 * to the element that prevented the insertion) and a bool denoting
	 * whether the insertion took place.
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 * @throw pmem::transaction_alloc_error when allocating new memory
	 * failed.
	 * @throw rethrows constructor exception.
	 */
	template <typename... Args>
	std::pair<iterator, bool>
	try_emplace(const key_type &k, Args &&... args)
	{
		return internal_try_emplace(k, std::forward<Args>(args)...);
	}

	/**
	 * If a key equivalent to k already exists in the container, does
	 * nothing. Otherwise, behaves like emplace except that the element is
	 * constructed as value_type(std::piecewise_construct,
	 * std::forward_as_tuple(std::move(k)),
	 * std::forward_as_tuple(std::forward<Args>(args)...)).
	 *
	 * No iterators or references are invalidated.
	 *
	 * @param[in] k the key used both to look up and to insert if not found.
	 * @param[in] args arguments to forward to the constructor of the
	 * element.
	 *
	 * @return a pair consisting of an iterator to the inserted element (or
	 * to the element that prevented the insertion) and a bool denoting
	 * whether the insertion took place.
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 * @throw pmem::transaction_alloc_error when allocating new memory
	 * failed.
	 * @throw rethrows constructor exception.
	 */
	template <typename... Args>
	std::pair<iterator, bool>
	try_emplace(key_type &&k, Args &&... args)
	{
		return internal_try_emplace(std::move(k),
					    std::forward<Args>(args)...);
	}

	/**
	 * If a key equivalent to k already exists in the container, does
	 * nothing. Otherwise, behaves like emplace except that the element is
	 * constructed as value_type(std::piecewise_construct,
	 * std::forward_as_tuple(std::move(k)),
	 * std::forward_as_tuple(std::forward<Args>(args)...)).
	 * This overload only participates in overload resolution if the
	 * qualified-id Compare::is_transparent is valid and denotes a type and
	 * std::is_constructible<Key, K &&>::value == true . It allows calling
	 * this function without constructing an instance of Key.
	 *
	 * No iterators or references are invalidated.
	 *
	 * @param[in] k the key used both to look up and to insert if not found.
	 * @param[in] args arguments to forward to the constructor of the
	 * element.
	 *
	 * @return a pair consisting of an iterator to the inserted element (or
	 * to the element that prevented the insertion) and a bool denoting
	 * whether the insertion took place.
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 * @throw pmem::transaction_alloc_error when allocating new memory
	 * failed.
	 * @throw rethrows constructor exception.
	 */
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

	/**
	 * Removes the element at pos from the container.
	 * References and iterators to the erased elements are invalidated.
	 * Other references and iterators are not affected.
	 *
	 * @pre The iterator pos must be valid and dereferenceable. Thus the
	 * end() iterator (which is valid, but is not dereferenceable) cannot be
	 * used as a value for pos.
	 *
	 * @param[in] pos iterator to the element to remove.
	 *
	 * @return iterator following the removed element.
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 * @throw rethrows destructor exception.
	 */
	iterator
	unsafe_erase(iterator pos)
	{
		auto &size_diff = tls_data.local().size_diff;
		return internal_erase(pos, size_diff);
	}

	/**
	 * Removes the element at pos from the container.
	 * References and iterators to the erased elements are invalidated.
	 * Other references and iterators are not affected.
	 *
	 * @pre The iterator pos must be valid and dereferenceable. Thus the
	 * end() iterator (which is valid, but is not dereferenceable) cannot be
	 * used as a value for pos.
	 *
	 * @param[in] pos iterator to the element to remove.
	 *
	 * @return iterator following the removed element.
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 * @throw rethrows destructor exception.
	 */
	iterator
	unsafe_erase(const_iterator pos)
	{
		return unsafe_erase(get_iterator(pos));
	}

	/**
	 * Removes the elements in the range [first; last), which must be a
	 * valid range in *this.
	 * References and iterators to the erased elements are invalidated.
	 * Other references and iterators are not affected.
	 *
	 * @param[in] first first iterator in the range of elements to remove.
	 * @param[in] last last iterator in the range of elements to remove.
	 *
	 * @return iterator following the last removed element.
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 * @throw rethrows destructor exception.
	 */
	iterator
	unsafe_erase(const_iterator first, const_iterator last)
	{
		obj::pool_base pop = get_pool_base();
		auto &size_diff = tls_data.local().size_diff;

		obj::transaction::run(pop, [&] {
			while (first != last) {
				first = internal_erase(first, size_diff);
			}
		});

		return get_iterator(first);
	}

	/**
	 * Removes the element (if one exists) with the key equivalent to key.
	 * References and iterators to the erased elements are invalidated.
	 * Other references and iterators are not affected.
	 *
	 * @param[in] key key value of the elements to remove.
	 *
	 * @return Number of elements removed.
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 * @throw rethrows destructor exception.
	 */
	size_type
	unsafe_erase(const key_type &key)
	{
		std::pair<iterator, iterator> range = equal_range(key);
		size_type sz = static_cast<size_type>(
			std::distance(range.first, range.second));
		unsafe_erase(range.first, range.second);
		return sz;
	}

	/**
	 * Removes the element (if one exists) with the key equivalent to key.
	 * References and iterators to the erased elements are invalidated.
	 * Other references and iterators are not affected.
	 * This overload only participates in overload resolution if the
	 * qualified-id Compare::is_transparent is valid and denotes a type and
	 * std::is_convertible<K, iterator>::value != true &&
	 * std::is_convertible<K, const_iterator>::value != true.
	 * It allows calling this function without constructing an instance of
	 * Key.
	 *
	 * @param[in] key key value of the elements to remove.
	 *
	 * @return Number of elements removed.
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 * @throw rethrows destructor exception.
	 */
	template <
		typename K,
		typename = typename std::enable_if<
			has_is_transparent<key_compare>::value &&
				!std::is_convertible<K, iterator>::value &&
				!std::is_convertible<K, const_iterator>::value,
			K>::type>
	size_type
	unsafe_erase(const K &key)
	{
		std::pair<iterator, iterator> range = equal_range(key);
		size_type sz = static_cast<size_type>(
			std::distance(range.first, range.second));
		unsafe_erase(range.first, range.second);
		return sz;
	}

	/**
	 * Returns an iterator pointing to the first element that is not less
	 * than (i.e. greater or equal to) key.
	 *
	 * @param[in] key key value to compare the elements to.
	 *
	 * @return Iterator pointing to the first element that is not less than
	 * key. If no such element is found, a past-the-end iterator is
	 * returned.
	 */
	iterator
	lower_bound(const key_type &key)
	{
		return internal_get_bound(key, _compare);
	}

	/**
	 * Returns an iterator pointing to the first element that is not less
	 * than (i.e. greater or equal to) key.
	 *
	 * @param[in] key key value to compare the elements to.
	 *
	 * @return Iterator pointing to the first element that is not less than
	 * key. If no such element is found, a past-the-end iterator is
	 * returned.
	 */
	const_iterator
	lower_bound(const key_type &key) const
	{
		return internal_get_bound(key, _compare);
	}

	/**
	 * Returns an iterator pointing to the first element that compares not
	 * less (i.e. greater or equal) to the value x. This overload only
	 * participates in overload resolution if the qualified-id
	 * Compare::is_transparent is valid and denotes a type. They allow
	 * calling this function without constructing an instance of Key.
	 *
	 * @param[in] x alternative value that can be compared to Key.
	 *
	 * @return Iterator pointing to the first element that is not less than
	 * key. If no such element is found, a past-the-end iterator is
	 * returned.
	 */
	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	iterator
	lower_bound(const K &x)
	{
		return internal_get_bound(x, _compare);
	}

	/**
	 * Returns an iterator pointing to the first element that compares not
	 * less (i.e. greater or equal) to the value x. This overload only
	 * participates in overload resolution if the qualified-id
	 * Compare::is_transparent is valid and denotes a type. They allow
	 * calling this function without constructing an instance of Key.
	 *
	 * @param[in] x alternative value that can be compared to Key.
	 *
	 * @return Iterator pointing to the first element that is not less than
	 * key. If no such element is found, a past-the-end iterator is
	 * returned.
	 */
	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	const_iterator
	lower_bound(const K &x) const
	{
		return internal_get_bound(x, _compare);
	}

	/**
	 * Returns an iterator pointing to the first element that is greater
	 * than key.
	 *
	 * @param[in] key key value to compare the elements to.
	 *
	 * @return Iterator pointing to the first element that is greater than
	 * key. If no such element is found, a past-the-end iterator is
	 * returned.
	 */
	iterator
	upper_bound(const key_type &key)
	{
		return internal_get_bound(key, not_greater_compare(_compare));
	}

	/**
	 * Returns an iterator pointing to the first element that is greater
	 * than key.
	 *
	 * @param[in] key key value to compare the elements to.
	 *
	 * @return Iterator pointing to the first element that is greater than
	 * key. If no such element is found, a past-the-end iterator is
	 * returned.
	 */
	const_iterator
	upper_bound(const key_type &key) const
	{
		return internal_get_bound(key, not_greater_compare(_compare));
	}

	/**
	 * Returns an iterator pointing to the first element that compares
	 * greater to the value x. This overload only participates in overload
	 * resolution if the qualified-id Compare::is_transparent is valid and
	 * denotes a type. They allow calling this function without constructing
	 * an instance of Key.
	 *
	 * @param[in] x alternative value that can be compared to Key.
	 *
	 * @return Iterator pointing to the first element that is greater than
	 * key. If no such element is found, a past-the-end iterator is
	 * returned.
	 */
	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	iterator
	upper_bound(const K &x)
	{
		return internal_get_bound(x, not_greater_compare(_compare));
	}

	/**
	 * Returns an iterator pointing to the first element that compares
	 * greater to the value x. This overload only participates in overload
	 * resolution if the qualified-id Compare::is_transparent is valid and
	 * denotes a type. They allow calling this function without constructing
	 * an instance of Key.
	 *
	 * @param[in] x alternative value that can be compared to Key.
	 *
	 * @return Iterator pointing to the first element that is greater than
	 * key. If no such element is found, a past-the-end iterator is
	 * returned.
	 */
	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	const_iterator
	upper_bound(const K &x) const
	{
		return internal_get_bound(x, not_greater_compare(_compare));
	}

	/**
	 * Finds an element with key equivalent to key.
	 *
	 * @param[in] key key value of the element to search for.
	 *
	 * @return Iterator to an element with key equivalent to key. If no such
	 * element is found, past-the-end iterator is returned.
	 */
	iterator
	find(const key_type &key)
	{
		return internal_find(key);
	}

	/**
	 * Finds an element with key equivalent to key.
	 *
	 * @param[in] key key value of the element to search for.
	 *
	 * @return Iterator to an element with key equivalent to key. If no such
	 * element is found, past-the-end iterator is returned.
	 */
	const_iterator
	find(const key_type &key) const
	{
		return internal_find(key);
	}

	/**
	 * Finds an element with key that compares equivalent to the value x.
	 * This overload only participates in overload resolution if the
	 * qualified-id Compare::is_transparent is valid and denotes a type. It
	 * allows calling this function without constructing an instance of Key.
	 *
	 * @param[in] x a value of any type that can be transparently compared
	 * with a key.
	 *
	 * @return Iterator to an element with key equivalent to key. If no such
	 * element is found, past-the-end iterator is returned.
	 */
	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	iterator
	find(const K &x)
	{
		return internal_find(x);
	}

	/**
	 * Finds an element with key that compares equivalent to the value x.
	 * This overload only participates in overload resolution if the
	 * qualified-id Compare::is_transparent is valid and denotes a type. It
	 * allows calling this function without constructing an instance of Key.
	 *
	 * @param[in] x a value of any type that can be transparently compared
	 * with a key.
	 *
	 * @return Iterator to an element with key equivalent to key. If no such
	 * element is found, past-the-end iterator is returned.
	 */
	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	const_iterator
	find(const K &x) const
	{
		return internal_find(x);
	}

	/**
	 * Returns the number of elements with key that compares equivalent to
	 * the specified argument.
	 *
	 * @param[in] key key value of the element to count.
	 *
	 * @return Number of elements with key that compares equivalent to the
	 * specified argument.
	 */
	size_type
	count(const key_type &key) const
	{
		return internal_count(key);
	}

	/**
	 * Returns the number of elements with key that compares equivalent to
	 * the specified argument. This overload only participates in overload
	 * resolution if the qualified-id Compare::is_transparent is valid and
	 * denotes a type. They allow calling this function without constructing
	 * an instance of Key.
	 *
	 * @param[in] x alternative value to compare to the keys.
	 *
	 * @return Number of elements with key that compares equivalent to the
	 * specified argument.
	 */
	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	size_type
	count(const K &key) const
	{
		return internal_count(key);
	}

	/**
	 * Checks if there is an element with key equivalent to key in the
	 * container.
	 *
	 * @param[in] key key value of the element to search for.
	 *
	 * @return true if there is such an element, otherwise false.
	 */
	bool
	contains(const key_type &key) const
	{
		return find(key) != end();
	}

	/**
	 * Checks if there is an element with key that compares equivalent to
	 * the value x. This overload only participates in overload resolution
	 * if the qualified-id Compare::is_transparent is valid and denotes a
	 * type. It allows calling this function without constructing an
	 * instance of Key.
	 *
	 * @param[in] x a value of any type that can be transparently compared
	 * with a key.
	 *
	 * @return true if there is such an element, otherwise false.
	 */
	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	bool
	contains(const K &x) const
	{
		return find(x) != end();
	}

	/**
	 * Erases all elements from the container transactionally.
	 *
	 * @post size() == 0
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 * @throw rethrows destructor exception.
	 */
	void
	clear()
	{
		assert(dummy_head(pool_uuid)->height() > 0);
		obj::pool_base pop = get_pool_base();

		persistent_node_ptr current = dummy_head(pool_uuid)->next(0);

		obj::transaction::run(pop, [&] {
			while (current) {
				assert(current(pool_uuid)->height() > 0);
				persistent_node_ptr next =
					current(pool_uuid)->next(0);
				delete_node(current);
				current = next;
			}

			node_ptr head = dummy_head.get(pool_uuid);
			for (size_type i = 0; i < head->height(); ++i) {
				head->set_next(i, nullptr);
			}

			on_init_size = 0;
			tls_data.clear();
		});

		_size = 0;
	}

	/**
	 * Returns an iterator to the first element of the container.
	 * If the map is empty, the returned iterator will be equal to end().
	 *
	 * @return Iterator to the first element.
	 */
	iterator
	begin()
	{
		return iterator(
			pool_uuid,
			dummy_head.get(pool_uuid)->next(0).get(pool_uuid));
	}

	/**
	 * Returns an iterator to the first element of the container.
	 * If the map is empty, the returned iterator will be equal to end().
	 *
	 * @return Iterator to the first element.
	 */
	const_iterator
	begin() const
	{
		return const_iterator(
			pool_uuid,
			dummy_head.get(pool_uuid)->next(0).get(pool_uuid));
	}

	/**
	 * Returns an iterator to the first element of the container.
	 * If the map is empty, the returned iterator will be equal to end().
	 *
	 * @return Iterator to the first element.
	 */
	const_iterator
	cbegin() const
	{
		return const_iterator(
			pool_uuid,
			dummy_head.get(pool_uuid)->next(0).get(pool_uuid));
	}

	/**
	 * Returns an iterator to the element following the last element of the
	 * map. This element acts as a placeholder; attempting to access it
	 * results in undefined behavior.
	 *
	 * @return Iterator to the element following the last element.
	 */
	iterator
	end()
	{
		return iterator(pool_uuid, nullptr);
	}

	/**
	 * Returns an iterator to the element following the last element of the
	 * map. This element acts as a placeholder; attempting to access it
	 * results in undefined behavior.
	 *
	 * @return Iterator to the element following the last element.
	 */
	const_iterator
	end() const
	{
		return const_iterator(pool_uuid, nullptr);
	}

	/**
	 * Returns an iterator to the element following the last element of the
	 * map. This element acts as a placeholder; attempting to access it
	 * results in undefined behavior.
	 *
	 * @return Iterator to the element following the last element.
	 */
	const_iterator
	cend() const
	{
		return const_iterator(pool_uuid, nullptr);
	}

	/**
	 * Returns the number of elements in the container, i.e.
	 * std::distance(begin(), end()).
	 *
	 * @return The number of elements in the container.
	 */
	size_type
	size() const
	{
		return _size.load(std::memory_order_relaxed);
	}

	/**
	 * Returns the maximum number of elements the container is able to hold
	 * due to system or library implementation limitations, i.e.
	 * std::distance(begin(), end()) for the largest container.
	 *
	 * @return Maximum number of elements.
	 */
	size_type
	max_size() const
	{
		return _node_allocator.max_size();
	}

	/**
	 * Checks if the container has no elements, i.e. whether begin() ==
	 * end().
	 *
	 * @return true if the container is empty, false otherwise.
	 */
	bool
	empty() const
	{
		return 0 == size();
	}

	/**
	 * Returns a const reference to the allocator associated with the
	 * container.
	 *
	 * @return Const reference to the associated allocator.
	 */
	const allocator_type &
	get_allocator() const
	{
		return _node_allocator;
	}

	/**
	 * Returns a reference to the allocator associated with the container.
	 *
	 * @return Reference to the associated allocator.
	 */
	allocator_type &
	get_allocator()
	{
		return _node_allocator;
	}

	/**
	 * Exchanges the contents of the container with those of other
	 * transactionally. Does not invoke any move, copy, or swap operations
	 * on individual elements.
	 *
	 * @throw pmem::transaction_error when snapshotting failed.
	 */
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
			on_init_size.swap(other.on_init_size);
		});

		_size = other._size.exchange(_size, std::memory_order_relaxed);
	}

	/**
	 * Returns a range containing all elements with the given key in the
	 * container. The range is defined by two iterators, one pointing to the
	 * first element that is not less than key and another pointing to the
	 * first element greater than key. Alternatively, the first iterator may
	 * be obtained with lower_bound(), and the second with upper_bound().
	 *
	 * Compares the keys to key.
	 *
	 * @param[in] key key value to compare the elements to.
	 *
	 * @return std::pair containing a pair of iterators defining the wanted
	 * range: the first pointing to the first element that is not less than
	 * key and the second pointing to the first element greater than key. If
	 * there are no elements not less than key, past-the-end (see end())
	 * iterator is returned as the first element. Similarly if there are no
	 * elements greater than key, past-the-end iterator is returned as the
	 * second element.
	 */
	std::pair<iterator, iterator>
	equal_range(const key_type &key)
	{
		return std::pair<iterator, iterator>(lower_bound(key),
						     upper_bound(key));
	}

	/**
	 * Returns a range containing all elements with the given key in the
	 * container. The range is defined by two iterators, one pointing to the
	 * first element that is not less than key and another pointing to the
	 * first element greater than key. Alternatively, the first iterator may
	 * be obtained with lower_bound(), and the second with upper_bound().
	 *
	 * Compares the keys to key.
	 *
	 * @param[in] key key value to compare the elements to.
	 *
	 * @return std::pair containing a pair of iterators defining the wanted
	 * range: the first pointing to the first element that is not less than
	 * key and the second pointing to the first element greater than key. If
	 * there are no elements not less than key, past-the-end (see end())
	 * iterator is returned as the first element. Similarly if there are no
	 * elements greater than key, past-the-end iterator is returned as the
	 * second element.
	 */
	std::pair<const_iterator, const_iterator>
	equal_range(const key_type &key) const
	{
		return std::pair<const_iterator, const_iterator>(
			lower_bound(key), upper_bound(key));
	}

	/**
	 * Returns a range containing all elements with the given key in the
	 * container. The range is defined by two iterators, one pointing to the
	 * first element that is not less than key and another pointing to the
	 * first element greater than key. Alternatively, the first iterator may
	 * be obtained with lower_bound(), and the second with upper_bound().
	 *
	 * Compares the keys to the value x. This overload only participates in
	 * overload resolution if the qualified-id Compare::is_transparent is
	 * valid and denotes a type. They allow calling this function without
	 * constructing an instance of Key.
	 *
	 * @param[in] x alternative value that can be compared to Key.
	 *
	 * @return std::pair containing a pair of iterators defining the wanted
	 * range: the first pointing to the first element that is not less than
	 * key and the second pointing to the first element greater than key. If
	 * there are no elements not less than key, past-the-end (see end())
	 * iterator is returned as the first element. Similarly if there are no
	 * elements greater than key, past-the-end iterator is returned as the
	 * second element.
	 */
	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	std::pair<iterator, iterator>
	equal_range(const K &x)
	{
		return std::pair<iterator, iterator>(lower_bound(x),
						     upper_bound(x));
	}

	/**
	 * Returns a range containing all elements with the given key in the
	 * container. The range is defined by two iterators, one pointing to the
	 * first element that is not less than key and another pointing to the
	 * first element greater than key. Alternatively, the first iterator may
	 * be obtained with lower_bound(), and the second with upper_bound().
	 *
	 * Compares the keys to the value x. This overload only participates in
	 * overload resolution if the qualified-id Compare::is_transparent is
	 * valid and denotes a type. They allow calling this function without
	 * constructing an instance of Key.
	 *
	 * @param[in] x alternative value that can be compared to Key.
	 *
	 * @return std::pair containing a pair of iterators defining the wanted
	 * range: the first pointing to the first element that is not less than
	 * key and the second pointing to the first element greater than key. If
	 * there are no elements not less than key, past-the-end (see end())
	 * iterator is returned as the first element. Similarly if there are no
	 * elements greater than key, past-the-end iterator is returned as the
	 * second element.
	 */
	template <typename K,
		  typename = typename std::enable_if<
			  has_is_transparent<key_compare>::value, K>::type>
	std::pair<const_iterator, const_iterator>
	equal_range(const K &key) const
	{
		return std::pair<const_iterator, const_iterator>(
			lower_bound(key), upper_bound(key));
	}

	/**
	 * Returns a const reference to the object that compares the keys.
	 *
	 * @return Const reference to the key comparison function object.
	 */
	const key_compare &
	key_comp() const
	{
		return _compare;
	}

	/**
	 * Returns a reference to the object that compares the keys.
	 *
	 * @return Reference to the key comparison function object.
	 */
	key_compare &
	key_comp()
	{
		return _compare;
	}

private:
	/* Status flags stored in insert_stage field */
	enum insert_stage_type : uint8_t { not_started = 0, in_progress = 1 };
	/*
	 * Structure of thread local data.
	 * Size should be 64 bytes.
	 */
	struct tls_entry_type {
		persistent_node_ptr ptr;
		obj::p<difference_type> size_diff;
		obj::p<insert_stage_type> insert_stage;

		char reserved[64 - sizeof(ptr) - sizeof(size_diff) -
			      sizeof(insert_stage)];
	};
	static_assert(sizeof(tls_entry_type) == 64,
		      "The size of tls_entry_type should be 64 bytes.");

	/**
	 * Private helper function. Checks if current transaction stage is equal
	 * to TX_STAGE_WORK and throws an exception otherwise.
	 *
	 * @throw pmem::transaction_scope_error if current transaction stage is
	 * not equal to TX_STAGE_WORK.
	 */
	void
	check_tx_stage_work() const
	{
		if (pmemobj_tx_stage() != TX_STAGE_WORK)
			throw pmem::transaction_scope_error(
				"Function called out of transaction scope.");
	}

	void
	init()
	{
		pool_uuid = pmemobj_oid(this).pool_uuid_lo;
		if (pool_uuid == 0)
			throw pmem::pool_error("Invalid pool handle.");

#if LIBPMEMOBJ_CPP_VG_PMEMCHECK_ENABLED
		/*
		 * We do not need to add _size to TX.
		 * On restart we restore the size using data from TLS.
		 */
		VALGRIND_PMC_REMOVE_PMEM_MAPPING(&_size, sizeof(_size));
#endif

		_size = 0;
		on_init_size = 0;
		create_dummy_head();
	}

	void
	internal_move(concurrent_skip_list &&other)
	{
		assert(this->empty());
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);
		dummy_head = other.dummy_head;
		other.dummy_head = nullptr;
		other.create_dummy_head();

		_size.store(other._size.load(std::memory_order_relaxed),
			    std::memory_order_relaxed);
		on_init_size = other.on_init_size;
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
			       const K &key, const comparator &cmp) const
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

	/**
	 * The method finds insert position for the given @arg key. It finds
	 * successor and predecessr nodes on each level of the skip list.
	 * @param[out] prev_nodes array of pointers to predecessor nodes on each
	 * level.
	 * @param[out] next_nodes array of pointers to successor nodes on each
	 * level.
	 * @param[in] key inserted key.
	 * @param[in] comp comparator functor used for the search.
	 */
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
		return internal_insert(
			key, std::piecewise_construct,
			std::forward_as_tuple(std::forward<K>(key)),
			std::forward_as_tuple(std::forward<Args>(args)...));
	}

	template <typename... Args>
	std::pair<iterator, bool>
	internal_emplace(Args &&... args)
	{
		tls_entry_type &tls_entry = tls_data.local();
		obj::pool_base pop = get_pool_base();

		obj::transaction::run(pop, [&] {
			assert(tls_entry.ptr == nullptr);
			tls_entry.ptr =
				create_node(std::forward<Args>(args)...);
			++tls_entry.size_diff;
			tls_entry.insert_stage = not_started;
		});

		node_ptr n = tls_entry.ptr.get(pool_uuid);
		size_type height = n->height();

		std::pair<iterator, bool> insert_result = internal_insert_node(
			get_key(n), height,
			[&](const next_array_type &next_nodes)
				-> persistent_node_ptr & {
				assert(tls_entry.insert_stage == not_started);
				assert(tls_entry.ptr != nullptr);

				n->set_nexts(pop, next_nodes.data(), height);

				tls_entry.insert_stage = in_progress;
				pop.persist(&(tls_entry.insert_stage),
					    sizeof(tls_entry.insert_stage));

				return tls_entry.ptr;
			});

		if (!insert_result.second) {
			assert(tls_entry.ptr != nullptr);
			assert(tls_entry.insert_stage == not_started);

			obj::transaction::run(pop, [&] {
				--tls_entry.size_diff;
				delete_node(tls_entry.ptr);
				tls_entry.ptr = nullptr;
			});
		}

		assert(tls_entry.ptr == nullptr);
		return insert_result;
	}

	template <typename K, typename... Args>
	std::pair<iterator, bool>
	internal_insert(const K &key, Args &&... args)
	{
		tls_entry_type &tls_entry = tls_data.local();
		assert(tls_entry.ptr == nullptr);

		size_type height = random_level();

		std::pair<iterator, bool> insert_result = internal_insert_node(
			key, height,
			[&](const next_array_type &next_nodes)
				-> persistent_node_ptr & {
				obj::pool_base pop = get_pool_base();

				obj::transaction::manual tx(pop);
				tls_entry.ptr = create_node(
					std::forward_as_tuple(
						height, next_nodes.data()),
					std::forward_as_tuple(
						std::forward<Args>(args)...));

				++(tls_entry.size_diff);
				tls_entry.insert_stage = in_progress;
				obj::transaction::commit();

				assert(tls_entry.ptr != nullptr);
				return tls_entry.ptr;
			});

		assert(tls_entry.ptr == nullptr);

		return insert_result;
	}

	template <typename K, typename PrepareNode>
	std::pair<iterator, bool>
	internal_insert_node(const K &key, size_type height,
			     PrepareNode &&prepare_new_node)
	{
		prev_array_type prev_nodes;
		next_array_type next_nodes;
		node_ptr n = nullptr;

		do {
			if (allow_multimapping) {
				fill_prev_next_arrays(
					prev_nodes, next_nodes, key,
					not_greater_compare(_compare));
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

		} while ((n = try_insert_node(prev_nodes, next_nodes, height,
					      std::forward<PrepareNode>(
						      prepare_new_node))) ==
			 nullptr);

		assert(n);
		return std::pair<iterator, bool>(iterator(pool_uuid, n), true);
	}

	/**
	 * Try to insert new node to the skip list.
	 * @returns pointer to the new node if it was inserted. Otherwise,
	 * returns nullptr.
	 */
	template <typename PrepareNode>
	node_ptr
	try_insert_node(prev_array_type &prev_nodes,
			const next_array_type &next_nodes, size_type height,
			PrepareNode &&prepare_new_node)
	{
		assert(dummy_head(pool_uuid)->height() >= height);

		lock_array locks;
		if (!try_lock_nodes(height, prev_nodes, next_nodes, locks)) {
			return nullptr;
		}

		node_lock_type new_node_lock;
		obj::pool_base pop = get_pool_base();

		persistent_node_ptr &new_node = prepare_new_node(next_nodes);
		assert(new_node != nullptr);
		node_ptr n = new_node.get(pool_uuid);

		/*
		 * We need to hold lock to the new node until changes
		 * are committed to persistent domain. Otherwise, the
		 * new node would be visible to concurrent inserts
		 * before it is persisted.
		 */
		new_node_lock = n->acquire();

		/*
		 * In the loop below we are linking a new node to all layers of
		 * the skip list. Transaction is not required because in case of
		 * failure the node is reachable via a pointer from persistent
		 * TLS. During recovery, we will complete the insert. It is also
		 * OK if concurrent readers will see not a fully-linked node
		 * because during recovery the insert procedure will be
		 * completed.
		 */
		for (size_type level = 0; level < height; ++level) {
			assert(prev_nodes[level]->height() > level);
			assert(prev_nodes[level]->next(level) ==
			       next_nodes[level]);
			prev_nodes[level]->set_next(pop, level, new_node);
		}

#ifndef NDEBUG
		try_insert_node_finish_marker();
#endif

		new_node = nullptr;
		/* We need to persist the node pointer. Otherwise, on a restart,
		 * this pointer might be not null but the node can be already
		 * deleted. */
		pop.persist(&new_node, sizeof(new_node));

		++_size;

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
		       const next_array_type &nexts, lock_array &locks)
	{
		assert(check_prev_array(prevs, height));

		for (size_type l = 0; l < height; ++l) {
			if (l == 0 || prevs[l] != prevs[l - 1]) {
				locks[l] = prevs[l]->acquire();
			}

			persistent_node_ptr next = prevs[l]->next(l);
			if (next != nexts[l])
				/* Other thread inserted to this position and
				 * modified the pointer before we acquired the
				 * lock */
				return false;
		}

		return true;
	}

	/**
	 * Returns an iterator pointing to the first element from the list for
	 * which cmp(element, key) is false.
	 *
	 * @param[in] key key value to compare the elements to.
	 *
	 * @return Iterator pointing to the first element for which
	 * cmp(element, key) is false. If no such element is found, a
	 * past-the-end iterator is returned.
	 */
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

	/**
	 * Returns an iterator pointing to the first element from the list for
	 * which cmp(element, key) is false.
	 *
	 * @param[in] key key value to compare the elements to.
	 *
	 * @return Iterator pointing to the first element for which
	 * cmp(element, key) is false. If no such element is found, a
	 * past-the-end iterator is returned.
	 */
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

	iterator
	internal_erase(const_iterator pos, obj::p<difference_type> &size_diff)
	{
		assert(pos != end());

		obj::pool_base pop = get_pool_base();

		std::pair<persistent_node_ptr, persistent_node_ptr>
			extract_result(nullptr, nullptr);

		obj::transaction::run(pop, [&] {
			extract_result = internal_extract(pos);

			/* Make sure that node was extracted */
			assert(extract_result.first != nullptr);
			delete_node(extract_result.first);
			--size_diff;
		});

		--_size;

		return iterator(pool_uuid,
				extract_result.second.get(pool_uuid));
	}

	/**
	 * @returns a pointer to extracted node and a pointer to next node
	 */
	std::pair<persistent_node_ptr, persistent_node_ptr>
	internal_extract(const_iterator it)
	{
		assert(dummy_head(pool_uuid)->height() > 0);
		assert(it != end());
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);

		const key_type &key = traits_type::get_key(*it);

		prev_array_type prev_nodes;
		next_array_type next_nodes;

		fill_prev_next_arrays(prev_nodes, next_nodes, key, _compare);

		node_ptr erase_node = next_nodes[0].get(pool_uuid);
		assert(erase_node != nullptr);

		if (!_compare(key, get_key(erase_node))) {
			/* XXX: this assertion will fail in case of multimap
			 * because we take the first node with the same key.
			 * Need to extend algorithm for mutimap. */
			assert(erase_node == it.node);
			return internal_extract_node(prev_nodes, next_nodes,
						     erase_node);
		}

		return std::pair<persistent_node_ptr, persistent_node_ptr>(
			nullptr, nullptr);
	}

	std::pair<persistent_node_ptr, persistent_node_ptr>
	internal_extract_node(const prev_array_type &prev_nodes,
			      const next_array_type &next_nodes,
			      node_ptr erase_node)
	{
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);
		assert(erase_node != nullptr);
		for (size_type level = 0; level < erase_node->height();
		     ++level) {
			assert(prev_nodes[level]->height() > level);
			assert(next_nodes[level].get(pool_uuid) == erase_node);
			prev_nodes[level]->set_next(level,
						    erase_node->next(level));
		}

		return std::pair<persistent_node_ptr, persistent_node_ptr>(
			next_nodes[0], erase_node->next(0));
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

		on_init_size = sz;
		/*
		 * As internal_swap can only be called from one thread, and
		 * there can be an outer transaction we must make sure that mask
		 * and size changes are transactional
		 */
		obj::transaction::snapshot((size_type *)&_size);
		_size = sz;
	}

	/** Generate random level */
	size_type
	random_level()
	{
		return _rnd_generator();
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

		return create_node(
			std::forward_as_tuple(levels),
			std::forward_as_tuple(std::forward<Args>(args)...));
	}

	template <typename... NodeArgs, typename... ValueArgs>
	persistent_node_ptr
	create_node(std::tuple<NodeArgs...> &&node_args,
		    std::tuple<ValueArgs...> &&value_args)
	{
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);

		persistent_node_ptr node = creates_dummy_node(
			std::forward<std::tuple<NodeArgs...>>(node_args),
			index_sequence_for<NodeArgs...>{});

		construct_value_type(
			node,
			std::forward<std::tuple<ValueArgs...>>(value_args),
			index_sequence_for<ValueArgs...>{});

		return node;
	}

	template <typename Tuple, size_t... I>
	void
	construct_value_type(persistent_node_ptr node, Tuple &&args,
			     index_sequence<I...>)
	{
		node_ptr new_node = node.get(pool_uuid);

		node_allocator_traits::construct(
			_node_allocator, new_node->get(),
			std::get<I>(std::forward<Tuple>(args))...);
	}

	/**
	 * Creates dummy head.
	 *
	 * @pre Always called from ctor.
	 */
	void
	create_dummy_head()
	{
		dummy_head = creates_dummy_node(MAX_LEVEL);
	}

	template <typename Tuple, size_t... I>
	persistent_node_ptr
	creates_dummy_node(Tuple &&args, index_sequence<I...>)
	{
		return creates_dummy_node(
			std::get<I>(std::forward<Tuple>(args))...);
	}

	/**
	 * Creates new node, value_type should be constructed separately.
	 * Each node object has different size which depends on number of layers
	 * the node is linked. In this method we calculate the size of the new
	 * node based on the node height. Then required amount of bytes are
	 * allcoated and casted to the persistent_node_ptr.
	 *
	 * @pre Should be called inside transaction.
	 */
	template <typename... Args>
	persistent_node_ptr
	creates_dummy_node(size_type height, Args &&... args)
	{
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);
		size_type sz = calc_node_size(height);

		persistent_node_ptr n =
			node_allocator_traits::allocate(_node_allocator, sz)
				.raw();

		assert(n != nullptr);

		node_allocator_traits::construct(_node_allocator,
						 n.get(pool_uuid), height,
						 std::forward<Args>(args)...);

		return n;
	}

	template <bool is_dummy = false>
	void
	delete_node(persistent_node_ptr &node)
	{
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);
		node_ptr n = node.get(pool_uuid);
		size_type sz = calc_node_size(n->height());

		/* Destroy value */
		if (!is_dummy)
			node_allocator_traits::destroy(_node_allocator,
						       n->get());
		/* Destroy node */
		node_allocator_traits::destroy(_node_allocator, n);
		/* Deallocate memory */
		deallocate_node(node, sz);
		node = nullptr;
	}

	void
	deallocate_node(persistent_node_ptr &node, size_type sz)
	{
		/*
		 * Each node object has different size which depends on number
		 * of layers the node is linked. Therefore, allocate/deallocate
		 * just a raw byte array. persistent_ptr<uint8_t> is used as a
		 * pointer to raw array of bytes.
		 */
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

	/** Process any information which was saved to tls and clears tls */
	void
	tls_restore()
	{
		int64_t last_run_size = 0;
		obj::pool_base pop = get_pool_base();

		for (auto &tls_entry : tls_data) {
			persistent_node_ptr &node = tls_entry.ptr;
			auto &size_diff = tls_entry.size_diff;
			if (node) {
				if (tls_entry.insert_stage == in_progress) {
					complete_insert(tls_entry);
				} else {
					obj::transaction::run(pop, [&] {
						--(tls_entry.size_diff);
						delete_node(node);
						node = nullptr;
					});
				}
			}

			assert(node == nullptr);

			last_run_size += size_diff;
		}

		/* Make sure that on_init_size + last_run_size >= 0 */
		assert(last_run_size >= 0 ||
		       on_init_size >
			       static_cast<size_type>(std::abs(last_run_size)));
		obj::transaction::run(pop, [&] {
			tls_data.clear();
			on_init_size += static_cast<size_t>(last_run_size);
		});
		_size = on_init_size;
	}

	void
	complete_insert(tls_entry_type &tls_entry)
	{
		persistent_node_ptr &node = tls_entry.ptr;
		assert(node != nullptr);
		assert(tls_entry.insert_stage == in_progress);
		prev_array_type prev_nodes;
		next_array_type next_nodes;
		node_ptr n = node.get(pool_uuid);
		const key_type &key = get_key(n);
		size_type height = n->height();

		fill_prev_next_arrays(prev_nodes, next_nodes, key, _compare);
		obj::pool_base pop = get_pool_base();

		/* Node was partially linked */
		for (size_type level = 0; level < height; ++level) {
			assert(prev_nodes[level]->height() > level);
			assert(prev_nodes[level]->next(level) ==
			       next_nodes[level]);

			if (prev_nodes[level]->next(level) != node) {
				/* Otherwise, node already linked on
				 * this layer */
				assert(n->next(level) == next_nodes[level]);
				prev_nodes[level]->set_next(pop, level, node);
			}
		}

		node = nullptr;
#if LIBPMEMOBJ_CPP_VG_PMEMCHECK_ENABLED
		pop.persist(&node, sizeof(node));
#endif
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
	random_level_generator_type _rnd_generator;
	persistent_node_ptr dummy_head;

	enumerable_thread_specific<tls_entry_type> tls_data;

	std::atomic<size_type> _size;

	/**
	 * This variable holds real size after the skip list is initialized.
	 * It holds real value of size only after initialization (before any
	 * insert/remove).
	 */
	obj::p<size_type> on_init_size;
}; /* class concurrent_skip_list */

} /* namespace detail */
} /* namespace pmem */

#endif /* PMEMOBJ_CONCURRENT_SKIP_LIST_IMPL_HPP */
