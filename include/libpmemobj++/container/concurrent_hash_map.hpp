/*
 * Copyright 2019-2020, Intel Corporation
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

/**
 * @file
 * A persistent version of concurrent hash map implementation
 * Ref: https://arxiv.org/abs/1509.02235
 */

#ifndef PMEMOBJ_CONCURRENT_HASH_MAP_HPP
#define PMEMOBJ_CONCURRENT_HASH_MAP_HPP

#include <libpmemobj++/detail/atomic_backoff.hpp>
#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/pair.hpp>
#include <libpmemobj++/detail/template_helpers.hpp>

#include <libpmemobj++/defrag.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/transaction.hpp>

#include <libpmemobj++/detail/persistent_pool_ptr.hpp>
#include <libpmemobj++/shared_mutex.hpp>

#include <libpmemobj++/detail/enumerable_thread_specific.hpp>

#include <atomic>
#include <cassert>
#include <functional>
#include <initializer_list>
#include <iterator> // for std::distance
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace std
{
/**
 * Specialization of std::hash for p<T>
 */
template <typename T>
struct hash<pmem::obj::p<T>> {
	size_t
	operator()(const pmem::obj::p<T> &x) const
	{
		return hash<T>()(x.get_ro());
	}
};
} /* namespace std */

namespace pmem
{
namespace obj
{

namespace concurrent_hash_map_internal
{
template <typename SharedMutexT>
class shared_mutex_scoped_lock {
	using rw_mutex_type = SharedMutexT;

public:
	shared_mutex_scoped_lock(const shared_mutex_scoped_lock &) = delete;
	shared_mutex_scoped_lock &
	operator=(const shared_mutex_scoped_lock &) = delete;

	/** Default constructor. Construct lock that has not acquired a mutex.*/
	shared_mutex_scoped_lock() : mutex(nullptr), is_writer(false)
	{
	}

	/** Acquire lock on given mutex. */
	shared_mutex_scoped_lock(rw_mutex_type &m, bool write = true)
	    : mutex(nullptr)
	{
		acquire(m, write);
	}

	/** Release lock (if lock is held). */
	~shared_mutex_scoped_lock()
	{
		if (mutex)
			release();
	}

	/** Acquire lock on given mutex. */
	void
	acquire(rw_mutex_type &m, bool write = true)
	{
		is_writer = write;
		mutex = &m;
		if (write)
			mutex->lock();
		else
			mutex->lock_shared();
	}

	/**
	 * Release lock.
	 */
	void
	release()
	{
		assert(mutex);
		rw_mutex_type *m = mutex;
		mutex = nullptr;
		if (is_writer) {
			m->unlock();
		} else {
			m->unlock_shared();
		}
	}

	/**
	 * Try acquire lock on given mutex.
	 *
	 */
	bool
	try_acquire(rw_mutex_type &m, bool write = true)
	{
		assert(!mutex);
		bool result;
		is_writer = write;
		result = write ? m.try_lock() : m.try_lock_shared();
		if (result)
			mutex = &m;
		return result;
	}

protected:
	/**
	 * The pointer to the current mutex that is held, or NULL if no mutex is
	 * held.
	 */
	rw_mutex_type *mutex;
	/**
	 * If mutex!=NULL, then is_writer is true if holding a writer lock,
	 * false if holding a reader lock. Not defined if not holding a lock.
	 */
	bool is_writer;
}; /* class shared_mutex_scoped_lock */

template <typename ScopedLockType>
using scoped_lock_upgrade_to_writer =
	decltype(std::declval<ScopedLockType>().upgrade_to_writer());

template <typename ScopedLockType>
using scoped_lock_has_upgrade_to_writer =
	detail::supports<ScopedLockType, scoped_lock_upgrade_to_writer>;

template <typename ScopedLockType>
using scoped_lock_downgrade_to_reader =
	decltype(std::declval<ScopedLockType>().downgrade_to_reader());

template <typename ScopedLockType>
using scoped_lock_has_downgrade_to_reader =
	detail::supports<ScopedLockType, scoped_lock_downgrade_to_reader>;

template <typename ScopedLockType,
	  bool = scoped_lock_has_upgrade_to_writer<ScopedLockType>::value
		  &&scoped_lock_has_downgrade_to_reader<ScopedLockType>::value>
class scoped_lock_traits {
public:
	using scope_lock_type = ScopedLockType;

	static bool
	initial_rw_state(bool write)
	{
		/* For upgradeable locks, initial state is always read */
		return false;
	}

	static bool
	upgrade_to_writer(scope_lock_type &lock)
	{
		return lock.upgrade_to_writer();
	}

	static bool
	downgrade_to_reader(scope_lock_type &lock)
	{
		return lock.downgrade_to_reader();
	}
};

template <typename ScopedLockType>
class scoped_lock_traits<ScopedLockType, false> {
public:
	using scope_lock_type = ScopedLockType;

	static bool
	initial_rw_state(bool write)
	{
		/* For non-upgradeable locks, we take lock in required mode
		 * immediately */
		return write;
	}

	static bool
	upgrade_to_writer(scope_lock_type &lock)
	{
		/* This overload is for locks which do not support upgrade
		 * operation. For those locks, upgrade_to_writer should not be
		 * called when holding a read lock */
		return true;
	}

	static bool
	downgrade_to_reader(scope_lock_type &lock)
	{
		/* This overload is for locks which do not support downgrade
		 * operation. For those locks, downgrade_to_reader should never
		 * be called */
		assert(false);

		return false;
	}
};
}

template <typename Key, typename T, typename Hash = std::hash<Key>,
	  typename KeyEqual = std::equal_to<Key>,
	  typename MutexType = pmem::obj::shared_mutex,
	  typename ScopedLockType = concurrent_hash_map_internal::
		  shared_mutex_scoped_lock<MutexType>>
class concurrent_hash_map;

/** @cond INTERNAL */
namespace concurrent_hash_map_internal
{
/* Helper method which throws an exception when called in a tx */
static inline void
check_outside_tx()
{
	if (pmemobj_tx_stage() != TX_STAGE_NONE)
		throw pmem::transaction_scope_error(
			"Function called inside transaction scope.");
}

template <typename Hash>
using transparent_key_equal = typename Hash::transparent_key_equal;

template <typename Hash>
using has_transparent_key_equal = detail::supports<Hash, transparent_key_equal>;

template <typename Hash, typename Pred,
	  bool = has_transparent_key_equal<Hash>::value>
struct key_equal_type {
	using type = typename Hash::transparent_key_equal;
};

template <typename Hash, typename Pred>
struct key_equal_type<Hash, Pred, false> {
	using type = Pred;
};

template <typename Mutex, typename ScopedLockType>
void
assert_not_locked(Mutex &mtx)
{
#ifndef NDEBUG
	ScopedLockType scoped_lock;
	assert(scoped_lock.try_acquire(mtx));
	scoped_lock.release();
#else
	(void)mtx;
#endif
}

template <typename Key, typename T, typename MutexType, typename ScopedLockType>
struct hash_map_node {
	/**Mutex type. */
	using mutex_t = MutexType;

	/** Scoped lock type for mutex. */
	using scoped_t = ScopedLockType;

	using value_type = detail::pair<const Key, T>;

	/** Persistent pointer type for next. */
	using node_ptr_t = detail::persistent_pool_ptr<
		hash_map_node<Key, T, mutex_t, scoped_t>>;

	/** Next node in chain. */
	node_ptr_t next;

	/** Node mutex. */
	mutex_t mutex;

	/** Item stored in node */
	value_type item;

	hash_map_node(const node_ptr_t &_next, const Key &key)
	    : next(_next),
	      item(std::piecewise_construct, std::forward_as_tuple(key),
		   std::forward_as_tuple())
	{
	}

	hash_map_node(const node_ptr_t &_next, const Key &key, const T &t)
	    : next(_next), item(key, t)
	{
	}

	hash_map_node(const node_ptr_t &_next, value_type &&i)
	    : next(_next), item(std::move(i))
	{
	}

	template <typename... Args>
	hash_map_node(const node_ptr_t &_next, Args &&... args)
	    : next(_next), item(std::forward<Args>(args)...)
	{
	}

	hash_map_node(const node_ptr_t &_next, const value_type &i)
	    : next(_next), item(i)
	{
	}

	/** Copy constructor is deleted */
	hash_map_node(const hash_map_node &) = delete;

	/** Assignment operator is deleted */
	hash_map_node &operator=(const hash_map_node &) = delete;
}; /* struct node */

/**
 * The class provides the way to access certain properties of segments
 * used by hash map
 */
template <typename Bucket>
class segment_traits {
public:
	/** segment index type */
	using segment_index_t = size_t;
	using size_type = size_t;
	using bucket_type = Bucket;

protected:
	/** PMDK has limitation for allocation size. */
	constexpr static size_type max_allocation_size = PMEMOBJ_MAX_ALLOC_SIZE;

	/** First big block that has fixed size. */
	constexpr static segment_index_t first_big_block = 27;
	/* TODO: avoid hardcoded value; need constexpr  similar to:
	 * Log2(max_allocation_size / sizeof(bucket_type)) */

	/** Max number of buckets per segment. */
	constexpr static size_type big_block_size = size_type(1)
		<< first_big_block;

	/* Block size in bytes cannot exceed max_allocation_size */
	static_assert((big_block_size * sizeof(bucket_type)) <
			      max_allocation_size,
		      "Block size exceeds max_allocation_size");

	/** @returns index of the first block in the @arg seg. */
	constexpr static segment_index_t
	first_block_in_segment(segment_index_t seg)
	{
		return seg < first_big_block
			? seg
			: (first_big_block +
			   (segment_index_t(1) << (seg - first_big_block)) - 1);
	}

	/** @returns number of blocks in the @arg seg. */
	constexpr static size_type
	blocks_in_segment(segment_index_t seg)
	{
		return seg < first_big_block
			? segment_index_t(1)
			: segment_index_t(1) << (seg - first_big_block);
	}

	/** @returns number of buckets in the @arg b. */
	constexpr static size_type
	block_size(segment_index_t b)
	{
		return b < first_big_block ? segment_size(b ? b : 1)
					   : big_block_size;
	}

public:
	/** Number of embedded segments*/
	constexpr static segment_index_t embedded_segments = 1;

	/** Count of buckets in the embedded segments */
	constexpr static size_type embedded_buckets = 1 << embedded_segments;

	/** Maximum number of segments */
	constexpr static segment_index_t number_of_segments = 32;

	/** Count of segments in the first block. */
	static const size_type first_block = 8;

	/** @return maximum number of blocks */
	constexpr static segment_index_t
	number_of_blocks()
	{
		return first_block_in_segment(number_of_segments);
	}

	/** @return segment index of given index in the array. */
	static segment_index_t
	segment_index_of(size_type index)
	{
		return segment_index_t(detail::Log2(index | 1));
	}

	/** @return the first array index of given segment. */
	constexpr static segment_index_t
	segment_base(segment_index_t k)
	{
		return (segment_index_t(1) << k) & ~segment_index_t(1);
	}

