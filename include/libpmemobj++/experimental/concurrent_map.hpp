// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#ifndef PMEMOBJ_CONCURRENT_MAP_HPP
#define PMEMOBJ_CONCURRENT_MAP_HPP

#include <libpmemobj++/allocator.hpp>
#include <libpmemobj++/container/detail/concurrent_skip_list_impl.hpp>
#include <libpmemobj++/detail/pair.hpp>

namespace pmem
{
namespace obj
{
namespace experimental
{
/**
 * Persistent memory aware implementation of Intel TBB concurrent_map. It is a
 * sorted associative container that contains key-value pairs with unique keys.
 * Keys are sorted by using the comparison function Compare. Search, removal,
 * and insertion operations have average logarithmic complexity. Everywhere the
 * concurrent_map uses the Compare requirements, uniqueness is determined by
 * using the equivalence relation. In imprecise terms, two objects a and b are
 * considered equivalent (not unique) if neither compares less than the other:
 * !comp(a, b) && !comp(b, a).
 *
 * The implementation is based on the lock-based concurrent skip list algorithm
 * described in
 * https://www.cs.tau.ac.il/~shanir/nir-pubs-web/Papers/OPODIS2006-BA.pdf.
 * Our concurrent skip list implementation supports concurrent insertion and
 * traversal, but not concurrent erasure. The erase method is prefixed with
 * unsafe_, to indicate that there is no concurrency safety.
 *
 * Each time, the pool with concurrent_map is being opened, the concurrent_map
 * requires runtime_initialize() to be called in order to restore the map state
 * after process restart.
 *
 * Key, Value, Comp and Allcoator types should be persistent memory aware types.
 * Allocator type should satisfies the named requirements
 * (https://en.cppreference.com/w/cpp/named_req/Allocator). The allocate() and
 * deallocate() methods are called inside transactions.
 */
template <typename Key, typename Value, typename Comp = std::less<Key>,
	  typename Allocator =
		  pmem::obj::allocator<detail::pair<const Key, Value>>>
class concurrent_map
    : public detail::concurrent_skip_list<detail::map_traits<
	      Key, Value, Comp, detail::default_random_generator, Allocator,
	      false, 64>> {
	using traits_type = detail::map_traits<Key, Value, Comp,
					       detail::default_random_generator,
					       Allocator, false, 64>;
	using base_type = pmem::detail::concurrent_skip_list<traits_type>;

public:
	using key_type = typename base_type::key_type;
	using mapped_type = typename base_type::mapped_type;
	using value_type = typename base_type::value_type;
	using size_type = typename base_type::size_type;
	using difference_type = typename base_type::difference_type;
	using key_compare = Comp;
	using allocator_type = Allocator;
	using reference = typename base_type::reference;
	using const_reference = typename base_type::const_reference;
	using pointer = typename base_type::pointer;
	using const_pointer = typename base_type::const_pointer;
	using iterator = typename base_type::iterator;
	using const_iterator = typename base_type::const_iterator;
	using reverse_iterator = typename base_type::reverse_iterator;
	using const_reverse_iterator =
		typename base_type::const_reverse_iterator;

	/**
	 * Default constructor.
	 */
	concurrent_map() = default;

	/**
	 * Copy constructor.
	 */
	concurrent_map(const concurrent_map &table) : base_type(table)
	{
	}

	/**
	 * Move constructor.
	 */
	concurrent_map(concurrent_map &&table) : base_type(std::move(table))
	{
	}

	/**
	 * Constructs the map with the contents of the range [first, last).
	 */
	template <class InputIt>
	concurrent_map(InputIt first, InputIt last,
		       const key_compare &comp = Comp(),
		       const allocator_type &alloc = allocator_type())
	    : base_type(first, last, comp, alloc)
	{
	}

	/**
	 * Constructs the map with initializer list
	 */
	concurrent_map(std::initializer_list<value_type> ilist)
	    : base_type(ilist.begin(), ilist.end())
	{
	}

	/**
	 * Assignment operator
	 */
	concurrent_map &
	operator=(const concurrent_map &other)
	{
		return static_cast<concurrent_map &>(
			base_type::operator=(other));
	}

	/**
	 * Move-assignment operator
	 */
	concurrent_map &
	operator=(concurrent_map &&other)
	{
		return static_cast<concurrent_map &>(
			base_type::operator=(std::move(other)));
	}

	/**
	 *
	 */
	concurrent_map &
	operator=(std::initializer_list<value_type> ilist)
	{
		return static_cast<concurrent_map &>(
			base_type::operator=(ilist));
	}
};

} /* namespace experimental */
} /* namespace obj */
} /* namespace pmem */
#endif /* PMEMOBJ_CONCURRENT_MAP_HPP */
