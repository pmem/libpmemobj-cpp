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

#ifndef __PMEM_persistent_concurrent_hash_map_H
#define __PMEM_persistent_concurrent_hash_map_H

#include "tbb/spin_rw_mutex.h"
#include "tbb/tbb_allocator.h"
#include "tbb/atomic.h"
#include "tbb/tbb_exception.h"
#include "tbb/tbb_profiling.h"
#include "tbb/internal/_tbb_hash_compare_impl.h"
#if __TBB_INITIALIZER_LISTS_PRESENT
#include <initializer_list>
#endif

#include <libpmem.h>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/v.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/mutex.hpp>

#include "persistent_pool_ptr.h"

#include <memory>
#include <mutex>
#include <type_traits>

namespace pmem {

namespace interface5 {
    using namespace pmem::obj;
    
    template<typename Key, typename T, typename HashCompare = tbb::tbb_hash_compare<Key> >
    class persistent_concurrent_hash_map;

//! @cond INTERNAL
namespace internal {
    using namespace tbb::internal;

    template <typename T>
    inline T itt_hide_load_word( const tbb::atomic<T>& src ) {
#if TBB_USE_THREADING_TOOLS
        //TODO: This assertion should be replaced with static_assert
        __TBB_ASSERT( sizeof( T ) == sizeof( void * ), "Type must be word-sized." );
        T result = (T)itt_load_pointer_v3( &src );
        return result;
#else
        return src;
#endif
    }

    template <typename T>
    inline T itt_load_with_acquire( const T& src ) {
        return __TBB_load_with_acquire( src );
    }

    struct persistent_hash_map_node_base : tbb::internal::no_copy {
        //! Mutex type
        typedef tbb::spin_rw_mutex mutex_t;
        //! Scoped lock type for mutex
        typedef mutex_t::scoped_lock scoped_t;
        //! Persistent pointer type for next
        typedef persistent_pool_ptr<persistent_hash_map_node_base> node_base_ptr_t;
        //! Next node in chain
        node_base_ptr_t next;
        pmem::obj::v<mutex_t> mutex;

        persistent_hash_map_node_base() : next(OID_NULL) {}
        persistent_hash_map_node_base( const node_base_ptr_t &_next ) : next( _next ) {}
#if __TBB_CPP11_RVALUE_REF_PRESENT
        persistent_hash_map_node_base( node_base_ptr_t &&_next ) : next(std::move(_next) ) {}
#endif //__TBB_CPP11_RVALUE_REF_PRESENT
    };
    //! Incompleteness flag value
    static persistent_ptr<persistent_hash_map_node_base> const rehash_req = PMEMoid( {0, 3} );
    //! Rehashed bucket flag
    static persistent_ptr<persistent_hash_map_node_base> const rehashed = OID_NULL;
    //! Rehashed empty bucket flag
    static persistent_pool_ptr<persistent_hash_map_node_base> const empty_bucket = OID_NULL;

    //! base class of persistent_concurrent_hash_map
    class persistent_hash_map_base {
    public:
        //! Size type
        typedef size_t size_type;
        //! Type of a hash code.
        typedef size_t hashcode_t;
        //! Segment index type
        typedef size_t segment_index_t;
        //! Node base type
        typedef persistent_hash_map_node_base node_base;
        //! Node base pointer
        typedef persistent_pool_ptr<node_base> node_base_ptr_t;
        //! tmp node pointer
        typedef persistent_ptr<node_base> tmp_node_ptr_t;

        //! Bucket type
        struct bucket : tbb::internal::no_copy {
            //! Mutex type for buckets
            typedef tbb::spin_rw_mutex mutex_t;
            //! Scoped lock type for mutex
            typedef mutex_t::scoped_lock scoped_t;
            pmem::obj::v<mutex_t> mutex;
            node_base_ptr_t node_list;
            tmp_node_ptr_t tmp_node;
        };
        //! Count of segments in the first block
        static size_type const embedded_block = 1;
        //! Count of segments in the first block
        static size_type const embedded_buckets = 1 << embedded_block;
        //! Count of segments in the first block
        static size_type const first_block = 8; //including embedded_block. perfect with bucket size 16, so the allocations are power of 4096
        //! Size of a pointer / table size
        static size_type const pointers_per_table = sizeof( segment_index_t ) * 8; // one segment per bit
        //! Segment pointer
        typedef persistent_ptr<bucket[]> segment_ptr_t;
        //! Bucket pointer
        typedef persistent_ptr<bucket> bucket_ptr_t;
        //! Segment pointers table type
        typedef segment_ptr_t segments_table_t[pointers_per_table];
        //! Hash mask = sum of allocated segment sizes - 1
        p<hashcode_t> my_mask;
        //! Segment pointers table. Also prevents false sharing between my_mask and my_size
        segments_table_t my_table;
        //! Size of container in stored items
        p<size_type> my_size; // It must be in separate cache line from my_mask due to performance effects
        //! Zero segment
        bucket my_embedded_segment[embedded_buckets];

        p<uint64_t> my_pool_uuid;

        typedef pmem::obj::mutex segment_enable_mutex_t;
        segment_enable_mutex_t my_segment_enable_mutex;

        //! @return segment index of given index in the array
        static segment_index_t segment_index_of(size_type index) {
            return segment_index_t(__TBB_Log2(index | 1));
        }

        //! @return the first array index of given segment
        static segment_index_t segment_base(segment_index_t k) {
            return (segment_index_t(1) << k & ~segment_index_t(1));
        }

        //! @return segment size except for @arg k == 0
        static size_type segment_size(segment_index_t k) {
            return size_type(1) << k; // fake value for k==0
        }

        //! @return true if @arg ptr is valid pointer
        static bool is_valid(void *ptr) {
            return reinterpret_cast<uintptr_t>(ptr) > uintptr_t(63);
        }

        template<typename U>
        static bool is_valid(const persistent_pool_ptr<U>& ptr) {
            return ptr.raw() > uint64_t(63);
        }

        template<typename U>
        static bool is_valid(const persistent_ptr<U>& ptr) {
            return ptr.raw().off > uint64_t(63);
        }

        class segment_traits_t {
        public:
            //! PMDK has limitation for allocation size. It is 16 Gb now.
            static size_type const max_allocation_size = 16ull * 1024ull * 1024ull * 1024ull;
            //! 
            static size_t const first_big_block = 28; // TODO: avoid hardcoded value; need constexpr similar to: __TBB_Log2(max_allocation_size / sizeof(bucket))
                                                      //! Max number of buckets per segment
            static size_type const big_block_size = size_type(1) << first_big_block;
            //! 
            static size_type const big_block_start_index = size_type(1) << first_big_block;

            constexpr static size_t number_of_blocks(size_t number_of_seg) {
                return first_block_in_segment(number_of_seg);
            }
        protected:
            constexpr static size_t first_block_in_segment(size_t seg) {
                return seg < first_big_block ? seg : first_big_block + (size_t(1) << (seg - first_big_block)) - 1;
            }

            constexpr static size_t blocks_in_segment(size_t seg) {
                return seg < first_big_block ? size_t(1) : size_t(1) << (seg - first_big_block);
            }

            static size_t block_size(size_t b) {
                if (b < first_big_block)
                    return segment_size(b ? b : 1);
                return big_block_size;
            }
        }; // class segment_traits_t

        template <typename BlockTable, typename SegmentTraits, bool is_const>
        class segment_facade_impl : public SegmentTraits {
        public:
            typedef typename std::conditional<is_const, const BlockTable&, BlockTable&>::type table_reference;
            typedef typename std::conditional<is_const, const BlockTable*, BlockTable*>::type table_pointer;

            typedef SegmentTraits traits_type;
            using traits_type::number_of_blocks;
        private:
            using traits_type::first_block_in_segment;
            using traits_type::blocks_in_segment;
            using traits_type::block_size;
        public:
            segment_facade_impl(table_reference table, size_t s): my_table(&table), my_seg(s) {}

            segment_facade_impl(const segment_facade_impl& src) : my_table(src.my_table), my_seg(src.my_seg) {}

            segment_facade_impl(segment_facade_impl&& src) = default;

            ~segment_facade_impl() {}

            segment_facade_impl& operator=(const segment_facade_impl& src) {
                my_table = src.my_table;
                my_seg = src.my_seg;
                return *this;
            }

            segment_facade_impl& operator=(segment_facade_impl&& src) {
                my_table = src.my_table;
                my_seg = src.my_seg;
                return *this;
            }

            bucket& operator[](size_t i) const {
                __TBB_ASSERT(i < size(), "Index exceed segment size");
                size_t table_block = first_block_in_segment(my_seg);
                size_t b_size = block_size(table_block);
                table_block += i / b_size;
                i = i % b_size;
                return (*my_table)[table_block][i];
            }

            segment_facade_impl& operator++() {
                ++my_seg;
                return *this;
            }

            segment_facade_impl operator++(int) {
                segment_facade_impl tmp = *this;
                ++(*this);
                return tmp;
            }