	/** @return segment size except for @arg k == 0. */
	constexpr static size_type
	segment_size(segment_index_t k)
	{
		return size_type(1) << k; // fake value for k == 0
	}
	static_assert(
		embedded_segments < first_big_block,
		"Number of embedded segments cannot exceed max_allocation_size");
}; /* End of class segment_traits */

/**
 * Implements logic to work with segments in the hashmap.
 *
 * When number of elements stored in the hashmap exceeds the threshold,
 * the rehash operation is performed. Each new segment doubles the
 * number of buckets in the hashmap.
 *
 * PMDK has limitation for max allocation size. Therefore, at some
 * point new segment cannot be allocated as one contiguous memory block.
 *
 * block - array of buckets, continues in memory
 * segment - logical abstraction, might consist of several blocks.
 *
 * @class segment_facade_impl provides an abstraction and hides details
 * of how actually segment is allocated in memory.
 */
template <typename BlockTable, typename SegmentTraits, bool is_const>
class segment_facade_impl : public SegmentTraits {
private:
	using traits_type = SegmentTraits;
	using traits_type::block_size;
	using traits_type::blocks_in_segment;
	using traits_type::embedded_buckets;
	using traits_type::embedded_segments;
	using traits_type::first_block;
	using traits_type::first_block_in_segment;
	using traits_type::segment_base;
	using traits_type::segment_size;

public:
	using table_reference =
		typename std::conditional<is_const, const BlockTable &,
					  BlockTable &>::type;

	using table_pointer =
		typename std::conditional<is_const, const BlockTable *,
					  BlockTable *>::type;

	using bucket_type = typename traits_type::bucket_type;
	using segment_index_t = typename traits_type::segment_index_t;
	using size_type = typename traits_type::size_type;

	/** Constructor */
	segment_facade_impl(table_reference table, segment_index_t s)
	    : my_table(&table), my_seg(s)
	{
		assert(my_seg < traits_type::number_of_segments);
	}

	/** Copy constructor */
	segment_facade_impl(const segment_facade_impl &src)
	    : my_table(src.my_table), my_seg(src.my_seg)
	{
	}

	segment_facade_impl(segment_facade_impl &&src) = default;

	/** Assignment operator */
	segment_facade_impl &
	operator=(const segment_facade_impl &src)
	{
		my_table = src.my_table;
		my_seg = src.my_seg;
		return *this;
	}

	/** Move assignment operator */
	segment_facade_impl &
	operator=(segment_facade_impl &&src)
	{
		my_table = src.my_table;
		my_seg = src.my_seg;
		return *this;
	}

	/**
	 * Access @arg i bucket in the segment.
	 * @param[in] i bucket index in the segment. Should be in range
	 * [0, segment size).
	 * @returns reference to the bucket.
	 */
	bucket_type &operator[](size_type i) const
	{
		assert(i < size());

		segment_index_t table_block = first_block_in_segment(my_seg);
		size_type b_size = block_size(table_block);

		table_block += i / b_size;
		i = i % b_size;

		return (*my_table)[table_block][static_cast<std::ptrdiff_t>(i)];
	}

	/**
	 * Go to the next segment.
	 */
	segment_facade_impl &
	operator++()
	{
		++my_seg;
		return *this;
	}

	/**
	 * Go to the next segment. Postfix form.
	 */
	segment_facade_impl
	operator++(int)
	{
		segment_facade_impl tmp = *this;
		++(*this);
		return tmp;
	}

	/**
	 * Go to the previous segment.
	 */
	segment_facade_impl &
	operator--()
	{
		--my_seg;
		return *this;
	}

	/**
	 * Go to the previous segment. Postfix form.
	 */
	segment_facade_impl
	operator--(int)
	{
		segment_facade_impl tmp = *this;
		--(*this);
		return tmp;
	}

	/**
	 * Increments given segment by @arg off elements
	 */
	segment_facade_impl &
	operator+=(segment_index_t off)
	{
		my_seg += off;
		return *this;
	}

	/**
	 * Decrements given segment by @arg off elements
	 */
	segment_facade_impl &
	operator-=(segment_index_t off)
	{
		my_seg -= off;
		return *this;
	}

	/**
	 * @returns new segment which current segment + @arg off
	 */
	segment_facade_impl
	operator+(segment_index_t off) const
	{
		return segment_facade_impl(*(this->my_table),
					   this->my_seg + off);
	}

	/**
	 * @returns new segment which current segment - @arg off
	 */
	segment_facade_impl
	operator-(segment_index_t off) const
	{
		return segment_facade_impl(*(this->my_table),
					   this->my_seg - off);
	}

	/**
	 * Allocates new segment.
	 */
	void
	enable(pool_base &pop)
	{
		assert(my_seg >= embedded_segments);

		if (my_seg < first_block) {
			enable_first_block(pop);
		} else {
			enable_big_segment(pop);
		}
	}

	/**
	 * Deallocates the segment.
	 */
	void
	disable()
	{
		assert(my_seg >= embedded_segments);

		if (my_seg < first_block) {
			if (my_seg == embedded_segments) {
				size_type sz = segment_size(first_block) -
					embedded_buckets;
				delete_persistent<bucket_type[]>(
					(*my_table)[my_seg], sz);
			}
			(*my_table)[my_seg] = nullptr;
		} else {
			block_range blocks = segment_blocks(my_seg);

			for (segment_index_t b = blocks.first;
			     b < blocks.second; ++b) {
				if ((*my_table)[b] != nullptr) {
					delete_persistent<bucket_type[]>(
						(*my_table)[b], block_size(b));
					(*my_table)[b] = nullptr;
				}
			}
		}
	}

	/**
	 * @returns size of the segment.
	 */
	constexpr size_type
	size() const
	{
		return segment_size(my_seg ? my_seg : 1);
	}

	/**
	 * Checks if the segment is enabled.
	 * @returns true if the segment is ebabled. Otherwise returns
	 * false.
	 */
	bool
	is_valid() const
	{
		block_range blocks = segment_blocks(my_seg);

		for (segment_index_t b = blocks.first; b < blocks.second; ++b) {
			if ((*my_table)[b] == nullptr)
				return false;
		}

		return true;
	}

private:
	using block_range = std::pair<segment_index_t, segment_index_t>;

	/**
	 * @return block indexes [begin, end) for corresponding segment
	 */
	static block_range
	segment_blocks(segment_index_t seg)
	{
		segment_index_t begin = first_block_in_segment(seg);

		return block_range(begin, begin + blocks_in_segment(seg));
	}

	void
	enable_first_block(pool_base &pop)
	{
		assert(my_seg == embedded_segments);
		{
			transaction::manual tx(pop);

			size_type sz =
				segment_size(first_block) - embedded_buckets;
			(*my_table)[my_seg] =
				make_persistent<bucket_type[]>(sz);

			persistent_ptr<bucket_type> base =
				(*my_table)[embedded_segments].raw();

			for (segment_index_t s = my_seg + 1; s < first_block;
			     ++s) {
				std::ptrdiff_t off =
					static_cast<std::ptrdiff_t>(
						segment_base(s) -
						segment_base(my_seg));

				(*my_table)[s] = (base + off).raw();
			}

			transaction::commit();
		}
	}

	void
	enable_big_segment(pool_base &pop)
	{
		block_range blocks = segment_blocks(my_seg);
		{
			transaction::manual tx(pop);

			for (segment_index_t b = blocks.first;
			     b < blocks.second; ++b) {
				assert((*my_table)[b] == nullptr);
				(*my_table)[b] = make_persistent<bucket_type[]>(
					block_size(b));
			}

			transaction::commit();
		}
	}

	/** Pointer to the table of blocks */
	table_pointer my_table;

	/** Segment index */
	segment_index_t my_seg;
}; /* End of class segment_facade_impl */

/**
 * Base class of concurrent_hash_map.
 * Implements logic not dependent to Key/Value types.
 * MutexType - type of mutex used by buckets.
 * ScopedLockType - type of scoped lock for mutex.
 */
template <typename Key, typename T, typename MutexType, typename ScopedLockType>
class hash_map_base {
public:
	using mutex_t = MutexType;
	using scoped_t = ScopedLockType;

	/** Size type. */
	using size_type = size_t;

	/** Type of a hash code. */
	using hashcode_type = size_t;

	/** Node base type. */
	using node = hash_map_node<Key, T, mutex_t, scoped_t>;

	/** Node base pointer. */
	using node_ptr_t = detail::persistent_pool_ptr<node>;

	/** Bucket type. */
	struct bucket {
		using mutex_t = MutexType;
		using scoped_t = ScopedLockType;

		/** Bucket mutex. */
		mutex_t mutex;

		/** Atomic flag to indicate if bucket rehashed */
		p<std::atomic<uint64_t>> rehashed;

		/** List of the nodes stored in the bucket. */
		node_ptr_t node_list;

		/** Default constructor */
		bucket() : node_list(nullptr)
		{
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
			VALGRIND_HG_DISABLE_CHECKING(&rehashed,
						     sizeof(rehashed));
#endif
			rehashed.get_rw() = false;
		}

		/**
		 * @returns true if bucket rehashed and ready to use.
		 * Otherwise, @returns false if rehash is
		 * required
		 */
		bool
		is_rehashed(std::memory_order order)
		{
			return rehashed.get_ro().load(order);
		}

		void
		set_rehashed(std::memory_order order)
		{
			rehashed.get_rw().store(true, order);
		}

		/** Copy constructor is deleted */
		bucket(const bucket &) = delete;

		/** Assignment operator is deleted */
		bucket &operator=(const bucket &) = delete;
	}; /* End of struct bucket */

	/** Segment traits */
	using segment_traits_t = segment_traits<bucket>;

	/** Segment index type. */
	using segment_index_t = typename segment_traits_t::segment_index_t;

	/** Count of buckets in the embedded segments */
	static const size_type embedded_buckets =
		segment_traits_t::embedded_buckets;

	/** Count of segments in the first block. */
	static const size_type first_block = segment_traits_t::first_block;

	/** Size of a block_table. */
	constexpr static size_type block_table_size =
		segment_traits_t::number_of_blocks();

	/** Segment pointer. */
	using segment_ptr_t = persistent_ptr<bucket[]>;

	/** Bucket pointer. */
	using bucket_ptr_t = persistent_ptr<bucket>;

	/** Block pointers table type. */
	using blocks_table_t = segment_ptr_t[block_table_size];

	/** Segment mutex type. */
	using segment_enable_mutex_t = pmem::obj::mutex;

	/** Data specific for every thread using concurrent_hash_map */
	struct tls_data_t {
		p<int64_t> size_diff = 0;
		std::aligned_storage<56, 8> padding;
	};

	using tls_t = detail::enumerable_thread_specific<tls_data_t>;

	enum feature_flags : uint32_t { FEATURE_CONSISTENT_SIZE = 1 };

	/** Compat and incompat features of a layout */
	struct features {
		p<uint32_t> compat;
		p<uint32_t> incompat;
	};

	/* --------------------------------------------------------- */

	/** ID of persistent memory pool where hash map resides. */
	p<uint64_t> my_pool_uuid;

	/** Specifies features of a hashmap, used to check compatibility between
	 * header and the data */
	features layout_features;

