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

/*
 * iterator_traits.cpp -- iterator traits tests
 */

#include "unittest.hpp"

#include <libpmemobj++/detail/iterator_traits.hpp>

void
test_is_type_of_iterator()
{
	/* test is_output_iterator */
	UT_ASSERT(true ==
		  pmem::detail::is_output_iterator<
			  test_support::output_it<int *>>::value);
	UT_ASSERT(false ==
		  pmem::detail::is_output_iterator<
			  test_support::input_it<int *>>::value);
	UT_ASSERT(false ==
		  pmem::detail::is_output_iterator<
			  test_support::forward_it<int *>>::value);
	UT_ASSERT(false ==
		  pmem::detail::is_output_iterator<
			  test_support::bidirectional_it<int *>>::value);
	UT_ASSERT(false ==
		  pmem::detail::is_output_iterator<
			  test_support::random_access_it<int *>>::value);
	UT_ASSERT(false == pmem::detail::is_output_iterator<int>::value);

	/* test is_input_iterator */
	UT_ASSERT(false ==
		  pmem::detail::is_input_iterator<
			  test_support::output_it<int *>>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_input_iterator<
			  test_support::input_it<int *>>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_input_iterator<
			  test_support::forward_it<int *>>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_input_iterator<
			  test_support::bidirectional_it<int *>>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_input_iterator<
			  test_support::random_access_it<int *>>::value);
	UT_ASSERT(false == pmem::detail::is_input_iterator<int>::value);
	UT_ASSERT(true == pmem::detail::is_input_iterator<int *>::value);

	/* test forward_iterator */
	UT_ASSERT(false ==
		  pmem::detail::is_forward_iterator<
			  test_support::output_it<int *>>::value);
	UT_ASSERT(false ==
		  pmem::detail::is_forward_iterator<
			  test_support::input_it<int *>>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_forward_iterator<
			  test_support::forward_it<int *>>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_forward_iterator<
			  test_support::bidirectional_it<int *>>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_forward_iterator<
			  test_support::random_access_it<int *>>::value);
	UT_ASSERT(false == pmem::detail::is_forward_iterator<int>::value);
	UT_ASSERT(true == pmem::detail::is_forward_iterator<int *>::value);

	/* test bidirectional_iterator */
	UT_ASSERT(false ==
		  pmem::detail::is_bidirectional_iterator<
			  test_support::output_it<int *>>::value);
	UT_ASSERT(false ==
		  pmem::detail::is_bidirectional_iterator<
			  test_support::input_it<int *>>::value);
	UT_ASSERT(false ==
		  pmem::detail::is_bidirectional_iterator<
			  test_support::forward_it<int *>>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_bidirectional_iterator<
			  test_support::bidirectional_it<int *>>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_bidirectional_iterator<
			  test_support::random_access_it<int *>>::value);
	UT_ASSERT(false == pmem::detail::is_bidirectional_iterator<int>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_bidirectional_iterator<int *>::value);

	/* test random_access_iterator */
	UT_ASSERT(false ==
		  pmem::detail::is_random_access_iterator<
			  test_support::output_it<int *>>::value);
	UT_ASSERT(false ==
		  pmem::detail::is_random_access_iterator<
			  test_support::input_it<int *>>::value);
	UT_ASSERT(false ==
		  pmem::detail::is_random_access_iterator<
			  test_support::forward_it<int *>>::value);
	UT_ASSERT(false ==
		  pmem::detail::is_random_access_iterator<
			  test_support::bidirectional_it<int *>>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_random_access_iterator<
			  test_support::random_access_it<int *>>::value);
	UT_ASSERT(false == pmem::detail::is_random_access_iterator<int>::value);
	UT_ASSERT(true ==
		  pmem::detail::is_random_access_iterator<int *>::value);
}

int
main()
{
	START();

	test_is_type_of_iterator();

	return 0;
}