            segment_facade_impl& operator--() {
                --my_seg;
                return *this;
            }

            segment_facade_impl operator--(int) {
                segment_facade_impl tmp = *this;
                --(*this);
                return tmp;
            }

            segment_facade_impl& operator+=(size_t off) {
                my_seg += off;
                return *this;
            }

            segment_facade_impl& operator-=(size_t off) {
                my_seg -= off;
                return *this;
            }

            segment_facade_impl operator+(size_t off) const {
                return segment_facade_impl(*(this->my_table), this->my_seg + off);
            }

            segment_facade_impl operator-(size_t off) const {
                return segment_facade_impl(*(this->my_table), this->my_seg - off);
            }

            void enable(pool_base& pop) {
                block_range blocks = segment_blocks(my_seg);
                for (size_t b = blocks.first; b < blocks.second; ++b) {
                    if ((*my_table)[b] == nullptr)
                        make_persistent_atomic<bucket[]>( pop, (*my_table)[b], block_size(b) );
                }
            }
            void disable() {
                block_range blocks = segment_blocks(my_seg);
                for (size_t b = blocks.first; b < blocks.second; ++b) {
                    if ((*my_table)[b] != nullptr) {
                        delete_persistent<bucket[]>((*my_table)[b], block_size(b));
                        (*my_table)[b] = OID_NULL;
                    }
                }
            }

            size_type size() const {
                return segment_size(my_seg ? my_seg : 1);
            }

            bool is_valid() const {
                block_range blocks = segment_blocks(my_seg);
                for (size_t b = blocks.first; b < blocks.second; ++b) {
                    if ((*my_table)[b] == nullptr) {
                        return false;
                    }
                }
                return true;
            }

        private:
            typedef std::pair<size_t, size_t> block_range;

            // returns blocks [begin, end) for corrsponding segment
            static block_range segment_blocks(size_t seg) {
                size_t begin = first_block_in_segment(seg);
                return block_range(begin, begin + blocks_in_segment(seg));
            }

            table_pointer my_table;
            size_t my_seg;
        };

        typedef segment_facade_impl<segments_table_t, segment_traits_t, true> const_segment_facade_t;
        typedef segment_facade_impl<segments_table_t, segment_traits_t, false> segment_facade_t;

        //! Constructor
        persistent_hash_map_base() {
            std::memset( this, 0, pointers_per_table * sizeof( segment_ptr_t ) // 32*4=128   or 64*8=512
                + sizeof( my_size ) + sizeof( my_mask )  // 4+4 or 8+8
                + embedded_buckets * sizeof( bucket ) ); // n*8 or n*16
            PMEMoid oid = pmemobj_oid( this );
            __TBB_ASSERT( !OID_IS_NULL(oid), "Persistent concurrent hash map was not allocated on persistent memory" );
            my_pool_uuid = oid.pool_uuid_lo;
            for ( size_type i = 0; i < embedded_block; i++ ) // fill the table
                my_table[i] = pmemobj_oid( my_embedded_segment + segment_base( i ) );
            my_mask = embedded_buckets - 1;
            __TBB_ASSERT( embedded_block <= first_block, "The first block number must include embedded blocks" );
        }

        //! Initialize buckets
        void init_buckets( segment_ptr_t ptr, size_type sz, bool is_initial ) {
            persistent_ptr<node_base> rehashed_flag = is_initial ? rehashed : rehash_req;
            bucket* b = ptr.get();
            for ( size_type i = 0; i < sz; ++i, ++b ) {
                __TBB_ASSERT(*reinterpret_cast<intptr_t*>(&b->mutex.get()) == 0, "Bucket mutex should be in unlocked state during initialization");
                b->tmp_node = rehashed_flag;
                b->node_list = empty_bucket;
            }
            get_pool_base().persist( ptr.get(), sizeof( ptr[0] )*sz );
        }

        void init_buckets(pool_base& pop, segment_facade_t& segment, bool is_initial) {
            persistent_ptr<node_base> rehashed_flag = is_initial ? rehashed : rehash_req;
            for (size_type i = 0; i < segment.size(); ++i) {
                bucket* b = &(segment[i]);
                __TBB_ASSERT(*reinterpret_cast<intptr_t*>(&b->mutex.get()) == 0, "Bucket mutex should be in unlocked state during initialization");
                b->tmp_node = rehashed_flag;
                b->node_list = empty_bucket;
            }
            // Flush in separate loop to avoid read-after-flush
            for (size_type i = 0; i < segment.size(); ++i) {
                bucket* b = &(segment[i]);
                pop.flush(b, sizeof(bucket));
            }
            pop.drain();
        }

        //! Add node @arg n to bucket @arg b
        void add_to_bucket( bucket *b, pool_base& pop ) {
            __TBB_ASSERT( b->tmp_node != rehash_req, NULL );
            __TBB_ASSERT( is_valid(b->tmp_node), NULL);
            __TBB_ASSERT( b->tmp_node->next == b->node_list, NULL );
            b->node_list = b->tmp_node; // its under lock and flag is set
            pop.persist( &(b->node_list), sizeof( b->node_list ) );
        }

        //! Enable segment
        void enable_segment( segment_index_t k, bool is_initial = false ) {
            __TBB_ASSERT( k, "Zero segment must be embedded" );
            pool_base pop = get_pool_base();
            size_type sz;
            if ( k >= first_block ) {
#if INTEL_PRIVATE
                TRACEP( "enable %u\n", unsigned( k ) );
#endif
                segment_facade_t new_segment(my_table, k);
                sz = new_segment.size();
                if (!new_segment.is_valid()) {
                    new_segment.enable(pop);
                }

                init_buckets( pop, new_segment, is_initial );
                sz <<= 1;// double it to get entire capacity of the container
            }
            else { // the first block
#if INTEL_PRIVATE
                TRACEP( "enable first block\n" );
#endif//INTEL_PRIVATE
                // TODO: refactor this code to incapsulate logic in segment_facade_t class
                __TBB_STATIC_ASSERT(first_block < segment_facade_t::first_big_block, "first_block should be less than first_big_block");
                __TBB_ASSERT( k == embedded_block, "Wrong segment index" );
                sz = segment_size( first_block );
                if ( !is_valid( my_table[embedded_block] ) ) { // Otherwise, it was allocated on previous run but wasn't enabled
                    make_persistent_atomic<bucket[]>( pop, my_table[embedded_block], sz - embedded_buckets );
                }
                init_buckets( my_table[embedded_block], sz - embedded_buckets, is_initial );
                //TODO: fix this hack with tmp
                bucket_ptr_t tmp = my_table[embedded_block].raw();
                for ( segment_index_t i = embedded_block + 1; i < first_block; ++i ) { // calc the offsets
                    std::ptrdiff_t off = static_cast<std::ptrdiff_t>(segment_base( i ) - segment_base( embedded_block ));
                    my_table[i] = (tmp + off).raw();
                    pop.persist( my_table[i] );
                }     
            }
#if INTEL_PRIVATE
            TRACEP( "posting the mask\n" );
#endif
            itt_store_word_with_release( as_atomic( my_mask.get_rw() ), sz - 1 );
            pop.persist( my_mask );
        }

        // TODO: maybe we need to return persistent_pool_ptr<bucket>
        //! Get bucket by (masked) hashcode
        bucket *get_bucket( hashcode_t h ) const throw() { // TODO: add throw() everywhere?
            segment_index_t s = segment_index_of( h );
            h -= segment_base( s );
            const_segment_facade_t segment(my_table, s);
            __TBB_ASSERT(segment.is_valid(), "hashcode must be cut by valid mask for allocated segments");
            return &(segment[h]);
        }

        // internal serial rehashing helper
        void mark_rehashed_levels( hashcode_t h ) throw () {
            segment_index_t s = segment_index_of( h );
            segment_facade_t segment(my_table, s);
            while ((++segment).is_valid())
                if (segment[h].tmp_node == rehash_req) {
                    segment[h].tmp_node = rehashed;
                    mark_rehashed_levels(h + ((hashcode_t)1 << s)); // optimized segment_base(s)
                }
        }

        //! Check for mask race
        // Splitting into two functions should help inlining
        inline bool check_mask_race( const hashcode_t h, hashcode_t &m ) const {
            hashcode_t m_now, m_old = m;
            m_now = (hashcode_t)itt_load_word_with_acquire( as_atomic(my_mask.get_ro() ) );
            if ( m_old != m_now )
                return check_rehashing_collision( h, m_old, m = m_now );
            return false;
        }

