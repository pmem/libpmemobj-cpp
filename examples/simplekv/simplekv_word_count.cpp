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
 * simplekv_word_count.cpp -- implementation of a map-reduce algorithm
 * for counting words in text files.
 */

#include "simplekv.hpp"

#include <fstream>
#include <libpmemobj_cpp_examples_common.hpp>
#include <thread>

#include <numeric>

#define LAYOUT "simplekv"

using pmem_string = pmem::obj::experimental::string;

struct string_hash {
	std::size_t
	operator()(const pmem_string &data, int n)
	{
		assert(n <= 1);

		static constexpr std::size_t params[] = {
			0xff51afd7ed558ccd,
			0xc4ceb9fe1a85ec53,
			0x5fcdfd7ed551af8c,
			0xec53ba85e9fe1c4c,
		};
		std::string str(data.cbegin(), data.cend());
		std::size_t key = std::hash<std::string>{}(str);
		key ^= key >> 33;
		key *= params[n * 2];
		key ^= key >> 33;
		key *= params[(n * 2) + 1];
		key ^= key >> 33;
		return key;
	}
};

using pmem::obj::delete_persistent;
using pmem::obj::make_persistent;
using pmem::obj::p;
using pmem::obj::persistent_ptr;
using pmem::obj::pool;
using pmem::obj::transaction;

namespace ptl = pmem::obj::experimental;

using kv_type = examples::kv<pmem_string, uint64_t, string_hash, (1 << 20)>;
using dict_type = std::pair<kv_type, int>;
using dict_set = ptl::vector<dict_type>;

struct root {
	persistent_ptr<dict_set> dicts;
};

void
count_words(pool<root> &pop, dict_type &dict, std::string fname)
{
	std::ifstream file;
	file.open(fname);

	/* start from where we stopped */
	file.seekg(dict.second);

	std::string word;
	while (file >> word) {
		transaction::run(pop, [&] {
			/* save current position */
			dict.second = file.tellg();

			/* create word containing only alpha chars */
			auto pstring = make_persistent<pmem_string>(
				word.begin(),
				std::remove_if(
					word.begin(), word.end(),
					[](char c) { return !isalpha(c); }));

			try {
				dict.first.at(*pstring)++;
			} catch (std::out_of_range &) {
				/* there is no entry - insert new one */
				dict.first.insert(*pstring, 1);
			}

			delete_persistent<pmem_string>(pstring);
		});
	}

	file.close();
}

void
run_map(pool<root> &pop, std::vector<std::string> fnames)
{
	auto r = pop.root();
	auto nfiles = fnames.size();

	std::vector<std::thread> threads;

	threads.reserve(nfiles);
	r->dicts->resize(nfiles);

	for (int t = 0; t < static_cast<int>(nfiles); t++) {
		threads.emplace_back([t, &pop, &fnames, &r] {
			count_words(pop, r->dicts->at(t), fnames.at(t));
		});
	}

	for (auto &t : threads)
		t.join();
}

void
run_reduce(pool<root> &pop, std::string op_type)
{
	/* XXX */

	auto r = pop.root();

	for (const auto &d : *r->dicts) {
		for (const auto &e : d.first) {
			std::cout << e.key.c_str() << ": " << e.value << " ";
		}
		std::cout << std::endl;
	}
}

int
main(int argc, char *argv[])
{
	if (argc < 3) {
		std::cerr << "usage: " << argv[0]
			  << " file-name reduce_op file1.txt file2.txt ..."
			  << std::endl;
		return 1;
	}

	auto path = argv[1];
	auto reduce_op = argv[2];

	pool<root> pop;

	if (file_exists(path) != 0) {
		pop = pool<root>::create(path, LAYOUT, (1 << 30),
					 CREATE_MODE_RW);
	} else {
		pop = pool<root>::open(path, LAYOUT);
	}

	auto r = pop.root();
	if (r->dicts == nullptr) {
		transaction::run(
			pop, [&]() { r->dicts = make_persistent<dict_set>(); });
	}

	run_map(pop, std::vector<std::string>(argv + 2, argv + argc));
	run_reduce(pop, reduce_op);

	pop.close();

	return 0;
}
