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

#ifndef LIBPMEMOBJ_CPP_EXAMPLES_DICTIONARY_HPP
#define LIBPMEMOBJ_CPP_EXAMPLES_DICTIONARY_HPP

#include "libpmemobj_cpp_examples_common.hpp"

#include <cstring>
#include <iostream>

#include <libpmemobj++/experimental/slice.hpp>
#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using namespace pmem;
using namespace pmem::obj;

namespace examples
{

template <typename T>
using pmem_vector = experimental::vector<T>;

std::ostream &
operator<<(std::ostream &out, const pmem_vector<char> &v)
{
	for (const auto el : v)
		out << el;
	return out;
}

/*
 * Dictionary radix tree implemented using libpmemobj C++ bindings.
 */
class pmem_dictionary {
private:
	struct pmem_dictionary_node;
	using node_pptr = persistent_ptr<pmem_dictionary_node>;

	struct pmem_dictionary_node {
		p<int> is_word; /* indicate if prefixes completes the word */
		pmem_vector<char> prefix;	/* current prefix */
		pmem_vector<node_pptr> children; /* pointers to children */
		node_pptr parent;		 /* pointer to parent */

		/* default constructor */
		pmem_dictionary_node()
		    : is_word(0), prefix(), children(), parent(nullptr){};
	};

	/* private helper functions */
	size_t get_prefix_length(const std::string &s1,
				 const pmem_vector<char> &s2,
				 const size_t from = 0) const noexcept;
	pool_base pool_get() const noexcept;
	void print_debug(const pmem_dictionary_node &node, const std::string &s,
			 const std::string &prefix_current) const;
	void print_node(const pmem_dictionary_node &node, const std::string &s,
			bool debug) const;
	void print_preorder(const pmem_dictionary_node &node,
			    const std::string &s, const bool debug) const;
	node_pptr find(const std::string &word, size_t from,
		       const node_pptr &node) const;
	void child_add(const std::string &word, const size_t from,
		       node_pptr node);
	void insert_split(const std::string &word, const size_t from,
			  node_pptr node, const size_t prefix);
	void insert_helper(const std::string &word, size_t from, node_pptr node,
			   size_t prefix_len);
	void node_balance(node_pptr node);
	void remove_postorder(pmem_dictionary_node &node);

	/* dictionary members */
	p<int> size;
	node_pptr root;

public:
	pmem_dictionary();
	~pmem_dictionary();