        //! Process mask race, check for rehashing collision
        bool check_rehashing_collision( const hashcode_t h, hashcode_t m_old, hashcode_t m ) const {
            __TBB_ASSERT( m_old != m, NULL ); // TODO?: m arg could be optimized out by passing h = h&m
            if ( (h & m_old) != (h & m) ) { // mask changed for this hashcode, rare event
                                            // condition above proves that 'h' has some other bits set beside 'm_old'
                                            // find next applicable mask after m_old    //TODO: look at bsl instruction
                for ( ++m_old; !(h & m_old); m_old <<= 1 ) // at maximum few rounds depending on the first block size
                    ;
                m_old = (m_old << 1) - 1; // get full mask from a bit
                __TBB_ASSERT( (m_old&(m_old + 1)) == 0 && m_old <= m, NULL );
#if INTEL_PRIVATE
                TRACEP( "%x: mask changed=%x checking with %x\n", unsigned( h ), unsigned( m ), unsigned( m_old ) );
#endif
                // check whether it is rehashing/ed
                if ( itt_load_word_with_acquire( get_bucket( h & m_old )->tmp_node.raw().off ) != rehash_req.raw().off ) // Workaround: just comparing off part. Need to investigate how to properly load with acquire PMEMoid (128 bit)
                {
#if __TBB_STATISTICS
                    my_info_restarts++; // race collisions
#endif
                    return true;
                }
            }
            return false;
        }

        //! correct bucket state after crash.
        void correct_bucket( bucket *b ) {
            if ( b->tmp_node != nullptr ) {
                if( b->tmp_node->next == b->node_list ) // Than
                    insert_new_node( b );
                b->tmp_node = nullptr;
                get_pool_base().persist( b->tmp_node );
            }
        }

        //! Insert a node and check for load factor. @return segment index to enable.
        size_type insert_new_node( bucket *b ) {
            pool_base pop = get_pool_base();
            add_to_bucket( b , pop );
            size_t sz = ++as_atomic( my_size.get_rw() ); // prefix form is to enforce allocation after the first item inserted
            pop.persist( &my_size, sizeof( my_size ) );
            b->tmp_node = nullptr;
            pop.persist( b->tmp_node );
            return sz;
        }

        //! Checks load factor and decides if new segment should be allocated. @return true if new segment was allocated and false otherwise
        bool check_growth( hashcode_t mask, size_type sz ) {
            if ( sz >= mask ) {
                segment_index_t new_seg = static_cast<segment_index_t>(__TBB_Log2( mask + 1 )); //optimized segment_index_of
                __TBB_ASSERT(segment_facade_t(my_table, new_seg - 1).is_valid(), "new allocations must not publish new mask until segment has allocated");

                std::unique_lock<segment_enable_mutex_t> lock( my_segment_enable_mutex, std::try_to_lock );
                if ( lock ) {
                    if ( itt_load_word_with_acquire( as_atomic( my_mask.get_ro() ) ) == mask ) { // Otherwise, other thread enable this segment
                        enable_segment( new_seg );
                        return true;
                    }
                }
            }
            return false;
        }

        //! Prepare enough segments for number of buckets
        void reserve( size_type buckets ) {
            if ( !buckets-- ) return;
            bool is_initial = !my_size;
            for ( size_type m = my_mask; buckets > m; m = my_mask )
                enable_segment( segment_index_of( m + 1 ), is_initial );
        }
        //! Swap hash_map_bases
        void internal_swap( persistent_hash_map_base &table ) {
            pool_base p = get_pool_base();
            transaction::manual tx( p );
            this->my_pool_uuid.swap( table.my_pool_uuid );
            this->my_mask.swap( table.my_mask );
            this->my_size.swap( table.my_size );
            for ( size_type i = 0; i < embedded_buckets; i++ )
                this->my_embedded_segment[i].node_list.swap( table.my_embedded_segment[i].node_list );
            for ( size_type i = embedded_block; i < pointers_per_table; i++ )
                this->my_table[i].swap( table.my_table[i] );

            transaction::commit();
        }

        pool_base get_pool_base() {
            PMEMobjpool *pop = pmemobj_pool_by_oid( PMEMoid{my_pool_uuid, 0} );
            return pool_base ( pop );
        }
    };

    template<typename Iterator>
    class persistent_hash_map_range;

    //! Meets requirements of a forward iterator for STL */
    /** Value is either the T or const T type of the container.
    @ingroup containers */
    template<typename Container, typename Value>
    class persistent_hash_map_iterator
        : public std::iterator<std::forward_iterator_tag, Value>
    {
        typedef Container map_type;
        typedef typename Container::node node;
        typedef persistent_hash_map_base::node_base node_base;
        typedef persistent_hash_map_base::bucket bucket;

        template<typename C, typename T, typename U>
        friend bool operator==( const persistent_hash_map_iterator<C, T>& i, const persistent_hash_map_iterator<C, U>& j );

        template<typename C, typename T, typename U>
        friend bool operator!=( const persistent_hash_map_iterator<C, T>& i, const persistent_hash_map_iterator<C, U>& j );

        template<typename C, typename T, typename U>
        friend ptrdiff_t operator-( const persistent_hash_map_iterator<C, T>& i, const persistent_hash_map_iterator<C, U>& j );

        template<typename C, typename U>
        friend class persistent_hash_map_iterator;

        template<typename I>
        friend class persistent_hash_map_range;

        void advance_to_next_bucket() { // TODO?: refactor to iterator_base class
            size_t k = my_index + 1;
            __TBB_ASSERT( my_bucket, "advancing an invalid iterator?" );
            while ( k <= my_map->my_mask ) {
                // Following test uses 2's-complement wizardry
                if ( k&(k - 2) ) // not the beginning of a segment
                    ++my_bucket;
                else my_bucket = my_map->get_bucket( k );
                if ( persistent_hash_map_base::is_valid( my_bucket->node_list ) ) {
                    my_node = static_cast<node*>(my_bucket->node_list.get( my_map->my_pool_uuid ) );
                    my_index = k; return;
                }
                ++k;
            }
            my_bucket = 0; my_node = 0; my_index = k; // the end
        }
#if !defined(_MSC_VER) || defined(__INTEL_COMPILER)
        template<typename Key, typename T, typename HashCompare, typename A>
        friend class interface5::persistent_concurrent_hash_map;
#else
    public: // workaround
#endif
        //! concurrent_hash_map over which we are iterating.
        const Container *my_map;

        //! Index in hash table for current item
        size_t my_index;

        //! Pointer to bucket
        const bucket *my_bucket;

        //! Pointer to node that has current item
        node *my_node;

        persistent_hash_map_iterator( const Container &map, size_t index, const bucket *b, node_base *n );

    public:
        //! Construct undefined iterator
        persistent_hash_map_iterator() : my_map(), my_index(), my_bucket(), my_node() {}
        persistent_hash_map_iterator( const persistent_hash_map_iterator<Container, typename Container::value_type> &other ) :
            my_map( other.my_map ),
            my_index( other.my_index ),
            my_bucket( other.my_bucket ),
            my_node( other.my_node )
        {}
        Value& operator*() const {
            __TBB_ASSERT( persistent_hash_map_base::is_valid( my_node ), "iterator uninitialized or at end of container?" );
            return my_node->item;
        }
        Value* operator->() const { return &operator*(); }
        persistent_hash_map_iterator& operator++();

        //! Post increment
        persistent_hash_map_iterator operator++( int ) {
            persistent_hash_map_iterator old( *this );
            operator++();
            return old;
        }
    };

    template<typename Container, typename Value>
    persistent_hash_map_iterator<Container, Value>::persistent_hash_map_iterator( const Container &map, size_t index, const bucket *b, node_base *n ) :
        my_map( &map ),
        my_index( index ),
        my_bucket( b ),
        my_node( static_cast<node*>(n) )
    {
        if ( b && !persistent_hash_map_base::is_valid( n ) )
            advance_to_next_bucket();
    }

    template<typename Container, typename Value>
    persistent_hash_map_iterator<Container, Value>& persistent_hash_map_iterator<Container, Value>::operator++() {
        my_node = static_cast<node*>(my_node->next.get( (my_map->my_pool_uuid) ) );
        if ( !my_node ) advance_to_next_bucket();
        return *this;
    }

    template<typename Container, typename T, typename U>
    bool operator==( const persistent_hash_map_iterator<Container, T>& i, const persistent_hash_map_iterator<Container, U>& j ) {
        return i.my_node == j.my_node && i.my_map == j.my_map;
    }

    template<typename Container, typename T, typename U>
    bool operator!=( const persistent_hash_map_iterator<Container, T>& i, const persistent_hash_map_iterator<Container, U>& j ) {
        return i.my_node != j.my_node || i.my_map != j.my_map;
    }

    //! Range class used with persistent_concurrent_hash_map
    /** @ingroup containers */
    template<typename Iterator>
    class hash_map_range {
        typedef typename Iterator::map_type map_type;
        Iterator my_begin;
        Iterator my_end;
        mutable Iterator my_midpoint;
        size_t my_grainsize;
        //! Set my_midpoint to point approximately half way between my_begin and my_end.
        void set_midpoint() const;
        template<typename U> friend class hash_map_range;
    public:
        //! Type for size of a range
        typedef std::size_t size_type;
        typedef typename Iterator::value_type value_type;
        typedef typename Iterator::reference reference;
        typedef typename Iterator::difference_type difference_type;
        typedef Iterator iterator;