	/** In future, my_mask can be implemented using v<> (8 bytes
	 * overhead) */
	std::aligned_storage<sizeof(size_t), sizeof(size_t)>::type
		my_mask_reserved;

	/** Hash mask = sum of allocated segment sizes - 1. */
	/* my_mask always restored on restart. */
	std::atomic<hashcode_type> my_mask;

	/* Size of value (key and value pair) stored in a pool */
	std::size_t value_size;

	/** Padding to the end of cacheline */
	std::aligned_storage<24, 8>::type padding1;

	/**
	 * Segment pointers table. Also prevents false sharing between my_mask
	 * and my_size.
	 */
	blocks_table_t my_table;

	/* It must be in separate cache line from my_mask due to performance
	 * effects */
	/** Size of container in stored items. */
	std::atomic<size_type> my_size;

	/** Padding to the end of cacheline */
	std::aligned_storage<24, 8>::type padding2;

	/** Thread specific data */
	persistent_ptr<tls_t> tls_ptr;

	/**
	 * This variable holds real size after hash_map is initialized.
	 * It holds real value of size only after initialization (before any
	 * insert/remove).
	 */
	p<size_t> on_init_size;

	/** Reserved for future use */
	std::aligned_storage<40, 8>::type reserved;

	/** Segment mutex used to enable new segment. */
	segment_enable_mutex_t my_segment_enable_mutex;

	/** Zero segment. */
	bucket my_embedded_segment[embedded_buckets];

	/* --------------------------------------------------------- */

	/** Features supported by this header */
	static constexpr features
	header_features()
	{
		return {FEATURE_CONSISTENT_SIZE, 0};
	}

	const std::atomic<hashcode_type> &
	mask() const noexcept
	{
		return my_mask;
	}

	std::atomic<hashcode_type> &
	mask() noexcept
	{
		return my_mask;
	}

	size_t
	size() const
	{
		return my_size.load(std::memory_order_relaxed);
	}

	p<int64_t> &
	thread_size_diff()
	{
		assert(this->tls_ptr != nullptr);
		return this->tls_ptr->local().size_diff;
	}

	/** Process any information which was saved to tls and clears tls */
	void
	tls_restore()
	{
		assert(this->tls_ptr != nullptr);

		pool_base pop = pool_base{pmemobj_pool_by_ptr(this)};

		int64_t last_run_size = 0;
		for (auto &data : *tls_ptr)
			last_run_size += data.size_diff;

		/* Make sure that on_init_size + last_run_size >= 0 */
		assert(last_run_size >= 0 ||
		       static_cast<int64_t>(static_cast<size_t>(last_run_size) +
					    on_init_size) >= 0);

		transaction::run(pop, [&] {
			on_init_size += static_cast<size_t>(last_run_size);
			tls_ptr->clear();
		});

		this->my_size = on_init_size;
	}

	/** Const segment facade type */
	using const_segment_facade_t =
		segment_facade_impl<blocks_table_t, segment_traits_t, true>;

	/** Non-const segment facade type */
	using segment_facade_t =
		segment_facade_impl<blocks_table_t, segment_traits_t, false>;

	/** Default constructor */
	hash_map_base()
	{
		static_assert(
			sizeof(size_type) == sizeof(std::atomic<size_type>),
			"std::atomic should have the same layout as underlying integral type");

#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
		VALGRIND_HG_DISABLE_CHECKING(&my_mask, sizeof(my_mask));
#endif
		layout_features = {0, 0};

		PMEMoid oid = pmemobj_oid(this);

		assert(!OID_IS_NULL(oid));

		my_pool_uuid = oid.pool_uuid_lo;

		pool_base pop = get_pool_base();
		/* enable embedded segments */
		for (size_type i = 0; i < segment_traits_t::embedded_segments;
		     ++i) {
			my_table[i] =
				pmemobj_oid(my_embedded_segment +
					    segment_traits_t::segment_base(i));
			segment_facade_t seg(my_table, i);
			mark_rehashed<false>(pop, seg);
		}

		on_init_size = 0;

		value_size = 0;

		this->tls_ptr = nullptr;
	}

	/*
	 * Should be called before concurrent_hash_map destructor is called.
	 * Otherwise, program can terminate if an exception occurs wile freeing
	 * memory inside dtor.
	 */
	void
	free_tls()
	{
		auto pop = get_pool_base();

		if ((layout_features.compat & FEATURE_CONSISTENT_SIZE) &&
		    tls_ptr) {
			transaction::run(pop, [&] {
				delete_persistent<tls_t>(tls_ptr);
				tls_ptr = nullptr;
			});
		}
	}

	/**
	 * Re-calculate mask value on each process restart.
	 */
	void
	calculate_mask()
	{
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
		VALGRIND_HG_DISABLE_CHECKING(&my_size, sizeof(my_size));
		VALGRIND_HG_DISABLE_CHECKING(&my_mask, sizeof(my_mask));
#endif
#if LIBPMEMOBJ_CPP_VG_PMEMCHECK_ENABLED
		VALGRIND_PMC_REMOVE_PMEM_MAPPING(&my_size, sizeof(my_size));
		VALGRIND_PMC_REMOVE_PMEM_MAPPING(&my_mask, sizeof(my_mask));
#endif

		hashcode_type m = embedded_buckets - 1;

		const_segment_facade_t segment(
			my_table, segment_traits_t::embedded_segments);

		while (segment.is_valid()) {
			m += segment.size();
			++segment;
		}

		mask().store(m, std::memory_order_relaxed);
	}

	/**
	 * Initialize buckets in the new segment.
	 */
	template <bool Flush = true>
	void
	mark_rehashed(pool_base &pop, segment_facade_t &segment)
	{
		for (size_type i = 0; i < segment.size(); ++i) {
			bucket *b = &(segment[i]);

			assert_not_locked<mutex_t, scoped_t>(b->mutex);

			b->set_rehashed(std::memory_order_relaxed);
		}

		if (Flush) {
			/* Flush in separate loop to avoid read-after-flush */
			for (size_type i = 0; i < segment.size(); ++i) {
				bucket *b = &(segment[i]);
				pop.flush(b->rehashed);
			}

			pop.drain();
		}
	}

	/**
	 * Enable new segment in the hashmap
	 */
	void
	enable_segment(segment_index_t k, bool is_initial = false)
	{
		assert(k);

		pool_base pop = get_pool_base();
		size_type sz;

		if (k >= first_block) {
			segment_facade_t new_segment(my_table, k);

			sz = new_segment.size();
			if (!new_segment.is_valid())
				new_segment.enable(pop);

			if (is_initial) {
				mark_rehashed(pop, new_segment);
			}

			/* double it to get entire capacity of the container */
			sz <<= 1;
		} else {
			/* the first block */
			assert(k == segment_traits_t::embedded_segments);

			for (segment_index_t i = k; i < first_block; ++i) {
				segment_facade_t new_segment(my_table, i);

				if (!new_segment.is_valid())
					new_segment.enable(pop);

				if (is_initial) {
					mark_rehashed(pop, new_segment);
				}
			}

			sz = segment_traits_t::segment_size(first_block);
		}
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
		ANNOTATE_HAPPENS_BEFORE(&my_mask);
#endif
		mask().store(sz - 1, std::memory_order_release);
	}

	/**
	 * Get bucket by (masked) hashcode.
	 * @return pointer to the bucket.
	 */
	bucket *
	get_bucket(hashcode_type h) const
	{
		segment_index_t s = segment_traits_t::segment_index_of(h);

		h -= segment_traits_t::segment_base(s);

		const_segment_facade_t segment(my_table, s);

		assert(segment.is_valid());

		return &(segment[h]);
	}

	/**
	 * Check for mask race
	 */
	inline bool
	check_mask_race(hashcode_type h, hashcode_type &m) const
	{
		hashcode_type m_now, m_old = m;

		m_now = mask().load(std::memory_order_acquire);
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
		ANNOTATE_HAPPENS_AFTER(&(this->my_mask));
#endif

		if (m_old != m_now)
			return check_rehashing_collision(h, m_old, m = m_now);

		return false;
	}

	/**
	 * Process mask race, check for rehashing collision
	 */
	bool
	check_rehashing_collision(hashcode_type h, hashcode_type m_old,
				  hashcode_type m) const
	{
		assert(m_old != m);

		if ((h & m_old) != (h & m)) {
			/* mask changed for this hashcode, rare event condition
			 * above proves that 'h' has some other bits set beside
			 * 'm_old', find next applicable mask after m_old */

			for (++m_old; !(h & m_old); m_old <<= 1)
				;

			m_old = (m_old << 1) - 1; /* get full mask from a bit */

			assert((m_old & (m_old + 1)) == 0 && m_old <= m);

			/* check whether it is rehashing/ed */
			bucket *b = get_bucket(h & m_old);
			return b->is_rehashed(std::memory_order_acquire);
		}

		return false;
	}

	/**
	 * Insert a node to bucket.
	 * @pre must be called inside transaction.
	 */
	template <typename Node, typename... Args>
	void
	insert_new_node_internal(bucket *b,
				 detail::persistent_pool_ptr<Node> &new_node,
				 Args &&... args)
	{
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);

		new_node = pmem::obj::make_persistent<Node>(
			b->node_list, std::forward<Args>(args)...);
		b->node_list = new_node; /* bucket is locked */
	}

	/**
	 * Insert a node.
	 * @return new size.
	 */
	template <typename Node, typename... Args>
	size_type
	insert_new_node(bucket *b, detail::persistent_pool_ptr<Node> &new_node,
			Args &&... args)
	{
		pool_base pop = get_pool_base();

		/*
		 * This is only true when called from singlethreaded methods
		 * like swap() or operator=. In that case it's safe to directly
		 * modify on_init_size.
		 */
		if (pmemobj_tx_stage() == TX_STAGE_WORK) {
			insert_new_node_internal(b, new_node,
						 std::forward<Args>(args)...);
			this->on_init_size++;
		} else {
			auto &size_diff = thread_size_diff();

			pmem::obj::transaction::run(pop, [&] {
				insert_new_node_internal(
					b, new_node,
					std::forward<Args>(args)...);
				++size_diff;
			});
		}

		/* Increment volatile size */
		return ++(this->my_size);
	}

	/**
	 * Checks load factor and decides if new segment should be allocated.
	 * @return true if new segment was allocated and false otherwise
	 */
	bool
	check_growth(hashcode_type m, size_type sz)
	{
		if (sz >= m) {
			segment_index_t new_seg =
				static_cast<segment_index_t>(detail::Log2(
					m +
					1)); /* optimized segment_index_of */

			assert(segment_facade_t(my_table, new_seg - 1)
				       .is_valid());

			std::unique_lock<segment_enable_mutex_t> lock(
				my_segment_enable_mutex, std::try_to_lock);

			if (lock) {
				if (mask().load(std::memory_order_relaxed) ==
				    m) {
					/* Otherwise, other thread enable this
					 * segment */
					enable_segment(new_seg);

					return true;
				}
			}
		}

		return false;
	}

