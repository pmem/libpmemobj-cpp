/*
 * Copyright 2018-2019, Intel Corporation
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
 * Const policy class for segmentation management.
 */

namespace pmem
{
namespace obj
{

template <typename T>
class segment_policy {
public:
	static const long unsigned int MAX_SEGMENTS = 64;

	/* Traits */
	using value_type = T;
	using size_type = std::size_t;

	/**
	 * @return index of segment where should locate element with specified
	 * index.
	 */
	static constexpr size_type
	get_segment(size_type idx)
	{
		return static_cast<size_type>(detail::Log2(idx | 1));
	}

	/**
	 * @return index of first element in specified segment index.
	 */
	static constexpr size_type
	segment_top(size_type segment_idx)
	{
		return (size_type(1) << segment_idx) & ~size_type(1);
	}

	/**
	 * @return return size for specified segment index.
	 */
	static constexpr size_type
	segment_size(size_type segment_idx)
	{
		return (segment_idx == 0) ? 2 : segment_top(segment_idx);
	}

	/**
	 * @return item number in the segment in which it should be.
	 */
	static constexpr size_type
	segment_local(size_type idx)
	{
		return idx - segment_top(get_segment(idx));
	}

	static constexpr size_type
	largest_segment()
	{
		return get_segment(PMEMOBJ_MAX_ALLOC_SIZE / sizeof(value_type));
	}

	static constexpr size_type
	max_size()
	{
		return 2 * segment_size(largest_segment());
	}
};

} /* namespace obj */
} /* namespace pmem */