	/* Dictionary API */
	void print(const bool debug = false) const;
	bool lookup(const std::string &word) const;
	void insert(const std::string &word);
	void remove(const std::string &word);
	void remove_all();
};

/*
 * Returns length of common prefix of s1 (starting from index from) and s2.
 */
size_t
pmem_dictionary::get_prefix_length(const std::string &s1,
				   const pmem_vector<char> &s2,
				   const size_t from) const noexcept
{
	size_t i = 0;
	for (; i + from < s1.size() && i < s2.size(); ++i)
		if (s1[i + from] != s2.const_at(i))
			break;

	return i;
};

/*
 * Returns handle to pool, where dictionary resides.
 */
pool_base
pmem_dictionary::pool_get() const noexcept
{
	auto pop = pmemobj_pool_by_ptr(this);
	assert(pop != nullptr);
	return pool_base(pop);
};

/*
 * Prints node detail information.
 */
void
pmem_dictionary::print_debug(const pmem_dictionary_node &node,
			     const std::string &s,
			     const std::string &prefix_current) const
{
	std::cout << "current node:" << std::endl;
	std::cout << "             word: " << s + prefix_current << std::endl;
	std::cout << "             prefix: " << prefix_current << std::endl;
	std::cout << "             is_word: " << node.is_word << std::endl;
	std::cout << "             children(s): ";
	if (node.children.empty()) {
		std::cout << "(leaf)";
	} else {
		for (const auto ch : node.children)
			std::cout << ch->prefix << " ";
	}
	std::cout << std::endl;
	std::cout << "             parent: ";
	if (node.parent == nullptr) {
		std::cout << "nullptr (this is root node)";
	} else {
		std::cout << node.parent->prefix;
		if (node.parent == root)
			std::cout << "(root)";
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

/*
 * Prints node infromation. Flag 'debug' indicates if word or detailed
 * information should be printed.
 */
void
pmem_dictionary::print_node(const pmem_dictionary_node &node,
			    const std::string &s, bool debug) const
{
	auto curr = std::string(node.prefix.cbegin(), node.prefix.cend());

	if (debug)
		print_debug(node, s, curr);
	else if (node.is_word)
		std::cout << s + curr << std::endl;
};

/*
 * Preorder traverse given tree root 'node' and prints each node
 * information.
 */
void
pmem_dictionary::print_preorder(const pmem_dictionary_node &node,
				const std::string &s, const bool debug) const
{
	/* visit node */
	print_node(node, s, debug);

	for (const auto &el : node.children) {
		std::string v = s +
			std::string(node.prefix.cbegin(), node.prefix.cend());
		/* traverse children recursively */
		print_preorder(*el, v, debug);
	}
};

/*
 * Finds node with string 'word' (beginning from 'from' index) in given tree.
 */
pmem_dictionary::node_pptr
pmem_dictionary::find(const std::string &word, size_t from,
		      const node_pptr &node) const
{
	const auto &current = *node;
	size_t prefix_len = get_prefix_length(word, current.prefix, from);

	if (prefix_len < current.prefix.size())
		return nullptr;

	if (word.size() - from == current.prefix.size())
		return current.is_word ? node : nullptr;

	from += current.prefix.size();

	for (const auto &el : current.children) {
		const auto ret = find(word, from, el);
		if (ret != nullptr)
			return ret;
	}

	return nullptr;
};

/*
 * Adds child with given word to given node.
 */
void
pmem_dictionary::child_add(const std::string &word, const size_t from,
			   node_pptr node)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(node != nullptr);

	node_pptr node_new = make_persistent<pmem_dictionary_node>();

	std::ptrdiff_t advance = static_cast<std::ptrdiff_t>(from);
	node_new->prefix.assign(word.cbegin() + advance, word.cend());
	node_new->is_word = 1;
	node_new->parent = node;

	node->children.push_back(node_new);

	/* bulk snapshot all elements in vector and sort the container */
	auto slice = node->children.range(0, node->children.size());
	std::sort(slice.begin(), slice.end(), [](node_pptr lhs, node_pptr rhs) {
		return lhs->prefix < rhs->prefix;
	});

	size++;
};

/*
 * Inserts new word to dictionary, by spliting given node.
 * The example below shows how to add word 'ax' to existing dictionary which
 * already contains words 'abc' and 'ab':
 *
 *    ab        a
 *    |   ->   / \
 *    c       b   x
 *            |
 *            c
 */
void
pmem_dictionary::insert_split(const std::string &word, const size_t from,
			      node_pptr node, const size_t prefix)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(node != nullptr);
	assert(prefix < node->prefix.size());

	node_pptr child_moved = make_persistent<pmem_dictionary_node>();

	child_moved->is_word = node->is_word;
	child_moved->children = std::move(node->children);
	for (auto ch : child_moved->children)
		ch->parent = child_moved;
	child_moved->parent = node;
	child_moved->prefix.assign(node->prefix.cbegin() + prefix,
				   node->prefix.cend());

	std::ptrdiff_t advance = static_cast<std::ptrdiff_t>(prefix);
	node->prefix.erase(node->prefix.begin() + advance, node->prefix.end());
	/* node->children after being moved is empty now; add first element */
	node->children.push_back(child_moved);

	if (word.size() == from + prefix) {
		node->is_word = 1;
		size++;
	} else {
		node->is_word = 0;
		child_add(word, from + prefix, node);
	}
};

/*
 * Helper function for managing insert operation.
 */
void
pmem_dictionary::insert_helper(const std::string &word, size_t from,
			       node_pptr node, size_t prefix_len)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(node != nullptr);

	auto &current = *node;
	/*
	 * there are three cases:
	 *	1 common prefix of current string and string to be inserted is
	 *	  smaller than a length of current string. In this case split
	 *	  current string to match common prefix and insert sufix of word
	 *	  to be inserted
	 *	2 common prefix of current string and string to be inserted is
	 *	  equal to the length of current string and current string is
	 *	  equal to sufix of the string to be inserted. In this case just
	 *	  set is_word flag accordingly.
	 *	3 common prefix of current string and string to be inserted is
	 *	  equal to the length of current string and current string is
	 *	  not equal to sufix of the string to be insrted. In this case
	 *	  either continuue searching for more common prefixes in
	 *	  children vector, or add new child if existing children do not
	 *	  have common prefixes with string to be inserted..
	 */

	/* allow prefix_len to be 0 in order to support empty prefix in root */
	assert(prefix_len <= current.prefix.size());

	/* case 1 */
	if (prefix_len < current.prefix.size())
		return insert_split(word, from, node, prefix_len);

	/* case 2 */
	if (word.size() - from == current.prefix.size()) {
		if (!current.is_word) {
			current.is_word = 1;
			size++;
		}
		return;
	}

