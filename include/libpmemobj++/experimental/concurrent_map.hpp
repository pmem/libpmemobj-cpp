// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019, Intel Corporation */

#ifndef PMEMOBJ_CONCURRENT_MAP_HPP
#define PMEMOBJ_CONCURRENT_MAP_HPP

#include <libpmemobj++/allocator.hpp>
#include <libpmemobj++/detail/concurrent_skip_list_impl.hpp>

namespace pmem
{
namespace obj
{
namespace experimental
{

template <typename Key, typename Value, typename KeyCompare,
	  typename RND_GENERATOR, typename Allocator, bool AllowMultimapping,
	  size_t MAX_LEVEL>
class map_traits {
public:
	static constexpr size_t max_level = MAX_LEVEL;
	using random_generator_type = RND_GENERATOR;
	using key_type = Key;
	using mapped_type = Value;
	using compare_type = KeyCompare;
	using value_type = std::pair<const key_type, mapped_type>;
	using reference = value_type &;
	using const_reference = const value_type &;
	using allocator_type = Allocator;

	/**
	 * pmem::detail::concurrent_skip_list allows multimapping. If this flag
	 * is true we can store multiple entries with the same key. For
	 * concurrent_map it should be false; For concurrent_multimap it should
	 * be true;
	 */
	constexpr static bool allow_multimapping = AllowMultimapping;

	static const key_type &
	get_key(const_reference val)
	{
		return val.first;
	}
}; /* class map_traits */

template <typename Key, typename Value, typename Comp = std::less<Key>,
	  typename Allocator =
		  pmem::obj::allocator<std::pair<const Key, Value>>>
class concurrent_map
    : public detail::concurrent_skip_list<
	      map_traits<Key, Value, Comp, detail::default_random_generator,
			 Allocator, false, 64>> {
	using traits_type =
		map_traits<Key, Value, Comp, detail::default_random_generator,
			   Allocator, false, 64>;
	using base_type = pmem::detail::concurrent_skip_list<traits_type>;

public:
	using value_type = typename base_type::value_type;
	using key_compare = Comp;
	using allocator_type = Allocator;

	using iterator = typename base_type::iterator;
	using const_iterator = typename base_type::const_iterator;
	using reverse_iterator = typename base_type::reverse_iterator;
	using const_reverse_iterator =
		typename base_type::const_reverse_iterator;

	using size_type = typename base_type::size_type;
	using difference_type = typename base_type::difference_type;

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
	 * Construct the map with initializer list
	 */
	concurrent_map(std::initializer_list<value_type> il)
	    : base_type(il.begin(), il.end())
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
};

} /* namespace experimental */
} /* namespace obj */
} /* namespace pmem */
#endif /* PMEMOBJ_CONCURRENT_MAP_HPP */
