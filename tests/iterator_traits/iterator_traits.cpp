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

/*
 * iterator_traits.cpp -- iterator traits tests
 */

#include "unittest.hpp"

#include "libpmemobj++/detail/iterator_traits.hpp"
#include <iterator>

struct test_iterator_base {
	using value_type = void;
	using difference_type = void;
	using pointer = void;
	using reference = void;
};

struct test_output_iterator : public test_iterator_base {
	using iterator_category = std::output_iterator_tag;
};

struct test_input_iterator : public test_iterator_base {
	using iterator_category = std::input_iterator_tag;
};

struct test_forward_iterator : public test_iterator_base {
	using iterator_category = std::forward_iterator_tag;
};

struct test_bidirectional_iterator : public test_iterator_base {
	using iterator_category = std::bidirectional_iterator_tag;
};

struct test_random_access_iterator : public test_iterator_base {
	using iterator_category = std::random_access_iterator_tag;
};

void
test_is_type_of_iterator()
{
	/* test is_input_iterator */
	UT_ASSERT(false ==
		  pmem::detail::is_input_iterator<test_output_iterator>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_input_iterator<test_input_iterator>::value);
	UT_ASSERT(
		true ==
		pmem::detail::is_input_iterator<test_forward_iterator>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_input_iterator<
			  test_bidirectional_iterator>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_input_iterator<
			  test_random_access_iterator>::value);
	UT_ASSERT(false == pmem::detail::is_input_iterator<int>::value);

	/* test forward_iterator */
	UT_ASSERT(
		false ==
		pmem::detail::is_forward_iterator<test_output_iterator>::value);
	UT_ASSERT(
		false ==
		pmem::detail::is_forward_iterator<test_input_iterator>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_forward_iterator<
			  test_forward_iterator>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_forward_iterator<
			  test_bidirectional_iterator>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_forward_iterator<
			  test_random_access_iterator>::value);

	/* test bidirectional_iterator */
	UT_ASSERT(false ==
		  pmem::detail::is_bidirectional_iterator<
			  test_output_iterator>::value);
	UT_ASSERT(false ==
		  pmem::detail::is_bidirectional_iterator<
			  test_input_iterator>::value);
	UT_ASSERT(false ==
		  pmem::detail::is_bidirectional_iterator<
			  test_forward_iterator>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_bidirectional_iterator<
			  test_bidirectional_iterator>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_bidirectional_iterator<
			  test_random_access_iterator>::value);

	/* test random_access_iterator */
	UT_ASSERT(false ==
		  pmem::detail::is_random_access_iterator<
			  test_output_iterator>::value);
	UT_ASSERT(false ==
		  pmem::detail::is_random_access_iterator<
			  test_input_iterator>::value);
	UT_ASSERT(false ==
		  pmem::detail::is_random_access_iterator<
			  test_forward_iterator>::value);
	UT_ASSERT(false ==
		  pmem::detail::is_random_access_iterator<
			  test_bidirectional_iterator>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_random_access_iterator<
			  test_random_access_iterator>::value);
}

int
main()
{
	START();

	test_is_type_of_iterator();

	return 0;
}