        //! True if range is empty.
        bool empty() const { return my_begin == my_end; }

        //! True if range can be partitioned into two subranges.
        bool is_divisible() const {
            return my_midpoint != my_end;
        }
        //! Split range.
        hash_map_range( hash_map_range& r, tbb::split ) :
            my_end( r.my_end ),
            my_grainsize( r.my_grainsize )
        {
            r.my_end = my_begin = r.my_midpoint;
            __TBB_ASSERT( !empty(), "Splitting despite the range is not divisible" );
            __TBB_ASSERT( !r.empty(), "Splitting despite the range is not divisible" );
            set_midpoint();
            r.set_midpoint();
        }
        //! type conversion
        template<typename U>
        hash_map_range( hash_map_range<U>& r ) :
            my_begin( r.my_begin ),
            my_end( r.my_end ),
            my_midpoint( r.my_midpoint ),
            my_grainsize( r.my_grainsize )
        {}
        //! Init range with container and grainsize specified
        hash_map_range( const map_type &map, size_type grainsize_ = 1 ) :
            my_begin( Iterator( map, 0, map.my_embedded_segment, map.my_embedded_segment->node_list ) ),
            my_end( Iterator( map, map.my_mask + 1, 0, 0 ) ),
            my_grainsize( grainsize_ )
        {
            __TBB_ASSERT( grainsize_>0, "grainsize must be positive" );
            set_midpoint();
        }
        const Iterator& begin() const { return my_begin; }
        const Iterator& end() const { return my_end; }
        //! The grain size for this range.
        size_type grainsize() const { return my_grainsize; }
    };

    template<typename Iterator>
    void hash_map_range<Iterator>::set_midpoint() const {
        // Split by groups of nodes
        size_t m = my_end.my_index - my_begin.my_index;
        if ( m > my_grainsize ) {
            m = my_begin.my_index + m / 2u;
            persistent_hash_map_base::bucket *b = my_begin.my_map->get_bucket( m );
            my_midpoint = Iterator( *my_begin.my_map, m, b, b->node_list );
        }
        else {
            my_midpoint = my_end;
        }
        __TBB_ASSERT( my_begin.my_index <= my_midpoint.my_index,
            "my_begin is after my_midpoint" );
        __TBB_ASSERT( my_midpoint.my_index <= my_end.my_index,
            "my_midpoint is after my_end" );
        __TBB_ASSERT( my_begin != my_midpoint || my_begin == my_end,
            "[my_begin, my_midpoint) range should not be empty" );
    }
    
} // namespace internal
//! @endcond

template<typename Key, typename T, typename HashCompare>
class persistent_concurrent_hash_map : protected internal::persistent_hash_map_base {
    template<typename Container, typename Value>
    friend class internal::persistent_hash_map_iterator;

    template<typename I>
    friend class internal::hash_map_range;

public:
    typedef Key key_type;
    typedef T mapped_type;
    typedef std::pair<const Key, T> value_type;
    typedef persistent_hash_map_base::size_type size_type;
    typedef ptrdiff_t difference_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef internal::persistent_hash_map_iterator<persistent_concurrent_hash_map, value_type> iterator;
    typedef internal::persistent_hash_map_iterator<persistent_concurrent_hash_map, const value_type> const_iterator;
    typedef internal::hash_map_range<iterator> range_type;
    typedef internal::hash_map_range<const_iterator> const_range_type;

protected:
    friend class const_accessor;
    struct node;
    HashCompare my_hash_compare;

    struct node : public node_base {
        value_type item;
        node( const Key &key, const node_base_ptr_t &_next = OID_NULL ) : node_base(_next), item( key, T() ) {}
        node( const Key &key, const T &t, const node_base_ptr_t &_next = OID_NULL ) : node_base( _next ), item( key, t ) {}
#if __TBB_CPP11_RVALUE_REF_PRESENT
        node( const Key &key, T &&t, node_base_ptr_t &&_next = OID_NULL ) : node_base(std::move(_next) ), item( key, std::move( t ) ) {}
        node( value_type&& i, node_base_ptr_t &&_next = OID_NULL ) : node_base( std::move( _next ) ), item( std::move( i ) ) {}
#if __TBB_CPP11_VARIADIC_TEMPLATES_PRESENT
        template<typename... Args>
        node( Args&&... args, node_base_ptr_t &&_next = OID_NULL ) : node_base( std::forward<node_base_ptr_t>(_next) ), item( std::forward<Args>( args )... ) {}
#if __TBB_COPY_FROM_NON_CONST_REF_BROKEN
        node( value_type& i, node_base_ptr_t &_next = OID_NULL ) : node_base( _next ), item( const_cast<const value_type&>(i) ) {}
#endif //__TBB_COPY_FROM_NON_CONST_REF_BROKEN
#endif //__TBB_CPP11_VARIADIC_TEMPLATES_PRESENT
#endif //__TBB_CPP11_RVALUE_REF_PRESENT
        node( const value_type& i, const node_base_ptr_t &_next = OID_NULL ) : node_base( _next ), item( i ) {}
    };

    typedef internal::persistent_pool_ptr<node> persistent_node_ptr_t;

    void delete_node( const node_base_ptr_t& n ) {
        delete_persistent<node>( internal::static_persistent_pool_pointer_cast<node>(n).get_persistent_ptr(my_pool_uuid) );
    }

    static void allocate_node_copy_construct( pool_base &pop, persistent_ptr<node>& node_ptr, const Key &key, const T * t, const node_base_ptr_t &next = OID_NULL ) {
        make_persistent_atomic<node>( pop, node_ptr, key, *t, next );
    }

#if __TBB_CPP11_RVALUE_REF_PRESENT
    static void allocate_node_move_construct( pool_base &pop, persistent_ptr<node>& node_ptr, const Key &key, const T * t, const node_base_ptr_t &next = OID_NULL ) {
        make_persistent_atomic<node>( pop, node_ptr, key, std::move( *const_cast<T*>(t) ), next );
    }
#if __TBB_CPP11_VARIADIC_TEMPLATES_PRESENT
    template<typename... Args>
    static void allocate_node_emplace_construct( pool_base &pop, persistent_ptr<node>& node_ptr, Args&&... args ) {
        make_persistent_atomic<node>( pop, node_ptr, std::forward<Args>( args )... );
    }
#endif //#if __TBB_CPP11_VARIADIC_TEMPLATES_PRESENT
#endif

    static void allocate_node_default_construct( pool_base& pop, persistent_ptr<node>& node_ptr, const Key &key, const T *, const node_base_ptr_t &next = OID_NULL ) {
        make_persistent_atomic<node>( pop, node_ptr, key, next );
    }

    static void do_not_allocate_node( pool_base&, persistent_ptr<node>& node_ptr, const Key &, const T *, const node_base_ptr_t &next = OID_NULL ) {
        __TBB_ASSERT( false, "this dummy function should not be called" );
    }

    persistent_node_ptr_t search_bucket( const key_type &key, bucket *b ) const {
        __TBB_ASSERT( b->tmp_node != internal::rehash_req, "Search can be executed only for rehashed bucket" );
        persistent_node_ptr_t n = internal::static_persistent_pool_pointer_cast<node>( b->node_list );
        while ( is_valid( n ) && !my_hash_compare.equal( key, n.get(my_pool_uuid)->item.first ) )
            n = internal::static_persistent_pool_pointer_cast<node>( n.get( my_pool_uuid )->next );
        return n;
    }

    //! bucket accessor is to find, rehash, acquire a lock, and access a bucket
    class bucket_accessor : public bucket::scoped_t {
        bucket *my_b;
    public:
        bucket_accessor( persistent_concurrent_hash_map *base, const hashcode_t h, bool writer = false ) { acquire( base, h, writer ); }
        //! find a bucket by masked hashcode, optionally rehash, and acquire the lock
        inline void acquire( persistent_concurrent_hash_map *base, const hashcode_t h, bool writer = false ) {
            my_b = base->get_bucket( h );

            // TODO: actually, notification is unnecessary here, just hiding double-check
            if ( itt_load_word_with_acquire( tbb::internal::as_atomic(my_b->tmp_node.raw().off) ) != uint64_t(0) // Workaround: need to load tmp_node persistent ptr with acquire, but seems only off is enough.
                && try_acquire( my_b->mutex.get(), /*write=*/true ) )
            {
                if ( my_b->tmp_node == internal::rehash_req ) {
                    base->rehash_bucket( my_b, h ); //recursive rehashing
                }
                else {
                    base->correct_bucket( my_b );
                }
            }
            else
            {
                bucket::scoped_t::acquire( my_b->mutex.get(), writer );
            }
            __TBB_ASSERT( my_b->tmp_node == nullptr, NULL );
        }
        //! check whether bucket is locked for write
        bool is_writer() const { return bucket::scoped_t::is_writer; }
        //! get bucket pointer
        bucket *operator() () { return my_b; }
    };

    hashcode_t get_hash_code( node_base_ptr_t& n ) {
        return my_hash_compare.hash( internal::static_persistent_pool_pointer_cast<node>(n)(my_pool_uuid)->item.first );
    }

