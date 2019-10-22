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
 * concurrent_hash_map_insert_lookup.cpp -- pmem::obj::concurrent_hash_map test
 *
 */

#include "../concurrent_hash_map/concurrent_hash_map_test.hpp"
#include "unittest.hpp"

int
main(int argc, char *argv[])
{
	START();

	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<root>::create(
			path, LAYOUT, PMEMOBJ_MIN_POOL * 20, S_IWUSR | S_IRUSR);
		pmem::obj::transaction::run(pop, [&] {
			pop.root()->cons =
				nvobj::make_persistent<persistent_map_type>();
		});
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	/* Test that scoped_lock traits is working correctly */
#if LIBPMEMOBJ_CPP_USE_TBB_RW_MUTEX
	UT_ASSERT(pmem::obj::concurrent_hash_map_internal::scoped_lock_traits<
			  tbb::spin_rw_mutex::scoped_lock>::
			  initial_rw_state(true) == false);
#else
	UT_ASSERT(pmem::obj::concurrent_hash_map_internal::scoped_lock_traits<
			  pmem::obj::concurrent_hash_map_internal::
				  shared_mutex_scoped_lock<
					  pmem::obj::shared_mutex>>::
			  initial_rw_state(true) == true);
#endif

	size_t concurrency = 8;
	if (On_drd)
		concurrency = 2;
	std::cout << "Running tests for " << concurrency << " threads"
		  << std::endl;

	insert_and_lookup_key_test<persistent_map_type::const_accessor, int>(
		pop, concurrency);

	insert_and_lookup_key_test<persistent_map_type::accessor, int>(
		pop, concurrency);

	insert_and_lookup_value_type_test<
		persistent_map_type::const_accessor,
		const persistent_map_type::value_type>(pop, concurrency);

	insert_and_lookup_value_type_test<
		persistent_map_type::accessor,
		const persistent_map_type::value_type>(pop, concurrency);

	insert_and_lookup_value_type_test<persistent_map_type::const_accessor,
					  persistent_map_type::value_type>(
		pop, concurrency);

	insert_and_lookup_value_type_test<persistent_map_type::accessor,
					  persistent_map_type::value_type>(
		pop, concurrency);

	insert_and_lookup_value_type_test<persistent_map_type::value_type>(
		pop, concurrency);

	insert_and_lookup_value_type_test<
		const persistent_map_type::value_type>(pop, concurrency);

	insert_and_lookup_initializer_list_test(pop, concurrency);

	insert_and_lookup_iterator_test(pop, concurrency);

	pop.close();
	return 0;
}
