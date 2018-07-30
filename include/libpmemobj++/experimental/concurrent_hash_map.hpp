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

/**
 * A persistent version of concurrent hash map implementation
 * Ref: https://arxiv.org/abs/1509.02235
 */

#ifndef PMEMOBJ_CONCURRENT_HASH_MAP_HPP
#define PMEMOBJ_CONCURRENT_HASH_MAP_HPP

#include <libpmemobj++/experimental/v.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/transaction.hpp>
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
#include <libpmemobj++/detail/common.hpp>
#endif

#if LIBPMEMOBJ_CPP_USE_TBB_RW_MUTEX
#include "tbb/spin_rw_mutex.h"
#else
#include <libpmemobj++/shared_mutex.hpp>
#endif

#include <libpmemobj++/detail/persistent_pool_ptr.hpp>

#include <atomic>
#include <cassert>
#include <functional>
#include <initializer_list>
#include <iterator> // for std::distance
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>

#if _MSC_VER
#include <intrin.h>
#include <windows.h>
#endif

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
namespace experimental
{

using namespace pmem::obj;

/**
 * hash_compare that is default argument for concurrent_hash_map
 */
template <typename Key>
struct hash_compare {
	static size_t
	hash(const Key &a)
	{
		return std::hash<Key>{}(a);
	}

	static bool
	equal(const Key &a, const Key &b)
	{
		return std::equal_to<Key>{}(a, b);
	}
};

template <typename Key, typename T, typename HashCompare = hash_compare<Key>>
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

template <typename Mutex>
void
assert_not_locked(pmem::obj::experimental::v<Mutex> &mtx)
{
	assert_not_locked<Mutex>(mtx.get());
}

#if _MSC_VER
static inline int
Log2(uint64_t x)
{
	unsigned long j;
	_BitScanReverse64(&j, x);
	return static_cast<int>(j);
}
#elif __GNUC__ || __clang__
static inline int
Log2(uint64_t x)
{
	// __builtin_clz builtin count _number_ of leading zeroes
	return 8 * int(sizeof(x)) - __builtin_clzll(x) - 1;
}
#else
static inline int
Log2(uint64_t x)
{
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	x |= (x >> 32);

	static const int table[64] = {
		0,  58, 1,  59, 47, 53, 2,  60, 39, 48, 27, 54, 33, 42, 3,  61,
		51, 37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4,  62,
		57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21, 56,
		45, 25, 31, 35, 16, 9,  12, 44, 24, 15, 8,  23, 7,  6,  5,  63};

	return table[(x * 0x03f6eaf2cd271461) >> 58];
}
#endif

class atomic_backoff {
	/**
	 * Time delay, in units of "pause" instructions.
	 * Should be equal to approximately the number of "pause" instructions
	 * that take the same time as an context switch. Must be a power of two.
	 */
	static const int32_t LOOPS_BEFORE_YIELD = 16;
	int32_t count;

	static inline void
	__pause(int32_t delay)
	{
		for (; delay > 0; --delay) {
#if _MSC_VER
			YieldProcessor();
#elif __GNUC__ && (__i386__ || __x86_64__)
			// Only i386 and x86-64 have pause instruction
			__builtin_ia32_pause();
#endif
		}
	}

public:
	/**
	 * Deny copy constructor
	 */
	atomic_backoff(const atomic_backoff &) = delete;
	/**
	 * Deny assignment
	 */
	void operator=(const atomic_backoff &) = delete;

	/** Default constructor */
	/* In many cases, an object of this type is initialized eagerly on hot
	 * path, as in for(atomic_backoff b; ; b.pause()) {...} For this reason,
	 * the construction cost must be very small! */
	atomic_backoff() : count(1)
	{
	}

	/**
	 * This constructor pauses immediately; do not use on hot paths!
	 */
	atomic_backoff(bool) : count(1)
	{
		pause();
	}

	/**
	 * Pause for a while.
	 */
	void
	pause()
	{
		if (count <= LOOPS_BEFORE_YIELD) {
			__pause(count);
			/* Pause twice as long the next time. */
			count *= 2;
		} else {
			/* Pause is so long that we might as well yield CPU to
			 * scheduler. */
			std::this_thread::yield();
		}
	}

	/**
	 * Pause for a few times and return false if saturated.
	 */
	bool
	bounded_pause()
	{
		__pause(count);
		if (count < LOOPS_BEFORE_YIELD) {
			/* Pause twice as long the next time. */
			count *= 2;
			return true;
		} else {
			return false;
		}
	}