    // TODO refactor to hash_base
    void rehash_bucket( bucket *b_new, const hashcode_t h ) {
        __TBB_ASSERT( *(intptr_t*)(&b_new->mutex.get()), "b_new must be locked (for write)" );
        __TBB_ASSERT( h > 1, "The lowermost buckets can't be rehashed" );
        
        hashcode_t mask = (1u << __TBB_Log2( h )) - 1; // get parent mask from the topmost bit
#if __TBB_STATISTICS
        my_info_rehashes++; // invocations of rehash_bucket
#endif

        bucket_accessor b_old( this, h & mask );
#if INTEL_PRIVATE
        TRACEP( "%x: got %s lock on %x\n", unsigned( h ), b_old.is_writer() ? "Write" : "Read", unsigned( h&mask ) );
#endif

        mask = (mask << 1) | 1; // get full mask for new bucket
        __TBB_ASSERT( (mask&(mask + 1)) == 0 && (h & mask) == h, NULL );

        pool_base pop = get_pool_base();
        node_base_ptr_t *p_new = &(b_new->node_list);
        bool restore_after_crash = *p_new != nullptr;

    restart:
        for ( node_base_ptr_t *p_old = &(b_old()->node_list), n = __TBB_load_with_acquire<const uint64_t>( p_old->raw() ); is_valid( n ); n = *p_old ) {
            hashcode_t c = get_hash_code( n );
#if TBB_USE_ASSERT
            hashcode_t bmask = h & (mask >> 1);
            bmask = bmask == 0 ? 1 : (1u << (__TBB_Log2( bmask ) + 1)) - 1; // minimal mask of parent bucket
            __TBB_ASSERT( (c & bmask) == (h & bmask), "hash() function changed for key in table" );
#endif
            if ( (c & mask) == h ) {
                if ( !b_old.is_writer() ) {
                    if ( !b_old.upgrade_to_writer() ) {
#if INTEL_PRIVATE
                        TRACEP( "%x: upgraded with contention (for %x)\n", unsigned( h ), unsigned( c ) );
#endif
                        goto restart; // node ptr can be invalid due to concurrent erase
                    }
                }
#if INTEL_PRIVATE
                TRACEP( "%x: moving %x\n", unsigned( h ), unsigned( c ) );
#endif
                if ( restore_after_crash ) {
                    while ( *p_new != nullptr && (mask & get_hash_code( *p_new ) ) == h && *p_new != n ) {
                        p_new = &((*p_new)(my_pool_uuid)->next);
                    }
                    restore_after_crash = false;
                }
                *p_new = n; // Add to new b_new
                pop.persist( p_new, sizeof( *p_new ) );
                *p_old = n( my_pool_uuid )->next; // exclude from b_old
                pop.persist( p_old, sizeof( *p_old ) );
                p_new = &(n( my_pool_uuid )->next);
            }
            else p_old = &(n( my_pool_uuid )->next); // iterate to next item
        }

        if ( restore_after_crash )
            while ( *p_new != nullptr && (mask & get_hash_code( *p_new ) ) == h )
                p_new = &((*p_new)(my_pool_uuid)->next);
        *p_new = nullptr;
        pop.persist( p_new, sizeof( *p_new ) );
        // TODO: now we update only offset field, because pool_uuid is the same. Need to assign whole pointer, but there is compilation issue
        //__TBB_store_with_release( b_new->tmp_node, internal::rehashed ); // mark rehashed
        __TBB_store_with_release( b_new->tmp_node.raw_ptr()->off, internal::rehashed.raw().off ); // mark rehashed
        //b_new->tmp_node = internal::rehashed;
        pop.persist( b_new->tmp_node );
    }

    struct call_clear_on_leave {
        persistent_concurrent_hash_map* my_ch_map;
        call_clear_on_leave( persistent_concurrent_hash_map* a_ch_map ) : my_ch_map( a_ch_map ) {}
        void dismiss() { my_ch_map = 0; }
        ~call_clear_on_leave() {
            if ( my_ch_map ) {
                my_ch_map->clear();
            }
        }
    };
public:

    class accessor;
    //! Combines data access, locking, and garbage collection.
    class const_accessor : private node::scoped_t /*which derived from no_copy*/ {
        friend class persistent_concurrent_hash_map<Key, T, HashCompare>;
        friend class accessor;
        typedef pmem::obj::persistent_ptr<node> node_ptr_t;
    public:
        //! Type of value
        typedef const typename persistent_concurrent_hash_map::value_type value_type;

        //! True if result is empty.
        bool empty() const { return !my_node; }

        //! Set to null
        void release() {
            if ( my_node ) {
                node::scoped_t::release();
                my_node = 0;
            }
        }

        //! Return reference to associated value in hash table.
        const_reference operator*() const {
            __TBB_ASSERT( my_node, "attempt to dereference empty accessor" );
            return my_node->item;
        }

        //! Return pointer to associated value in hash table.
        const_pointer operator->() const {
            return &operator*();
        }

        //! Create empty result
        const_accessor() : my_node( OID_NULL ) {}

        //! Destroy result after releasing the underlying reference.
        ~const_accessor() {
            my_node = OID_NULL; // scoped lock's release() is called in its destructor
        }
    protected:
        bool is_writer() { return node::scoped_t::is_writer; }
        node_ptr_t my_node;
        hashcode_t my_hash;
    };

    //! Allows write access to elements and combines data access, locking, and garbage collection.
    class accessor : public const_accessor {
    public:
        //! Type of value
        typedef typename persistent_concurrent_hash_map::value_type value_type;

        //! Return reference to associated value in hash table.
        reference operator*() const {
            __TBB_ASSERT( this->my_node, "attempt to dereference empty accessor" );
            return this->my_node->item;
        }

        //! Return pointer to associated value in hash table.
        pointer operator->() const {
            return &operator*();
        }
    };

    //! Construct empty table.
    persistent_concurrent_hash_map()
        : internal::persistent_hash_map_base()
    {
    }

    //! Construct empty table with n preallocated buckets. This number serves also as initial concurrency level.
    persistent_concurrent_hash_map( size_type n )
        : internal::persistent_hash_map_base()
    {
        reserve( n );
    }

    //! Copy constructor
    persistent_concurrent_hash_map( const persistent_concurrent_hash_map &table )
        : internal::persistent_hash_map_base()
    {
        __TBB_ASSERT( false, "internal_copy is not implemented" );
        internal_copy( table );
    }

#if __TBB_CPP11_RVALUE_REF_PRESENT
    //! Move constructor
    persistent_concurrent_hash_map( persistent_concurrent_hash_map &&table )
        : internal::persistent_hash_map_base()
    {
        swap( table );
    }
#endif //__TBB_CPP11_RVALUE_REF_PRESENT

    //! Construction with copying iteration range and given allocator instance
    template<typename I>
    persistent_concurrent_hash_map( I first, I last )
    {
        reserve( std::distance( first, last ) ); // TODO: load_factor?
        internal_copy( first, last );
    }

#if __TBB_INITIALIZER_LISTS_PRESENT
    //! Construct empty table with n preallocated buckets. This number serves also as initial concurrency level.
    persistent_concurrent_hash_map( std::initializer_list<value_type> il )
    {
        reserve( il.size() );
        internal_copy( il.begin(), il.end() );
    }

#endif //__TBB_INITIALIZER_LISTS_PRESENT

    //! Assignment
    persistent_concurrent_hash_map& operator=( const persistent_concurrent_hash_map &table ) {
        if ( this != &table ) {
            clear();
            internal_copy( table );
        }
        return *this;
    }

#if __TBB_INITIALIZER_LISTS_PRESENT
    //! Assignment
    persistent_concurrent_hash_map& operator=( std::initializer_list<value_type> il ) {
        clear();
        reserve( il.size() );
        internal_copy( il.begin(), il.end() );
        return *this;
    }
#endif //__TBB_INITIALIZER_LISTS_PRESENT

    //! Rehashes and optionally resizes the whole table.
    /** Useful to optimize performance before or after concurrent operations.
    Also enables using of find() and count() concurrent methods in serial context. */
    void rehash( size_type n = 0 );

    //! Clear table
    void clear();

    //! Clear table and destroy it.
    ~persistent_concurrent_hash_map() { clear(); }

    //------------------------------------------------------------------------
    // Parallel algorithm support
    //------------------------------------------------------------------------
    range_type range( size_type grainsize = 1 ) {
        return range_type( *this, grainsize );
    }
    const_range_type range( size_type grainsize = 1 ) const {
        return const_range_type( *this, grainsize );
    }

