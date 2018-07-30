/*
 * Copyright 2018, Intel Corporation
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

#ifndef PMEMOBJ_PERSISTENT_CONCURRENT_HASH_MAP_HPP
#define PMEMOBJ_PERSISTENT_CONCURRENT_HASH_MAP_HPP

#include "tbb/atomic.h"
#include "tbb/concurrent_hash_map.h"
#include "tbb/spin_rw_mutex.h"

#include <libpmemobj++/experimental/v.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/transaction.hpp>

#include "persistent_pool_ptr.hpp"

#include <cassert>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <type_traits>

namespace pmem
{
namespace obj
{
namespace experimental
{

using namespace pmem::obj;

template <typename Key, typename T,
	  typename HashCompare = tbb::tbb_hash_compare<Key>>
class concurrent_hash_map;

/** @cond INTERNAL */
namespace internal
{
template <typename Mutex>
void
assert_not_locked(Mutex &mtx)
{
#ifndef NDEBUG
	assert(mtx.try_lock());
	mtx.unlock();
#else
	(void)mtx;
#endif
}

template <typename T>
inline tbb::atomic<T> &
as_atomic(T &t)
{
	return (tbb::atomic<T> &)t;
}

struct hash_map_node_base {
	/** Mutex type. */
	using mutex_t = tbb::spin_rw_mutex;

	/** Scoped lock type for mutex. */
	using scoped_t = mutex_t::scoped_lock;

	/** Persistent pointer type for next. */
	using node_base_ptr_t = persistent_pool_ptr<hash_map_node_base>;

	/** Next node in chain. */
	node_base_ptr_t next;

	/** Node mutex. */
	pmem::obj::experimental::v<mutex_t> mutex;

	hash_map_node_base() : next(OID_NULL)
	{
	}

	hash_map_node_base(const node_base_ptr_t &_next) : next(_next)
	{
	}

	hash_map_node_base(node_base_ptr_t &&_next) : next(std::move(_next))
	{
	}

	/** Copy constructor is deleted */
	hash_map_node_base(const hash_map_node_base &) = delete;

	/** Assignment operator is deleted */
	void operator=(const hash_map_node_base &) = delete;
};

/** Incompleteness flag value. */
static persistent_ptr<hash_map_node_base> const rehash_req = PMEMoid({0, 3});

/** Rehashed bucket flag. */
static persistent_ptr<hash_map_node_base> const rehashed = OID_NULL;

/** Rehashed empty bucket flag. */
static persistent_pool_ptr<hash_map_node_base> const empty_bucket = OID_NULL;

/**
 * Base class of concurrent_hash_map.
 * Implements logic not dependant to Key/Value types.
 */
class hash_map_base {
public:
	/** Size type. */
	using size_type = size_t;

	/** Type of a hash code. */
	using hashcode_t = size_t;

	/** Segment index type. */
	using segment_index_t = size_t;

	/** Node base type. */
	using node_base = hash_map_node_base;

	/** Node base pointer. */
	using node_base_ptr_t = persistent_pool_ptr<node_base>;

	/** tmp node pointer. */
	using tmp_node_ptr_t = persistent_ptr<node_base>;

	/** Bucket type. */
	struct bucket {
		/** Mutex type for buckets. */
		using mutex_t = tbb::spin_rw_mutex;

		/** Scoped lock type for mutex. */
		using scoped_t = mutex_t::scoped_lock;

		/** Bucket mutex. */
		pmem::obj::experimental::v<mutex_t> mutex;

		/** List of the nodes stored in the bucket. */
		node_base_ptr_t node_list;

		/** Pointer used to allocate new node. */
		tmp_node_ptr_t tmp_node;

		/** Default constructor */
		bucket() = default;

		/** Copy constructor is deleted */
		bucket(const bucket &) = delete;

		/** Assignment operator is deleted */
		void operator=(const bucket &) = delete;
	}; /* End of struct bucket */

	/** Count of embedded blocks. */
	static const size_type embedded_block = 1;

	/** Count of buckets in the embedded blocks. */
	static const size_type embedded_buckets = 1 << embedded_block;

	/** Count of segments in the first block. */
	static const size_type first_block = 8;

	/** Size of a pointer / table size. */
	static size_type const pointers_per_table =
		sizeof(segment_index_t) * 8; // one segment per bit

	/** Segment pointer. */
	using segment_ptr_t = persistent_ptr<bucket[]>;

	/** Bucket pointer. */
	using bucket_ptr_t = persistent_ptr<bucket>;

	/** Segment pointers table type. */
	using segments_table_t = segment_ptr_t[pointers_per_table];

	/** ID of persistent memory pool where hash map resides. */
	p<uint64_t> my_pool_uuid;

	/** Hash mask = sum of allocated segment sizes - 1. */
	p<hashcode_t> my_mask = embedded_buckets - 1;

	/**
	 * Segment pointers table. Also prevents false sharing between my_mask
	 * and my_size.
	 */
	segments_table_t my_table;

	/** Size of container in stored items. */
	p<size_type> my_size = 0; // It must be in separate cache line from
				  // my_mask due to performance effects

	/** Zero segment. */
	bucket my_embedded_segment[embedded_buckets];

	/** Segment mutex type. */
	using segment_enable_mutex_t = pmem::obj::mutex;

	/** Segment mutex used to enable new segment. */
	segment_enable_mutex_t my_segment_enable_mutex;

