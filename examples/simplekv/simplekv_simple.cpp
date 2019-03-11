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

/*
 * simplekv_simple.cpp -- implementation of a simple key-value store
 */

#include "simplekv.hpp"

#include <libpmemobj_cpp_examples_common.hpp>

#include <numeric>

#define LAYOUT "simplekv"

struct simple_hash {
	std::size_t
	operator()(const uint64_t &data, int n)
	{
		assert(n <= 1);

		static constexpr std::size_t params[] = {
			0xff51afd7ed558ccd,
			0xc4ceb9fe1a85ec53,
			0x5fcdfd7ed551af8c,
			0xec53ba85e9fe1c4c,
		};

		std::size_t key = data;
		key ^= data >> 33;
		key *= params[n * 2];
		key ^= key >> 33;
		key *= params[(n * 2) + 1];
		key ^= key >> 33;
		return key;
	}
};

using kv_type = examples::kv<uint64_t, uint64_t, simple_hash, 16>;

struct root {
	pmem::obj::persistent_ptr<kv_type> kv;
};

namespace
{

void
execute_get(pmem::obj::pool<root> &pop, uint64_t key)
{
	auto kv = pop.root()->kv;

	std::cout << kv->at(key) << std::endl;
}

void
execute_insert(pmem::obj::pool<root> &pop, uint64_t key, uint64_t value)
{
	auto kv = pop.root()->kv;

	kv->insert(key, value);
}

void
execute_print(pmem::obj::pool<root> &pop)
{
	auto kv = pop.root()->kv;

	for (const auto &e : *kv) {
		std::cout << e.key << " " << e.value << std::endl;
	}
}
}

int
main(int argc, char *argv[])
{
	if (argc < 3) {
		std::cerr << "usage: " << argv[0]
			  << " file-name [get key|insert key value|print]"
			  << std::endl;
		return 1;
	}

	auto path = argv[1];
	auto op = std::string(argv[2]);

	pmem::obj::pool<root> pop;

	if (file_exists(path) != 0) {
		pop = pmem::obj::pool<root>::create(
			path, LAYOUT, PMEMOBJ_MIN_POOL, CREATE_MODE_RW);

		pmem::obj::transaction::run(pop, [&] {
			pop.root()->kv = pmem::obj::make_persistent<kv_type>();
		});
	} else {
		pop = pmem::obj::pool<root>::open(path, LAYOUT);
	}

	try {
		if (op == "get") {
			execute_get(pop, std::stoull(argv[3]));
		} else if (op == "insert") {
			execute_insert(pop, std::stoull(argv[3]),
				       std::stoull(argv[4]));
		} else if (op == "print") {
			execute_print(pop);
		} else {
			throw std::runtime_error("wrong operation");
		}
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}

	pop.close();

	return 0;
}