    //------------------------------------------------------------------------
    // STL support - not thread-safe methods
    //------------------------------------------------------------------------
    iterator begin() { return iterator( *this, 0, my_embedded_segment, my_embedded_segment->node_list.get(my_pool_uuid) ); }
    iterator end() { return iterator( *this, 0, 0, 0 ); }
    const_iterator begin() const { return const_iterator( *this, 0, my_embedded_segment, my_embedded_segment->node_list.get( my_pool_uuid ) ); }
    const_iterator end() const { return const_iterator( *this, 0, 0, 0 ); }
    std::pair<iterator, iterator> equal_range( const Key& key ) { return internal_equal_range( key, end() ); }
    std::pair<const_iterator, const_iterator> equal_range( const Key& key ) const { return internal_equal_range( key, end() ); }

    //! Number of items in table.
    size_type size() const { return my_size.get_ro(); }

    //! True if size()==0.
    bool empty() const { return my_size.get_ro() == 0; }

    //! Upper bound on size.
    size_type max_size() const { return (~size_type( 0 )) / sizeof( node ); }

    //! Returns the current number of buckets
    size_type bucket_count() const { return my_mask.get_ro() + 1; }

    //! swap two instances. Iterators are invalidated
    void swap( persistent_concurrent_hash_map &table );

    //------------------------------------------------------------------------
    // concurrent map operations
    //------------------------------------------------------------------------

    //! Return count of items (0 or 1)
    size_type count( const Key &key ) const {
        return const_cast<persistent_concurrent_hash_map*>(this)->lookup(/*insert*/false, key, NULL, NULL, /*write=*/false, &do_not_allocate_node );
    }

    //! Find item and acquire a read lock on the item.
    /** Return true if item is found, false otherwise. */
    bool find( const_accessor &result, const Key &key ) const {
        result.release();
        return const_cast<persistent_concurrent_hash_map*>(this)->lookup(/*insert*/false, key, NULL, &result, /*write=*/false, &do_not_allocate_node );
    }

    //! Find item and acquire a write lock on the item.
    /** Return true if item is found, false otherwise. */
    bool find( accessor &result, const Key &key ) {
        result.release();
        return lookup(/*insert*/false, key, NULL, &result, /*write=*/true, &do_not_allocate_node );
    }

    //! Insert item (if not already present) and acquire a read lock on the item.
    /** Returns true if item is new. */
    bool insert( const_accessor &result, const Key &key ) {
        result.release();
        return lookup(/*insert*/true, key, NULL, &result, /*write=*/false, &allocate_node_default_construct );
    }

    //! Insert item (if not already present) and acquire a write lock on the item.
    /** Returns true if item is new. */
    bool insert( accessor &result, const Key &key ) {
        result.release();
        return lookup(/*insert*/true, key, NULL, &result, /*write=*/true, &allocate_node_default_construct );
    }

    //! Insert item by copying if there is no such key present already and acquire a read lock on the item.
    /** Returns true if item is new. */
    bool insert( const_accessor &result, const value_type &value ) {
        result.release();
        return lookup(/*insert*/true, value.first, &value.second, &result, /*write=*/false, &allocate_node_copy_construct );
    }

    //! Insert item by copying if there is no such key present already and acquire a write lock on the item.
    /** Returns true if item is new. */
    bool insert( accessor &result, const value_type &value ) {
        result.release();
        return lookup(/*insert*/true, value.first, &value.second, &result, /*write=*/true, &allocate_node_copy_construct );
    }

    //! Insert item by copying if there is no such key present already
    /** Returns true if item is inserted. */
    bool insert( const value_type &value ) {
        return lookup(/*insert*/true, value.first, &value.second, NULL, /*write=*/false, &allocate_node_copy_construct );
    }

#if __TBB_CPP11_RVALUE_REF_PRESENT
    //! Insert item by copying if there is no such key present already and acquire a read lock on the item.
    /** Returns true if item is new. */
    bool insert( const_accessor &result, value_type && value ) {
        return generic_move_insert( result, std::move( value ) );
    }

    //! Insert item by copying if there is no such key present already and acquire a write lock on the item.
    /** Returns true if item is new. */
    bool insert( accessor &result, value_type && value ) {
        return generic_move_insert( result, std::move( value ) );
    }

    //! Insert item by copying if there is no such key present already
    /** Returns true if item is inserted. */
    bool insert( value_type && value ) {
        return generic_move_insert( accessor_not_used(), std::move( value ) );
    }

#if __TBB_CPP11_VARIADIC_TEMPLATES_PRESENT
    //! Insert item by copying if there is no such key present already and acquire a read lock on the item.
    /** Returns true if item is new. */
    template<typename... Args>
    bool emplace( const_accessor &result, Args&&... args ) {
        return generic_emplace( result, std::forward<Args>( args )... );
    }

    //! Insert item by copying if there is no such key present already and acquire a write lock on the item.
    /** Returns true if item is new. */
    template<typename... Args>
    bool emplace( accessor &result, Args&&... args ) {
        return generic_emplace( result, std::forward<Args>( args )... );
    }

    //! Insert item by copying if there is no such key present already
    /** Returns true if item is inserted. */
    template<typename... Args>
    bool emplace( Args&&... args ) {
        return generic_emplace( accessor_not_used(), std::forward<Args>( args )... );
    }
#endif //__TBB_CPP11_VARIADIC_TEMPLATES_PRESENT
#endif //__TBB_CPP11_RVALUE_REF_PRESENT

    //! Insert range [first, last)
    template<typename I>
    void insert( I first, I last ) {
        for ( ; first != last; ++first )
            insert( *first );
    }

#if __TBB_INITIALIZER_LISTS_PRESENT
    //! Insert initializer list
    void insert( std::initializer_list<value_type> il ) {
        insert( il.begin(), il.end() );
    }
#endif //__TBB_INITIALIZER_LISTS_PRESENT

    //! Erase item.
    /** Return true if item was erased by particularly this call. */
    bool erase( const Key& key );

    //! Erase item by const_accessor.
    /** Return true if item was erased by particularly this call. */
    bool erase( const_accessor& item_accessor ) {
        return exclude( item_accessor );
    }

    //! Erase item by accessor.
    /** Return true if item was erased by particularly this call. */
    bool erase( accessor& item_accessor ) {
        return exclude( item_accessor );
    }
protected:
    //! Insert or find item and optionally acquire a lock on the item.
    bool lookup( bool op_insert, const Key &key, const T *t, const_accessor *result, bool write, void (*allocate_node)(pool_base&, persistent_ptr<node>&, const Key &, const T *, const node_base_ptr_t&) );

    struct accessor_not_used { void release() {} };
    friend const_accessor* accessor_location( accessor_not_used const& ) { return NULL; }
    friend const_accessor* accessor_location( const_accessor & a ) { return &a; }

    friend bool is_write_access_needed( accessor const& ) { return true; }
    friend bool is_write_access_needed( const_accessor const& ) { return false; }
    friend bool is_write_access_needed( accessor_not_used const& ) { return false; }

#if __TBB_CPP11_RVALUE_REF_PRESENT
    template<typename Accessor>
    bool generic_move_insert( Accessor && result, value_type && value ) {
        result.release();
        return lookup(/*insert*/true, value.first, &value.second, accessor_location( result ), is_write_access_needed( result ), &allocate_node_move_construct );
    }

#if __TBB_CPP11_VARIADIC_TEMPLATES_PRESENT
    template<typename Accessor, typename... Args>
    bool generic_emplace( Accessor && result, Args &&... args ) {
        result.release();
        node * node_ptr = allocate_node_emplace_construct(std::forward<Args>( args )... );
        return lookup(/*insert*/true, node_ptr->item.first, NULL, accessor_location( result ), is_write_access_needed( result ), &do_not_allocate_node, node_ptr );
    }
#endif //__TBB_CPP11_VARIADIC_TEMPLATES_PRESENT
#endif //__TBB_CPP11_RVALUE_REF_PRESENT

    //! delete item by accessor
    bool exclude( const_accessor &item_accessor );

    //! Copy "source" to *this, where *this must start out empty.
    void internal_copy( const persistent_concurrent_hash_map& source );