	/** @return segment index of given index in the array. */
	static segment_index_t
	segment_index_of(size_type index)
	{
		return segment_index_t(__TBB_Log2(index | 1));
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

	/** @return true if @arg ptr is valid pointer. */
	static bool
	is_valid(void *ptr)
	{
		return reinterpret_cast<uintptr_t>(ptr) > uintptr_t(63);
	}

	template <typename U>
	static bool
	is_valid(const persistent_pool_ptr<U> &ptr)
	{
		return ptr.raw() > uint64_t(63);
	}

	template <typename U>
	static bool
	is_valid(const persistent_ptr<U> &ptr)
	{
		return ptr.raw().off > uint64_t(63);
	}

	/**
	 * The class provides the way to access certain properties of segments
	 * used by hash map
	 */
	class segment_traits_t {
	public:
		/** PMDK has limitation for allocation size. */
		constexpr static size_type max_allocation_size =
			PMEMOBJ_MAX_ALLOC_SIZE;

		/** First big block that has fixed size. */
		constexpr static segment_index_t first_big_block = 28;
		// TODO: avoid hardcoded value; need constexpr
		// similar to: __TBB_Log2(max_allocation_size /
		// sizeof(bucket))

		/** Max number of buckets per segment. */
		constexpr static size_type big_block_size = size_type(1)
			<< first_big_block;

		/** Index of the first element in the first big block. */
		constexpr static size_type big_block_start_index = size_type(1)
			<< first_big_block;

	protected:
		/** @returns index of the first block in the @arg seg. */
		constexpr static segment_index_t
		first_block_in_segment(segment_index_t seg)
		{
			return seg < first_big_block ? seg
						     : first_big_block +
					(segment_index_t(1)
					 << (seg - first_big_block)) -
					1;
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
	}; /* End of class segment_traits_t */

	/**
	 * Implements logic to work with segments in the hashmap.
	 *
	 * When number of elements stored in the hashmap exceeds the threshold,
	 * the rehash operation is performed. Each new segment doubles the
	 * number of buckets in the hashmap.
	 *
	 * PMDK has limitation for max allocation size. Therefore, at some
	 * point new segment cannot be allocated as one contigious memory block.
	 * @class segment_facade_impl provides an abstraction and hides details
	 * of how actually segment is allocated in memory.
	 */
	template <typename BlockTable, typename SegmentTraits, bool is_const>
	class segment_facade_impl : public SegmentTraits {
	public:
		using table_reference =
			typename std::conditional<is_const, const BlockTable &,
						  BlockTable &>::type;

		using table_pointer =
			typename std::conditional<is_const, const BlockTable *,
						  BlockTable *>::type;

		using traits_type = SegmentTraits;

	private:
		using traits_type::block_size;
		using traits_type::blocks_in_segment;
		using traits_type::first_block_in_segment;

	public:
		segment_facade_impl(table_reference table, segment_index_t s)
		    : my_table(&table), my_seg(s)
		{
		}

		segment_facade_impl(const segment_facade_impl &src)
		    : my_table(src.my_table), my_seg(src.my_seg)
		{
		}

		segment_facade_impl(segment_facade_impl &&src) = default;

		~segment_facade_impl()
		{
		}

		segment_facade_impl &
		operator=(const segment_facade_impl &src)
		{
			my_table = src.my_table;
			my_seg = src.my_seg;
			return *this;
		}

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
		bucket &operator[](size_type i) const
		{
			assert(i < size());

			segment_index_t table_block =
				first_block_in_segment(my_seg);
			size_type b_size = block_size(table_block);

			table_block += i / b_size;
			i = i % b_size;

			return (*my_table)[table_block]
					  [static_cast<std::ptrdiff_t>(i)];
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
			block_range blocks = segment_blocks(my_seg);

			for (segment_index_t b = blocks.first;
			     b < blocks.second; ++b) {
				if ((*my_table)[b] == nullptr)
					make_persistent_atomic<bucket[]>(
						pop, (*my_table)[b],
						block_size(b));
			}
		}

		/**
		 * Deallocates the segment.
		 */
		void
		disable()
		{
			block_range blocks = segment_blocks(my_seg);

			for (segment_index_t b = blocks.first;
			     b < blocks.second; ++b) {
				if ((*my_table)[b] != nullptr) {
					delete_persistent<bucket[]>(
						(*my_table)[b], block_size(b));
					(*my_table)[b] = nullptr;
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

			for (segment_index_t b = blocks.first;
			     b < blocks.second; ++b) {
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

			return block_range(begin,
					   begin + blocks_in_segment(seg));
		}

		/** Pointer to the table of blocks */
		table_pointer my_table;

		/** Segment index */
		segment_index_t my_seg;
	}; /* End of class segment_facade_impl */

	/** Const segment facade type */
	using const_segment_facade_t =
		segment_facade_impl<segments_table_t, segment_traits_t, true>;

	/** Non-const segment facade type */
	using segment_facade_t =
		segment_facade_impl<segments_table_t, segment_traits_t, false>;

	/** Default constructor */
	hash_map_base()
	{
		PMEMoid oid = pmemobj_oid(this);

		assert(!OID_IS_NULL(oid));

		my_pool_uuid = oid.pool_uuid_lo;

		for (size_type i = 0; i < embedded_block; ++i) // fill the table
			my_table[i] = pmemobj_oid(my_embedded_segment +
						  segment_base(i));

		assert(embedded_block <= first_block);
	}

	/**
	 * Initialize buckets in the block.
	 */
	void
	init_buckets(segment_ptr_t ptr, size_type sz, bool is_initial)
	{
		persistent_ptr<node_base> rehashed_flag =
			is_initial ? rehashed : rehash_req;

		bucket *b = ptr.get();
		for (size_type i = 0; i < sz; ++i) {
			assert_not_locked(b[i].mutex.get());

			b[i].tmp_node = rehashed_flag;
			b[i].node_list = empty_bucket;
		}

		get_pool_base().persist(ptr.get(), sizeof(ptr[0]) * sz);
	}

	/**
	 * Initialize buckets in the new segment.
	 */
	void
	init_buckets(pool_base &pop, segment_facade_t &segment, bool is_initial)
	{
		persistent_ptr<node_base> rehashed_flag =
			is_initial ? rehashed : rehash_req;

		for (size_type i = 0; i < segment.size(); ++i) {
			bucket *b = &(segment[i]);

			assert_not_locked(b->mutex.get());

			b->tmp_node = rehashed_flag;
			b->node_list = empty_bucket;
		}

		// Flush in separate loop to avoid read-after-flush
		for (size_type i = 0; i < segment.size(); ++i) {
			bucket *b = &(segment[i]);
			pop.flush(b, sizeof(bucket));
		}

		pop.drain();
	}

	/**
	 * Add new node pointed by @arg b->tmp_node to bucket @arg b
	 */
	void
	add_to_bucket(bucket *b, pool_base &pop)
	{
		assert(b->tmp_node != rehash_req);
		assert(is_valid(b->tmp_node));
		assert(b->tmp_node->next == b->node_list);

		b->node_list = b->tmp_node; // bucket is locked
		pop.persist(&(b->node_list), sizeof(b->node_list));
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

			init_buckets(pop, new_segment, is_initial);

			// double it to get entire capacity of the container
			sz <<= 1;
		} else {
			// the first block
			// TODO: refactor this code to encapsulate logic in
			// segment_facade_t class
			static_assert(first_block <
					      segment_facade_t::first_big_block,
				      "first_block should be less than "
				      "first_big_block");

			assert(k == embedded_block);

			sz = segment_size(first_block);
			if (!is_valid(my_table[embedded_block])) {
				// Otherwise, it was allocated on previous run
				// but wasn't enabled
				make_persistent_atomic<bucket[]>(
					pop, my_table[embedded_block],
					sz - embedded_buckets);
			}

			init_buckets(my_table[embedded_block],
				     sz - embedded_buckets, is_initial);

			// TODO: fix this hack with tmp
			bucket_ptr_t tmp = my_table[embedded_block].raw();
			for (segment_index_t i = embedded_block + 1;
			     i < first_block; ++i) { // calc the offsets
				std::ptrdiff_t off =
					static_cast<std::ptrdiff_t>(
						segment_base(i) -
						segment_base(embedded_block));

				my_table[i] = (tmp + off).raw();
				pop.persist(my_table[i]);
			}
		}

		as_atomic(my_mask.get_rw()) = sz - 1;
		pop.persist(my_mask);
	}

	/**
	 * Get bucket by (masked) hashcode.
	 * @return pointer to the bucket.
	 */
	bucket *
	get_bucket(hashcode_t h) const
	{
		segment_index_t s = segment_index_of(h);

		h -= segment_base(s);

		const_segment_facade_t segment(my_table, s);

		assert(segment.is_valid());

		return &(segment[h]);
	}

	/**
	 * Internal serial rehashing helper
	 */
	void
	mark_rehashed_levels(hashcode_t h)
	{
		segment_index_t s = segment_index_of(h);

		segment_facade_t segment(my_table, s);

		while ((++segment).is_valid()) {
			if (segment[h].tmp_node == rehash_req) {
				segment[h].tmp_node = rehashed;

				// optimized segment_base(s)
				mark_rehashed_levels(h + ((hashcode_t)1 << s));
			}
		}
	}

	/**
	 * Check for mask race
	 */
	inline bool
	check_mask_race(hashcode_t h, hashcode_t &m) const
	{
		hashcode_t m_now, m_old = m;

		m_now = (hashcode_t)tbb::load<tbb::acquire>(
			as_atomic(my_mask.get_ro()));

		if (m_old != m_now)
			return check_rehashing_collision(h, m_old, m = m_now);

		return false;
	}

	/**
	 * Process mask race, check for rehashing collision
	 */
	bool
	check_rehashing_collision(hashcode_t h, hashcode_t m_old,
				  hashcode_t m) const
	{
		assert(m_old != m);

		// TODO?: m arg could be optimized out by
		// passing h = h&m
		if ((h & m_old) != (h & m)) {
			// mask changed for this hashcode, rare event
			// condition above proves that 'h' has some other
			// bits set beside 'm_old' find next applicable mask
			// after m_old

			for (++m_old; !(h & m_old); m_old <<= 1)
				;

			m_old = (m_old << 1) - 1; // get full mask from a bit

			assert((m_old & (m_old + 1)) == 0 && m_old <= m);

			// check whether it is rehashing/ed
			// Workaround: just comparing off part.
			// Need to investigate how to properly load with acquire
			// PMEMoid (128 bit)
			if (tbb::load<tbb::acquire>(as_atomic(
				    get_bucket(h & m_old)
					    ->tmp_node.raw()
					    .off)) != rehash_req.raw().off) {
				return true;
			}
		}

		return false;
	}

	/**
	 * Correct bucket state after crash.
	 */
	void
	correct_bucket(bucket *b)
	{
		pool_base pop = get_pool_base();

		if (b->tmp_node != nullptr) {
			if (b->tmp_node->next == b->node_list)
				insert_new_node(pop, b);

			b->tmp_node = nullptr;
			pop.persist(b->tmp_node);
		}
	}

	/**
	 * Insert a node.
	 * @return new size.
	 */
	size_type
	insert_new_node(pool_base &pop, bucket *b)
	{
		add_to_bucket(b, pop);

		// prefix form is to enforce allocation after the first item
		// inserted
		size_t sz = ++as_atomic(my_size.get_rw());
		pop.persist(&my_size, sizeof(my_size));

		b->tmp_node = nullptr;
		pop.persist(b->tmp_node);

		return sz;
	}

	/**
	 * Checks load factor and decides if new segment should be allocated.
	 * @return true if new segment was allocated and false otherwise
	 */
	bool
	check_growth(hashcode_t mask, size_type sz)
	{
		if (sz >= mask) {
			segment_index_t new_seg = static_cast<segment_index_t>(
				__TBB_Log2(mask +
					   1)); // optimized segment_index_of

			assert(segment_facade_t(my_table, new_seg - 1)
				       .is_valid());

			std::unique_lock<segment_enable_mutex_t> lock(
				my_segment_enable_mutex, std::try_to_lock);

			if (lock) {
				if (tbb::load<tbb::acquire>(as_atomic(
					    my_mask.get_ro())) == mask) {
					// Otherwise, other thread enable
					// this segment
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

		bool is_initial = !my_size;

		for (size_type m = my_mask; buckets > m; m = my_mask)
			enable_segment(segment_index_of(m + 1), is_initial);
	}

	/**
	 * Swap hash_map_base
	 * @throws std::runtime_error in case of PMDK transaction failed
	 */
	void
	internal_swap(hash_map_base &table)
	{
		pool_base p = get_pool_base();
		try {
			transaction::manual tx(p);

			this->my_pool_uuid.swap(table.my_pool_uuid);
			this->my_mask.swap(table.my_mask);
			this->my_size.swap(table.my_size);

			for (size_type i = 0; i < embedded_buckets; ++i)
				this->my_embedded_segment[i].node_list.swap(
					table.my_embedded_segment[i].node_list);

			for (size_type i = embedded_block;
			     i < pointers_per_table; ++i)
				this->my_table[i].swap(table.my_table[i]);

			transaction::commit();
		} catch (const pmem::transaction_error &e) {
			throw std::runtime_error(e);
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
template <typename Container, typename Value>
class persistent_hash_map_iterator
    : public std::iterator<std::forward_iterator_tag, Value> {
	using map_type = Container;
	using node = typename Container::node;
	using node_base = hash_map_base::node_base;
	using bucket = hash_map_base::bucket;

	template <typename C, typename T, typename U>
	friend bool operator==(const persistent_hash_map_iterator<C, T> &i,
			       const persistent_hash_map_iterator<C, U> &j);

	template <typename C, typename T, typename U>
	friend bool operator!=(const persistent_hash_map_iterator<C, T> &i,
			       const persistent_hash_map_iterator<C, U> &j);

	template <typename C, typename T, typename U>
	friend ptrdiff_t operator-(const persistent_hash_map_iterator<C, T> &i,
				   const persistent_hash_map_iterator<C, U> &j);

	template <typename C, typename U>
	friend class persistent_hash_map_iterator;

	void
	advance_to_next_bucket()
	{ // TODO?: refactor to iterator_base class
		size_t k = my_index + 1;

		assert(my_bucket);

		while (k <= my_map->my_mask) {
			// Following test uses 2's-complement wizardry
			if (k & (k - 2)) // not the beginning of a segment
				++my_bucket;
			else
				my_bucket = my_map->get_bucket(k);

			if (hash_map_base::is_valid(my_bucket->node_list)) {
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

#if !defined(_MSC_VER) || defined(__INTEL_COMPILER)
	template <typename Key, typename T, typename HashCompare>
	friend class experimental::concurrent_hash_map;
#else
public: // workaround
#endif
	/** Concurrent_hash_map over which we are iterating. */
	const Container *my_map;

	/** Index in hash table for current item. */
	size_t my_index;

	/** Pointer to bucket. */
	const bucket *my_bucket;

	/** Pointer to node that has current item. */
	node *my_node;

	persistent_hash_map_iterator(const Container &map, size_t index,
				     const bucket *b, node_base *n);

public:
	/** Construct undefined iterator. */
	persistent_hash_map_iterator()
	    : my_map(), my_index(), my_bucket(), my_node()
	{
	}

	/** Copy constructor. */
	persistent_hash_map_iterator(
		const persistent_hash_map_iterator<
			Container, typename Container::value_type> &other)
	    : my_map(other.my_map),
	      my_index(other.my_index),
	      my_bucket(other.my_bucket),
	      my_node(other.my_node)
	{
	}

	/** Indirection (dereference). */
	Value &operator*() const
	{
		assert(hash_map_base::is_valid(my_node));
		return my_node->item;
	}

	/** Member access. */
	Value *operator->() const
	{
		return &operator*();
	}

	/** Prefix increment. */
	persistent_hash_map_iterator &operator++();

	/** Postfix increment. */
	persistent_hash_map_iterator
	operator++(int)
	{
		persistent_hash_map_iterator old(*this);
		operator++();
		return old;
	}
};

template <typename Container, typename Value>
persistent_hash_map_iterator<Container, Value>::persistent_hash_map_iterator(
	const Container &map, size_t index, const bucket *b, node_base *n)
    : my_map(&map),
      my_index(index),
      my_bucket(b),
      my_node(static_cast<node *>(n))
{
	if (b && !hash_map_base::is_valid(n))
		advance_to_next_bucket();
}

template <typename Container, typename Value>
persistent_hash_map_iterator<Container, Value> &
persistent_hash_map_iterator<Container, Value>::operator++()
{
	my_node =
		static_cast<node *>(my_node->next.get((my_map->my_pool_uuid)));

	if (!my_node)
		advance_to_next_bucket();

	return *this;
}

template <typename Container, typename T, typename U>
bool
operator==(const persistent_hash_map_iterator<Container, T> &i,
	   const persistent_hash_map_iterator<Container, U> &j)
{
	return i.my_node == j.my_node && i.my_map == j.my_map;
}

template <typename Container, typename T, typename U>
bool
operator!=(const persistent_hash_map_iterator<Container, T> &i,
	   const persistent_hash_map_iterator<Container, U> &j)
{
	return i.my_node != j.my_node || i.my_map != j.my_map;
}

/**
 * Range class used with concurrent_hash_map
 * @ingroup containers.
 */
template <typename Iterator>
class hash_map_range {
	using map_type = typename Iterator::map_type;
	Iterator my_begin;
	Iterator my_end;
	mutable Iterator my_midpoint;
	size_t my_grainsize;
	/** Set my_midpoint to point approximately half way between my_begin and
	 * my_end. */
	void set_midpoint() const;
	template <typename U>
	friend class hash_map_range;

public:
	/** Type for size of a range. */
	using size_type = std::size_t;
	using value_type = typename Iterator::value_type;
	using reference = typename Iterator::reference;
	using difference_type = typename Iterator::difference_type;
	using iterator = Iterator;

	/** True if range is empty. */
	bool
	empty() const
	{
		return my_begin == my_end;
	}

	/** True if range can be partitioned into two subranges. */
	bool
	is_divisible() const
	{
		return my_midpoint != my_end;
	}

	/** Split range. */
	hash_map_range(hash_map_range &r, tbb::split)
	    : my_end(r.my_end), my_grainsize(r.my_grainsize)
	{
		r.my_end = my_begin = r.my_midpoint;

		assert(!empty());
		assert(!r.empty());

		set_midpoint();

		r.set_midpoint();
	}

	/** Type conversion. */
	template <typename U>
	hash_map_range(hash_map_range<U> &r)
	    : my_begin(r.my_begin),
	      my_end(r.my_end),
	      my_midpoint(r.my_midpoint),
	      my_grainsize(r.my_grainsize)
	{
	}

	/**
	 * Init range with container and grainsize specified
	 */
	hash_map_range(const map_type &map, size_type grainsize_ = 1)
	    : my_begin(Iterator(map, 0, map.my_embedded_segment,
				map.my_embedded_segment->node_list)),
	      my_end(Iterator(map, map.my_mask + 1, 0, 0)),
	      my_grainsize(grainsize_)
	{
		assert(grainsize_ > 0);

		set_midpoint();
	}

	/**
	 * @returns iterator to the range begin.
	 */
	const Iterator &
	begin() const
	{
		return my_begin;
	}

	/**
	 * @returns iterator to the range end.
	 */
	const Iterator &
	end() const
	{
		return my_end;
	}

	/**
	 * The grain size for this range.
	 */
	size_type
	grainsize() const
	{
		return my_grainsize;
	}
};

template <typename Iterator>
void
hash_map_range<Iterator>::set_midpoint() const
{
	// Split by groups of nodes
	size_t m = my_end.my_index - my_begin.my_index;

	if (m > my_grainsize) {
		m = my_begin.my_index + m / 2u;

		hash_map_base::bucket *b = my_begin.my_map->get_bucket(m);

		my_midpoint = Iterator(*my_begin.my_map, m, b, b->node_list);
	} else {
		my_midpoint = my_end;
	}

	assert(my_begin.my_index <= my_midpoint.my_index);
	assert(my_midpoint.my_index <= my_end.my_index);
	assert(my_begin != my_midpoint || my_begin == my_end);
}

} // namespace internal
//! @endcond

/**
 * Persistent memory aware implementation of Intel TBB concurrent_hash_map.
 */
template <typename Key, typename T, typename HashCompare>
class concurrent_hash_map : protected internal::hash_map_base {
	template <typename Container, typename Value>
	friend class internal::persistent_hash_map_iterator;

	template <typename I>
	friend class internal::hash_map_range;

public:
	using key_type = Key;
	using mapped_type = T;
	using value_type = std::pair<const Key, T>;
	using size_type = hash_map_base::size_type;
	using difference_type = ptrdiff_t;
	using pointer = value_type *;
	using const_pointer = const value_type *;
	using reference = value_type &;
	using const_reference = const value_type &;
	using iterator =
		internal::persistent_hash_map_iterator<concurrent_hash_map,
						       value_type>;
	using const_iterator =
		internal::persistent_hash_map_iterator<concurrent_hash_map,
						       const value_type>;
	using range_type = internal::hash_map_range<iterator>;
	using const_range_type = internal::hash_map_range<const_iterator>;

protected:
	friend class const_accessor;
	struct node;
	HashCompare my_hash_compare;

	/**
	 * Node structure to store Key/Value pair.
	 */
	struct node : public node_base {
		value_type item;
		node(const Key &key, const node_base_ptr_t &_next = OID_NULL)
		    : node_base(_next), item(key, T())
		{
		}

		node(const Key &key, const T &t,
		     const node_base_ptr_t &_next = OID_NULL)
		    : node_base(_next), item(key, t)
		{
		}

		node(const Key &key, T &&t, node_base_ptr_t &&_next = OID_NULL)
		    : node_base(std::move(_next)), item(key, std::move(t))
		{
		}

		node(value_type &&i, node_base_ptr_t &&_next = OID_NULL)
		    : node_base(std::move(_next)), item(std::move(i))
		{
		}

		template <typename... Args>
		node(Args &&... args, node_base_ptr_t &&_next = OID_NULL)
		    : node_base(std::forward<node_base_ptr_t>(_next)),
		      item(std::forward<Args>(args)...)
		{
		}

#if __TBB_COPY_FROM_NON_CONST_REF_BROKEN
		node(value_type &i, node_base_ptr_t &_next = OID_NULL)
		    : node_base(_next), item(const_cast<const value_type &>(i))
		{
		}
#endif //__TBB_COPY_FROM_NON_CONST_REF_BROKEN

		node(const value_type &i,
		     const node_base_ptr_t &_next = OID_NULL)
		    : node_base(_next), item(i)
		{
		}
	};

	using persistent_node_ptr_t = persistent_pool_ptr<node>;

	void
	delete_node(const node_base_ptr_t &n)
	{
		delete_persistent<node>(
			static_persistent_pool_pointer_cast<node>(n)
				.get_persistent_ptr(my_pool_uuid));
	}

	static void
	allocate_node_copy_construct(pool_base &pop,
				     persistent_ptr<node> &node_ptr,
				     const Key &key, const T *t,
				     const node_base_ptr_t &next = OID_NULL)
	{
		make_persistent_atomic<node>(pop, node_ptr, key, *t, next);
	}

	static void
	allocate_node_move_construct(pool_base &pop,
				     persistent_ptr<node> &node_ptr,
				     const Key &key, const T *t,
				     const node_base_ptr_t &next = OID_NULL)
	{
		make_persistent_atomic<node>(pop, node_ptr, key,
					     std::move(*const_cast<T *>(t)),
					     next);
	}
	template <typename... Args>
	static void
	allocate_node_emplace_construct(pool_base &pop,
					persistent_ptr<node> &node_ptr,
					Args &&... args)
	{
		make_persistent_atomic<node>(pop, node_ptr,
					     std::forward<Args>(args)...);
	}

	static void
	allocate_node_default_construct(pool_base &pop,
					persistent_ptr<node> &node_ptr,
					const Key &key, const T *,
					const node_base_ptr_t &next = OID_NULL)
	{
		make_persistent_atomic<node>(pop, node_ptr, key, next);
	}

	static void
	do_not_allocate_node(pool_base &, persistent_ptr<node> &node_ptr,
			     const Key &, const T *,
			     const node_base_ptr_t &next = OID_NULL)
	{
		assert(false);
	}

	persistent_node_ptr_t
	search_bucket(const key_type &key, bucket *b) const
	{
		assert(b->tmp_node != internal::rehash_req);

		persistent_node_ptr_t n =
			static_persistent_pool_pointer_cast<node>(b->node_list);

		while (is_valid(n) &&
		       !my_hash_compare.equal(
			       key, n.get(my_pool_uuid)->item.first)) {
			n = static_persistent_pool_pointer_cast<node>(
				n.get(my_pool_uuid)->next);
		}

		return n;
	}

	/**
	 * Bucket accessor is to find, rehash, acquire a lock, and access a
	 * bucket
	 */
	class bucket_accessor : public bucket::scoped_t {
		bucket *my_b;

	public:
		bucket_accessor(concurrent_hash_map *base, const hashcode_t h,
				bool writer = false)
		{
			acquire(base, h, writer);
		}

		/**
		 * Find a bucket by masked hashcode, optionally rehash, and
		 * acquire the lock
		 */
		inline void
		acquire(concurrent_hash_map *base, const hashcode_t h,
			bool writer = false)
		{
			my_b = base->get_bucket(h);

			// TODO: actually, notification is unnecessary here,
			// just hiding double-check
			// Workaround: need to load tmp_node persistent ptr with
			// acquire, but seems only off is enough.
			if (tbb::load<tbb::acquire>(internal::as_atomic(
				    my_b->tmp_node.raw().off)) != uint64_t(0) &&
			    try_acquire(my_b->mutex.get(), /*write=*/true)) {
				if (my_b->tmp_node == internal::rehash_req) {
					// recursive rehashing
					base->rehash_bucket<false>(my_b, h);
				} else {
					base->correct_bucket(my_b);
				}
			} else {
				bucket::scoped_t::acquire(my_b->mutex.get(),
							  writer);
			}

			assert(my_b->tmp_node == nullptr);
		}

		/**
		 * Check whether bucket is locked for write
		 */
		bool
		is_writer() const
		{
			return bucket::scoped_t::is_writer;
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
				       const hashcode_t h, bool writer = false)
		{
			acquire(base, h, writer);
		}

		/*
		 * Find a bucket by masked hashcode, optionally rehash
		 */
		inline void
		acquire(concurrent_hash_map *base, const hashcode_t h,
			bool writer = false)
		{
			my_b = base->get_bucket(h);

			if (my_b->tmp_node != nullptr) {
				if (my_b->tmp_node == internal::rehash_req) {
					// recursive rehashing
					base->rehash_bucket<true>(my_b, h);
				} else {
					base->correct_bucket(my_b);
				}
			}

			assert(my_b->tmp_node == nullptr);
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
		 * This method is added for consistency with bucket_accessor
		 * class
		 *
		 * @return Always returns true
		 */
		bool
		upgrade_to_writer() const
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

	hashcode_t
	get_hash_code(node_base_ptr_t &n)
	{
		return my_hash_compare.hash(
			static_persistent_pool_pointer_cast<node>(n)(
				my_pool_uuid)
				->item.first);
	}

	template <bool serial>
	void
	rehash_bucket(bucket *b_new, const hashcode_t h)
	{
		using accessor_type = typename std::conditional<
			serial, serial_bucket_accessor, bucket_accessor>::type;

		if (!serial) {
			assert(*(intptr_t *)(&b_new->mutex.get()));
		}

		assert(h > 1);

		// get parent mask from the topmost bit
		hashcode_t mask = (1u << __TBB_Log2(h)) - 1;

		accessor_type b_old(this, h & mask);

		mask = (mask << 1) | 1; // get full mask for new bucket

		assert((mask & (mask + 1)) == 0 && (h & mask) == h);

		pool_base pop = get_pool_base();
		node_base_ptr_t *p_new = &(b_new->node_list);
		bool restore_after_crash = *p_new != nullptr;

	restart:
		for (node_base_ptr_t *
			     p_old = &(b_old->node_list),
			    n = __TBB_load_with_acquire<const uint64_t>(
				    p_old->raw());
		     is_valid(n); n = *p_old) {
			hashcode_t c = get_hash_code(n);
#ifndef NDEBUG
			hashcode_t bmask = h & (mask >> 1);

			bmask = bmask == 0
				? 1 // minimal mask of parent bucket
				: (1u << (__TBB_Log2(bmask) + 1)) - 1;

			assert((c & bmask) == (h & bmask));
#endif

			if ((c & mask) == h) {
				if (!b_old.is_writer() &&
				    !b_old.upgrade_to_writer()) {
					goto restart;
					// node ptr can be invalid due to
					// concurrent erase
				}

				if (restore_after_crash) {
					while (*p_new != nullptr &&
					       (mask & get_hash_code(*p_new)) ==
						       h &&
					       *p_new != n) {
						p_new = &((*p_new)(my_pool_uuid)
								  ->next);
					}

					restore_after_crash = false;
				}

				// Add to new b_new
				*p_new = n;
				pop.persist(p_new, sizeof(*p_new));

				// exclude from b_old
				*p_old = n(my_pool_uuid)->next;
				pop.persist(p_old, sizeof(*p_old));

				p_new = &(n(my_pool_uuid)->next);
			} else {
				// iterate to next item
				p_old = &(n(my_pool_uuid)->next);
			}
		}

		if (restore_after_crash) {
			while (*p_new != nullptr &&
			       (mask & get_hash_code(*p_new)) == h)
				p_new = &((*p_new)(my_pool_uuid)->next);
		}

		*p_new = nullptr;
		pop.persist(p_new, sizeof(*p_new));

		// TODO: now we update only offset field, because pool_uuid is
		// the same. Need to assign whole pointer, but there is
		// compilation issue
		//__TBB_store_with_release( b_new->tmp_node, internal::rehashed
		//); // mark rehashed
		__TBB_store_with_release(
			b_new->tmp_node.raw_ptr()->off,
			internal::rehashed.raw().off); // mark rehashed
		pop.persist(b_new->tmp_node);
	}

	struct call_clear_on_leave {
		concurrent_hash_map *my_ch_map;
		call_clear_on_leave(concurrent_hash_map *a_ch_map)
		    : my_ch_map(a_ch_map)
		{
		}

		void
		dismiss()
		{
			my_ch_map = 0;
		}

		~call_clear_on_leave()
		{
			if (my_ch_map)
				my_ch_map->clear();
		}
	};

public:
	class accessor;
	//! Combines data access, locking, and garbage collection.
	class const_accessor
	    : private node::scoped_t /*which derived from no_copy*/ {
		friend class concurrent_hash_map<Key, T, HashCompare>;
		friend class accessor;
		using node_ptr_t = pmem::obj::persistent_ptr<node>;

	public:
		//! Type of value
		using value_type =
			const typename concurrent_hash_map::value_type;

		//! True if result is empty.
		bool
		empty() const
		{
			return !my_node;
		}

		//! Set to null
		void
		release()
		{
			if (my_node) {
				node::scoped_t::release();
				my_node = 0;
			}
		}

		//! Return reference to associated value in hash table.
		const_reference operator*() const
		{
			assert(my_node);

			return my_node->item;
		}

		//! Return pointer to associated value in hash table.
		const_pointer operator->() const
		{
			return &operator*();
		}

		//! Create empty result
		const_accessor() : my_node(OID_NULL)
		{
		}

		//! Destroy result after releasing the underlying reference.
		~const_accessor()
		{
			my_node = OID_NULL; // scoped lock's release() is called
					    // in its destructor
		}

	protected:
		bool
		is_writer()
		{
			return node::scoped_t::is_writer;
		}

		node_ptr_t my_node;

		hashcode_t my_hash;
	};

	//! Allows write access to elements and combines data access, locking,
	//! and garbage collection.
	class accessor : public const_accessor {
	public:
		//! Type of value
		using value_type = typename concurrent_hash_map::value_type;

		//! Return reference to associated value in hash table.
		reference operator*() const
		{
			assert(this->my_node);

			return this->my_node->item;
		}

		//! Return pointer to associated value in hash table.
		pointer operator->() const
		{
			return &operator*();
		}
	};

	//! Construct empty table.
	concurrent_hash_map() : internal::hash_map_base()
	{
	}

	//! Construct empty table with n preallocated buckets. This number
	//! serves also as initial concurrency level.
	concurrent_hash_map(size_type n) : internal::hash_map_base()
	{
		reserve(n);
	}

	//! Copy constructor
	concurrent_hash_map(const concurrent_hash_map &table)
	    : internal::hash_map_base()
	{
		assert(false);

		internal_copy(table);
	}

	//! Move constructor
	concurrent_hash_map(concurrent_hash_map &&table)
	    : internal::hash_map_base()
	{
		swap(table);
	}

	//! Construction table with copying iteration range
	template <typename I>
	concurrent_hash_map(I first, I last)
	{
		reserve(std::distance(first, last)); // TODO: load_factor?

		internal_copy(first, last);
	}

	//! Construct table with initializer list
	concurrent_hash_map(std::initializer_list<value_type> il)
	{
		reserve(il.size());

		internal_copy(il.begin(), il.end());
	}

	/**
	 * Assignment
	 * @throws std::runtime_error in case of PMDK transaction failure
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
	 * @throws std::runtime_error in case of PMDK transaction failure
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
	 */
	void rehash(size_type n = 0);

	/**
	 * Clear hash map content
	 * @throws std::runtime_error in case of PMDK transaction failure
	 */
	void clear();

	//! Clear table and destroy it.
	~concurrent_hash_map()
	{
		clear();
	}

	//------------------------------------------------------------------------
	// Parallel algorithm support
	//------------------------------------------------------------------------
	range_type
	range(size_type grainsize = 1)
	{
		return range_type(*this, grainsize);
	}

	const_range_type
	range(size_type grainsize = 1) const
	{
		return const_range_type(*this, grainsize);
	}

	//------------------------------------------------------------------------
	// STL support - not thread-safe methods
	//------------------------------------------------------------------------
	iterator
	begin()
	{
		return iterator(
			*this, 0, my_embedded_segment,
			my_embedded_segment->node_list.get(my_pool_uuid));
	}

	iterator
	end()
	{
		return iterator(*this, 0, 0, 0);
	}

	const_iterator
	begin() const
	{
		return const_iterator(
			*this, 0, my_embedded_segment,
			my_embedded_segment->node_list.get(my_pool_uuid));
	}

	const_iterator
	end() const
	{
		return const_iterator(*this, 0, 0, 0);
	}

	std::pair<iterator, iterator>
	equal_range(const Key &key)
	{
		return internal_equal_range(key, end());
	}

	std::pair<const_iterator, const_iterator>
	equal_range(const Key &key) const
	{
		return internal_equal_range(key, end());
	}

	//! @returns number of items in table.
	size_type
	size() const
	{
		return my_size.get_ro();
	}

	//! @returns true if size()==0.
	bool
	empty() const
	{
		return my_size.get_ro() == 0;
	}

	//! Upper bound on size.
	size_type
	max_size() const
	{
		return (~size_type(0)) / sizeof(node);
	}

	//! @returns the current number of buckets
	size_type
	bucket_count() const
	{
		return my_mask.get_ro() + 1;
	}

	//! swap two instances. Iterators are invalidated
	void swap(concurrent_hash_map &table);

	//------------------------------------------------------------------------
	// concurrent map operations
	//------------------------------------------------------------------------

	//! @return count of items (0 or 1)
	size_type
	count(const Key &key) const
	{
		return const_cast<concurrent_hash_map *>(this)->lookup(
			/*insert*/ false, key, nullptr, nullptr,
			/*write=*/false, &do_not_allocate_node);
	}

	/**
	 * Find item and acquire a read lock on the item.
	 * @return true if item is found, false otherwise.
	 */
	bool
	find(const_accessor &result, const Key &key) const
	{
		result.release();

		return const_cast<concurrent_hash_map *>(this)->lookup(
			/*insert*/ false, key, nullptr, &result,
			/*write=*/false, &do_not_allocate_node);
	}

	/**
	 * Find item and acquire a write lock on the item.
	 * @return true if item is found, false otherwise.
	 */
	bool
	find(accessor &result, const Key &key)
	{
		result.release();

		return lookup(/*insert*/ false, key, nullptr, &result,
			      /*write*/ true, &do_not_allocate_node);
	}

	/**
	 * Insert item (if not already present) and
	 * acquire a read lock on the item.
	 * @return true if item is new.
	 * @throw std::bad_alloc on allocation failure.
	 */
	bool
	insert(const_accessor &result, const Key &key)
	{
		result.release();

		return lookup(/*insert*/ true, key, nullptr, &result,
			      /*write=*/false,
			      &allocate_node_default_construct);
	}

	/**
	 * Insert item (if not already present) and
	 * acquire a write lock on the item.
	 * @returns true if item is new.
	 * @throw std::bad_alloc on allocation failure.
	 */
	bool
	insert(accessor &result, const Key &key)
	{
		result.release();

		return lookup(/*insert*/ true, key, nullptr, &result,
			      /*write=*/true, &allocate_node_default_construct);
	}

	/**
	 * Insert item by copying if there is no such key present already and
	 * acquire a read lock on the item.
	 * @return true if item is new.
	 * @throw std::bad_alloc on allocation failure.
	 */
	bool
	insert(const_accessor &result, const value_type &value)
	{
		result.release();

		return lookup(/*insert*/ true, value.first, &value.second,
			      &result, /*write=*/false,
			      &allocate_node_copy_construct);
	}

	//! Insert item by copying if there is no such key present already and
	//! acquire a write lock on the item.
	/** @return true if item is new. */
	bool
	insert(accessor &result, const value_type &value)
	{
		result.release();

		return lookup(/*insert*/ true, value.first, &value.second,
			      &result, /*write=*/true,
			      &allocate_node_copy_construct);
	}

	//! Insert item by copying if there is no such key present already
	/** @return true if item is inserted. */
	bool
	insert(const value_type &value)
	{
		return lookup(/*insert*/ true, value.first, &value.second,
			      nullptr,
			      /*write=*/false, &allocate_node_copy_construct);
	}

	/** Insert item by copying if there is no such key present already and
	 * acquire a read lock on the item.
	 * @return true if item is new.
	 * @throw std::bad_alloc on allocation failure.
	 */
	bool
	insert(const_accessor &result, value_type &&value)
	{
		return generic_move_insert(result, std::move(value));
	}

	/**
	 * Insert item by copying if there is no such key present already and
	 * acquire a write lock on the item.
	 * @return true if item is new.
	 * @throw std::bad_alloc on allocation failure.
	 */
	bool
	insert(accessor &result, value_type &&value)
	{
		return generic_move_insert(result, std::move(value));
	}

	/**
	 * Insert item by copying if there is no such key present already
	 * @return true if item is inserted.
	 * @throw std::bad_alloc on allocation failure.
	 */
	bool
	insert(value_type &&value)
	{
		return generic_move_insert(accessor_not_used(),
					   std::move(value));
	}

	/**
	 * Insert item by copying if there is no such key present already and
	 * acquire a read lock on the item.
	 * @return true if item is new.
	 * @throw std::bad_alloc on allocation failure.
	 */
	template <typename... Args>
	bool
	emplace(const_accessor &result, Args &&... args)
	{
		return generic_emplace(result, std::forward<Args>(args)...);
	}

	/**
	 * Insert item by copying if there is no such key present already and
	 * acquire a write lock on the item.
	 * @return true if item is new.
	 * @throw std::bad_alloc on allocation failure.
	 */
	template <typename... Args>
	bool
	emplace(accessor &result, Args &&... args)
	{
		return generic_emplace(result, std::forward<Args>(args)...);
	}

	/**
	 * Insert item by copying if there is no such key present already
	 * @return true if item is inserted.
	 * @throw std::bad_alloc on allocation failure.
	 */
	template <typename... Args>
	bool
	emplace(Args &&... args)
	{
		return generic_emplace(accessor_not_used(),
				       std::forward<Args>(args)...);
	}

	/**
	 * Insert range [first, last)
	 * @throw std::bad_alloc on allocation failure.
	 */
	template <typename I>
	void
	insert(I first, I last)
	{
		for (; first != last; ++first)
			insert(*first);
	}

	/**
	 * Insert initializer list
	 * @throw std::bad_alloc on allocation failure.
	 */
	void
	insert(std::initializer_list<value_type> il)
	{
		insert(il.begin(), il.end());
	}

	/**
	 * Remove element with corresponding key
	 * @return true if element was deleted by this call
	 * @throws std::runtime_error in case of PMDK unable to free the memory
	 */
	bool erase(const Key &key);

	/**
	 * Remove element by const_accessor
	 * @return true if element was deleted by this call
	 * @throws std::runtime_error in case of PMDK unable to free the memory
	 */
	bool
	erase(const_accessor &item_accessor)
	{
		return exclude(item_accessor);
	}

	/**
	 * Remove element by accessor
	 * @return true if element was deleted by this call
	 * @throws std::runtime_error in case of PMDK unable to free the memory
	 */
	bool
	erase(accessor &item_accessor)
	{
		return exclude(item_accessor);
	}

protected:
	//! Insert or find item and optionally acquire a lock on the item.
	bool lookup(bool op_insert, const Key &key, const T *t,
		    const_accessor *result, bool write,
		    void (*allocate_node)(pool_base &, persistent_ptr<node> &,
					  const Key &, const T *,
					  const node_base_ptr_t &));

	struct accessor_not_used {
		void
		release()
		{
		}
	};

	friend const_accessor *
	accessor_location(accessor_not_used const &)
	{
		return nullptr;
	}

	friend const_accessor *
	accessor_location(const_accessor &a)
	{
		return &a;
	}

	friend bool
	is_write_access_needed(accessor const &)
	{
		return true;
	}

	friend bool
	is_write_access_needed(const_accessor const &)
	{
		return false;
	}

	friend bool
	is_write_access_needed(accessor_not_used const &)
	{
		return false;
	}

	template <typename Accessor>
	bool
	generic_move_insert(Accessor &&result, value_type &&value)
	{
		result.release();
		return lookup(/*insert*/ true, value.first, &value.second,
			      accessor_location(result),
			      is_write_access_needed(result),
			      &allocate_node_move_construct);
	}

	template <typename Accessor, typename... Args>
	bool
	generic_emplace(Accessor &&result, Args &&... args)
	{
		result.release();
		node *node_ptr = allocate_node_emplace_construct(
			std::forward<Args>(args)...);
		return lookup(/*insert*/ true, node_ptr->item.first, NULL,
			      accessor_location(result),
			      is_write_access_needed(result),
			      &do_not_allocate_node, node_ptr);
	}

	//! delete item by accessor
	bool exclude(const_accessor &item_accessor);

	void clear_segment(segment_index_t s);

	//! Copy "source" to *this, where *this must start out empty.
	void internal_copy(const concurrent_hash_map &source);

	template <typename I>
	void internal_copy(I first, I last);

}; // class concurrent_hash_map

template <typename Key, typename T, typename HashCompare>
bool
concurrent_hash_map<Key, T, HashCompare>::lookup(
	bool op_insert, const Key &key, const T *t, const_accessor *result,
	bool write,
	void (*allocate_node)(pool_base &, persistent_ptr<node> &, const Key &,
			      const T *, const node_base_ptr_t &))
{
	assert(!result || !result->my_node);

	bool return_value = false;
	hashcode_t const h = my_hash_compare.hash(key);
	hashcode_t m = (hashcode_t)tbb::load<tbb::acquire>(
		internal::as_atomic(my_mask.get_ro()));
	persistent_node_ptr_t n;
	size_type sz = 0;

restart : { // lock scope
	assert((m & (m + 1)) == 0);

	return_value = false;

	// get bucket
	bucket_accessor b(this, h & m);

	// find a node
	n = search_bucket(key, b.get());

	if (op_insert) {
		// [opt] insert a key
		if (!n) {
			if (!b.is_writer() &&
			    !b.upgrade_to_writer()) { // TODO: improved
						      // insertion
				// Rerun search_list, in case another thread
				// inserted the item during the upgrade.
				n = search_bucket(key, b.get());
				if (is_valid(n)) { // unfortunately, it did
					b.downgrade_to_reader();
					goto exists;
				}
			}

			if (check_mask_race(h, m))
				goto restart; // b.release() is done in ~b().

			assert(b->tmp_node == nullptr);

			// insert and set flag to grow the container
			pool_base pop = get_pool_base();

			allocate_node(pop,
				      reinterpret_cast<persistent_ptr<node> &>(
					      b->tmp_node),
				      key, t, b->node_list);

			n = b->tmp_node;
			sz = insert_new_node(pop, b.get());
			return_value = true;
		}
	} else { // find or count
		if (!n) {
			if (check_mask_race(h, m))
				goto restart; // b.release() is done in ~b().
					      // TODO: replace by continue
			return false;
		}

		return_value = true;
	}
exists:
	if (!result)
		goto check_growth;

	// TODO: the following seems as generic/regular operation
	// acquire the item
	if (!result->try_acquire(n.get(my_pool_uuid)->mutex.get(), write)) {
		for (tbb::internal::atomic_backoff backoff(true);;) {
			if (result->try_acquire(
				    n.get(my_pool_uuid)->mutex.get(), write))
				break;

			if (!backoff.bounded_pause()) {
				// the wait takes really long, restart the
				// operation
				b.release();

				assert(!op_insert || !return_value);

				__TBB_Yield();

				m = (hashcode_t)tbb::load<tbb::acquire>(
					internal::as_atomic(my_mask.get_ro()));

				goto restart;
			}
		}
	}
} // lock scope

	result->my_node = n.get_persistent_ptr(my_pool_uuid);
	result->my_hash = h;
check_growth:
	// [opt] grow the container
	check_growth(m, sz);

	return return_value;
}

template <typename Key, typename T, typename HashCompare>
bool
concurrent_hash_map<Key, T, HashCompare>::exclude(const_accessor &item_accessor)
{
	assert(item_accessor.my_node);

	node_base_ptr_t const n = item_accessor.my_node;
	hashcode_t const h = item_accessor.my_hash;
	hashcode_t m = (hashcode_t)tbb::load<tbb::acquire>(
		internal::as_atomic(my_mask.get_ro()));
	pool_base pop = get_pool_base();

	do {
		// get bucket
		bucket_accessor b(this, h & m, /*writer=*/true);
		node_base_ptr_t *p = &b->node_list;

		while (*p && *p != n)
			p = &(*p)(my_pool_uuid)->next;

		if (!*p) { // someone else was first
			if (check_mask_race(h, m))
				continue;

			item_accessor.release();

			return false;
		}

		assert(*p == n);

		b->tmp_node = n(my_pool_uuid);
		pop.persist(b->tmp_node);

		*p = b->tmp_node->next; // remove from container
		pop.persist(p, sizeof(node_base_ptr_t));

		--(internal::as_atomic(my_size.get_rw()));
		pop.persist(my_size);

		if (!item_accessor.is_writer()) // need to get exclusive lock
			item_accessor.upgrade_to_writer();

		item_accessor.release();

		try {
			transaction::manual tx(pop);

			// Only one thread can delete it due
			// to write lock on the bucket
			delete_node(b->tmp_node);

			b->tmp_node = nullptr;

			transaction::commit();
		} catch (const pmem::transaction_free_error &e) {
			throw std::runtime_error(e);
		}

		break;
	} while (true);

	return true;
}

template <typename Key, typename T, typename HashCompare>
bool
concurrent_hash_map<Key, T, HashCompare>::erase(const Key &key)
{
	node_base_ptr_t n;
	hashcode_t const h = my_hash_compare.hash(key);
	hashcode_t m = (hashcode_t)tbb::load<tbb::acquire>(
		internal::as_atomic(my_mask.get_ro()));
	pool_base pop = get_pool_base();

restart : {
	// lock scope
	// get bucket
	bucket_accessor b(this, h & m);

search:
	node_base_ptr_t *p = &b->node_list;
	n = *p;

	while (is_valid(n) &&
	       !my_hash_compare.equal(key,
				      static_persistent_pool_pointer_cast<node>(
					      n)(my_pool_uuid)
					      ->item.first)) {
		p = &n(my_pool_uuid)->next;
		n = *p;
	}

	if (!n) {
		// not found, but mask could be changed
		if (check_mask_race(h, m))
			goto restart;

		return false;
	} else if (!b.is_writer() && !b.upgrade_to_writer()) {
		if (check_mask_race(h, m)) // contended upgrade, check mask
			goto restart;

		goto search;
	}

	b->tmp_node = n(my_pool_uuid);
	pop.persist(b->tmp_node);

	*p = b->tmp_node->next;
	pop.persist(p, sizeof(node_base_ptr_t));

	--(internal::as_atomic(my_size.get_rw()));
	pop.persist(my_size);

	{
		typename node::scoped_t item_locker(b->tmp_node->mutex.get(),
						    /*write=*/true);
	}

	try {
		transaction::manual tx(pop);

		// Only one thread can delete it due to
		// write lock on the bucket
		delete_node(b->tmp_node);

		b->tmp_node = nullptr;

		transaction::commit();
	} catch (const pmem::transaction_free_error &e) {
		throw std::runtime_error(e);
	}
}

	return true;
}

template <typename Key, typename T, typename HashCompare>
void
concurrent_hash_map<Key, T, HashCompare>::swap(
	concurrent_hash_map<Key, T, HashCompare> &table)
{
	// TODO: respect C++11 allocator_traits<A>::propogate_on_constainer_swap
	using std::swap;
	swap(this->my_hash_compare, table.my_hash_compare);
	internal_swap(table);
}

template <typename Key, typename T, typename HashCompare>
void
concurrent_hash_map<Key, T, HashCompare>::rehash(size_type sz)
{
	reserve(sz); // TODO: add reduction of number of buckets as well

	hashcode_t mask = my_mask;

	// only the last segment should be scanned for rehashing
	// size or first index of the last segment
	hashcode_t b = (mask + 1) >> 1;

	assert((b & (b - 1)) == 0); // zero or power of 2

	for (; b <= mask; ++b) {
		bucket *bp = get_bucket(b);
		node_base_ptr_t n = bp->node_list;

		assert(is_valid(n) || n == internal::empty_bucket ||
		       bp->tmp_node == internal::rehash_req);

		internal::assert_not_locked(bp->mutex.get());

		if (bp->tmp_node == internal::rehash_req)
			rehash_bucket<true>(bp, b);
	}
}

template <typename Key, typename T, typename HashCompare>
void
concurrent_hash_map<Key, T, HashCompare>::clear()
{
	hashcode_t m = my_mask;

	assert((m & (m + 1)) == 0);

#ifndef NDEBUG
	// check consistency
	for (segment_index_t b = 0; b <= m; ++b) {
		bucket *bp = get_bucket(b);
		node_base_ptr_t n = bp->node_list;

		assert(is_valid(n) || n == internal::empty_bucket ||
		       bp->tmp_node == internal::rehash_req);

		internal::assert_not_locked(bp->mutex.get());
	}
#endif

	pool_base pop = get_pool_base();
	try { // transaction scope

		transaction::manual tx(pop);

		my_size = 0;
		segment_index_t s = segment_index_of(m);

		assert(s + 1 == pointers_per_table ||
		       !segment_facade_t(my_table, s + 1).is_valid());

		do {
			clear_segment(s);
		} while (s-- > 0);

		my_mask = embedded_buckets - 1;

		transaction::commit();
	} catch (const pmem::transaction_error &e) {
		throw std::runtime_error(e);
	}
}

template <typename Key, typename T, typename HashCompare>
void
concurrent_hash_map<Key, T, HashCompare>::clear_segment(segment_index_t s)
{
	segment_facade_t segment(my_table, s);

	assert(segment.is_valid());

	size_type sz = segment.size();
	for (segment_index_t i = 0; i < sz; ++i) {
		for (node_base_ptr_t n = segment[i].node_list; is_valid(n);
		     n = segment[i].node_list) {
			segment[i].node_list = n(my_pool_uuid)->next;
			delete_node(n);
		}
	}

	if (s >= first_block) // the first segment or the next
		segment.disable();
	else if (s == embedded_block && embedded_block != first_block) {
		size_t size = segment_size(first_block) - embedded_buckets;
		delete_persistent<bucket[]>(my_table[s], size);

		if (s >= embedded_block)
			my_table[s] = nullptr;
	}
}

template <typename Key, typename T, typename HashCompare>
void
concurrent_hash_map<Key, T, HashCompare>::internal_copy(
	const concurrent_hash_map &source)
{
	reserve(source.my_size); // TODO: load_factor?
	internal_copy(source.begin(), source.end());
}

template <typename Key, typename T, typename HashCompare>
template <typename I>
void
concurrent_hash_map<Key, T, HashCompare>::internal_copy(I first, I last)
{
	hashcode_t m = my_mask;
	pool_base pop = get_pool_base();

	for (; first != last; ++first) {
		hashcode_t h = my_hash_compare.hash((*first).first);
		bucket *b = get_bucket(h & m);

		assert(b->tmp_node != internal::rehash_req);

		allocate_node_copy_construct(
			pop,
			reinterpret_cast<persistent_ptr<node> &>(b->tmp_node),
			(*first).first, (*first).second, b->tmp_node);

		insert_new_node(pop, b);
	}
}

template <typename Key, typename T, typename HashCompare>
inline bool
operator==(const concurrent_hash_map<Key, T, HashCompare> &a,
	   const concurrent_hash_map<Key, T, HashCompare> &b)
{
	if (a.size() != b.size())
		return false;

	typename concurrent_hash_map<Key, T, HashCompare>::const_iterator i(
		a.begin()),
		i_end(a.end());

	typename concurrent_hash_map<Key, T, HashCompare>::const_iterator j,
		j_end(b.end());

	for (; i != i_end; ++i) {
		j = b.equal_range(i->first).first;

		if (j == j_end || !(i->second == j->second))
			return false;
	}

	return true;
}

template <typename Key, typename T, typename HashCompare>
inline bool
operator!=(const concurrent_hash_map<Key, T, HashCompare> &a,
	   const concurrent_hash_map<Key, T, HashCompare> &b)
{
	return !(a == b);
}

template <typename Key, typename T, typename HashCompare>
inline void
swap(concurrent_hash_map<Key, T, HashCompare> &a,
     concurrent_hash_map<Key, T, HashCompare> &b)
{
	a.swap(b);
}

} // namespace experimental
} // namespace obj
} // namespace pmem

#endif // PMEMOBJ_PERSISTENT_CONCURRENT_HASH_MAP_HPP
