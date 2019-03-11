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
 *
 * create the pool for this program using pmempool, for example:
 *	pmempool create obj --layout=simplekv -s 1G word_count
 */

#include "simplekv.hpp"

#include <algorithm>
#include <fstream>
#include <numeric>
#include <thread>
#include <unordered_map>
#include <vector>

#include <numeric>

#define LAYOUT "simplekv"

using pmem::obj::delete_persistent;
using pmem::obj::make_persistent;
using pmem::obj::p;
using pmem::obj::persistent_ptr;
using pmem::obj::pool;
using pmem::obj::transaction;

namespace ptl = pmem::obj::experimental;

using simplekv_type =
	examples::kv<ptl::string, ptl::vector<ptl::string>, (1 << 20)>;
using word_count_kv = std::unordered_map<std::string, uint64_t>;

struct root {
	persistent_ptr<simplekv_type> simplekv;
};

void
read_file(pool<root> &pop, std::string fname)
{
	std::ifstream file;
	file.open(fname);

	auto r = pop.root();

	transaction::run(pop, [&] {
		auto vec = make_persistent<ptl::vector<ptl::string>>();
		auto pname = make_persistent<ptl::string>(fname);

		std::string word;
		while (file >> word) {
			vec->emplace_back(
				word.begin(),
				std::remove_if(
					word.begin(), word.end(),
					[](char c) { return !isalpha(c); }));
		}

		r->simplekv->insert(*pname, *vec);

		delete_persistent<ptl::vector<ptl::string>>(vec);
		delete_persistent<ptl::string>(pname);
	});
}

word_count_kv
map(const simplekv_type::value_type &vec)
{
	word_count_kv map;

	for (const auto &e : vec.value) {
		map[std::string(e.begin(), e.end())]++;
	}

	return map;
}

word_count_kv &
reduce(word_count_kv &m1, const word_count_kv &m2)
{
	for (const auto &e : m2)
		m1[e.first] += e.second;

	return m1;
}

int
main(int argc, char *argv[])
{
	if (argc < 3) {
		std::cerr << "usage: " << argv[0]
			  << " file-name file1.txt file2.txt ..." << std::endl;
		return 1;
	}

	auto path = argv[1];

	auto pop = pool<root>::open(path, LAYOUT);
	auto r = pop.root();

	if (r->simplekv == nullptr) {
		transaction::run(pop, [&]() {
			r->simplekv = make_persistent<simplekv_type>();
		});
	}

	for (int argn = 2; argn < argc; argn++)
		read_file(pop, argv[argn]);

	std::vector<word_count_kv> word_counts;

	std::transform(r->simplekv->begin(), r->simplekv->end(),
		       std::back_inserter(word_counts), map);

	auto result = std::accumulate(word_counts.begin(), word_counts.end(),
				      word_count_kv{}, reduce);

	for (const auto &e : result) {
		std::cout << e.first << " " << e.second << std::endl;
	}

	pop.close();

	return 0;
}