	void
	reset()
	{
		count = 1;
	}
}; /* class atomic_backoff */

/**
 * This wrapper for std::atomic<T> allows to initialize volatile atomic fields
 * with custom initializer
 */
template <typename T, typename InitFunctor>
class atomic_wrapper {
public:
	using value_type = T;
	using atomic_type = std::atomic<value_type>;
	using init_type = InitFunctor;

	atomic_wrapper() noexcept
	{
	}

	atomic_wrapper(const value_type &val) noexcept : my_atomic(val)
	{
	}

	template <typename... Args>
	atomic_wrapper(Args &&... args)
	    : my_atomic(init_type()(std::forward<Args>(args)...))
	{
	}

	operator atomic_type &() noexcept
	{
		return my_atomic;
	}

private:
	atomic_type my_atomic;
};

/**
 * Wrapper around PMDK allocator
 * @throw std::bad_alloc on allocation failure.
 */
template <typename T, typename U, typename... Args>
void
make_persistent_object(pool_base &pop, persistent_ptr<U> &ptr, Args &&... args)
{
#if LIBPMEMOBJ_CPP_CONCURRENT_HASH_MAP_USE_ATOMIC_ALLOCATOR
	make_persistent_atomic<T>(pop, ptr, std::forward<Args>(args)...);
#else
	try {
		transaction::manual tx(pop);
		ptr = make_persistent<T>(std::forward<Args>(args)...);
		transaction::commit();
	} catch (...) {
		throw std::bad_alloc();
	}
#endif
}

#if !LIBPMEMOBJ_CPP_USE_TBB_RW_MUTEX
class shared_mutex_scoped_lock {
	using rw_mutex_type = pmem::obj::shared_mutex;

public:
	shared_mutex_scoped_lock(const shared_mutex_scoped_lock &) = delete;
	void operator=(const shared_mutex_scoped_lock &) = delete;

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
	 * Upgrade reader to become a writer.
	 * This method is added for compatibility with tbb::spin_rw_mutex which
	 * supports upgrade operation.
	 *
	 * @returns Always return false because persistent shared mutex cannot
	 * be upgraded without releasing and re-acquiring the lock
	 */
	bool
	upgrade_to_writer()
	{
		assert(!is_writer);
		mutex->unlock_shared();
		is_writer = true;
		mutex->lock();
		return false;
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
	 * Downgrade writer to become a reader.
	 * This method is added for compatibility with tbb::spin_rw_mutex which
	 * supports downgrade operation.
	 * @returns false.
	 */
	bool
	downgrade_to_reader()
	{
		assert(is_writer);
		return false;
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
#endif

struct hash_map_node_base {
#if LIBPMEMOBJ_CPP_USE_TBB_RW_MUTEX
	/** Mutex type. */
	using mutex_t = pmem::obj::experimental::v<tbb::spin_rw_mutex>;

	/** Scoped lock type for mutex. */
	using scoped_t = tbb::spin_rw_mutex::scoped_lock;
#else
	/** Mutex type. */
	using mutex_t = pmem::obj::shared_mutex;

	/** Scoped lock type for mutex. */
	using scoped_t = shared_mutex_scoped_lock;
#endif

	/** Persistent pointer type for next. */
	using node_base_ptr_t = detail::persistent_pool_ptr<hash_map_node_base>;

	/** Next node in chain. */
	node_base_ptr_t next;

	/** Node mutex. */
	mutex_t mutex;

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
}; /* struct hash_map_node_base */

/** Empty bucket flag. */
static detail::persistent_pool_ptr<hash_map_node_base> const empty_bucket =
	OID_NULL;

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
		return segment_index_t(Log2(index | 1));
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
 * point new segment cannot be allocated as one contigious memory block.
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
		try {
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
		} catch (...) {
			throw std::bad_alloc();
		}
	}

	void
	enable_big_segment(pool_base &pop)
	{
		block_range blocks = segment_blocks(my_seg);
#if LIBPMEMOBJ_CPP_CONCURRENT_HASH_MAP_USE_ATOMIC_ALLOCATOR
		for (segment_index_t b = blocks.first; b < blocks.second; ++b) {
			if ((*my_table)[b] == nullptr)
				make_persistent_atomic<bucket_type[]>(
					pop, (*my_table)[b], block_size(b));
		}
#else
		try {
			transaction::manual tx(pop);

			for (segment_index_t b = blocks.first;
			     b < blocks.second; ++b) {
				assert((*my_table)[b] == nullptr);
				(*my_table)[b] = make_persistent<bucket_type[]>(
					block_size(b));
			}

			transaction::commit();
		} catch (...) {
			throw std::bad_alloc();
		}
#endif
	}

	/** Pointer to the table of blocks */
	table_pointer my_table;

	/** Segment index */
	segment_index_t my_seg;
}; /* End of class segment_facade_impl */

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