    template<typename I>
    void internal_copy( I first, I last );

}; // class persistent_concurrent_hash_map

template<typename Key, typename T, typename HashCompare>
bool persistent_concurrent_hash_map<Key, T, HashCompare>::lookup( bool op_insert, const Key &key, const T *t, const_accessor *result, bool write, void( *allocate_node )(pool_base&, persistent_ptr<node>&, const Key&, const T*, const node_base_ptr_t&) ) {
    __TBB_ASSERT( !result || !result->my_node, NULL );
    bool return_value = false;
    hashcode_t const h = my_hash_compare.hash( key );
    hashcode_t m = (hashcode_t)itt_load_word_with_acquire( tbb::internal::as_atomic( my_mask.get_ro() ) );
    persistent_node_ptr_t n;
    size_type sz = 0;
restart:
    {//lock scope
        __TBB_ASSERT( (m&(m + 1)) == 0, "data structure is invalid" );
        return_value = false;
        // get bucket
        bucket_accessor b( this, h & m );
#if INTEL_PRIVATE
        TRACEP( "%x: got %s lock on %x\n", unsigned( h ), b.is_writer() ? "Write" : "Read", unsigned( h&m ) );
#endif
        // find a node
        n = search_bucket( key, b() );
        if ( op_insert ) {
            // [opt] insert a key
            if ( !n ) {
                if ( !b.is_writer() && !b.upgrade_to_writer() ) { // TODO: improved insertion
#if INTEL_PRIVATE
                    TRACEP( "%x: upgrade with contention on %x\n", unsigned( h ), unsigned( h&m ) );
#endif
                    // Rerun search_list, in case another thread inserted the item during the upgrade.
                    n = search_bucket( key, b() );
                    if ( is_valid( n ) ) { // unfortunately, it did
                        b.downgrade_to_reader();
#if INTEL_PRIVATE
                        TRACEP( "%x: contention: another thread inserted faster\n", unsigned( h ) );
#endif
                        goto exists;
                    }
                }
                if ( check_mask_race( h, m ) )
                    goto restart; // b.release() is done in ~b().

                __TBB_ASSERT( b()->tmp_node == nullptr, "tmp_node pointer is not null" );

                // insert and set flag to grow the container
                pool_base p = get_pool_base();
                allocate_node( p, reinterpret_cast<persistent_ptr<node>& >(b()->tmp_node), key, t, b()->node_list );
                n = b()->tmp_node;
                sz = insert_new_node( b() );
                return_value = true;
#if INTEL_PRIVATE
                TRACEP( "%x: inserted\n", unsigned( h ) );
#endif
            }
        }
        else { // find or count
            if ( !n ) {
                if ( check_mask_race( h, m ) )
                    goto restart; // b.release() is done in ~b(). TODO: replace by continue
                return false;
            }
            return_value = true;
        }
    exists:
        if ( !result ) goto check_growth;
        // TODO: the following seems as generic/regular operation
        // acquire the item
        if ( !result->try_acquire( n.get( my_pool_uuid )->mutex.get(), write ) ) {
#if INTEL_PRIVATE
            TRACEP( "%x: node contention\n", unsigned( h ) );
#endif
            for ( tbb::internal::atomic_backoff backoff( true );; ) {
                if ( result->try_acquire( n.get( my_pool_uuid )->mutex.get(), write ) ) break;
                if ( !backoff.bounded_pause() ) {
                    // the wait takes really long, restart the operation
                    b.release();
                    __TBB_ASSERT( !op_insert || !return_value, "Can't acquire new item in locked bucket?" );
                    __TBB_Yield();
                    m = (hashcode_t)itt_load_word_with_acquire( tbb::internal::as_atomic( my_mask.get_ro() ) );
                    goto restart;
                }
            }
        }
    } //lock scope
    result->my_node = n.get_persistent_ptr(my_pool_uuid);
    result->my_hash = h;
check_growth:
    // [opt] grow the container
    if ( check_growth( m, sz ) ) {
#if __TBB_STATISTICS
        my_info_resizes++; // concurrent ones
#endif
    }

    // this code is not required since allocated memory automatically deleted if transaction rolled back
    /*if ( tmp_n ) // if op_insert only
        delete_node( tmp_n );*/
    return return_value;
}

template<typename Key, typename T, typename HashCompare>
bool persistent_concurrent_hash_map<Key, T, HashCompare>::exclude( const_accessor &item_accessor ) {
    __TBB_ASSERT( item_accessor.my_node, NULL );
    node_base_ptr_t const n = item_accessor.my_node;
    hashcode_t const h = item_accessor.my_hash;
    hashcode_t m = (hashcode_t)itt_load_word_with_acquire( internal::as_atomic( my_mask.get_ro() ) );
    pool_base p = get_pool_base();
    // NVML could throw transaction error exaption, do we need to catch them or it is user resposibility
    transaction::manual tx( p );
    do {
        // get bucket
        bucket_accessor b( this, h & m, /*writer=*/true );
        node_base_ptr_t *p = &b()->node_list;
        while ( *p && *p != n )
            p = &(*p)(my_pool_uuid)->next;
        if ( !*p ) { // someone else was first
            if ( check_mask_race( h, m ) )
                continue;
            item_accessor.release();
            return false;
        }
        __TBB_ASSERT( *p == n, NULL );
        *p = n(my_pool_uuid)->next; // remove from container
        --(internal::as_atomic( my_size.get_rw() ));
        break;
    } while ( true );
    if ( !item_accessor.is_writer() ) // need to get exclusive lock
        item_accessor.upgrade_to_writer(); // return value means nothing here
    item_accessor.release();
    delete_node( n ); // Only one thread can delete it
    transaction::commit();
    return true;
}

template<typename Key, typename T, typename HashCompare>
bool persistent_concurrent_hash_map<Key, T, HashCompare>::erase( const Key &key ) {
    node_base_ptr_t n;
    hashcode_t const h = my_hash_compare.hash( key );
    hashcode_t m = (hashcode_t)itt_load_word_with_acquire( internal::as_atomic( my_mask.get_ro() ) );
    pool_base p = get_pool_base();
    // NVML could throw transaction error exaption, do we need to catch them or it is user resposibility
    transaction::manual tx( p );
restart:
    {//lock scope
     // get bucket
        bucket_accessor b( this, h & m );
    search:
        node_base_ptr_t *p = &b()->node_list;
        n = *p;
        while ( is_valid( n ) && !my_hash_compare.equal( key, internal::static_persistent_pool_pointer_cast<node>(n)(my_pool_uuid)->item.first ) ) {
            p = &n(my_pool_uuid)->next;
            n = *p;
        }
        if ( !n ) { // not found, but mask could be changed
            if ( check_mask_race( h, m ) )
                goto restart;
            return false;
        }
        else if ( !b.is_writer() && !b.upgrade_to_writer() ) {
            if ( check_mask_race( h, m ) ) // contended upgrade, check mask
                goto restart;
            goto search;
        }
        *p = n(my_pool_uuid)->next;
        --(internal::as_atomic( my_size.get_rw() ));
    }
    {
        typename node::scoped_t item_locker( n(my_pool_uuid)->mutex.get(), /*write=*/true );
    }
    // note: there should be no threads pretending to acquire this mutex again, do not try to upgrade const_accessor!
    delete_node( n ); // Only one thread can delete it due to write lock on the bucket
    transaction::commit();
    return true;
}

template<typename Key, typename T, typename HashCompare>
void persistent_concurrent_hash_map<Key, T, HashCompare>::swap( persistent_concurrent_hash_map<Key, T, HashCompare> &table ) {
    //TODO: respect C++11 allocator_traits<A>::propogate_on_constainer_swap
    using std::swap;
    swap( this->my_hash_compare, table.my_hash_compare );
    internal_swap( table );
}

template<typename Key, typename T, typename HashCompare>
void persistent_concurrent_hash_map<Key, T, HashCompare>::rehash( size_type sz ) {
    pool_base pop = get_pool_base();

    reserve( sz ); // TODO: add reduction of number of buckets as well

    hashcode_t mask = my_mask;
    hashcode_t b = (mask + 1) >> 1; // size or first index of the last segment
    __TBB_ASSERT( (b&(b - 1)) == 0, NULL ); // zero or power of 2
    bucket *bp = get_bucket( b ); // only the last segment should be scanned for rehashing

    for ( ; b <= mask; b++, bp++ ) {
        node_base_ptr_t n = bp->node_list;
        __TBB_ASSERT( is_valid( n ) || n == internal::empty_bucket || bp->tmp_node == internal::rehash_req, "Broken internal structure" );
        __TBB_ASSERT( *reinterpret_cast<intptr_t*>(&bp->mutex.get()) == 0, "concurrent or unexpectedly terminated operation during rehash() execution" );
        if ( bp->tmp_node == internal::rehash_req ) { // rehash bucket, conditional because rehashing of a previous bucket may affect this one
            hashcode_t h = b; bucket *b_old = bp;
            do {
                __TBB_ASSERT( h > 1, "The lowermost buckets can't be rehashed" );
                hashcode_t m = (1u << __TBB_Log2( h )) - 1; // get parent mask from the topmost bit
                b_old = get_bucket( h &= m );
            } while ( b_old->tmp_node == internal::rehash_req );
            // now h - is index of the root rehashed bucket b_old

            { // transaction scope
                // PMDK could throw transaction error exaption, do we need to catch them or it is user resposibility
                transaction::manual tx( pop );
                mark_rehashed_levels( h ); // mark all non-rehashed children recursively across all segments
                for ( node_base_ptr_t *p = &b_old->node_list, q = *p; is_valid( q ); q = *p ) {
                    hashcode_t c = my_hash_compare.hash( internal::static_persistent_pool_pointer_cast<node>(q)(my_pool_uuid)->item.first );
                    if ( (c & mask) != h ) { // should be rehashed
                        *p = q( my_pool_uuid )->next; // exclude from b_old
                        bucket *b_new = get_bucket( c & mask );
                        __TBB_ASSERT( b_new->tmp_node != internal::rehash_req, "hash() function changed for key in table or internal error" );
                        b_new->tmp_node = q( my_pool_uuid );
                        add_to_bucket( b_new, pop );
                    }
                    else p = &q( my_pool_uuid )->next; // iterate to next item
                }
                transaction::commit();
            }
        }
    }
}

template<typename Key, typename T, typename HashCompare>
void persistent_concurrent_hash_map<Key, T, HashCompare>::clear() {
    hashcode_t m = my_mask;
    __TBB_ASSERT( (m&(m + 1)) == 0, "data structure is invalid" );
#if TBB_USE_ASSERT || TBB_USE_PERFORMANCE_WARNINGS || __TBB_STATISTICS
#if TBB_USE_PERFORMANCE_WARNINGS || __TBB_STATISTICS
    int current_size = int( my_size ), buckets = int( m ) + 1, empty_buckets = 0, overpopulated_buckets = 0; // usage statistics
    static bool reported = false;
#endif
    bucket *bp = 0;
    // check consistency
    for ( segment_index_t b = 0; b <= m; b++ ) {
        if ( b & (b - 2) ) ++bp; // not the beginning of a segment
        else bp = get_bucket( b );
        node_base_ptr_t n = bp->node_list;
        __TBB_ASSERT( is_valid( n ) || n == internal::empty_bucket || bp->tmp_node == internal::rehash_req, "Broken internal structure" );
        __TBB_ASSERT( *reinterpret_cast<intptr_t*>(&bp->mutex.get()) == 0, "concurrent or unexpectedly terminated operation during clear() execution" );
#if TBB_USE_PERFORMANCE_WARNINGS || __TBB_STATISTICS
        if ( bp->tmp_node == internal::rehash_req ) buckets--;
        else if ( n == internal::empty_bucket ) empty_buckets++;
        else if ( n(my_pool_uuid)->next ) overpopulated_buckets++;
#endif
#if __TBB_EXTRA_DEBUG
        for ( ; is_valid( n ); n = n->next ) {
            hashcode_t h = my_hash_compare.hash( static_cast<node*>(n)->item.first );
#if INTEL_PRIVATE
            if ( (h & m) != b ) std::printf( "Incomplete rehashing: h=%08X & %03X b=%03X b_home->node_list=%p\n",
                unsigned( h ), unsigned( m ), unsigned( b ), get_bucket( h & m )->node_list );
#endif
            h &= m;
            __TBB_ASSERT( h == b || get_bucket( h )->tmp_node == internal::rehash_req, "hash() function changed for key in table or internal error" );
        }
#endif
    }
#if TBB_USE_PERFORMANCE_WARNINGS || __TBB_STATISTICS
#if __TBB_STATISTICS
    printf( "items=%d buckets: capacity=%d rehashed=%d empty=%d overpopulated=%d"
        " concurrent: resizes=%u rehashes=%u restarts=%u\n",
        current_size, int( m + 1 ), buckets, empty_buckets, overpopulated_buckets,
        unsigned( my_info_resizes ), unsigned( my_info_rehashes ), unsigned( my_info_restarts ) );
    my_info_resizes = 0; // concurrent ones
    my_info_restarts = 0; // race collisions
    my_info_rehashes = 0;  // invocations of rehash_bucket
#endif
    if ( buckets > current_size ) empty_buckets -= buckets - current_size;
    else overpopulated_buckets -= current_size - buckets; // TODO: load_factor?
    if ( !reported && buckets >= 512 && (2 * empty_buckets > current_size || 2 * overpopulated_buckets > current_size) ) {
        tbb::internal::runtime_warning(
            "Performance is not optimal because the hash function produces bad randomness in lower bits in %s.\nSize: %d  Empties: %d  Overlaps: %d",
#if __TBB_USE_OPTIONAL_RTTI
            typeid(*this).name(),
#else
            "persistent_concurrent_hash_map",
#endif
            current_size, empty_buckets, overpopulated_buckets );
        reported = true;
    }
#endif
#endif//TBB_USE_ASSERT || TBB_USE_PERFORMANCE_WARNINGS || __TBB_STATISTICS
    pool_base p = get_pool_base();
    { // transaction scope
        // PMDK could throw transaction error exaption, do we need to catch them or it is user resposibility
        transaction::manual tx( p );

        my_size = 0;
        segment_index_t s = segment_index_of( m );
        __TBB_ASSERT( s + 1 == pointers_per_table || !segment_facade_t(my_table,s + 1).is_valid(), "wrong mask or concurrent grow" );
        do {
            segment_facade_t segment(my_table, s);
            __TBB_ASSERT(segment.is_valid(), "wrong mask or concurrent grow");
            size_type sz = segment.size();
            for (segment_index_t i = 0; i < sz; i++)
                for (node_base_ptr_t n = segment[i].node_list; is_valid(n); n = segment[i].node_list) {
                    segment[i].node_list = n(my_pool_uuid)->next;
                    delete_node(n);
                }
            if (s >= first_block) // the first segment or the next
                segment.disable();
            else if (s == embedded_block && embedded_block != first_block) {
                delete_persistent<bucket[]>(my_table[s], segment_size(first_block) - embedded_buckets);
                if (s >= embedded_block) my_table[s] = 0;
            }
        } while ( s-- > 0 );
        my_mask = embedded_buckets - 1;
        transaction::commit();
    }
}

template<typename Key, typename T, typename HashCompare>
void persistent_concurrent_hash_map<Key, T, HashCompare>::internal_copy( const persistent_concurrent_hash_map& source ) {
    reserve( source.my_size ); // TODO: load_factor?
    hashcode_t mask = source.my_mask;
    if ( my_mask == mask ) { // optimized version
        bucket *dst = 0, *src = 0;
        bool rehash_required = false;
        for ( hashcode_t k = 0; k <= mask; k++ ) {
            if ( k & (k - 2) ) ++dst, src++; // not the beginning of a segment
            else { dst = get_bucket( k ); src = source.get_bucket( k ); }
            __TBB_ASSERT( dst->tmp_node != internal::rehash_req, "Invalid bucket in destination table" );
            node_base_ptr_t n = src->node_list;
            if ( src->tmp_node == internal::rehash_req ) { // source is not rehashed, items are in previous buckets
                rehash_required = true;
                dst->tmp_node = internal::rehash_req;
            }
            else for ( ; n; n = n(my_pool_uuid)->next ) {
                persistent_node_ptr_t src_node = internal::static_persistent_pool_pointer_cast<node>(n);
                add_to_bucket( dst, allocate_node_copy_construct( src_node(my_pool_uuid)->item.first, src_node(my_pool_uuid)->item.second ) );
                ++(internal::as_atomic( my_size.get_rw() ) ); // TODO: replace by non-atomic op
            }
        }
        if ( rehash_required ) rehash();
    }
    else internal_copy( source.begin(), source.end() );
}

template<typename Key, typename T, typename HashCompare>
template<typename I>
void persistent_concurrent_hash_map<Key, T, HashCompare>::internal_copy( I first, I last ) {
    hashcode_t m = my_mask;
    for ( ; first != last; ++first ) {
        hashcode_t h = my_hash_compare.hash( (*first).first );
        bucket *b = get_bucket( h & m );
        __TBB_ASSERT( b->tmp_node != internal::rehash_req, "Invalid bucket in destination table" );
        persistent_node_ptr_t n = allocate_node_copy_construct( (*first).first, (*first).second );
        add_to_bucket( b, n );
        ++(my_size.get_rw()); // TODO: replace by non-atomic op
    }
}

} // namespace interface5

namespace experimental {
using interface5::persistent_concurrent_hash_map;

#if INTEL_PRIVATE
// It is not necessary to put the following functions in namespace tbb::interfaceX because
// each of them has an argument that will carry a tbb::interfaceX tag in its name mangling.
#endif /* INTEL_PRIVATE */

template<typename Key, typename T, typename HashCompare>
inline bool operator==( const persistent_concurrent_hash_map<Key, T, HashCompare> &a, const persistent_concurrent_hash_map<Key, T, HashCompare> &b ) {
    if ( a.size() != b.size() ) return false;
    typename persistent_concurrent_hash_map<Key, T, HashCompare>::const_iterator i( a.begin() ), i_end( a.end() );
    typename persistent_concurrent_hash_map<Key, T, HashCompare>::const_iterator j, j_end( b.end() );
    for ( ; i != i_end; ++i ) {
        j = b.equal_range( i->first ).first;
        if ( j == j_end || !(i->second == j->second) ) return false;
    }
    return true;
}

template<typename Key, typename T, typename HashCompare>
inline bool operator!=( const persistent_concurrent_hash_map<Key, T, HashCompare> &a, const persistent_concurrent_hash_map<Key, T, HashCompare> &b )
{
    return !(a == b);
}

template<typename Key, typename T, typename HashCompare>
inline void swap( persistent_concurrent_hash_map<Key, T, HashCompare> &a, persistent_concurrent_hash_map<Key, T, HashCompare> &b )
{
    a.swap( b );
}

} // namespace experimental
} // namespace pmem

#endif // __PMEM_persistent_concurrent_hash_map_H