	/**
	 * Prepare enough segments for number of buckets
	 */
	void
	reserve(size_type buckets)
	{
		if (buckets == 0)
			return;

		--buckets;

		bool is_initial = this->size() == 0;

		for (size_type m = mask(); buckets > m; m = mask())
			enable_segment(
				segment_traits_t::segment_index_of(m + 1),
				is_initial);
	}

	/**
	 * Swap hash_map_base
	 * @throws std::transaction_error in case of PMDK transaction failed
	 */
	void
	internal_swap(hash_map_base<Key, T, mutex_t, scoped_t> &table)
	{
		pool_base p = get_pool_base();
		{
			transaction::manual tx(p);

			this->my_pool_uuid.swap(table.my_pool_uuid);

			/*
			 * As internal_swap can only be called
			 * from one thread, and there can be an outer
			 * transaction we must make sure that mask and size
			 * changes are transactional
			 */
			transaction::snapshot((size_t *)&this->my_mask);
			transaction::snapshot((size_t *)&this->my_size);

			this->mask() = table.mask().exchange(
				this->mask(), std::memory_order_relaxed);

			this->my_size = table.my_size.exchange(
				this->my_size, std::memory_order_relaxed);

			/* Swap consistent size */
			std::swap(this->tls_ptr, table.tls_ptr);

			for (size_type i = 0; i < embedded_buckets; ++i)
				this->my_embedded_segment[i].node_list.swap(
					table.my_embedded_segment[i].node_list);

			for (size_type i = segment_traits_t::embedded_segments;
			     i < block_table_size; ++i)
				this->my_table[i].swap(table.my_table[i]);

			transaction::commit();
		}
	}

	/**
	 * Get the persistent memory pool where hashmap resides.
	 * @returns pmem::obj::pool_base object.
	 */
	pool_base
	get_pool_base()
	{
		PMEMobjpool *pop =
			pmemobj_pool_by_oid(PMEMoid{my_pool_uuid, 0});

		return pool_base(pop);
	}
}; /* End of class hash_map_base */

/**
 * Meets requirements of a forward iterator for STL
 * Value is either the T or const T type of the container.
 * @ingroup containers
 */
template <typename Container, bool is_const>
class hash_map_iterator {
public:
	using iterator_category = std::forward_iterator_tag;
	using difference_type = ptrdiff_t;
	using map_type = Container;
	using value_type = typename map_type::value_type;
	using node = typename map_type::node;
	using bucket = typename map_type::bucket;
	using map_ptr = typename std::conditional<is_const, const map_type *,
						  map_type *>::type;
	using reference =
		typename std::conditional<is_const,
					  typename map_type::const_reference,
					  typename map_type::reference>::type;
	using pointer =
		typename std::conditional<is_const,
					  typename map_type::const_pointer,
					  typename map_type::pointer>::type;

	template <typename C, bool M, bool U>
	friend bool operator==(const hash_map_iterator<C, M> &i,
			       const hash_map_iterator<C, U> &j);

	template <typename C, bool M, bool U>
	friend bool operator!=(const hash_map_iterator<C, M> &i,
			       const hash_map_iterator<C, U> &j);

	friend class hash_map_iterator<map_type, true>;

#if !defined(_MSC_VER) || defined(__INTEL_COMPILER)
private:
	template <typename Key, typename T, typename Hash, typename KeyEqual,
		  typename MutexType, typename ScopedLockType>
	friend class ::pmem::obj::concurrent_hash_map;
#else
public: /* workaround */
#endif
	hash_map_iterator(map_ptr map, size_t index)
	    : my_map(map), my_index(index), my_bucket(nullptr), my_node(nullptr)
	{
		if (my_index <= my_map->mask()) {
			bucket_accessor acc(my_map, my_index);
			my_bucket = acc.get();
			my_node = static_cast<node *>(
				my_bucket->node_list.get(my_map->my_pool_uuid));

			if (!my_node) {
				advance_to_next_bucket();
			}
		}
	}

public:
	/** Construct undefined iterator. */
	hash_map_iterator() = default;

	/** Copy constructor. */
	hash_map_iterator(const hash_map_iterator &other)
	    : my_map(other.my_map),
	      my_index(other.my_index),
	      my_bucket(other.my_bucket),
	      my_node(other.my_node)
	{
	}

	/** Copy constructor for const iterator from non-const iterator */
	template <typename U = void,
		  typename = typename std::enable_if<is_const, U>::type>
	hash_map_iterator(const hash_map_iterator<map_type, false> &other)
	    : my_map(other.my_map),
	      my_index(other.my_index),
	      my_bucket(other.my_bucket),
	      my_node(other.my_node)
	{
	}

	hash_map_iterator &operator=(const hash_map_iterator &it) = default;

	/** Indirection (dereference). */
	reference operator*() const
	{
		assert(my_node);
		return my_node->item;
	}

	/** Member access. */
	pointer operator->() const
	{
		return &operator*();
	}

	/** Prefix increment. */
	hash_map_iterator &
	operator++()
	{
		my_node = static_cast<node *>(
			my_node->next.get((my_map->my_pool_uuid)));

		if (!my_node)
			advance_to_next_bucket();

		return *this;
	}

	/** Postfix increment. */
	hash_map_iterator
	operator++(int)
	{
		hash_map_iterator old(*this);
		operator++();
		return old;
	}

private:
	/** Concurrent_hash_map over which we are iterating. */
	map_ptr my_map = nullptr;

	/** Bucket index for current item. */
	size_t my_index = 0;

	/** Pointer to bucket. */
	bucket *my_bucket = nullptr;

	/** Pointer to node that has current item. */
	node *my_node = nullptr;

	class bucket_accessor {
	public:
		bucket_accessor(map_ptr m, size_t index)
		{
			my_bucket = m->get_bucket(index);
		}

		bucket *
		get() const
		{
			return my_bucket;
		}

	private:
		bucket *my_bucket;
	};