	/** Node base type. */
	using node_base = hash_map_node_base;

	/** Node base pointer. */
	using node_base_ptr_t = detail::persistent_pool_ptr<node_base>;

	/** tmp node pointer. */
	using tmp_node_ptr_t = persistent_ptr<node_base>;

	/** Bucket type. */
	struct bucket {
#if LIBPMEMOBJ_CPP_USE_TBB_RW_MUTEX
		/** Mutex type for buckets. */
		using mutex_t = pmem::obj::experimental::v<tbb::spin_rw_mutex>;

		/** Scoped lock type for mutex. */
		using scoped_t = tbb::spin_rw_mutex::scoped_lock;
#else
		/** Mutex type. */
		using mutex_t = pmem::obj::shared_mutex;

		/** Scoped lock type for mutex. */
		using scoped_t = shared_mutex_scoped_lock;
#endif

		/** Bucket mutex. */
		mutex_t mutex;

		/** Atomic flag to indicate if bucket rehashed */
		p<std::atomic<bool>> rehashed;

		/** List of the nodes stored in the bucket. */
		node_base_ptr_t node_list;

		/** Pointer used to allocate new node. */
		tmp_node_ptr_t tmp_node;

		/** Default constructor */
		bucket() : node_list(empty_bucket), tmp_node(nullptr)
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
		void operator=(const bucket &) = delete;
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

	class calculate_mask {
	public:
		hashcode_t
		operator()(const hash_map_base *map_base) const
		{
			return map_base->calculate_mask();
		}
	}; /* class mask_initializer */

	/** Type of my_mask field */
	using mask_type = v<atomic_wrapper<hashcode_t, calculate_mask>>;

	/** ID of persistent memory pool where hash map resides. */
	p<uint64_t> my_pool_uuid;

	/** Hash mask = sum of allocated segment sizes - 1. */
	/* my_mask always restored on restart. */
	mask_type my_mask;

	/**
	 * Segment pointers table. Also prevents false sharing between my_mask
	 * and my_size.
	 */
	blocks_table_t my_table;

	/* It must be in separate cache line from my_mask due to performance
	 * effects */
	/** Size of container in stored items. */
	p<std::atomic<size_type>> my_size;

	/** Zero segment. */
	bucket my_embedded_segment[embedded_buckets];

	/** Segment mutex type. */
	using segment_enable_mutex_t = pmem::obj::mutex;

	/** Segment mutex used to enable new segment. */
	segment_enable_mutex_t my_segment_enable_mutex;

	/** @return true if @arg ptr is valid pointer. */
	static bool
	is_valid(void *ptr)
	{
		return reinterpret_cast<uintptr_t>(ptr) > uintptr_t(63);
	}

	template <typename U>
	static bool
	is_valid(const detail::persistent_pool_ptr<U> &ptr)
	{
		return ptr.raw() > uint64_t(63);
	}

	template <typename U>
	static bool
	is_valid(const persistent_ptr<U> &ptr)
	{
		return ptr.raw().off > uint64_t(63);
	}

	const std::atomic<hashcode_t> &
	mask() const noexcept
	{
		return const_cast<mask_type &>(my_mask).get(this);
	}

	std::atomic<hashcode_t> &
	mask() noexcept
	{
		return my_mask.get(this);
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
		VALGRIND_HG_DISABLE_CHECKING(&my_size, sizeof(my_size));
		VALGRIND_HG_DISABLE_CHECKING(&my_mask, sizeof(my_mask));
#endif

		my_size.get_rw() = 0;
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

		assert(mask() == embedded_buckets - 1);
	}

	/**
	 * Re-calculate mask value on each process restart.
	 */
	hashcode_t
	calculate_mask() const
	{
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
		VALGRIND_HG_DISABLE_CHECKING(&my_size, sizeof(my_size));
		VALGRIND_HG_DISABLE_CHECKING(&my_mask, sizeof(my_mask));
#endif
		hashcode_t m = embedded_buckets - 1;

		const_segment_facade_t segment(
			my_table, segment_traits_t::embedded_segments);

		while (segment.is_valid()) {
			m += segment.size();
			++segment;
		}
		return m;
	}

