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
 * dictionary_cli.cpp -- dictionary example
 */

#include "dictionary.hpp"

#include <cstring>
#include <ctime>
#include <iostream>
#include <random>

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

using namespace pmem;
using namespace pmem::obj;

using pmem_dictionary = examples::pmem_dictionary;

namespace
{

const std::string LAYOUT = "dictionary_pool_layout";
const size_t POOL_SIZE = 1024 * 1024 * 64;
std::string prog_name;

/* available dictionary operations */
enum class dictionary_op {
	UNKNOWN,
	PRINT,
	PRINT_DEBUG,
	INSERT_GENERATE,
	INSERT,
	LOOKUP,
	DELETE_ELEMENTS,
	DELETE_ALL,

	MAX_ARRAY_OP
};

dictionary_op
parse_dictionary_op(const std::string &str)
{
	if (str == "print")
		return dictionary_op::PRINT;
	else if (str == "print_debug")
		return dictionary_op::PRINT_DEBUG;
	else if (str == "insert_generate")
		return dictionary_op::INSERT_GENERATE;
	else if (str == "insert")
		return dictionary_op::INSERT;
	else if (str == "lookup")
		return dictionary_op::LOOKUP;
	else if (str == "delete")
		return dictionary_op::DELETE_ELEMENTS;
	else if (str == "delete_all")
		return dictionary_op::DELETE_ALL;
	else
		return dictionary_op::UNKNOWN;
}

/*
 * Inserts num random generated strings of alpha-numeric chars with len in range
 * [1, len] to dictionary dict.
 */
void
insert_generate(pmem_dictionary &dict, const size_t num, const size_t max_len)
{
	static const char alphanum[] = "0123456789"
				       "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				       "abcdefghijklmnopqrstuvwxyz";

	std::mt19937 gen(std::random_device{}());
	std::uniform_int_distribution<size_t> dislen(1, max_len);
	std::uniform_int_distribution<size_t> dischar(0, sizeof(alphanum) - 2);

	for (size_t i = 0; i < num; ++i) {
		size_t len = dislen(gen);
		std::string s;
		s.reserve(len);
		while (len--)
			s += alphanum[dischar(gen)];
		std::cout << "Inserting word \"" << s << "\"..." << std::endl;
		dict.insert(s);
	}
}

void
print_usage()
{
	std::cout
		<< "dictionary_cli available commands:\n"
		   "<prog_name> <file_name> print\n"
		   "<prog_name> <file_name> print_debug\n"
		   "<prog_name> <file_name> insert_generate <number> <max_len>\n"
		   "<prog_name> <file_name> insert <word_1> <word_2> ... <word_n>\n"
		   "<prog_name> <file_name> lookup <word>\n"
		   "<prog_name> <file_name> delete <word_1> <word_2> ... <word_n>\n"
		   "<prog_name> <file_name> delete_all"
		<< std::endl;
};

struct root_dictionary {
	persistent_ptr<pmem_dictionary> pptr;
};
}

int
main(int argc, char *argv[])
{
	prog_name = argv[0];

	if (argc < 3) {
		print_usage();
		return 0;
	}

	const char *file = argv[1];
	dictionary_op op = parse_dictionary_op(argv[2]);

	pool<root_dictionary> pop;

	if (file_exists(file) != 0) {
		pop = pool<root_dictionary>::create(file, LAYOUT, POOL_SIZE,
						    CREATE_MODE_RW);
	} else {
		pop = pool<root_dictionary>::open(file, LAYOUT);
	}

	persistent_ptr<root_dictionary> r = pop.root();
	if (r->pptr == nullptr) {
		transaction::run(pop, [&] {
			r->pptr = make_persistent<pmem_dictionary>();
		});
	}

	auto &dictionary = *r->pptr;

	switch (op) {
		case dictionary_op::PRINT:
			if (argc == 3) {
				dictionary.print();
				break;
			}
		case dictionary_op::PRINT_DEBUG:
			if (argc == 3) {
				dictionary.print(true);
				break;
			}
		case dictionary_op::INSERT_GENERATE:
			if (argc == 5) {
				size_t num = (size_t)atoi(argv[3]);
				size_t max_len = (size_t)atoi(argv[4]);

				insert_generate(dictionary, num, max_len);
				break;
			}
		case dictionary_op::INSERT:
			if (argc > 3) {
				for (int i = 3; i < argc; ++i) {
					const std::string s(argv[i]);
					dictionary.insert(s);
				}
				break;
			}
		case dictionary_op::LOOKUP:
			if (argc == 4) {
				const std::string s(argv[3]);
				std::cout << dictionary.lookup(s) << std::endl;
				break;
			}
		case dictionary_op::DELETE_ELEMENTS:
			if (argc > 3) {
				for (int i = 3; i < argc; ++i) {
					const std::string s(argv[i]);
					dictionary.remove(s);
				}
				break;
			}
		case dictionary_op::DELETE_ALL:
			if (argc == 3) {
				dictionary.remove_all();
				break;
			}
		default:
			print_usage();
			break;
	}

	pop.close();

	return 0;
}
