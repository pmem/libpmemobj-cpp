// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, 4Paradigm Inc. */

#ifndef PMEMOBJ_SWMR_MAP_HPP
#define PMEMOBJ_SWMR_MAP_HPP

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
 *
 */
template <typename Key, typename Value, typename Comp = std::less<Key>,
	  typename Allocator =
		  pmem::obj::allocator<detail::pair<const Key, Value>>>
class swmr_map : public detail::concurrent_skip_list<detail::map_traits<
			 Key, Value, Comp, detail::default_random_generator,
			 Allocator, false, 64, std::true_type>> {
	using traits_type =
		detail::map_traits<Key, Value, Comp,
				   detail::default_random_generator, Allocator,
				   false, 64, std::true_type>;
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

	/**
	 * Default constructor.
	 */
	swmr_map() = default;

	/**
	 * Copy constructor.
	 */
	swmr_map(const swmr_map &table) : base_type(table)
	{
	}

	/**
	 * Move constructor.
	 */
	swmr_map(swmr_map &&table) : base_type(std::move(table))
	{
	}

	/**
	 * Construct the empty map
	 */
	explicit swmr_map(const key_compare &comp,
			  const allocator_type &alloc = allocator_type())
	    : base_type(comp, alloc)
	{
	}

	/**
	 * Constructs the map with the contents of the range [first, last).
	 */
	template <class InputIt>
	swmr_map(InputIt first, InputIt last, const key_compare &comp = Comp(),
		 const allocator_type &alloc = allocator_type())
	    : base_type(first, last, comp, alloc)
	{
	}

	/**
	 * Constructs the map with initializer list
	 */
	swmr_map(std::initializer_list<value_type> ilist)
	    : base_type(ilist.begin(), ilist.end())
	{
	}

	/**
	 * Assignment operator
	 */
	swmr_map &
	operator=(const swmr_map &other)
	{
		return static_cast<swmr_map &>(base_type::operator=(other));
	}

	/**
	 * Move-assignment operator
	 */
	swmr_map &
	operator=(swmr_map &&other)
	{
		return static_cast<swmr_map &>(
			base_type::operator=(std::move(other)));
	}

	/**
	 *
	 */
	swmr_map &
	operator=(std::initializer_list<value_type> ilist)
	{
		return static_cast<swmr_map &>(base_type::operator=(ilist));
	}
};

/** Non-member swap */
template <typename Key, typename Value, typename Comp, typename Allocator>
void
swap(swmr_map<Key, Value, Comp, Allocator> &lhs,
     swmr_map<Key, Value, Comp, Allocator> &rhs)
{
	lhs.swap(rhs);
}

} /* namespace experimental */
} /* namespace obj */
} /* namespace pmem */
#endif /* PMEMOBJ_SWMR_MAP_HPP */
