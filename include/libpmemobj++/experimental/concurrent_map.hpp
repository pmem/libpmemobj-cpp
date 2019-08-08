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

#ifndef PMEMOBJ_CONCURRENT_MAP_HPP
#define PMEMOBJ_CONCURRENT_MAP_HPP

#include <libpmemobj++/allocator.hpp>
#include <libpmemobj++/detail/concurrent_skip_list_impl.hpp>

namespace std
{
/**
 * Specialization of std::hash for p<T>
 */
template <typename T>
struct less<pmem::obj::p<T>> {
	size_t
	operator()(const pmem::obj::p<T> &lhs, const pmem::obj::p<T> &rhs) const
	{
		return lhs.get_ro() < rhs.get_ro();
	}
};
} /* namespace std */

namespace pmem
{
namespace obj
{
namespace experimental
{

template <typename Key, typename Value, typename KeyCompare,
	  typename RND_GENERATOR, typename Allocator, bool AllowMultimapping>
class map_traits {
public:
	static constexpr size_t MAX_LEVEL = RND_GENERATOR::max_level;
	using random_level_generator_type = RND_GENERATOR;
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
}; // class map_traits

template <typename Key, typename Value, typename Comp = std::less<Key>,
	  typename Allocator =
		  pmem::obj::allocator<std::pair<const Key, Value>>>
class concurrent_map
    : public detail::concurrent_skip_list<map_traits<
	      Key, Value, Comp, detail::geometric_level_generator<64>,
	      Allocator, false>> {
	using traits_type = map_traits<Key, Value, Comp,
				       detail::geometric_level_generator<64>,
				       Allocator, false>;
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

} // namespace experimental
} // namespace obj
} // namespace pmem
#endif // PMEMOBJ_CONCURRENT_MAP_HPP