	from += current.prefix.size();

	/* case 3 */
	for (auto &ch : current.children) {
		if (word[from] == ch->prefix.const_at(0)) {
			prefix_len = get_prefix_length(word, ch->prefix, from);
			return insert_helper(word, from, ch, prefix_len);
		}
	}

	return child_add(word, from, node);
};

/*
 * Balance given node.
 */
void
pmem_dictionary::node_balance(node_pptr node)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(node != nullptr);

	/*
	 * there are 3 cases:
	 *	1 node have more than one child or node is not a word, there is
	 *	  nothing to balance
	 *	2 node is a leaf (vector children is empty), just remove this
	 *	  node, update parent's children vector and try to balance
	 *	  parent node
	 *	3 node have only child, (vector children have one element),
	 *	  remove this node and update parent's vector to point to the
	 *	  only child instead of the node
	 */

	/* case 1 */
	if (node->is_word || node->children.size() > 1)
		return; /* there is nothing to balance */

	/* case 2 */
	if (node->children.empty()) {	  /* node is a leaf */
		if (node->parent == nullptr) { /* node is a root */
			root = nullptr;
		} else { /* node is not a root */
			auto it = node->parent->children.begin();
			while (*it != node)
				it++;
			node->parent->children.erase(it);

			node_balance(node->parent);
		}
	} else { /* case 3 */
		/* node have only child */
		auto only_child = node->children[0];

		auto prefix_begin = node->prefix.cbegin();
		auto prefix_end = node->prefix.cend();

		only_child->prefix.insert(only_child->prefix.begin(),
					  prefix_begin, prefix_end);

		if (node->parent == nullptr) { /* node is a root */
			root = only_child;
			only_child->parent = nullptr;
		} else { /* node is not a root */
			only_child->parent = node->parent;

			auto it = node->parent->children.begin();
			while (*it != node)
				it++;
			*it = only_child;
		}
	}

	delete_persistent<pmem_dictionary_node>(node);
}

/*
 * Postorder deletes nodes of given root 'node'.
 */
void
pmem_dictionary::remove_postorder(pmem_dictionary_node &node)
{
	for (const auto &el : node.children)
		remove_postorder(*el);

	/* visit node */
	delete_persistent<pmem_dictionary_node>(&node);
};

/*
 * Default constructor
 */
pmem_dictionary::pmem_dictionary() : size(0), root(nullptr){};

/*
 * Default destructor
 */
pmem_dictionary::~pmem_dictionary()
{
	pool_base pb = pool_get();

	transaction::run(pb, [&] {
		remove_all();
		delete_persistent<pmem_dictionary>(this);
	});
}

/*
 * Prints words in dictionary.
 * If true flag is passed, prints detailed nodes information.
 */
void
pmem_dictionary::print(const bool debug) const
{
	std::cout << "There are " << size
		  << " element(s) in dictionary (listed in alphabetic order):"
		  << std::endl;

	if (root == nullptr)
		return;
	std::string s;
	print_preorder(*root, s, debug);
};

/*
 * Checks if given word is in dictionary.
 */
bool
pmem_dictionary::lookup(const std::string &word) const
{
	if (root == nullptr)
		return false;

	return find(word, 0, root) == nullptr ? false : true;
};

/*
 * Inserts given word to dictionary.
 */
void
pmem_dictionary::insert(const std::string &word)
{
	pool_base pb = pool_get();

	transaction::run(pb, [&] {
		if (root == nullptr) { /* create first element */
			root = make_persistent<pmem_dictionary_node>();

			root->prefix.assign(word.cbegin(), word.cend());
			root->is_word = 1;

			size++;

			return;
		}

		size_t prefix_len = get_prefix_length(word, root->prefix);
		insert_helper(word, 0, root, prefix_len);
	});

	return;
};

/*
 * Remove given word from dictionary.
 */
void
pmem_dictionary::remove(const std::string &word)
{
	node_pptr rm = find(word, 0, root);
	if (rm == nullptr)
		return;

	pool_base pb = pool_get();

	transaction::run(pb, [&] {
		rm->is_word = 0;
		size--;

		node_balance(rm);
	});

	return;
};

/*
 * Removes all words from dictionary.
 */
void
pmem_dictionary::remove_all()
{
	if (root == nullptr)
		return;

	pool_base pb = pool_get();

	transaction::run(pb, [&] {
		remove_postorder(*root);

		root = nullptr;
		size = 0;
	});

	return;
};
}

#endif /* LIBPMEMOBJ_CPP_EXAMPLES_DICTIONARY_HPP */
