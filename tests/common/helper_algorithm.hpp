// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * Helper algorithms to test C++ containers
 */

#ifndef HELPER_ALGORITHM_HPP
#define HELPER_ALGORITHM_HPP

/**
 * Checks if the elements in range [first, last) are sorted in a strictly
 * ascending order.
 */
template <class ForwardIt, class Compare>
bool
is_strictly_increased(ForwardIt first, ForwardIt last, Compare comp)
{
	if (first != last) {
		ForwardIt next = first;
		while (++next != last) {
			if (!comp(*first, *next))
				return false;
			first = next;
		}
	}

	return true;
}

#endif /* HELPER_ALGORITHM_HPP */
