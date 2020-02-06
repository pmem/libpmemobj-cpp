// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

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
main(int argc, char *argv[])
{
	return run_test([&] { test_is_type_of_iterator(); });
}
