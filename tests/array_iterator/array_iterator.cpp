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

#include "unittest.hpp"

#include <iterator>

#include <libpmemobj++/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

int
main()
{
	{
		typedef pmem::obj::array<int, 5> C;
		C c;
		C::iterator i;
		i = c.begin();
		C::const_iterator j;
		j = c.cbegin();
		UT_ASSERT(i == j);
	}

	{
		typedef pmem::obj::array<int, 0> C;
		C c;
		C::iterator i;
		i = c.begin();
		C::const_iterator j;
		j = c.cbegin();
		UT_ASSERT(i == j);
	}

	{
		typedef pmem::obj::array<int, 5> C;
		C::iterator ii1{}, ii2{};
		C::iterator ii4 = ii1;
		C::const_iterator cii{};
		UT_ASSERT(ii1 == ii2);
		UT_ASSERT(ii1 == ii4);
		UT_ASSERT(ii1 == cii);

		UT_ASSERT(!(ii1 != ii2));
		UT_ASSERT(!(ii1 != cii));

		C c;
		UT_ASSERT(c.begin() == std::begin(c));
		UT_ASSERT(c.cbegin() == std::cbegin(c));
		UT_ASSERT(c.rbegin() == std::rbegin(c));
		UT_ASSERT(c.crbegin() == std::crbegin(c));
		UT_ASSERT(c.end() == std::end(c));
		UT_ASSERT(c.cend() == std::cend(c));
		UT_ASSERT(c.rend() == std::rend(c));
		UT_ASSERT(c.crend() == std::crend(c));

		UT_ASSERT(std::begin(c) != std::end(c));
		UT_ASSERT(std::rbegin(c) != std::rend(c));
		UT_ASSERT(std::cbegin(c) != std::cend(c));
		UT_ASSERT(std::crbegin(c) != std::crend(c));
	}
	{
		typedef pmem::obj::array<int, 0> C;
		C::iterator ii1{}, ii2{};
		C::iterator ii4 = ii1;
		C::const_iterator cii{};
		UT_ASSERT(ii1 == ii2);
		UT_ASSERT(ii1 == ii4);

		UT_ASSERT(!(ii1 != ii2));

		UT_ASSERT((ii1 == cii));
		UT_ASSERT((cii == ii1));
		UT_ASSERT(!(ii1 != cii));
		UT_ASSERT(!(cii != ii1));
		UT_ASSERT(!(ii1 < cii));
		UT_ASSERT(!(cii < ii1));
		UT_ASSERT((ii1 <= cii));
		UT_ASSERT((cii <= ii1));
		UT_ASSERT(!(ii1 > cii));
		UT_ASSERT(!(cii > ii1));
		UT_ASSERT((ii1 >= cii));
		UT_ASSERT((cii >= ii1));
		UT_ASSERT(cii - ii1 == 0);
		UT_ASSERT(ii1 - cii == 0);

		C c;
		UT_ASSERT(c.begin() == std::begin(c));
		UT_ASSERT(c.cbegin() == std::cbegin(c));
		UT_ASSERT(c.rbegin() == std::rbegin(c));
		UT_ASSERT(c.crbegin() == std::crbegin(c));
		UT_ASSERT(c.end() == std::end(c));
		UT_ASSERT(c.cend() == std::cend(c));
		UT_ASSERT(c.rend() == std::rend(c));
		UT_ASSERT(c.crend() == std::crend(c));

		UT_ASSERT(std::begin(c) == std::end(c));
		UT_ASSERT(std::rbegin(c) == std::rend(c));
		UT_ASSERT(std::cbegin(c) == std::cend(c));
		UT_ASSERT(std::crbegin(c) == std::crend(c));
	}

#if TEST_STD_VER > 14
	{
		typedef pmem::obj::array<int, 5> C;
		constexpr C c{0, 1, 2, 3, 4};

		static_assert(c.begin() == std::begin(c), "");
		static_assert(c.cbegin() == std::cbegin(c), "");
		static_assert(c.end() == std::end(c), "");
		static_assert(c.cend() == std::cend(c), "");

		static_assert(c.rbegin() == std::rbegin(c), "");
		static_assert(c.crbegin() == std::crbegin(c), "");
		static_assert(c.rend() == std::rend(c), "");
		static_assert(c.crend() == std::crend(c), "");

		static_assert(std::begin(c) != std::end(c), "");
		static_assert(std::rbegin(c) != std::rend(c), "");
		static_assert(std::cbegin(c) != std::cend(c), "");
		static_assert(std::crbegin(c) != std::crend(c), "");

		static_assert(*c.begin() == 0, "");
		static_assert(*c.rbegin() == 4, "");

		static_assert(*std::begin(c) == 0, "");
		static_assert(*std::cbegin(c) == 0, "");
		static_assert(*std::rbegin(c) == 4, "");
		static_assert(*std::crbegin(c) == 4, "");
	}
#endif
}