	void
	advance_to_next_bucket()
	{
		size_t k = my_index + 1;

		assert(my_bucket);

		while (k <= my_map->mask()) {
			bucket_accessor acc(my_map, k);
			my_bucket = acc.get();

			if (my_bucket->node_list) {
				my_node = static_cast<node *>(
					my_bucket->node_list.get(
						my_map->my_pool_uuid));

				my_index = k;

				return;
			}

			++k;
		}

		my_bucket = 0;
		my_node = 0;
		my_index = k;
	}
};

template <typename Container, bool M, bool U>
bool
operator==(const hash_map_iterator<Container, M> &i,
	   const hash_map_iterator<Container, U> &j)
{
	return i.my_node == j.my_node && i.my_map == j.my_map;
}

template <typename Container, bool M, bool U>
bool
operator!=(const hash_map_iterator<Container, M> &i,
	   const hash_map_iterator<Container, U> &j)
{
	return i.my_node != j.my_node || i.my_map != j.my_map;
}
} /* namespace concurrent_hash_map_internal */
/** @endcond */

/**
 * Persistent memory aware implementation of Intel TBB concurrent_hash_map.
 * The implementation is based on a concurrent hash table algorithm
 * (https://arxiv.org/ftp/arxiv/papers/1509/1509.02235.pdf) where elements
 * assigned to buckets based on a hash code are calculated from a key.
 * In addition to concurrent find, insert, and erase operations, the algorithm
 * employs resizing and on-demand per-bucket rehashing. The hash table consists
 * of an array of buckets, and each bucket consists of a list of nodes and a
 * read-write lock to control concurrent access by multiple threads.
 *
 * Each time, the pool with concurrent_hash_map is being opened, the
 * concurrent_hash_map requires runtime_initialize() to be called (in order to
 * recalculate mask and restore the size).
 *
 * find(), insert(), erase() (and all overloads) are guaranteed to be
 * thread-safe.
 *
 * When a thread holds accessor to an element with a certain key, it is not
 * allowed to call find, insert nor erase with that key.
 *
 * MutexType defines type of read write lock used in concurrent_hash_map.
 * ScopedLockType defines a mutex wrapper that provides RAII-style mechanism
 * for owning a mutex. It should implement following methods and constructors:
 * ScopedLockType()
 * ScopedLockType(rw_mutex_type &m, bool write = true)
 * void acquire(rw_mutex_type &m, bool write)
 * void release()
 * bool try_acquire(rw_mutex_type &m, bool write)
 *
 * and optionally:
 * bool upgrade_to_writer()
 * bool downgrade_to_reader()
 * bool is_writer (variable)
 *
 * Implementing all optional methods and supplying is_writer variable can
 * improve performance if MutexType supports efficient upgrading and
 * downgrading operations.
 *
 * Testing note:
 * In some case, helgrind and drd might report lock ordering errors for
 * concurrent_hash_map. This might happen when calling find, insert or erase
 * while already holding an accessor to some element.
 *
 * The typical usage example would be:
 * @snippet doc_snippets/concurrent_hash_map.cpp concurrent_hash_map_example
 */
template <typename Key, typename T, typename Hash, typename KeyEqual,
	  typename MutexType, typename ScopedLockType>
class concurrent_hash_map
    : protected concurrent_hash_map_internal::hash_map_base<Key, T, MutexType,
							    ScopedLockType> {
	template <typename Container, bool is_const>
	friend class concurrent_hash_map_internal::hash_map_iterator;

public:
	using size_type = typename concurrent_hash_map_internal::hash_map_base<
		Key, T, MutexType, ScopedLockType>::size_type;
	using hashcode_type =
		typename concurrent_hash_map_internal::hash_map_base<
			Key, T, MutexType, ScopedLockType>::hashcode_type;
	using key_type = Key;
	using mapped_type = T;
	using value_type = typename concurrent_hash_map_internal::hash_map_base<
		Key, T, MutexType, ScopedLockType>::node::value_type;
	using difference_type = ptrdiff_t;
	using pointer = value_type *;
	using const_pointer = const value_type *;
	using reference = value_type &;
	using const_reference = const value_type &;
	using iterator = concurrent_hash_map_internal::hash_map_iterator<
		concurrent_hash_map, false>;
	using const_iterator = concurrent_hash_map_internal::hash_map_iterator<
		concurrent_hash_map, true>;
	using hasher = Hash;
	using key_equal = typename concurrent_hash_map_internal::key_equal_type<
		Hash, KeyEqual>::type;

protected:
	using mutex_t = MutexType;
	using scoped_t = ScopedLockType;
	/*
	 * Explicitly use methods and types from template base class
	 */
	using hash_map_base =
		concurrent_hash_map_internal::hash_map_base<Key, T, mutex_t,
							    scoped_t>;
	using hash_map_base::calculate_mask;
	using hash_map_base::check_growth;
	using hash_map_base::check_mask_race;
	using hash_map_base::embedded_buckets;
	using hash_map_base::FEATURE_CONSISTENT_SIZE;
	using hash_map_base::get_bucket;
	using hash_map_base::get_pool_base;
	using hash_map_base::header_features;
	using hash_map_base::insert_new_node;
	using hash_map_base::internal_swap;
	using hash_map_base::layout_features;
	using hash_map_base::mask;
	using hash_map_base::reserve;
	using tls_t = typename hash_map_base::tls_t;
	using node = typename hash_map_base::node;
	using node_mutex_t = typename node::mutex_t;
	using node_ptr_t = typename hash_map_base::node_ptr_t;
	using bucket = typename hash_map_base::bucket;
	using bucket_lock_type = typename bucket::scoped_t;
	using segment_index_t = typename hash_map_base::segment_index_t;
	using segment_traits_t = typename hash_map_base::segment_traits_t;
	using segment_facade_t = typename hash_map_base::segment_facade_t;
	using scoped_lock_traits_type =
		concurrent_hash_map_internal::scoped_lock_traits<scoped_t>;

	friend class const_accessor;
	using persistent_node_ptr_t = detail::persistent_pool_ptr<node>;

	void
	delete_node(const node_ptr_t &n)
	{
		delete_persistent<node>(
			detail::static_persistent_pool_pointer_cast<node>(n)
				.get_persistent_ptr(this->my_pool_uuid));
	}

	template <typename K>
	persistent_node_ptr_t
	search_bucket(const K &key, bucket *b) const
	{
		assert(b->is_rehashed(std::memory_order_relaxed));

		persistent_node_ptr_t n =
			detail::static_persistent_pool_pointer_cast<node>(
				b->node_list);

		while (n &&
		       !key_equal{}(key,
				    n.get(this->my_pool_uuid)->item.first)) {
			n = detail::static_persistent_pool_pointer_cast<node>(
				n.get(this->my_pool_uuid)->next);
		}

		return n;
	}

	/**
	 * Bucket accessor is to find, rehash, acquire a lock, and access a
	 * bucket
	 */
	class bucket_accessor : public bucket_lock_type {
		bucket *my_b;

	public:
		bucket_accessor(bucket_accessor &&b) noexcept : my_b(b.my_b)
		{
			bucket_lock_type::mutex = b.bucket_lock_type::mutex;
			bucket_lock_type::is_writer =
				b.bucket_lock_type::is_writer;
			b.my_b = nullptr;
			b.bucket_lock_type::mutex = nullptr;
			b.bucket_lock_type::is_writer = false;
		}

		bucket_accessor(concurrent_hash_map *base,
				const hashcode_type h, bool writer = false)
		{
			acquire(base, h, writer);
		}

		/**
		 * Find a bucket by masked hashcode, optionally rehash, and
		 * acquire the lock
		 */
		inline void
		acquire(concurrent_hash_map *base, const hashcode_type h,
			bool writer = false)
		{
			my_b = base->get_bucket(h);

			if (my_b->is_rehashed(std::memory_order_acquire) ==
				    false &&
			    bucket_lock_type::try_acquire(this->my_b->mutex,
							  /*write=*/true)) {
				if (my_b->is_rehashed(
					    std::memory_order_relaxed) ==
				    false) {
					/* recursive rehashing */
					base->rehash_bucket<false>(my_b, h);
				}
			} else {
				bucket_lock_type::acquire(my_b->mutex, writer);
			}

			assert(my_b->is_rehashed(std::memory_order_relaxed));
		}

		/**
		 * Check whether bucket is locked for write
		 */
		bool
		is_writer() const
		{
			return bucket_lock_type::is_writer;
		}

		/**
		 * Get bucket pointer
		 * @return pointer to the underlying bucket
		 */
		bucket *
		get() const
		{
			return my_b;
		}

		/**
		 * Overloaded arrow operator
		 * @returns pointer to the underlying bucket
		 */
		bucket *operator->() const
		{
			return this->get();
		}
	};

	/**
	 * Serial bucket accessor used to access bucket in a serial operations.
	 */
	class serial_bucket_accessor {
		bucket *my_b;

	public:
		serial_bucket_accessor(concurrent_hash_map *base,
				       const hashcode_type h,
				       bool writer = false)
		{
			acquire(base, h, writer);
		}

		/*
		 * Find a bucket by masked hashcode, optionally rehash
		 */
		inline void
		acquire(concurrent_hash_map *base, const hashcode_type h,
			bool writer = false)
		{
			my_b = base->get_bucket(h);

			if (my_b->is_rehashed(std::memory_order_relaxed) ==
			    false) {
				/* recursive rehashing */
				base->rehash_bucket<true>(my_b, h);
			}

			assert(my_b->is_rehashed(std::memory_order_relaxed));
		}

		/**
		 * This method is added for consistency with bucket_accessor
		 * class
		 *
		 * @return Always returns true
		 */
		bool
		is_writer() const
		{
			return true;
		}

		/**
		 * Get bucket pointer
		 * @return pointer to the bucket
		 */
		bucket *
		get() const
		{
			return my_b;
		}

		/**
		 * Overloaded arrow operator
		 * @returns pointer to the underlying bucket
		 */
		bucket *operator->() const
		{
			return this->get();
		}
	};

	hashcode_type
	get_hash_code(node_ptr_t &n)
	{
		return hasher{}(
			detail::static_persistent_pool_pointer_cast<node>(n)(
				this->my_pool_uuid)
				->item.first);
	}

	template <bool serial>
	void
	rehash_bucket(bucket *b_new, const hashcode_type h)
	{
		using accessor_type = typename std::conditional<
			serial, serial_bucket_accessor, bucket_accessor>::type;

		using scoped_lock_traits_type =
			concurrent_hash_map_internal::scoped_lock_traits<
				accessor_type>;

		/* First two bucket should be always rehashed */
		assert(h > 1);

		pool_base pop = get_pool_base();
		node_ptr_t *p_new = &(b_new->node_list);

		/* This condition is only true when there was a failure just
		 * before setting rehashed flag */
		if (*p_new != nullptr) {
			assert(!b_new->is_rehashed(std::memory_order_relaxed));

			b_new->set_rehashed(std::memory_order_relaxed);
			pop.persist(b_new->rehashed);

			return;
		}

		/* get parent mask from the topmost bit */
		hashcode_type mask = (1u << detail::Log2(h)) - 1;
		assert((h & mask) < h);
		accessor_type b_old(
			this, h & mask,
			scoped_lock_traits_type::initial_rw_state(true));

		pmem::obj::transaction::run(pop, [&] {
			/* get full mask for new bucket */
			mask = (mask << 1) | 1;
			assert((mask & (mask + 1)) == 0 && (h & mask) == h);

		restart:
			for (node_ptr_t *p_old = &(b_old->node_list),
					n = *p_old;
			     n; n = *p_old) {
				hashcode_type c = get_hash_code(n);
#ifndef NDEBUG
				hashcode_type bmask = h & (mask >> 1);

				bmask = bmask == 0
					? 1 /* minimal mask of parent bucket */
					: (1u << (detail::Log2(bmask) + 1)) - 1;

				assert((c & bmask) == (h & bmask));
#endif

				if ((c & mask) == h) {
					if (!b_old.is_writer() &&
					    !scoped_lock_traits_type::
						    upgrade_to_writer(b_old)) {
						goto restart;
						/* node ptr can be invalid due
						 * to concurrent erase */
					}

					/* Add to new b_new */
					*p_new = n;

					/* exclude from b_old */
					*p_old = n(this->my_pool_uuid)->next;

					p_new = &(n(this->my_pool_uuid)->next);
				} else {
					/* iterate to next item */
					p_old = &(n(this->my_pool_uuid)->next);
				}
			}

			*p_new = nullptr;
		});

		/* mark rehashed */
		b_new->set_rehashed(std::memory_order_release);
		pop.persist(b_new->rehashed);
	}

	void
	check_incompat_features()
	{
		if (layout_features.incompat != header_features().incompat)
			throw pmem::layout_error(
				"Incompat flags mismatch, for more details go to: https://pmem.io/pmdk/cpp_obj/ \n");

		if ((layout_features.compat & FEATURE_CONSISTENT_SIZE) &&
		    this->value_size != sizeof(value_type))
			throw pmem::layout_error(
				"Size of value_type is different than the one stored in the pool \n");
	}

public:
	class accessor;
	/**
	 * Combines data access, locking, and garbage collection.
	 */
	class const_accessor
	    : protected node::scoped_t /*which derived from no_copy*/ {
		friend class concurrent_hash_map<Key, T, Hash, KeyEqual,
						 mutex_t, scoped_t>;
		friend class accessor;
		using node_ptr_t = pmem::obj::persistent_ptr<node>;
		using node::scoped_t::try_acquire;

	public:
		/**
		 * Type of value
		 */
		using value_type =
			const typename concurrent_hash_map::value_type;

		/**
		 * @returns true if accessor does not hold any element, false
		 * otherwise.
		 */
		bool
		empty() const
		{
			return !my_node;
		}

		/**
		 * Release accessor.
		 * Cannot be called inside of a transaction.
		 *
		 * @throw transaction_scope_error if called inside transaction
		 */
		void
		release()
		{
			concurrent_hash_map_internal::check_outside_tx();

			if (my_node) {
				node::scoped_t::release();
				my_node = 0;
			}
		}

		/**
		 * @return reference to associated value in hash table.
		 */
		const_reference operator*() const
		{
			assert(my_node);

			return my_node->item;
		}

		/**
		 * @returns pointer to associated value in hash table.
		 */
		const_pointer operator->() const
		{
			return &operator*();
		}

		/**
		 * Create empty result
		 *
		 * Cannot be used in a transaction.
		 */
		const_accessor() : my_node(OID_NULL), my_hash()
		{
			concurrent_hash_map_internal::check_outside_tx();
		}

		/**
		 * Destroy result after releasing the underlying reference.
		 */
		~const_accessor()
		{
			my_node = OID_NULL; // scoped lock's release() is called
					    // in its destructor
		}

	protected:
		node_ptr_t my_node;

		hashcode_type my_hash;
	};

	/**
	 * Allows write access to elements and combines data access, locking,
	 * and garbage collection.
	 */
	class accessor : public const_accessor {
	public:
		/** Type of value. */
		using value_type = typename concurrent_hash_map::value_type;

		/** Return reference to associated value in hash table. */
		reference operator*() const
		{
			assert(this->my_node);

			return this->my_node->item;
		}

		/** Return pointer to associated value in hash table. */
		pointer operator->() const
		{
			return &operator*();
		}
	};

	/**
	 * Construct empty table.
	 */
	concurrent_hash_map() : hash_map_base()
	{
		runtime_initialize();
	}

	/**
	 * Construct empty table with n preallocated buckets. This number
	 * serves also as initial concurrency level.
	 */
	concurrent_hash_map(size_type n) : hash_map_base()
	{
		runtime_initialize();

		reserve(n);
	}

	/**
	 * Copy constructor
	 */
	concurrent_hash_map(const concurrent_hash_map &table) : hash_map_base()
	{
		runtime_initialize();

		reserve(table.size());

		internal_copy(table);
	}

	/**
	 * Move constructor
	 */
	concurrent_hash_map(concurrent_hash_map &&table) : hash_map_base()
	{
		runtime_initialize();

		swap(table);
	}

	/**
	 * Construction table with copying iteration range
	 */
	template <typename I>
	concurrent_hash_map(I first, I last)
	{
		runtime_initialize();

		reserve(static_cast<size_type>(std::distance(first, last)));

		internal_copy(first, last);
	}

	/**
	 * Construct table with initializer list
	 */
	concurrent_hash_map(std::initializer_list<value_type> il)
	{
		runtime_initialize();

		reserve(il.size());

		internal_copy(il.begin(), il.end());
	}

	/**
	 * Initialize persistent concurrent hash map after process restart.
	 * MUST be called every time after process restart.
	 * Not thread safe.
	 *
	 * @throw pmem::layout_error if hashmap was created using incompatible
	 * version of libpmemobj-cpp
	 */
	void
	runtime_initialize()
	{
		check_incompat_features();

		calculate_mask();

		/*
		 * Handle case where hash_map was created without
		 * FEATURE_CONSISTENT_SIZE.
		 */
		if (!(layout_features.compat & FEATURE_CONSISTENT_SIZE)) {
			auto actual_size =
				std::distance(this->begin(), this->end());
			assert(actual_size >= 0);

			this->my_size = static_cast<size_t>(actual_size);

			auto pop = get_pool_base();
			transaction::run(pop, [&] {
				this->tls_ptr = make_persistent<tls_t>();
				this->on_init_size =
					static_cast<size_t>(actual_size);
				this->value_size = sizeof(value_type);

				layout_features.compat |=
					FEATURE_CONSISTENT_SIZE;
			});
		} else {
			assert(this->tls_ptr != nullptr);
			this->tls_restore();
		}

		assert(this->size() ==
		       size_type(std::distance(this->begin(), this->end())));
	}

	[[deprecated(
		"runtime_initialize(bool) is now deprecated, use runtime_initialize(void)")]] void
	runtime_initialize(bool graceful_shutdown)
	{
		check_incompat_features();

		calculate_mask();

		if (!graceful_shutdown) {
			auto actual_size =
				std::distance(this->begin(), this->end());
			assert(actual_size >= 0);
			this->my_size = static_cast<size_type>(actual_size);
		} else {
			assert(this->size() ==
			       size_type(std::distance(this->begin(),
						       this->end())));
		}
	}

	/**
	 * Assignment
	 * Not thread safe.
	 *
	 * @throw std::transaction_error in case of PMDK transaction failure
	 * @throw pmem::transaction_alloc_error when allocating new memory
	 * failed.
	 * @throw pmem::transaction_free_error when freeing old underlying array
	 * failed.
	 * @throw rethrows constructor exception.
	 */
	concurrent_hash_map &
	operator=(const concurrent_hash_map &table)
	{
		if (this != &table) {
			clear();
			internal_copy(table);
		}

		return *this;
	}

	/**
	 * Assignment
	 * Not thread safe.
	 *
	 * @throw std::transaction_error in case of PMDK transaction failure
	 * @throw pmem::transaction_alloc_error when allocating new memory
	 * failed.
	 * @throw pmem::transaction_free_error when freeing old underlying array
	 * failed.
	 * @throw rethrows constructor exception.
	 */
	concurrent_hash_map &
	operator=(std::initializer_list<value_type> il)
	{
		clear();

		reserve(il.size());

		internal_copy(il.begin(), il.end());

		return *this;
	}

	/**
	 * Rehashes and optionally resizes the whole table.
	 * Useful to optimize performance before or after concurrent
	 * operations.
	 * Not thread safe.
	 *
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	void rehash(size_type n = 0);

	/**
	 * Clear hash map content
	 * Not thread safe.
	 *
	 * @throws pmem::transaction_error in case of PMDK transaction failure
	 */
	void clear();

	/*
	 * Should be called before concurrent_hash_map destructor is called.
	 * Otherwise, program can terminate if an exception occurs while freeing
	 * memory inside dtor.
	 *
	 * Hash map can NOT be used after free_data() was called (unless this
	 * was done in a transaction and transaction aborted).
	 *
	 * @throw std::transaction_error in case of PMDK transaction failure
	 * @throw pmem::transaction_free_error when freeing underlying memory
	 * failed.
	 */
	void
	free_data()
	{
		auto pop = get_pool_base();

		transaction::run(pop, [&] {
			clear();
			this->free_tls();
		});
	}

	/**
	 * Clear table and destroy it.
	 */
	~concurrent_hash_map()
	{
		free_data();
	}

	//------------------------------------------------------------------------
	// STL support - not thread-safe methods
	//------------------------------------------------------------------------

	/**
	 * @returns an iterator to the beginning
	 * Not thread safe.
	 *
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	iterator
	begin()
	{
		return iterator(this, 0);
	}

	/**
	 * @returns an iterator to the end
	 * Not thread safe.
	 */
	iterator
	end()
	{
		return iterator(this, mask() + 1);
	}

	/**
	 * @returns an iterator to the beginning
	 * Not thread safe.
	 */
	const_iterator
	begin() const
	{
		return const_iterator(this, 0);
	}

	/**
	 * @returns an iterator to the end
	 * Not thread safe.
	 */
	const_iterator
	end() const
	{
		return const_iterator(this, mask() + 1);
	}

	/**
	 * @returns number of items in table.
	 */
	size_type
	size() const
	{
		return hash_map_base::size();
	}

	/**
	 * @returns true if size()==0.
	 */
	bool
	empty() const
	{
		return this->size() == 0;
	}

	/**
	 * Upper bound on size.
	 */
	size_type
	max_size() const
	{
		return (~size_type(0)) / sizeof(node);
	}

	/**
	 * @returns the current number of buckets
	 */
	size_type
	bucket_count() const
	{
		return mask() + 1;
	}

	/**
	 * Swap two instances. Iterators are invalidated. Not thread safe.
	 */
	void swap(concurrent_hash_map &table);

	//------------------------------------------------------------------------
	// concurrent map operations
	//------------------------------------------------------------------------

	/**
	 * @return count of items (0 or 1)
	 *
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	size_type
	count(const Key &key) const
	{
		concurrent_hash_map_internal::check_outside_tx();

		return const_cast<concurrent_hash_map *>(this)->internal_find(
			key, nullptr, false);
	}

	/**
	 * This overload only participates in overload resolution if the
	 * qualified-id Hash::transparent_key_equal is valid and denotes a type.
	 * This assumes that such Hash is callable with both K and Key type, and
	 * that its key_equal is transparent, which, together, allows calling
	 * this function without constructing an instance of Key
	 *
	 * @return count of items (0 or 1)
	 *
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	template <typename K,
		  typename = typename std::enable_if<
			  concurrent_hash_map_internal::
				  has_transparent_key_equal<hasher>::value,
			  K>::type>
	size_type
	count(const K &key) const
	{
		concurrent_hash_map_internal::check_outside_tx();

		return const_cast<concurrent_hash_map *>(this)->internal_find(
			key, nullptr, false);
	}

	/**
	 * Find item and acquire a read lock on the item.
	 * @return true if item is found, false otherwise.
	 *
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	bool
	find(const_accessor &result, const Key &key) const
	{
		concurrent_hash_map_internal::check_outside_tx();

		result.release();

		return const_cast<concurrent_hash_map *>(this)->internal_find(
			key, &result, false);
	}

	/**
	 * Find item and acquire a read lock on the item.
	 *
	 * This overload only participates in overload resolution if the
	 * qualified-id Hash::transparent_key_equal is valid and denotes a type.
	 * This assumes that such Hash is callable with both K and Key type, and
	 * that its key_equal is transparent, which, together, allows calling
	 * this function without constructing an instance of Key
	 *
	 * @return true if item is found, false otherwise.
	 *
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	template <typename K,
		  typename = typename std::enable_if<
			  concurrent_hash_map_internal::
				  has_transparent_key_equal<hasher>::value,
			  K>::type>
	bool
	find(const_accessor &result, const K &key) const
	{
		concurrent_hash_map_internal::check_outside_tx();

		result.release();

		return const_cast<concurrent_hash_map *>(this)->internal_find(
			key, &result, false);
	}

	/**
	 * Find item and acquire a write lock on the item.
	 * @return true if item is found, false otherwise.
	 *
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	bool
	find(accessor &result, const Key &key)
	{
		concurrent_hash_map_internal::check_outside_tx();

		result.release();

		return internal_find(key, &result, true);
	}

	/**
	 * Find item and acquire a write lock on the item.
	 *
	 * This overload only participates in overload resolution if the
	 * qualified-id Hash::transparent_key_equal is valid and denotes a type.
	 * This assumes that such Hash is callable with both K and Key type, and
	 * that its key_equal is transparent, which, together, allows calling
	 * this function without constructing an instance of Key
	 *
	 * @return true if item is found, false otherwise.
	 *
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	template <typename K,
		  typename = typename std::enable_if<
			  concurrent_hash_map_internal::
				  has_transparent_key_equal<hasher>::value,
			  K>::type>
	bool
	find(accessor &result, const K &key)
	{
		concurrent_hash_map_internal::check_outside_tx();

		result.release();

		return internal_find(key, &result, true);
	}
	/**
	 * Insert item (if not already present) and
	 * acquire a read lock on the item.
	 * @return true if item is new.
	 * @throw pmem::transaction_alloc_error on allocation failure.
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	bool
	insert(const_accessor &result, const Key &key)
	{
		concurrent_hash_map_internal::check_outside_tx();

		result.release();

		return internal_insert(key, &result, false, key);
	}

	/**
	 * Insert item (if not already present) and
	 * acquire a write lock on the item.
	 * @returns true if item is new.
	 * @throw pmem::transaction_alloc_error on allocation failure.
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	bool
	insert(accessor &result, const Key &key)
	{
		concurrent_hash_map_internal::check_outside_tx();

		result.release();

		return internal_insert(key, &result, true, key);
	}

	/**
	 * Insert item by copying if there is no such key present already and
	 * acquire a read lock on the item.
	 * @return true if item is new.
	 * @throw pmem::transaction_alloc_error on allocation failure.
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	bool
	insert(const_accessor &result, const value_type &value)
	{
		concurrent_hash_map_internal::check_outside_tx();

		result.release();

		return internal_insert(value.first, &result, false, value);
	}

	/**
	 * Insert item by copying if there is no such key present already and
	 * acquire a write lock on the item.
	 * @return true if item is new.
	 *
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	bool
	insert(accessor &result, const value_type &value)
	{
		concurrent_hash_map_internal::check_outside_tx();

		result.release();

		return internal_insert(value.first, &result, true, value);
	}

	/**
	 * Insert item by copying if there is no such key present already
	 * @return true if item is inserted.
	 *
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	bool
	insert(const value_type &value)
	{
		concurrent_hash_map_internal::check_outside_tx();

		return internal_insert(value.first, nullptr, false, value);
	}

	/**
	 * Insert item by copying if there is no such key present already and
	 * acquire a read lock on the item.
	 * @return true if item is new.
	 * @throw pmem::transaction_alloc_error on allocation failure.
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	bool
	insert(const_accessor &result, value_type &&value)
	{
		concurrent_hash_map_internal::check_outside_tx();

		result.release();

		return internal_insert(value.first, &result, false,
				       std::move(value));
	}

	/**
	 * Insert item by copying if there is no such key present already and
	 * acquire a write lock on the item.
	 * @return true if item is new.
	 * @throw pmem::transaction_alloc_error on allocation failure.
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	bool
	insert(accessor &result, value_type &&value)
	{
		concurrent_hash_map_internal::check_outside_tx();

		result.release();

		return internal_insert(value.first, &result, true,
				       std::move(value));
	}

	/**
	 * Insert item by copying if there is no such key present already
	 * @return true if item is inserted.
	 * @throw pmem::transaction_alloc_error on allocation failure.
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	bool
	insert(value_type &&value)
	{
		concurrent_hash_map_internal::check_outside_tx();

		return internal_insert(value.first, nullptr, false,
				       std::move(value));
	}

	/**
	 * Insert range [first, last)
	 * @throw pmem::transaction_alloc_error on allocation failure.
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	template <typename I>
	void
	insert(I first, I last)
	{
		concurrent_hash_map_internal::check_outside_tx();

		for (; first != last; ++first)
			insert(*first);
	}

	/**
	 * Insert initializer list
	 * @throw pmem::transaction_alloc_error on allocation failure.
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	void
	insert(std::initializer_list<value_type> il)
	{
		concurrent_hash_map_internal::check_outside_tx();

		insert(il.begin(), il.end());
	}

	/**
	 * Inserts item if there is no such key present already, assigns
	 * provided value otherwise.
	 * @return return true if the insertion took place and false if the
	 * assignment took place.
	 * @throw pmem::transaction_alloc_error on allocation failure.
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	template <typename M>
	bool
	insert_or_assign(const key_type &key, M &&obj)
	{
		concurrent_hash_map_internal::check_outside_tx();

		accessor acc;
		auto result = internal_insert(key, &acc, true, key,
					      std::forward<M>(obj));

		if (!result) {
			pool_base pop = get_pool_base();
			pmem::obj::transaction::manual tx(pop);
			acc->second = std::forward<M>(obj);
			pmem::obj::transaction::commit();
		}

		return result;
	}

	/**
	 * Inserts item if there is no such key present already, assigns
	 * provided value otherwise.
	 * @return return true if the insertion took place and false if the
	 * assignment took place.
	 * @throw pmem::transaction_alloc_error on allocation failure.
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	template <typename M>
	bool
	insert_or_assign(key_type &&key, M &&obj)
	{
		concurrent_hash_map_internal::check_outside_tx();

		accessor acc;
		auto result = internal_insert(key, &acc, true, std::move(key),
					      std::forward<M>(obj));

		if (!result) {
			pool_base pop = get_pool_base();
			pmem::obj::transaction::manual tx(pop);
			acc->second = std::forward<M>(obj);
			pmem::obj::transaction::commit();
		}

		return result;
	}

	/**
	 * Inserts item if there is no such key-comparable type present already,
	 * assigns provided value otherwise.
	 * @return return true if the insertion took place and false if the
	 * assignment took place.
	 * @throw pmem::transaction_alloc_error on allocation failure.
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	template <
		typename K, typename M,
		typename = typename std::enable_if<
			concurrent_hash_map_internal::has_transparent_key_equal<
				hasher>::value &&
				std::is_constructible<key_type, K>::value,
			K>::type>
	bool
	insert_or_assign(K &&key, M &&obj)
	{
		concurrent_hash_map_internal::check_outside_tx();

		accessor acc;
		auto result =
			internal_insert(key, &acc, true, std::forward<K>(key),
					std::forward<M>(obj));

		if (!result) {
			pool_base pop = get_pool_base();
			pmem::obj::transaction::manual tx(pop);
			acc->second = std::forward<M>(obj);
			pmem::obj::transaction::commit();
		}

		return result;
	}

	/**
	 * Remove element with corresponding key
	 *
	 * @return true if element was deleted by this call
	 * @throws pmem::transaction_free_error in case of PMDK unable to free
	 * the memory
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	bool
	erase(const Key &key)
	{
		concurrent_hash_map_internal::check_outside_tx();

		return internal_erase(key);
	}

	/**
	 * Defragment the given (by 'start_percent' and 'amount_percent') part
	 * of buckets of the hash map. The algorithm is 'opportunistic' -
	 * if it is not able to lock a bucket it will just skip it.
	 *
	 * @return result struct containing a number of relocated and total
	 *	processed objects.
	 *
	 * @throw std::range_error if the range:
	 *	[start_percent, start_percent + amount_percent]
	 *	is incorrect.
	 *
	 * @throw rethrows pmem::defrag_error when a failure during
	 *	defragmentation occurs. Even if this error is thrown,
	 *	some of objects could have been relocated,
	 *	see in such case defrag_error.result for summary stats.
	 *
	 */
	pobj_defrag_result
	defragment(double start_percent = 0, double amount_percent = 100)
	{
		double end_percent = start_percent + amount_percent;
		if (start_percent < 0 || start_percent >= 100 ||
		    end_percent < 0 || end_percent > 100 ||
		    start_percent >= end_percent) {
			throw std::range_error("incorrect range");
		}

		size_t max_index = mask().load(std::memory_order_acquire);
		size_t start_index = (start_percent * max_index) / 100;
		size_t end_index = (end_percent * max_index) / 100;

#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
		ANNOTATE_HAPPENS_AFTER(&(this->my_mask));
#endif

		/* Create defrag object for elements in the current pool */
		pmem::obj::defrag my_defrag(this->get_pool_base());
		mutex_vector mv;

		/*
		 * Locks are taken in the backward order to avoid deadlocks
		 * with the rehashing of buckets.
		 *
		 * We do '+ 1' and '- 1' to handle the 'i == 0' case.
		 */
		for (size_t i = end_index + 1; i >= start_index + 1; i--) {
			/*
			 * All locks will be unlocked automatically
			 * in the destructor of 'mv'.
			 */
			bucket *b = mv.push_and_try_lock(this, i - 1);
			if (b == nullptr)
				continue;

			defrag_save_nodes(b, my_defrag);
		}

		return my_defrag.run();
	}

	/**
	 * Remove element with corresponding key
	 *
	 * This overload only participates in overload resolution if the
	 * qualified-id Hash::transparent_key_equal is valid and denotes a type.
	 * This assumes that such Hash is callable with both K and Key type, and
	 * that its key_equal is transparent, which, together, allows calling
	 * this function without constructing an instance of Key
	 *
	 * @return true if element was deleted by this call
	 * @throws pmem::transaction_free_error in case of PMDK unable to free
	 * the memory
	 * @throw pmem::transaction_scope_error if called inside transaction
	 */
	template <typename K,
		  typename = typename std::enable_if<
			  concurrent_hash_map_internal::
				  has_transparent_key_equal<hasher>::value,
			  K>::type>
	bool
	erase(const K &key)
	{
		concurrent_hash_map_internal::check_outside_tx();

		return internal_erase(key);
	}

protected:
	/*
	 * Try to acquire the mutex for read or write.
	 *
	 * If acquiring succeeds returns true, otherwise retries for few times.
	 * If acquiring fails after all attempts returns false.
	 */
	bool try_acquire_item(const_accessor *result, node_mutex_t &mutex,
			      bool write);

	/**
	 * Vector of locks to be unlocked at the destruction time.
	 * MutexType - type of mutex used by buckets.
	 */
	class mutex_vector {
	public:
		using mutex_t = MutexType;

		/** Save pointer to the lock in the vector and lock it. */
		bucket *
		push_and_try_lock(concurrent_hash_map *base, hashcode_type h)
		{
			vec.emplace_back(base, h, true /*writer*/);
			bucket *b = vec.back().get();

			auto node_ptr = static_cast<node *>(
				b->node_list.get(base->my_pool_uuid));

			while (node_ptr) {
				const_accessor ca;
				if (!base->try_acquire_item(&ca,
							    node_ptr->mutex,
							    /*write=*/true)) {
					vec.pop_back();
					return nullptr;
				}

				node_ptr =
					static_cast<node *>(node_ptr->next.get(
						(base->my_pool_uuid)));
			}

			return b;
		}

	private:
		std::vector<bucket_accessor> vec;
	};

	template <typename K>
	bool internal_find(const K &key, const_accessor *result, bool write);

	template <typename K, typename... Args>
	bool internal_insert(const K &key, const_accessor *result, bool write,
			     Args &&... args);

	/* Obtain pointer to node and lock bucket */
	template <bool Bucket_rw_lock, typename K>
	persistent_node_ptr_t
	get_node(const K &key, bucket_accessor &b)
	{
		/* find a node */
		auto n = search_bucket(key, b.get());

		if (!n) {
			if (Bucket_rw_lock && !b.is_writer() &&
			    !scoped_lock_traits_type::upgrade_to_writer(b)) {
				/* Rerun search_list, in case another
				 * thread inserted the item during the
				 * upgrade. */
				n = search_bucket(key, b.get());
				if (n) {
					/* unfortunately, it did */
					scoped_lock_traits_type::
						downgrade_to_reader(b);
					return n;
				}
			}
		}

		return n;
	}

	template <typename K>
	bool internal_erase(const K &key);

	void clear_segment(segment_index_t s);

	/**
	 * Copy "source" to *this, where *this must start out empty.
	 */
	void internal_copy(const concurrent_hash_map &source);

	template <typename I>
	void internal_copy(I first, I last);

	/**
	 * Internal method used by defragment().
	 * Adds nodes to the defragmentation list.
	 */
	void
	defrag_save_nodes(bucket *b, pmem::obj::defrag &defrag)
	{
		auto node_ptr = static_cast<node *>(
			b->node_list.get(this->my_pool_uuid));

		while (node_ptr) {
			/*
			 * We do not perform the defragmentation
			 * on node pointers, because nodes
			 * always have the same size.
			 */
			defrag.add(node_ptr->item.first);
			defrag.add(node_ptr->item.second);

			node_ptr = static_cast<node *>(
				node_ptr->next.get((this->my_pool_uuid)));
		}
	}
}; // class concurrent_hash_map

template <typename Key, typename T, typename Hash, typename KeyEqual,
	  typename MutexType, typename ScopedLockType>
bool
concurrent_hash_map<Key, T, Hash, KeyEqual, MutexType,
		    ScopedLockType>::try_acquire_item(const_accessor *result,
						      node_mutex_t &mutex,
						      bool write)
{
	/* acquire the item */
	if (!result->try_acquire(mutex, write)) {
		for (detail::atomic_backoff backoff(true);;) {
			if (result->try_acquire(mutex, write))
				break;

			if (!backoff.bounded_pause())
				return false;
		}
	}

	return true;
}

template <typename Key, typename T, typename Hash, typename KeyEqual,
	  typename MutexType, typename ScopedLockType>
template <typename K>
bool
concurrent_hash_map<Key, T, Hash, KeyEqual, MutexType,
		    ScopedLockType>::internal_find(const K &key,
						   const_accessor *result,
						   bool write)
{
	assert(!result || !result->my_node);

	hashcode_type m = mask().load(std::memory_order_acquire);
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
	ANNOTATE_HAPPENS_AFTER(&(this->my_mask));
#endif

	assert((m & (m + 1)) == 0);

	hashcode_type const h = hasher{}(key);

	persistent_node_ptr_t node;

	while (true) {
		/* get bucket and acquire the lock */
		bucket_accessor b(
			this, h & m,
			scoped_lock_traits_type::initial_rw_state(false));
		node = get_node<false>(key, b);

		if (!node) {
			/* Element was possibly relocated, try again */
			if (check_mask_race(h, m)) {
				b.release();
				continue;
			} else {
				return false;
			}
		}

		/* No need to acquire the item or item acquired */
		if (!result ||
		    try_acquire_item(
			    result, node.get(this->my_pool_uuid)->mutex, write))
			break;

		/* the wait takes really long, restart the
		 * operation */
		b.release();

		std::this_thread::yield();

		m = mask().load(std::memory_order_acquire);
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
		ANNOTATE_HAPPENS_AFTER(&(this->my_mask));
#endif
	}

	if (result) {
		result->my_node = node.get_persistent_ptr(this->my_pool_uuid);
		result->my_hash = h;
	}

	return true;
}

template <typename Key, typename T, typename Hash, typename KeyEqual,
	  typename MutexType, typename ScopedLockType>
template <typename K, typename... Args>
bool
concurrent_hash_map<Key, T, Hash, KeyEqual, MutexType,
		    ScopedLockType>::internal_insert(const K &key,
						     const_accessor *result,
						     bool write,
						     Args &&... args)
{
	assert(!result || !result->my_node);

	hashcode_type m = mask().load(std::memory_order_acquire);
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
	ANNOTATE_HAPPENS_AFTER(&(this->my_mask));
#endif

	assert((m & (m + 1)) == 0);

	hashcode_type const h = hasher{}(key);

	persistent_node_ptr_t node;
	size_t new_size = 0;
	bool inserted = false;

	while (true) {
		/* get bucket and acquire the lock */
		bucket_accessor b(
			this, h & m,
			scoped_lock_traits_type::initial_rw_state(true));
		node = get_node<true>(key, b);

		if (!node) {
			/* Element was possibly relocated, try again */
			if (check_mask_race(h, m)) {
				b.release();
				continue;
			}

			/* insert and set flag to grow the container */
			new_size = insert_new_node(b.get(), node,
						   std::forward<Args>(args)...);
			inserted = true;
		}

		/* No need to acquire the item or item acquired */
		if (!result ||
		    try_acquire_item(
			    result, node.get(this->my_pool_uuid)->mutex, write))
			break;

		/* the wait takes really long, restart the
		 * operation */
		b.release();

		std::this_thread::yield();

		m = mask().load(std::memory_order_acquire);
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
		ANNOTATE_HAPPENS_AFTER(&(this->my_mask));
#endif
	}

	if (result) {
		result->my_node = node.get_persistent_ptr(this->my_pool_uuid);
		result->my_hash = h;
	}

	check_growth(m, new_size);

	return inserted;
}

template <typename Key, typename T, typename Hash, typename KeyEqual,
	  typename MutexType, typename ScopedLockType>
template <typename K>
bool
concurrent_hash_map<Key, T, Hash, KeyEqual, MutexType,
		    ScopedLockType>::internal_erase(const K &key)
{
	node_ptr_t n;
	hashcode_type const h = hasher{}(key);
	hashcode_type m = mask().load(std::memory_order_acquire);
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
	ANNOTATE_HAPPENS_AFTER(&(this->my_mask));
#endif

	pool_base pop = get_pool_base();

restart : {
	/* lock scope */
	/* get bucket */
	bucket_accessor b(this, h & m,
			  scoped_lock_traits_type::initial_rw_state(true));

search:
	node_ptr_t *p = &b->node_list;
	n = *p;

	while (n &&
	       !key_equal{}(key,
			    detail::static_persistent_pool_pointer_cast<node>(
				    n)(this->my_pool_uuid)
				    ->item.first)) {
		p = &n(this->my_pool_uuid)->next;
		n = *p;
	}

	if (!n) {
		/* not found, but mask could be changed */
		if (check_mask_race(h, m))
			goto restart;

		return false;
	} else if (!b.is_writer() &&
		   !scoped_lock_traits_type::upgrade_to_writer(b)) {
		if (check_mask_race(h, m)) /* contended upgrade, check mask */
			goto restart;

		goto search;
	}

	persistent_ptr<node> del = n(this->my_pool_uuid);

	{
		/* We cannot remove this element immediately because
		 * other threads might work with this element via
		 * accessors. The item_locker required to wait while
		 * other threads use the node. */
		const_accessor acc;
		if (!try_acquire_item(&acc, del->mutex, true)) {
			/* the wait takes really long, restart the operation */
			b.release();

			std::this_thread::yield();

			m = mask().load(std::memory_order_acquire);

			goto restart;
		}
	}

	assert(pmemobj_tx_stage() == TX_STAGE_NONE);

	auto &size_diff = this->thread_size_diff();

	/* Only one thread can delete it due to write lock on the bucket
	 */
	transaction::run(pop, [&] {
		*p = del->next;
		delete_node(del);

		--size_diff;
	});

	--(this->my_size);
}

	return true;
}

template <typename Key, typename T, typename Hash, typename KeyEqual,
	  typename MutexType, typename ScopedLockType>
void
concurrent_hash_map<Key, T, Hash, KeyEqual, MutexType, ScopedLockType>::swap(
	concurrent_hash_map<Key, T, Hash, KeyEqual, mutex_t, scoped_t> &table)
{
	internal_swap(table);
}

template <typename Key, typename T, typename Hash, typename KeyEqual,
	  typename MutexType, typename ScopedLockType>
void
concurrent_hash_map<Key, T, Hash, KeyEqual, MutexType, ScopedLockType>::rehash(
	size_type sz)
{
	concurrent_hash_map_internal::check_outside_tx();

	reserve(sz);
	hashcode_type m = mask();

	/* only the last segment should be scanned for rehashing size or first
	 * index of the last segment */
	hashcode_type b = (m + 1) >> 1;

	/* zero or power of 2 */
	assert((b & (b - 1)) == 0);

	for (; b <= m; ++b) {
		bucket *bp = get_bucket(b);

		concurrent_hash_map_internal::assert_not_locked<mutex_t,
								scoped_t>(
			bp->mutex);
		/* XXX Need to investigate if this statement is needed */
		if (bp->is_rehashed(std::memory_order_relaxed) == false)
			rehash_bucket<true>(bp, b);
	}
}

template <typename Key, typename T, typename Hash, typename KeyEqual,
	  typename MutexType, typename ScopedLockType>
void
concurrent_hash_map<Key, T, Hash, KeyEqual, MutexType, ScopedLockType>::clear()
{
	hashcode_type m = mask();

	assert((m & (m + 1)) == 0);

#ifndef NDEBUG
	/* check consistency */
	for (segment_index_t b = 0; b <= m; ++b) {
		bucket *bp = get_bucket(b);
		concurrent_hash_map_internal::assert_not_locked<mutex_t,
								scoped_t>(
			bp->mutex);
	}
#endif

	pool_base pop = get_pool_base();
	{ /* transaction scope */

		transaction::manual tx(pop);

		assert(this->tls_ptr != nullptr);
		this->tls_ptr->clear();

		this->on_init_size = 0;

		segment_index_t s = segment_traits_t::segment_index_of(m);

		assert(s + 1 == this->block_table_size ||
		       !segment_facade_t(this->my_table, s + 1).is_valid());

		do {
			clear_segment(s);
		} while (s-- > 0);

		/*
		 * As clear can only be called
		 * from one thread, and there can be an outer
		 * transaction we must make sure that mask and size
		 * changes are transactional
		 */
		transaction::snapshot((size_t *)&this->my_mask);
		transaction::snapshot((size_t *)&this->my_size);

		mask().store(embedded_buckets - 1, std::memory_order_relaxed);
		this->my_size = 0;

		transaction::commit();
	}
}

template <typename Key, typename T, typename Hash, typename KeyEqual,
	  typename MutexType, typename ScopedLockType>
void
concurrent_hash_map<Key, T, Hash, KeyEqual, MutexType,
		    ScopedLockType>::clear_segment(segment_index_t s)
{
	segment_facade_t segment(this->my_table, s);

	assert(segment.is_valid());

	size_type sz = segment.size();
	for (segment_index_t i = 0; i < sz; ++i) {
		for (node_ptr_t n = segment[i].node_list; n;
		     n = segment[i].node_list) {
			segment[i].node_list = n(this->my_pool_uuid)->next;
			delete_node(n);
		}
	}

	if (s >= segment_traits_t::embedded_segments)
		segment.disable();
}

template <typename Key, typename T, typename Hash, typename KeyEqual,
	  typename MutexType, typename ScopedLockType>
void
concurrent_hash_map<Key, T, Hash, KeyEqual, MutexType, ScopedLockType>::
	internal_copy(const concurrent_hash_map &source)
{
	auto pop = get_pool_base();

	reserve(source.size());
	internal_copy(source.begin(), source.end());
}

template <typename Key, typename T, typename Hash, typename KeyEqual,
	  typename MutexType, typename ScopedLockType>
template <typename I>
void
concurrent_hash_map<Key, T, Hash, KeyEqual, MutexType,
		    ScopedLockType>::internal_copy(I first, I last)
{
	hashcode_type m = mask();

	for (; first != last; ++first) {
		hashcode_type h = hasher{}(first->first);
		bucket *b = get_bucket(h & m);

		assert(b->is_rehashed(std::memory_order_relaxed));

		detail::persistent_pool_ptr<node> p;
		insert_new_node(b, p, *first);
	}
}

template <typename Key, typename T, typename Hash, typename KeyEqual,
	  typename MutexType, typename ScopedLockType>
inline bool
operator==(const concurrent_hash_map<Key, T, Hash, KeyEqual, MutexType,
				     ScopedLockType> &a,
	   const concurrent_hash_map<Key, T, Hash, KeyEqual, MutexType,
				     ScopedLockType> &b)
{
	if (a.size() != b.size())
		return false;

	typename concurrent_hash_map<Key, T, Hash, KeyEqual, MutexType,
				     ScopedLockType>::const_iterator
		i(a.begin()),
		i_end(a.end());

	typename concurrent_hash_map<Key, T, Hash, KeyEqual, MutexType,
				     ScopedLockType>::const_iterator j,
		j_end(b.end());

	for (; i != i_end; ++i) {
		j = b.equal_range(i->first).first;

		if (j == j_end || !(i->second == j->second))
			return false;
	}

	return true;
}

template <typename Key, typename T, typename Hash, typename KeyEqual,
	  typename MutexType, typename ScopedLockType>
inline bool
operator!=(const concurrent_hash_map<Key, T, Hash, KeyEqual, MutexType,
				     ScopedLockType> &a,
	   const concurrent_hash_map<Key, T, Hash, KeyEqual, MutexType,
				     ScopedLockType> &b)
{
	return !(a == b);
}

template <typename Key, typename T, typename Hash, typename KeyEqual,
	  typename MutexType, typename ScopedLockType>
inline void
swap(concurrent_hash_map<Key, T, Hash, KeyEqual, MutexType, ScopedLockType> &a,
     concurrent_hash_map<Key, T, Hash, KeyEqual, MutexType, ScopedLockType> &b)
{
	a.swap(b);
}

} /* namespace obj */
} /* namespace pmem */

#endif /* PMEMOBJ_CONCURRENT_HASH_MAP_HPP */