	void
	restore_size(size_type actual_size)
	{
		my_size.get_rw().store(actual_size, std::memory_order_relaxed);
		pool_base pop = get_pool_base();
		pop.persist(my_size);
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

			assert_not_locked(b->mutex);

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
	 * Add new node pointed by @arg b->tmp_node to bucket @arg b
	 */
	void
	add_to_bucket(bucket *b, pool_base &pop)
	{
		assert(b->is_rehashed(std::memory_order_relaxed) == true);
		assert(is_valid(b->tmp_node));
		assert(b->tmp_node->next == b->node_list);

		b->node_list = b->tmp_node; /* bucket is locked */
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
	get_bucket(hashcode_t h) const
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
	check_mask_race(hashcode_t h, hashcode_t &m) const
	{
		hashcode_t m_now, m_old = m;

		m_now = mask().load(std::memory_order_acquire);

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
	 * Correct bucket state after crash.
	 */
	void
	correct_bucket(bucket *b)
	{
		pool_base pop = get_pool_base();

		if (is_valid(b->tmp_node)) {
			if (b->tmp_node->next == b->node_list) {
				insert_new_node(pop, b);
			} else {
				b->tmp_node.raw_ptr()->off = 0;
				pop.persist(&(b->tmp_node.raw().off),
					    sizeof(b->tmp_node.raw().off));
			}
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

		/* prefix form is to enforce allocation after the first item
		 * inserted */
		size_t sz = ++(my_size.get_rw());
		pop.persist(&my_size, sizeof(my_size));

		b->tmp_node.raw_ptr()->off = 0;
		pop.persist(&(b->tmp_node.raw().off),
			    sizeof(b->tmp_node.raw().off));

		return sz;
	}

	/**
	 * Checks load factor and decides if new segment should be allocated.
	 * @return true if new segment was allocated and false otherwise
	 */
	bool
	check_growth(hashcode_t m, size_type sz)
	{
		if (sz >= m) {
			segment_index_t new_seg = static_cast<segment_index_t>(
				Log2(m + 1)); /* optimized segment_index_of */

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

		bool is_initial = (my_size.get_ro() == 0);

		for (size_type m = mask(); buckets > m; m = mask())
			enable_segment(
				segment_traits_t::segment_index_of(m + 1),
				is_initial);
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
			/* Swap my_mask */
			this->mask() = table.mask().exchange(
				this->mask(), std::memory_order_relaxed);

			/* Swap my_size */
			this->my_size.get_rw() =
				table.my_size.get_rw().exchange(
					this->my_size.get_ro(),
					std::memory_order_relaxed);

			for (size_type i = 0; i < embedded_buckets; ++i)
				this->my_embedded_segment[i].node_list.swap(
					table.my_embedded_segment[i].node_list);

			for (size_type i = segment_traits_t::embedded_segments;
			     i < block_table_size; ++i)
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
	template <typename Key, typename T, typename HashCompare>
	friend class experimental::concurrent_hash_map;
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

			if (!hash_map_base::is_valid(my_node)) {
				advance_to_next_bucket();
			}
		}
	}

public:
	/** Construct undefined iterator. */
	hash_map_iterator() : my_map(), my_index(), my_bucket(), my_node()
	{
	}

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

	/** Indirection (dereference). */
	reference operator*() const
	{
		assert(hash_map_base::is_valid(my_node));
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
	map_ptr my_map;

	/** Bucket index for current item. */
	size_t my_index;

	/** Pointer to bucket. */
	bucket *my_bucket;

	/** Pointer to node that has current item. */
	node *my_node;

	class bucket_accessor {
	public:
		bucket_accessor(map_ptr m, size_t index)
		{
			my_bucket = m->get_bucket(index);
			const_cast<map_type *>(m)->correct_bucket(my_bucket);
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

} /* namespace internal */
/** @endcond */

/**
 * Persistent memory aware implementation of Intel TBB concurrent_hash_map.
 */
template <typename Key, typename T, typename HashCompare>
class concurrent_hash_map : protected internal::hash_map_base {
	template <typename Container, bool is_const>
	friend class internal::hash_map_iterator;

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
		internal::hash_map_iterator<concurrent_hash_map, false>;
	using const_iterator =
		internal::hash_map_iterator<concurrent_hash_map, true>;

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

		node(value_type &&i, const node_base_ptr_t &_next = OID_NULL)
		    : node_base(_next), item(std::move(i))
		{
		}

		template <typename... Args>
		node(Args &&... args, node_base_ptr_t &&_next = OID_NULL)
		    : node_base(std::forward<node_base_ptr_t>(_next)),
		      item(std::forward<Args>(args)...)
		{
		}

		node(const value_type &i,
		     const node_base_ptr_t &_next = OID_NULL)
		    : node_base(_next), item(i)
		{
		}
	};

	using persistent_node_ptr_t = detail::persistent_pool_ptr<node>;

	void
	delete_node(const node_base_ptr_t &n)
	{
		delete_persistent<node>(
			detail::static_persistent_pool_pointer_cast<node>(n)
				.get_persistent_ptr(my_pool_uuid));
	}

	static void
	allocate_node_copy_construct(pool_base &pop,
				     persistent_ptr<node> &node_ptr,
				     const void *param,
				     const node_base_ptr_t &next = OID_NULL)
	{
		const value_type *v = static_cast<const value_type *>(param);
		internal::make_persistent_object<node>(pop, node_ptr, *v, next);
	}

	static void
	allocate_node_move_construct(pool_base &pop,
				     persistent_ptr<node> &node_ptr,
				     const void *param,
				     const node_base_ptr_t &next = OID_NULL)
	{
		const value_type *v = static_cast<const value_type *>(param);
		internal::make_persistent_object<node>(
			pop, node_ptr, std::move(*const_cast<value_type *>(v)),
			next);
	}

	static void
	allocate_node_default_construct(pool_base &pop,
					persistent_ptr<node> &node_ptr,
					const void *param,
					const node_base_ptr_t &next = OID_NULL)
	{
		const Key &key = *static_cast<const Key *>(param);
		internal::make_persistent_object<node>(pop, node_ptr, key,
						       next);
	}

	static void
	do_not_allocate_node(pool_base &, persistent_ptr<node> &node_ptr,
			     const void *,
			     const node_base_ptr_t &next = OID_NULL)
	{
		assert(false);
	}

	persistent_node_ptr_t
	search_bucket(const key_type &key, bucket *b) const
	{
		assert(b->is_rehashed(std::memory_order_relaxed));
		assert(!is_valid(b->tmp_node));

		persistent_node_ptr_t n =
			detail::static_persistent_pool_pointer_cast<node>(
				b->node_list);

		while (is_valid(n) &&
		       !my_hash_compare.equal(
			       key, n.get(my_pool_uuid)->item.first)) {
			n = detail::static_persistent_pool_pointer_cast<node>(
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

			if (my_b->is_rehashed(std::memory_order_acquire) ==
				    false &&
			    try_acquire(my_b->mutex, /*write=*/true)) {
				if (my_b->is_rehashed(
					    std::memory_order_relaxed) ==
				    false) {
					/* recursive rehashing */
					base->rehash_bucket<false>(my_b, h);
				}
			} else {
				bucket::scoped_t::acquire(my_b->mutex, writer);
			}

			if (is_valid(my_b->tmp_node)) {
				/* The condition is true only when insert
				 * operation was interupted on previous run */
				if (!this->is_writer())
					this->upgrade_to_writer();
				assert(this->is_writer());
				base->correct_bucket(my_b);
			}

			assert(my_b->is_rehashed(std::memory_order_relaxed));
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

			if (my_b->is_rehashed(std::memory_order_relaxed) ==
			    false) {
				/* recursive rehashing */
				base->rehash_bucket<true>(my_b, h);
			}

			base->correct_bucket(my_b);

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
			detail::static_persistent_pool_pointer_cast<node>(n)(
				my_pool_uuid)
				->item.first);
	}

	template <bool serial>
	void
	rehash_bucket(bucket *b_new, const hashcode_t h)
	{
		using accessor_type = typename std::conditional<
			serial, serial_bucket_accessor, bucket_accessor>::type;

		/* First two bucket should be always rehashed */
		assert(h > 1);

		pool_base pop = get_pool_base();
		node_base_ptr_t *p_new = &(b_new->node_list);
		bool restore_after_crash = *p_new != nullptr;

		/* get parent mask from the topmost bit */
		hashcode_t mask = (1u << internal::Log2(h)) - 1;
		assert((h & mask) < h);
		bool writer = false;
		accessor_type b_old(this, h & mask, writer);

		/* get full mask for new bucket */
		mask = (mask << 1) | 1;
		assert((mask & (mask + 1)) == 0 && (h & mask) == h);
	restart:
		for (node_base_ptr_t *p_old = &(b_old->node_list), n = *p_old;
		     is_valid(n); n = *p_old) {
			hashcode_t c = get_hash_code(n);
#ifndef NDEBUG
			hashcode_t bmask = h & (mask >> 1);

			bmask = bmask == 0
				? 1 /* minimal mask of parent bucket */
				: (1u << (internal::Log2(bmask) + 1)) - 1;

			assert((c & bmask) == (h & bmask));
#endif

			if ((c & mask) == h) {
				if (!b_old.is_writer() &&
				    !b_old.upgrade_to_writer()) {
					goto restart;
					/* node ptr can be invalid due to
					 * concurrent erase */
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

				/* Add to new b_new */
				*p_new = n;
				pop.persist(p_new, sizeof(*p_new));

				/* exclude from b_old */
				*p_old = n(my_pool_uuid)->next;
				pop.persist(p_old, sizeof(*p_old));

				p_new = &(n(my_pool_uuid)->next);
			} else {
				/* iterate to next item */
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

		/* mark rehashed */
		b_new->set_rehashed(std::memory_order_release);
		pop.persist(b_new->rehashed);
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
	/**
	 * Combines data access, locking, and garbage collection.
	 */
	class const_accessor
	    : private node::scoped_t /*which derived from no_copy*/ {
		friend class concurrent_hash_map<Key, T, HashCompare>;
		friend class accessor;
		using node_ptr_t = pmem::obj::persistent_ptr<node>;

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
		 */
		void
		release()
		{
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
		 */
		const_accessor() : my_node(OID_NULL)
		{
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
		bool
		is_writer()
		{
			return node::scoped_t::is_writer;
		}

		node_ptr_t my_node;

		hashcode_t my_hash;
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
	concurrent_hash_map() : internal::hash_map_base()
	{
	}

	/**
	 * Construct empty table with n preallocated buckets. This number
	 * serves also as initial concurrency level.
	 */
	concurrent_hash_map(size_type n) : internal::hash_map_base()
	{
		reserve(n);
	}

	/**
	 * Copy constructor
	 */
	concurrent_hash_map(const concurrent_hash_map &table)
	    : internal::hash_map_base()
	{
		reserve(table.size());

		internal_copy(table);
	}

	/**
	 * Move constructor
	 */
	concurrent_hash_map(concurrent_hash_map &&table)
	    : internal::hash_map_base()
	{
		swap(table);
	}

	/**
	 * Construction table with copying iteration range
	 */
	template <typename I>
	concurrent_hash_map(I first, I last)
	{
		reserve(static_cast<size_type>(std::distance(first, last)));

		internal_copy(first, last);
	}

	/**
	 * Construct table with initializer list
	 */
	concurrent_hash_map(std::initializer_list<value_type> il)
	{
		reserve(il.size());

		internal_copy(il.begin(), il.end());
	}

	/**
	 * Intialize persistent concurrent hash map after process restart.
	 * Should be called everytime after process restart.
	 * Not thread safe.
	 */
	void
	initialize(bool graceful_shutdown = false)
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
	}

	/**
	 * Assignment
	 * @throws std::runtime_error in case of PMDK transaction failure
	 * Not thread safe.
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
	 * Not thread safe.
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
	 */
	void rehash(size_type n = 0);

	/**
	 * Clear hash map content
	 * @throws std::runtime_error in case of PMDK transaction failure
	 * Not thread safe.
	 */
	void clear();

	/**
	 * Clear table and destroy it.
	 */
	~concurrent_hash_map()
	{
		clear();
	}

	//------------------------------------------------------------------------
	// STL support - not thread-safe methods
	//------------------------------------------------------------------------

	/**
	 * @returns an iterator to the beginning
	 * Not thread safe.
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
		return my_size.get_ro();
	}

	/**
	 * @returns true if size()==0.
	 */
	bool
	empty() const
	{
		return my_size.get_ro() == 0;
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
	 */
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

		return lookup(/*insert*/ true, key, &key, &result,
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

		return lookup(/*insert*/ true, key, &key, &result,
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

		return lookup(/*insert*/ true, value.first, &value, &result,
			      /*write=*/false, &allocate_node_copy_construct);
	}

	/**
	 * Insert item by copying if there is no such key present already and
	 * acquire a write lock on the item.
	 * @return true if item is new.
	 */
	bool
	insert(accessor &result, const value_type &value)
	{
		result.release();

		return lookup(/*insert*/ true, value.first, &value, &result,
			      /*write=*/true, &allocate_node_copy_construct);
	}

	/**
	 * Insert item by copying if there is no such key present already
	 * @return true if item is inserted.
	 */
	bool
	insert(const value_type &value)
	{
		return lookup(/*insert*/ true, value.first, &value, nullptr,
			      /*write=*/false, &allocate_node_copy_construct);
	}

	/**
	 * Insert item by copying if there is no such key present already and
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

protected:
	/**
	 * Insert or find item and optionally acquire a lock on the item.
	 */
	bool lookup(bool op_insert, const Key &key, const void *param,
		    const_accessor *result, bool write,
		    void (*allocate_node)(pool_base &, persistent_ptr<node> &,
					  const void *,
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
		return lookup(/*insert*/ true, value.first, &value,
			      accessor_location(result),
			      is_write_access_needed(result),
			      &allocate_node_move_construct);
	}

	void clear_segment(segment_index_t s);

	/**
	 * Copy "source" to *this, where *this must start out empty.
	 */
	void internal_copy(const concurrent_hash_map &source);

	template <typename I>
	void internal_copy(I first, I last);

}; // class concurrent_hash_map

template <typename Key, typename T, typename HashCompare>
bool
concurrent_hash_map<Key, T, HashCompare>::lookup(
	bool op_insert, const Key &key, const void *param,
	const_accessor *result, bool write,
	void (*allocate_node)(pool_base &, persistent_ptr<node> &, const void *,
			      const node_base_ptr_t &))
{
	assert(!result || !result->my_node);

	bool return_value = false;
	hashcode_t const h = my_hash_compare.hash(key);
	hashcode_t m = mask().load(std::memory_order_acquire);
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
	ANNOTATE_HAPPENS_AFTER(&my_mask);
#endif
	persistent_node_ptr_t n;
	size_type sz = 0;

restart : { /* lock scope */
	assert((m & (m + 1)) == 0);

	return_value = false;

	/* get bucket */
	bucket_accessor b(this, h & m);

	/* find a node */
	n = search_bucket(key, b.get());

	if (op_insert) {
		/* [opt] insert a key */
		if (!n) {
			if (!b.is_writer() && !b.upgrade_to_writer()) {
				/* Rerun search_list, in case another thread
				 * inserted the item during the upgrade. */
				n = search_bucket(key, b.get());
				if (is_valid(n)) {
					/* unfortunately, it did */
					b.downgrade_to_reader();
					goto exists;
				}
			}

			if (check_mask_race(h, m))
				goto restart; /* b.release() is done in ~b(). */

			assert(!is_valid(b->tmp_node));

			/* insert and set flag to grow the container */
			pool_base pop = get_pool_base();

			allocate_node(pop,
				      reinterpret_cast<persistent_ptr<node> &>(
					      b->tmp_node),
				      param, b->node_list);

			n = b->tmp_node;
			sz = insert_new_node(pop, b.get());
			return_value = true;
		}
	} else { /* find or count */
		if (!n) {
			if (check_mask_race(h, m))
				goto restart; /* b.release() is done in ~b(). */
			return false;
		}

		return_value = true;
	}
exists:
	if (!result)
		goto check_growth;

	/* acquire the item */
	if (!result->try_acquire(n.get(my_pool_uuid)->mutex, write)) {
		for (internal::atomic_backoff backoff(true);;) {
			if (result->try_acquire(n.get(my_pool_uuid)->mutex,
						write))
				break;

			if (!backoff.bounded_pause()) {
				/* the wait takes really long, restart the
				 * operation */
				b.release();

				assert(!op_insert || !return_value);

				std::this_thread::yield();

				m = mask().load(std::memory_order_acquire);

				goto restart;
			}
		}
	}
} /* lock scope */

	result->my_node = n.get_persistent_ptr(my_pool_uuid);
	result->my_hash = h;
check_growth:
	/* [opt] grow the container */
	check_growth(m, sz);

	return return_value;
}

template <typename Key, typename T, typename HashCompare>
bool
concurrent_hash_map<Key, T, HashCompare>::erase(const Key &key)
{
	node_base_ptr_t n;
	hashcode_t const h = my_hash_compare.hash(key);
	hashcode_t m = mask().load(std::memory_order_acquire);
	pool_base pop = get_pool_base();

restart : {
	/* lock scope */
	/* get bucket */
	bucket_accessor b(this, h & m);

search:
	node_base_ptr_t *p = &b->node_list;
	n = *p;

	while (is_valid(n) &&
	       !my_hash_compare.equal(
		       key,
		       detail::static_persistent_pool_pointer_cast<node>(n)(
			       my_pool_uuid)
			       ->item.first)) {
		p = &n(my_pool_uuid)->next;
		n = *p;
	}

	if (!n) {
		/* not found, but mask could be changed */
		if (check_mask_race(h, m))
			goto restart;

		return false;
	} else if (!b.is_writer() && !b.upgrade_to_writer()) {
		if (check_mask_race(h, m)) /* contended upgrade, check mask */
			goto restart;

		goto search;
	}

	try {
		transaction::manual tx(pop);

		tmp_node_ptr_t del = n(my_pool_uuid);

		*p = del->next;

		{
			/* We cannot remove this element immediately because
			 * other threads might work with this element via
			 * accessors. The item_locker required to wait while
			 * other threads use the node. */
			typename node::scoped_t item_locker(del->mutex,
							    /*write=*/true);
		}

		/* Only one thread can delete it due to write lock on the bucket
		 */
		delete_node(del);

		transaction::commit();
	} catch (const pmem::transaction_free_error &e) {
		throw std::runtime_error(e);
	}

	--(my_size.get_rw());
	pop.persist(my_size);
}

	return true;
}

template <typename Key, typename T, typename HashCompare>
void
concurrent_hash_map<Key, T, HashCompare>::swap(
	concurrent_hash_map<Key, T, HashCompare> &table)
{
	std::swap(this->my_hash_compare, table.my_hash_compare);
	internal_swap(table);
}

template <typename Key, typename T, typename HashCompare>
void
concurrent_hash_map<Key, T, HashCompare>::rehash(size_type sz)
{
	reserve(sz);
	hashcode_t m = mask();

	/* only the last segment should be scanned for rehashing size or first
	 * index of the last segment */
	hashcode_t b = (m + 1) >> 1;

	/* zero or power of 2 */
	assert((b & (b - 1)) == 0);

	for (; b <= m; ++b) {
		bucket *bp = get_bucket(b);
		node_base_ptr_t n = bp->node_list;

		assert(is_valid(n) || n == internal::empty_bucket ||
		       bp->is_rehashed(std::memory_order_relaxed) == false);

		internal::assert_not_locked(bp->mutex);

		if (bp->is_rehashed(std::memory_order_relaxed) == false)
			rehash_bucket<true>(bp, b);
	}
}

template <typename Key, typename T, typename HashCompare>
void
concurrent_hash_map<Key, T, HashCompare>::clear()
{
	hashcode_t m = mask();

	assert((m & (m + 1)) == 0);

#ifndef NDEBUG
	/* check consistency */
	for (segment_index_t b = 0; b <= m; ++b) {
		bucket *bp = get_bucket(b);
		node_base_ptr_t n = bp->node_list;

		assert(is_valid(n) || n == internal::empty_bucket ||
		       bp->is_rehashed(std::memory_order_relaxed) == false);

		internal::assert_not_locked(bp->mutex);
	}
#endif

	pool_base pop = get_pool_base();
	try { /* transaction scope */

		transaction::manual tx(pop);

		my_size.get_rw() = 0;
		segment_index_t s = segment_traits_t::segment_index_of(m);

		assert(s + 1 == block_table_size ||
		       !segment_facade_t(my_table, s + 1).is_valid());

		do {
			clear_segment(s);
		} while (s-- > 0);

		transaction::commit();
	} catch (const pmem::transaction_error &e) {
		throw std::runtime_error(e);
	}
	mask().store(embedded_buckets - 1, std::memory_order_relaxed);
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

	if (s >= segment_traits_t::embedded_segments)
		segment.disable();
}

template <typename Key, typename T, typename HashCompare>
void
concurrent_hash_map<Key, T, HashCompare>::internal_copy(
	const concurrent_hash_map &source)
{
	reserve(source.my_size.get_ro());
	internal_copy(source.begin(), source.end());
}

template <typename Key, typename T, typename HashCompare>
template <typename I>
void
concurrent_hash_map<Key, T, HashCompare>::internal_copy(I first, I last)
{
	hashcode_t m = mask();
	pool_base pop = get_pool_base();

	for (; first != last; ++first) {
		hashcode_t h = my_hash_compare.hash(first->first);
		bucket *b = get_bucket(h & m);

		assert(b->is_rehashed(std::memory_order_relaxed));

		allocate_node_copy_construct(
			pop,
			reinterpret_cast<persistent_ptr<node> &>(b->tmp_node),
			&(*first), b->tmp_node);

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

} /* namespace experimental */
} /* namespace obj */
} /* namespace pmem */

#endif /* PMEMOBJ_CONCURRENT_HASH_MAP_HPP */
