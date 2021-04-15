// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

/**
 * @file
 * Implementation of persistent radix tree.
 * Based on: https://github.com/pmem/vmemcache/blob/master/src/critnib.h
 *
 * The implementation is a variation of a PATRICIA trie - the internal
 * nodes do not store the path explicitly, but only a position at which
 * the keys differ. Keys are stored entirely in leaves.
 *
 * More info about radix tree: https://en.wikipedia.org/wiki/Radix_tree
 */

#ifndef LIBPMEMOBJ_CPP_RADIX_HPP
#define LIBPMEMOBJ_CPP_RADIX_HPP

#include <libpmemobj++/allocator.hpp>
#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/detail/pair.hpp>
#include <libpmemobj++/detail/template_helpers.hpp>
#include <libpmemobj++/experimental/inline_string.hpp>
#include <libpmemobj++/experimental/self_relative_ptr.hpp>
#include <libpmemobj++/experimental/v.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/string_view.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/utils.hpp>

#include <algorithm>
#include <iostream>
#include <string>
#if __cpp_lib_endian
#include <bit>
#endif

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/integer_sequence.hpp>
#include <libpmemobj++/detail/tagged_ptr.hpp>

namespace pmem
{

namespace detail
{
template <typename T, typename Enable = void>
struct bytes_view;
}

namespace obj
{

namespace experimental
{

/**
 * Radix tree is an associative, ordered container. Its API is similar
 * to the API of std::map.
 *
 * Unlike std::map radix tree does not use comparison (std::less or equivalent)
 * to locate elements. Instead, keys are mapped to a sequence of bytes using
 * user-provided BytesView type. The key's bytes uniquely define the position of
 * an element. In some way, it is similar to a hash table (BytesView can be
 * treated as a hash function) but with sorted elements.
 *
 * The elements' ordering is defined based on the BytesView mapping. The byte
 * sequences are compared using a function equivalent to std::string::compare.
 *
 * BytesView should accept a pointer to the key type in a constructor and
 * provide operator[] (should return a byte at the specified position in the
 * byte representation of value) and size (should return size of value in bytes)
 * method. The declaration should be as following:
 *
 * @code
 * struct BytesView {
 *  BytesView(const Type* t);
 *  char operator[](size_t pos) const; // Must be const!
 *  size_t size() const; // Must be const!
 * };
 * @endcode
 *
 * By default, implementation for pmem::obj::basic_inline_string<CharT, Traits>
 * and unsigned integral types is provided. Note that integral types are assumed
 * to be in little-endian.
 *
 * Iterators and references are stable (are not invalidated by inserts or erases
 * of other elements nor by assigning to the value) for all value types except
 * basic_inline_string<CharT, Traits>.
 *
 * In case of basic_inline_string<CharT, Traits>, iterators and references are
 * not invalidated by other inserts or erases, but might be invalidated by
 * assigning new value to the element. Using find(K).assign_val("new_value") may
 * invalidate other iterators and references to the element with key K.
 *
 * swap() invalidates all references and iterators.
 *
 * An example of custom BytesView implementation:
 * @snippet radix_tree/radix_tree_custom_key.cpp bytes_view_example
 */
template <typename Key, typename Value,
	  typename BytesView = detail::bytes_view<Key>>
class radix_tree {
	template <bool IsConst>
	struct radix_tree_iterator;

public:
	using key_type = Key;
	using mapped_type = Value;
	using value_type = detail::pair<const key_type, mapped_type>;
	using size_type = std::size_t;
	using reference = value_type &;
	using const_reference = const value_type &;
	using iterator = radix_tree_iterator<false>;
	using const_iterator = radix_tree_iterator<true>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using difference_type = std::ptrdiff_t;

	radix_tree();

	template <class InputIt>
	radix_tree(InputIt first, InputIt last);
	radix_tree(const radix_tree &m);
	radix_tree(radix_tree &&m);
	radix_tree(std::initializer_list<value_type> il);

	radix_tree &operator=(const radix_tree &m);
	radix_tree &operator=(radix_tree &&m);
	radix_tree &operator=(std::initializer_list<value_type> ilist);

	~radix_tree();

	template <class... Args>
	std::pair<iterator, bool> emplace(Args &&... args);

	std::pair<iterator, bool> insert(const value_type &v);
	std::pair<iterator, bool> insert(value_type &&v);
	template <typename P,
		  typename = typename std::enable_if<
			  std::is_constructible<value_type, P &&>::value,
			  P>::type>
	std::pair<iterator, bool> insert(P &&p);
	/* iterator insert(const_iterator position, const value_type &v); */
	/* iterator insert(const_iterator position, value_type &&v); */
	/* template <
		typename P,
		typename = typename std::enable_if<
			detail::has_is_transparent<BytesView>::value, P>::type>
	iterator insert(const_iterator position, P &&p); */
	template <class InputIterator>
	void insert(InputIterator first, InputIterator last);
	void insert(std::initializer_list<value_type> il);
	// insert_return_type insert(node_type&& nh);
	// iterator insert(const_iterator hint, node_type&& nh);

	template <class... Args>
	std::pair<iterator, bool> try_emplace(const key_type &k,
					      Args &&... args);
	template <class... Args>
	std::pair<iterator, bool> try_emplace(key_type &&k, Args &&... args);
	/*template <class... Args>
	iterator try_emplace(const_iterator hint, const key_type &k,
			     Args &&... args);*/
	/*template <class... Args>
	iterator try_emplace(const_iterator hint, key_type &&k,
			     Args &&... args);*/
	/* https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96976 */
	template <typename K, typename BV = BytesView, class... Args>
	auto try_emplace(K &&k, Args &&... args) -> typename std::enable_if<
		detail::has_is_transparent<BV>::value &&
			!std::is_same<typename std::remove_const<
					      typename std::remove_reference<
						      K>::type>::type,
				      key_type>::value,
		std::pair<iterator, bool>>::type;

	template <typename M>
	std::pair<iterator, bool> insert_or_assign(const key_type &k, M &&obj);
	template <typename M>
	std::pair<iterator, bool> insert_or_assign(key_type &&k, M &&obj);
	/*template <typename M>
	iterator insert_or_assign(const_iterator hint, const key_type &k,
				  M &&obj);*/
	/*template <typename M>
	iterator insert_or_assign(const_iterator hint, key_type &&k, M &&obj);*/
	template <
		typename M, typename K,
		typename = typename std::enable_if<
			detail::has_is_transparent<BytesView>::value, K>::type>
	std::pair<iterator, bool> insert_or_assign(K &&k, M &&obj);

	iterator erase(const_iterator pos);
	iterator erase(const_iterator first, const_iterator last);
	size_type erase(const key_type &k);
	template <typename K,
		  typename = typename std::enable_if<
			  detail::has_is_transparent<BytesView>::value &&
				  !std::is_same<K, iterator>::value &&
				  !std::is_same<K, const_iterator>::value,
			  K>::type>
	size_type erase(const K &k);

	void clear();

	size_type count(const key_type &k) const;
	template <
		typename K,
		typename = typename std::enable_if<
			detail::has_is_transparent<BytesView>::value, K>::type>
	size_type count(const K &k) const;

	iterator find(const key_type &k);
	const_iterator find(const key_type &k) const;
	template <
		typename K,
		typename = typename std::enable_if<
			detail::has_is_transparent<BytesView>::value, K>::type>
	iterator find(const K &k);
	template <
		typename K,
		typename = typename std::enable_if<
			detail::has_is_transparent<BytesView>::value, K>::type>
	const_iterator find(const K &k) const;

	iterator lower_bound(const key_type &k);
	const_iterator lower_bound(const key_type &k) const;
	template <
		typename K,
		typename = typename std::enable_if<
			detail::has_is_transparent<BytesView>::value, K>::type>
	iterator lower_bound(const K &k);
	template <
		typename K,
		typename = typename std::enable_if<
			detail::has_is_transparent<BytesView>::value, K>::type>
	const_iterator lower_bound(const K &k) const;

	iterator upper_bound(const key_type &k);
	const_iterator upper_bound(const key_type &k) const;
	template <
		typename K,
		typename = typename std::enable_if<
			detail::has_is_transparent<BytesView>::value, K>::type>
	iterator upper_bound(const K &k);
	template <
		typename K,
		typename = typename std::enable_if<
			detail::has_is_transparent<BytesView>::value, K>::type>
	const_iterator upper_bound(const K &k) const;

	iterator begin();
	iterator end();
	const_iterator cbegin() const;
	const_iterator cend() const;
	const_iterator begin() const;
	const_iterator end() const;

	reverse_iterator rbegin();
	reverse_iterator rend();
	const_reverse_iterator crbegin() const;
	const_reverse_iterator crend() const;
	const_reverse_iterator rbegin() const;
	const_reverse_iterator rend() const;

	/* capacity: */
	bool empty() const noexcept;
	size_type max_size() const noexcept;
	uint64_t size() const noexcept;

	void swap(radix_tree &rhs);

	template <typename K, typename V, typename BV>
	friend std::ostream &operator<<(std::ostream &os,
					const radix_tree<K, V, BV> &tree);

	void garbage_collect();

	void runtime_initialize_mt();

private:
	using byten_t = uint64_t;
	using bitn_t = uint8_t;

	/* Size of a chunk which differentiates subtrees of a node */
	static constexpr std::size_t SLICE = 4;
	/* Mask for SLICE */
	static constexpr std::size_t NIB = ((1ULL << SLICE) - 1);
	/* Number of children in internal nodes */
	static constexpr std::size_t SLNODES = (1 << SLICE);
	/* Mask for SLICE */
	static constexpr bitn_t SLICE_MASK = (bitn_t) ~(SLICE - 1);
	/* Position of the first SLICE */
	static constexpr bitn_t FIRST_NIB = 8 - SLICE;

	struct leaf;
	struct node;

	using atomic_pointer_type = std::atomic<detail::tagged_ptr<leaf, node>>;
	using pointer_type = detail::tagged_ptr<leaf, node>;

	/*** pmem members ***/
	atomic_pointer_type root;
	p<uint64_t> size_;
	v<uint64_t> mt;
	vector<pointer_type> garbage;

	/* helper functions */
	template <typename K, typename F, class... Args>
	std::pair<iterator, bool> internal_emplace(const K &, F &&);
	template <typename K>
	leaf *internal_find(const K &k) const;

	static atomic_pointer_type &parent_ref(pointer_type n);
	template <typename K1, typename K2>
	static bool keys_equal(const K1 &k1, const K2 &k2);
	template <typename K1, typename K2>
	static int compare(const K1 &k1, const K2 &k2, byten_t offset = 0);
	template <bool Direction, typename Iterator>
	static leaf *next_leaf(Iterator child, pointer_type parent);
	template <bool Direction>
	static leaf *find_leaf(pointer_type n);
	static unsigned slice_index(char k, uint8_t shift);
	template <typename K1, typename K2>
	static byten_t prefix_diff(const K1 &lhs, const K2 &rhs,
				   byten_t offset = 0);
	leaf *any_leftmost_leaf(pointer_type n, size_type min_depth) const;
	template <typename K1, typename K2>
	static bitn_t bit_diff(const K1 &leaf_key, const K2 &key, byten_t diff);
	template <typename K>
	leaf *common_prefix_leaf(const K &key) const;
	static void print_rec(std::ostream &os, radix_tree::pointer_type n);
	template <typename K>
	static BytesView bytes_view(const K &k);
	static string_view bytes_view(string_view s);
	static bool path_length_equal(size_t key_size, pointer_type n);
	template <typename K>
	std::tuple<const atomic_pointer_type *, pointer_type>
	descend(const K &k, byten_t diff, bitn_t sh) const;
	template <typename K>
	std::tuple<atomic_pointer_type *, pointer_type>
	descend(const K &k, byten_t diff, bitn_t sh);
	template <bool Lower, typename K>
	const_iterator internal_bound(const K &k) const;
	static bool is_leaf(const pointer_type &p);
	static leaf *get_leaf(const pointer_type &p);
	static node *get_node(const pointer_type &p);
	template <typename T>
	void free(persistent_ptr<T> ptr);

	void check_pmem();
	void check_tx_stage_work();

	static_assert(sizeof(node) == 256,
		      "Internal node should have size equal to 256 bytes.");
};

template <typename Key, typename Value, typename BytesView>
void swap(radix_tree<Key, Value, BytesView> &lhs,
	  radix_tree<Key, Value, BytesView> &rhs);

/**
 * This is the structure which 'holds' key/value pair. The data
 * is not stored as an object within this structure but rather
 * just after the structure (using emplace new). This is done
 * so that we can use inline_string and limit the number of allocations.
 *
 * Constructors of the leaf structure mimics those of std::pair<const Key,
 * Value>.
 */
template <typename Key, typename Value, typename BytesView>
struct radix_tree<Key, Value, BytesView>::leaf {
	using tree_type = radix_tree<Key, Value, BytesView>;

	leaf(const leaf &) = delete;
	leaf(leaf &&) = delete;

	leaf &operator=(const leaf &) = delete;
	leaf &operator=(leaf &&) = delete;

	~leaf();

	Key &key();
	Value &value();

	const Key &key() const;
	const Value &value() const;

	static persistent_ptr<leaf> make(pointer_type parent);

	template <typename... Args1, typename... Args2>
	static persistent_ptr<leaf>
	make(pointer_type parent, std::piecewise_construct_t pc,
	     std::tuple<Args1...> first_args, std::tuple<Args2...> second_args);
	template <typename K, typename V>
	static persistent_ptr<leaf> make(pointer_type parent, K &&k, V &&v);
	static persistent_ptr<leaf> make(pointer_type parent, const Key &k,
					 const Value &v);
	template <typename K, typename... Args>
	static persistent_ptr<leaf> make_key_args(pointer_type parent, K &&k,
						  Args &&... args);
	template <typename K, typename V>
	static persistent_ptr<leaf> make(pointer_type parent,
					 detail::pair<K, V> &&p);
	template <typename K, typename V>
	static persistent_ptr<leaf> make(pointer_type parent,
					 const detail::pair<K, V> &p);
	template <typename K, typename V>
	static persistent_ptr<leaf> make(pointer_type parent,
					 std::pair<K, V> &&p);
	template <typename K, typename V>
	static persistent_ptr<leaf> make(pointer_type parent,
					 const std::pair<K, V> &p);
	static persistent_ptr<leaf> make(pointer_type parent,
					 const leaf &other);

private:
	friend class radix_tree<Key, Value, BytesView>;

	leaf() = default;

	template <typename... Args1, typename... Args2, size_t... I1,
		  size_t... I2>
	static persistent_ptr<leaf>
	make(pointer_type parent, std::piecewise_construct_t,
	     std::tuple<Args1...> &first_args,
	     std::tuple<Args2...> &second_args, detail::index_sequence<I1...>,
	     detail::index_sequence<I2...>);

	atomic_pointer_type parent;
};

/**
 * This is internal node. It does not hold any values directly, but
 * can contain pointer to an embedded entry (see below).
 */
template <typename Key, typename Value, typename BytesView>
struct radix_tree<Key, Value, BytesView>::node {
	node(pointer_type parent, byten_t byte, bitn_t bit);

	/**
	 * Pointer to a parent node. Used by iterators.
	 */
	atomic_pointer_type parent;

	/**
	 * The embedded_entry ptr is used only for nodes for which length of the
	 * path from root is a multiple of byte (n->bit == FIRST_NIB). This
	 * entry holds a key which represents the entire subtree prefix (path
	 * from root).
	 */
	atomic_pointer_type embedded_entry;

	/* Children can be both leaves and internal nodes. */
	atomic_pointer_type child[SLNODES];

	/**
	 * Byte and bit together are used to calculate the NIB which is used to
	 * index the child array. The calculations are done in slice_index
	 * function.
	 *
	 * Let's say we have a key = 0xABCD
	 *
	 * For byte = 0, bit = 4 we have NIB = 0xA
	 * For byte = 0, bit = 0 we have NIB = 0xB
	 */
	byten_t byte;
	bitn_t bit;

	struct direction {
		static constexpr bool Forward = 0;
		static constexpr bool Reverse = 1;
	};

	struct forward_iterator;
	using reverse_iterator = std::reverse_iterator<forward_iterator>;

	template <bool Direction>
	using iterator =
		typename std::conditional<Direction == direction::Forward,
					  forward_iterator,
					  reverse_iterator>::type;

	template <bool Direction = direction::Forward>
	typename std::enable_if<
		Direction ==
			radix_tree<Key, Value,
				   BytesView>::node::direction::Forward,
		typename radix_tree<Key, Value,
				    BytesView>::node::forward_iterator>::type
	begin() const;

	template <bool Direction = direction::Forward>
	typename std::enable_if<
		Direction ==
			radix_tree<Key, Value,
				   BytesView>::node::direction::Forward,
		typename radix_tree<Key, Value,
				    BytesView>::node::forward_iterator>::type
	end() const;

	/* rbegin */
	template <bool Direction = direction::Forward>
	typename std::enable_if<
		Direction ==
			radix_tree<Key, Value,
				   BytesView>::node::direction::Reverse,
		typename radix_tree<Key, Value,
				    BytesView>::node::reverse_iterator>::type
	begin() const;

	/* rend */
	template <bool Direction = direction::Forward>
	typename std::enable_if<
		Direction ==
			radix_tree<Key, Value,
				   BytesView>::node::direction::Reverse,
		typename radix_tree<Key, Value,
				    BytesView>::node::reverse_iterator>::type
	end() const;

	template <bool Direction = direction::Forward, typename Ptr>
	auto find_child(const Ptr &n) const -> decltype(begin<Direction>());

	template <bool Direction = direction::Forward,
		  typename Enable = typename std::enable_if<
			  Direction == direction::Forward>::type>
	auto make_iterator(const atomic_pointer_type *ptr) const
		-> decltype(begin<Direction>());

	uint8_t padding[256 - sizeof(parent) - sizeof(leaf) - sizeof(child) -
			sizeof(byte) - sizeof(bit)];
};

/**
 * Radix tree iterator supports multipass and bidirectional iteration.
 * If Value type is inline_string, calling (*it).second = "new_value"
 * might cause reallocation and invalidate iterators to that element.
 */
template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
struct radix_tree<Key, Value, BytesView>::radix_tree_iterator {
private:
	using leaf_ptr =
		typename std::conditional<IsConst, const leaf *, leaf *>::type;
	using tree_ptr = typename std::conditional<IsConst, const radix_tree *,
						   radix_tree *>::type;
	friend struct radix_tree_iterator<true>;

public:
	using difference_type = std::ptrdiff_t;
	using value_type = radix_tree::leaf;
	using reference = typename std::conditional<IsConst, const value_type &,
						    value_type &>::type;
	using pointer = typename std::conditional<IsConst, value_type const *,
						  value_type *>::type;
	using iterator_category = std::bidirectional_iterator_tag;

	radix_tree_iterator() = default;
	radix_tree_iterator(leaf_ptr leaf_, tree_ptr tree);
	radix_tree_iterator(const radix_tree_iterator &rhs) = default;

	template <bool C = IsConst,
		  typename Enable = typename std::enable_if<C>::type>
	radix_tree_iterator(const radix_tree_iterator<false> &rhs);

	radix_tree_iterator &
	operator=(const radix_tree_iterator &rhs) = default;

	reference operator*() const;
	pointer operator->() const;

	template <typename V = Value,
		  typename Enable = typename std::enable_if<
			  detail::is_inline_string<V>::value>::type>
	void assign_val(basic_string_view<typename V::value_type,
					  typename V::traits_type>
				rhs);

	template <typename T, typename V = Value,
		  typename Enable = typename std::enable_if<
			  !detail::is_inline_string<V>::value>::type>
	void assign_val(T &&rhs);

	radix_tree_iterator &operator++();
	radix_tree_iterator &operator--();

	radix_tree_iterator operator++(int);
	radix_tree_iterator operator--(int);

	template <bool C>
	bool operator!=(const radix_tree_iterator<C> &rhs) const;

	template <bool C>
	bool operator==(const radix_tree_iterator<C> &rhs) const;

private:
	friend class radix_tree;

	leaf_ptr leaf_ = nullptr;
	tree_ptr tree = nullptr;

	template <typename T>
	void replace_val(T &&rhs);
};

template <typename Key, typename Value, typename BytesView>
struct radix_tree<Key, Value, BytesView>::node::forward_iterator {
	using difference_type = std::ptrdiff_t;
	using value_type = atomic_pointer_type;
	using pointer = const value_type *;
	using reference = const value_type &;
	using iterator_category = std::forward_iterator_tag;

	forward_iterator(pointer child, const node *node);

	forward_iterator operator++();
	forward_iterator operator++(int);

	forward_iterator operator--();

	reference operator*() const;
	pointer operator->() const;

	pointer get_node() const;

	bool operator!=(const forward_iterator &rhs) const;
	bool operator==(const forward_iterator &rhs) const;

private:
	pointer child;
	const node *n;
};

/**
 * Default radix tree constructor. Constructs an empty container.
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_scope_error if constructor wasn't called in
 * transaction.
 */
template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::radix_tree() : root(nullptr), size_(0)
{
	check_pmem();
	check_tx_stage_work();
}

/**
 * Constructs the container with the contents of the range [first,
 * last). If multiple elements in the range have keys that compare
 * equivalent, the first element is inserted.
 *
 * @param[in] first first iterator of inserted range.
 * @param[in] last last iterator of inserted range.
 *
 * InputIt must meet the requirements of LegacyInputIterator.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_scope_error if constructor wasn't called in
 * transaction.
 * @throw pmem::transaction_alloc_error when allocating memory for
 * inserted elements in transaction failed.
 * @throw rethrows element constructor exception.
 */
template <typename Key, typename Value, typename BytesView>
template <class InputIt>
radix_tree<Key, Value, BytesView>::radix_tree(InputIt first, InputIt last)
    : root(nullptr), size_(0)
{
	check_pmem();
	check_tx_stage_work();

	for (auto it = first; it != last; it++)
		emplace(*it);
}

/**
 * Copy constructor. Constructs the container with the copy of the
 * contents of other.
 *
 * @param[in] m reference to the radix_tree to be copied.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error when allocating memory in transaction
 * failed.
 * @throw pmem::transaction_scope_error if constructor wasn't called in
 * transaction.
 * @throw rethrows element constructor exception.
 */
template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::radix_tree(const radix_tree &m)
    : root(nullptr), size_(0)
{
	check_pmem();
	check_tx_stage_work();

	for (auto it = m.cbegin(); it != m.cend(); it++)
		emplace(*it);
}

/**
 * Move constructor. Constructs the container with the contents of other using
 * move semantics. After the move, other is guaranteed to be empty().
 *
 * @param[in] m rvalue reference to the radix_tree to be moved from.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_scope_error if constructor wasn't called in
 * transaction.
 */
template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::radix_tree(radix_tree &&m)
{
	check_pmem();
	check_tx_stage_work();

	root.store_with_snapshot_release(m.root.load_acquire());
	size_ = m.size_;
	m.root.store_with_snapshot_release(nullptr);
	m.size_ = 0;
}

/**
 * Constructs the container with the contents of the initializer list init.
 *
 * @param[in] il initializer list with content to be constructed.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error when allocating memory in transaction
 * failed.
 * @throw pmem::transaction_scope_error if constructor wasn't called in
 * transaction.
 * @throw rethrows element constructor exception.
 */
template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::radix_tree(
	std::initializer_list<value_type> il)
    : radix_tree(il.begin(), il.end())
{
}

/**
 * Copy assignment operator. Replaces the contents with a copy of the contents
 * of other transactionally.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw rethrows constructor's exception.
 */
template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView> &
radix_tree<Key, Value, BytesView>::operator=(const radix_tree &other)
{
	check_pmem();

	auto pop = pool_by_vptr(this);

	if (this != &other) {
		flat_transaction::run(pop, [&] {
			clear();

			this->root.store_with_snapshot_release(nullptr);
			this->size_ = 0;

			for (auto it = other.cbegin(); it != other.cend(); it++)
				emplace(*it);
		});
	}

	return *this;
}

/**
 * Move assignment operator. Replaces the contents with those of other using
 * move semantics (i.e. the data in other is moved from other into this
 * container) transactionally. Other is in a valid but empty state afterwards.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_error when snapshotting failed.
 */
template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView> &
radix_tree<Key, Value, BytesView>::operator=(radix_tree &&other)
{
	check_pmem();

	auto pop = pool_by_vptr(this);

	if (this != &other) {
		flat_transaction::run(pop, [&] {
			clear();

			this->root.store_with_snapshot_release(
				other.root.load_acquire());
			this->size_ = other.size_;
			other.root.store_with_snapshot_release(nullptr);
			other.size_ = 0;
		});
	}

	return *this;
}

/**
 * Replaces the contents with those identified by initializer list ilist
 * transactionally.
 *
 * @param[in] ilist initializer list to use as data source.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 */
template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView> &
radix_tree<Key, Value, BytesView>::operator=(
	std::initializer_list<value_type> ilist)
{
	check_pmem();

	auto pop = pool_by_vptr(this);

	transaction::run(pop, [&] {
		clear();

		this->root.store_with_snapshot_release(nullptr);
		this->size_ = 0;

		for (auto it = ilist.begin(); it != ilist.end(); it++)
			emplace(*it);
	});

	return *this;
}

/**
 * Destructor.
 */
template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::~radix_tree()
{
	try {
		clear();
		garbage_collect();
	} catch (...) {
		std::terminate();
	}
}

/**
 * Checks whether the container is empty.
 *
 * @return true if container is empty, false otherwise.
 */
template <typename Key, typename Value, typename BytesView>
bool
radix_tree<Key, Value, BytesView>::empty() const noexcept
{
	return size_ == 0;
}

/**
 * @return maximum number of elements the container is able to hold
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::size_type
radix_tree<Key, Value, BytesView>::max_size() const noexcept
{
	return std::numeric_limits<difference_type>::max();
}

/**
 * @return number of elements.
 */
template <typename Key, typename Value, typename BytesView>
uint64_t
radix_tree<Key, Value, BytesView>::size() const noexcept
{
	return this->size_;
}

/**
 * Member swap.
 *
 * Exchanges *this with @param rhs
 */
template <typename Key, typename Value, typename BytesView>
void
radix_tree<Key, Value, BytesView>::swap(radix_tree &rhs)
{
	auto pop = pool_by_vptr(this);

	flat_transaction::run(pop, [&] {
		this->size_.swap(rhs.size_);
		this->root.swap(rhs.root);
	});
}

/**
 * Transactionally collects and frees all garbage produced by erase,
 * clear, insert_or_assign or assing_val in concurrent mode (if
 * runtime_initialize_mt was called).
 *
 * Garbage is not automatically collected on move/copy ctor/assignment.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 */
template <typename Key, typename Value, typename BytesView>
void
radix_tree<Key, Value, BytesView>::garbage_collect()
{
	auto pop = pool_by_vptr(this);

	flat_transaction::run(pop, [&] {
		for (auto &e : garbage) {
			if (is_leaf(e))
				delete_persistent<radix_tree::leaf>(
					persistent_ptr<radix_tree::leaf>(
						get_leaf(e)));
			else
				delete_persistent<radix_tree::node>(
					persistent_ptr<radix_tree::node>(
						get_node(e)));
		}

		garbage.clear();
	});
}

/**
 * Enables single-writer multiple-readers concurrency with read uncommitted
 * isolation. This property is NOT persistent. Enabling must be done after
 * each application restart.
 *
 * This has the following effects:
 * - erase and clear does not free nodes/leaves immediately, instead they are
 * added to a garbage list which can be freed by calling garbage_collect()
 * - insert_or_assign and iterator.assign_val do not perform in-place update,
 *   instead a new leaf is allocated and the old one is added to the garbage
 * list
 * - memory-reclamation mechanisms are initialized
 *
 * By default, concurrency is not enabled.
 */
template <typename Key, typename Value, typename BytesView>
void
radix_tree<Key, Value, BytesView>::runtime_initialize_mt()
{
	mt.get(false) = true;
}

/*
 * Returns reference to n->parent (handles both internal and leaf nodes).
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::atomic_pointer_type &
radix_tree<Key, Value, BytesView>::parent_ref(pointer_type n)
{
	if (is_leaf(n))
		return get_leaf(n)->parent;

	return n->parent;
}

/*
 * Find a leftmost leaf in a subtree of @param n.
 *
 * @param min_depth specifies minimum depth of the leaf. If the
 * tree is shorter than min_depth, a bottom leaf is returned.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::leaf *
radix_tree<Key, Value, BytesView>::any_leftmost_leaf(
	typename radix_tree<Key, Value, BytesView>::pointer_type n,
	size_type min_depth) const
{
	assert(n);

	while (!is_leaf(n)) {
		if (n->embedded_entry.load_acquire() && n->byte >= min_depth)
			return get_leaf(n->embedded_entry.load_acquire());

		for (size_t i = 0; i < SLNODES; i++) {
			if (n->child[i].load_acquire()) {
				n = n->child[i].load_acquire();
				break;
			}
		}
	}

	return get_leaf(n);
}

/*
 * Descends to the leaf that shares a common prefix with the key.
 */
template <typename Key, typename Value, typename BytesView>
template <typename K>
typename radix_tree<Key, Value, BytesView>::leaf *
radix_tree<Key, Value, BytesView>::common_prefix_leaf(const K &key) const
{
	auto n = root.load_acquire();

	while (n && !is_leaf(n) && n->byte < key.size()) {
		auto nn = n->child[slice_index(key[n->byte], n->bit)]
				  .load_acquire();

		if (nn)
			n = nn;
		else {
			n = any_leftmost_leaf(n, key.size());
			break;
		}
	}

	/* This can happen when key is a prefix of some leaf or when the node at
	 * which the keys diverge isn't a leaf */
	if (!is_leaf(n))
		n = any_leftmost_leaf(n, key.size());

	return get_leaf(n);
}

template <typename Key, typename Value, typename BytesView>
template <typename K>
BytesView
radix_tree<Key, Value, BytesView>::bytes_view(const K &key)
{
	/* bytes_view accepts const pointer instead of reference to make sure
	 * there is no implicit conversion to a temporary type (and hence
	 * dangling references). */
	return BytesView(&key);
}

template <typename Key, typename Value, typename BytesView>
string_view
radix_tree<Key, Value, BytesView>::bytes_view(string_view key)
{
	return key;
}

/*
 * Checks for key equality.
 */
template <typename Key, typename Value, typename BytesView>
template <typename K1, typename K2>
bool
radix_tree<Key, Value, BytesView>::keys_equal(const K1 &k1, const K2 &k2)
{
	return k1.size() == k2.size() && compare(k1, k2) == 0;
}

/*
 * Checks for key equality.
 */
template <typename Key, typename Value, typename BytesView>
template <typename K1, typename K2>
int
radix_tree<Key, Value, BytesView>::compare(const K1 &k1, const K2 &k2,
					   byten_t offset)
{
	auto ret = prefix_diff(k1, k2, offset);

	if (ret != (std::min)(k1.size(), k2.size()))
		return (unsigned char)(k1[ret]) - (unsigned char)(k2[ret]);

	return k1.size() - k2.size();
}

/*
 * Returns length of common prefix of lhs and rhs.
 */
template <typename Key, typename Value, typename BytesView>
template <typename K1, typename K2>
typename radix_tree<Key, Value, BytesView>::byten_t
radix_tree<Key, Value, BytesView>::prefix_diff(const K1 &lhs, const K2 &rhs,
					       byten_t offset)
{
	byten_t diff;
	for (diff = offset; diff < (std::min)(lhs.size(), rhs.size()); diff++) {
		if (lhs[diff] != rhs[diff])
			return diff;
	}

	return diff;
}

/*
 * Checks whether length of the path from root to n is equal
 * to key_size.
 */
template <typename Key, typename Value, typename BytesView>
bool
radix_tree<Key, Value, BytesView>::path_length_equal(size_t key_size,
						     pointer_type n)
{
	return n->byte == key_size && n->bit == bitn_t(FIRST_NIB);
}

template <typename Key, typename Value, typename BytesView>
template <typename K1, typename K2>
typename radix_tree<Key, Value, BytesView>::bitn_t
radix_tree<Key, Value, BytesView>::bit_diff(const K1 &leaf_key, const K2 &key,
					    byten_t diff)
{
	auto min_key_len = (std::min)(leaf_key.size(), key.size());
	bitn_t sh = 8;

	/* If key differs from leaf_key at some point (neither is a prefix of
	 * another) we will descend to the point of divergence. Otherwise we
	 * will look for a node which represents the prefix. */
	if (diff < min_key_len) {
		auto at =
			static_cast<unsigned char>(leaf_key[diff] ^ key[diff]);
		sh = pmem::detail::mssb_index((uint32_t)at) & SLICE_MASK;
	}

	return sh;
}

template <typename Key, typename Value, typename BytesView>
template <typename K>
std::tuple<
	const typename radix_tree<Key, Value, BytesView>::atomic_pointer_type *,
	typename radix_tree<Key, Value, BytesView>::pointer_type>
radix_tree<Key, Value, BytesView>::descend(const K &key, byten_t diff,
					   bitn_t sh) const
{
	auto n = root.load_acquire();
	auto prev = n;
	auto slot = &root;

	while (n && !is_leaf(n) &&
	       (n->byte < diff || (n->byte == diff && n->bit >= sh))) {
		prev = n;
		slot = &n->child[slice_index(key[n->byte], n->bit)];
		n = slot->load_acquire();
	}

	return {slot, prev};
}

template <typename Key, typename Value, typename BytesView>
template <typename K>
std::tuple<typename radix_tree<Key, Value, BytesView>::atomic_pointer_type *,
	   typename radix_tree<Key, Value, BytesView>::pointer_type>
radix_tree<Key, Value, BytesView>::descend(const K &key, byten_t diff,
					   bitn_t sh)
{
	const atomic_pointer_type *slot;
	pointer_type prev;

	std::tie(slot, prev) =
		const_cast<const radix_tree *>(this)->descend(key, diff, sh);

	return {const_cast<atomic_pointer_type *>(slot), prev};
}

template <typename Key, typename Value, typename BytesView>
template <typename K, typename F, class... Args>
std::pair<typename radix_tree<Key, Value, BytesView>::iterator, bool>
radix_tree<Key, Value, BytesView>::internal_emplace(const K &k, F &&make_leaf)
{
	auto key = bytes_view(k);
	auto pop = pool_base(pmemobj_pool_by_ptr(this));

	if (!root.load_acquire()) {
		flat_transaction::run(pop, [&] {
			this->root.store_with_snapshot_release(
				make_leaf(nullptr));
		});
		return {iterator(get_leaf(root.load_acquire()), this), true};
	}

	/*
	 * Need to descend the tree twice. First to find a leaf that
	 * represents a subtree that shares a common prefix with the key.
	 * This is needed to find out the actual labels between nodes (they
	 * are not known due to a possible path compression). Second time to
	 * find the place for the new element.
	 */
	auto leaf = common_prefix_leaf(key);
	auto leaf_key = bytes_view(leaf->key());
	auto diff = prefix_diff(key, leaf_key);
	auto sh = bit_diff(leaf_key, key, diff);

	/* Key exists. */
	if (diff == key.size() && leaf_key.size() == key.size())
		return {iterator(leaf, this), false};

	/* Descend into the tree again. */
	atomic_pointer_type *slot;
	pointer_type prev;
	std::tie(slot, prev) = descend(key, diff, sh);

	auto n = slot->load_acquire();

	/*
	 * If the divergence point is at same nib as an existing node, and
	 * the subtree there is empty, just place our leaf there and we're
	 * done.  Obviously this can't happen if SLICE == 1.
	 */
	if (!n) {
		assert(diff < (std::min)(leaf_key.size(), key.size()));

		flat_transaction::run(pop, [&] {
			slot->store_with_snapshot_release(make_leaf(prev));
		});
		return {iterator(get_leaf(slot->load_acquire()), this), true};
	}

	/* New key is a prefix of the leaf key or they are equal. We need to add
	 * leaf ptr to internal node. */
	if (diff == key.size()) {
		if (!is_leaf(n) && path_length_equal(key.size(), n)) {
			assert(!n->embedded_entry.load_acquire());

			flat_transaction::run(pop, [&] {
				n->embedded_entry.store_with_snapshot_release(
					make_leaf(n));
			});

			return {iterator(get_leaf(n->embedded_entry
							  .load_acquire()),
					 this),
				true};
		}

		/* Path length from root to n is longer than key.size().
		 * We have to allocate new internal node above n. */
		pointer_type node;
		flat_transaction::run(pop, [&] {
			node = make_persistent<radix_tree::node>(
				parent_ref(n).load_acquire(), diff,
				bitn_t(FIRST_NIB));
			node->embedded_entry.store_with_snapshot_release(
				make_leaf(node));
			node->child[slice_index(leaf_key[diff],
						bitn_t(FIRST_NIB))]
				.store_with_snapshot_release(n);

			parent_ref(n).store_with_snapshot_release(node);
			slot->store_with_snapshot_release(node);
		});

		return {iterator(get_leaf(node->embedded_entry.load_acquire()),
				 this),
			true};
	}

	if (diff == leaf_key.size()) {
		/* Leaf key is a prefix of the new key. We need to convert leaf
		 * to a node. */
		pointer_type node;
		flat_transaction::run(pop, [&] {
			/* We have to add new node at the edge from parent to n
			 */
			node = make_persistent<radix_tree::node>(
				parent_ref(n).load_acquire(), diff,
				bitn_t(FIRST_NIB));
			node->embedded_entry.store_with_snapshot_release(n);
			node->child[slice_index(key[diff], bitn_t(FIRST_NIB))]
				.store_with_snapshot_release(make_leaf(node));

			parent_ref(n).store_with_snapshot_release(node);
			slot->store_with_snapshot_release(node);
		});

		return {iterator(
				get_leaf(node->child[slice_index(
							     key[diff],
							     bitn_t(FIRST_NIB))]
						 .load_acquire()),
				this),
			true};
	}

	/* There is already a subtree at the divergence point
	 * (slice_index(key[diff], sh)). This means that a tree is vertically
	 * compressed and we have to "break" this compression and add a new
	 * node. */
	pointer_type node;
	flat_transaction::run(pop, [&] {
		node = make_persistent<radix_tree::node>(
			parent_ref(n).load_acquire(), diff, sh);
		node->child[slice_index(leaf_key[diff], sh)]
			.store_with_snapshot_release(n);
		node->child[slice_index(key[diff], sh)]
			.store_with_snapshot_release(make_leaf(node));

		parent_ref(n).store_with_snapshot_release(node);
		slot->store_with_snapshot_release(node);
	});

	return {iterator(get_leaf(node->child[slice_index(key[diff], sh)]
					  .load_acquire()),
			 this),
		true};
}

/**
 * If a key equivalent to k already exists in the container, does
 * nothing. Otherwise, behaves like emplace except that the element is
 * constructed as value_type(std::piecewise_construct,
 * std::forward_as_tuple(k),
 * std::forward_as_tuple(std::forward<Args>(args)...))
 *
 * Unlike insert or emplace, this method do not move from rvalue arguments
 * if the insertion does not happen, which makes it easy to manipulate maps
 * whose values are move-only types. In addition, try_emplace treats the key and
 * the arguments to the mapped_type separately, unlike emplace, which requires
 * the arguments to construct a value_type (that is, a std::pair).
 *
 * No iterators or references are invalidated.
 *
 * @param[in] k the key used both to look up and to insert if not found.
 * @param[in] args arguments to forward to the constructor of the
 * element.
 *
 * @return a pair consisting of an iterator to the inserted element (or
 * to the element that prevented the insertion) and a bool denoting
 * whether the insertion took place.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename Key, typename Value, typename BytesView>
template <class... Args>
std::pair<typename radix_tree<Key, Value, BytesView>::iterator, bool>
radix_tree<Key, Value, BytesView>::try_emplace(const key_type &k,
					       Args &&... args)
{
	return internal_emplace(k, [&](pointer_type parent) {
		size_++;
		return leaf::make_key_args(parent, k,
					   std::forward<Args>(args)...);
	});
}

/**
 * Inserts a new element into the container constructed in-place with
 * the given args if there is no element with the key in the container.
 *
 * Careful use of emplace allows the new element to be constructed while
 * avoiding unnecessary copy or move operations. The constructor of the
 * new element (i.e. std::pair<const Key, T>) is called with exactly the
 * same arguments as supplied to emplace, forwarded via
 * std::forward<Args>(args).... The element may be constructed even if
 * there already is an element with the key in the container, in which
 * case the newly constructed element will be destroyed immediately.
 *
 * No iterators or references are invalidated.
 *
 * @param[in] args arguments to forward to the constructor of the
 * element
 *
 * @return a pair consisting of an iterator to the inserted element (or
 * to the element that prevented the insertion) and a bool denoting
 * whether the insertion took place.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename Key, typename Value, typename BytesView>
template <class... Args>
std::pair<typename radix_tree<Key, Value, BytesView>::iterator, bool>
radix_tree<Key, Value, BytesView>::emplace(Args &&... args)
{
	auto pop = pool_base(pmemobj_pool_by_ptr(this));
	std::pair<iterator, bool> ret;

	flat_transaction::run(pop, [&] {
		auto leaf_ = leaf::make(nullptr, std::forward<Args>(args)...);
		auto make_leaf = [&](pointer_type parent) {
			leaf_->parent.store_with_snapshot_release(parent);
			size_++;
			return leaf_;
		};

		ret = internal_emplace(leaf_->key(), make_leaf);

		if (!ret.second)
			delete_persistent<leaf>(leaf_);
	});

	return ret;
}

/**
 * Inserts element if the tree doesn't already contain an element with an
 * equivalent key.
 *
 * @param[in] v element value to insert.
 *
 * @return a pair consisting of an iterator to the inserted element (or
 * to the element that prevented the insertion) and a bool denoting
 * whether the insertion took place.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename Key, typename Value, typename BytesView>
std::pair<typename radix_tree<Key, Value, BytesView>::iterator, bool>
radix_tree<Key, Value, BytesView>::insert(const value_type &v)
{
	return try_emplace(v.first, v.second);
}

/**
 * Inserts element using move semantic if the tree doesn't already contain an
 * element with an equivalent key.
 *
 * @param[in] v element value to insert.
 *
 * @return a pair consisting of an iterator to the inserted element (or
 * to the element that prevented the insertion) and a bool denoting
 * whether the insertion took place.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename Key, typename Value, typename BytesView>
std::pair<typename radix_tree<Key, Value, BytesView>::iterator, bool>
radix_tree<Key, Value, BytesView>::insert(value_type &&v)
{
	return try_emplace(std::move(v.first), std::move(v.second));
}

/**
 * Inserts element if the tree doesn't already contain an element with an
 * equivalent key.
 *
 * This overload is equivalent to emplace(std::forward<P>(value)) and only
 * participates in overload resolution
 * if std::is_constructible<value_type, P&&>::value == true.
 *
 * @param[in] p element value to insert.
 *
 * @return a pair consisting of an iterator to the inserted element (or
 * to the element that prevented the insertion) and a bool denoting
 * whether the insertion took place.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename Key, typename Value, typename BytesView>
template <typename P, typename>
std::pair<typename radix_tree<Key, Value, BytesView>::iterator, bool>
radix_tree<Key, Value, BytesView>::insert(P &&p)
{
	return emplace(std::forward<P>(p));
}

/**
 * Inserts elements from range [first, last).
 *
 * @param[in] first first iterator of inserted range.
 * @param[in] last last iterator of inserted range.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename Key, typename Value, typename BytesView>
template <typename InputIterator>
void
radix_tree<Key, Value, BytesView>::insert(InputIterator first,
					  InputIterator last)
{
	for (auto it = first; it != last; it++)
		try_emplace((*it).first, (*it).second);
}

/**
 * Inserts elements from initializer list il.
 *
 * @param[in] il initializer list to insert the values from.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename Key, typename Value, typename BytesView>
void
radix_tree<Key, Value, BytesView>::insert(std::initializer_list<value_type> il)
{
	insert(il.begin(), il.end());
}

/**
 * If a key equivalent to k already exists in the container, does nothing.
 * Otherwise, behaves like emplace except that the element is constructed
 * as value_type(std::piecewise_construct, std::forward_as_tuple(std::move(k)),
 * std::forward_as_tuple(std::forward<Args>(args)...)).
 *
 * Unlike insert or emplace, this method do not move from rvalue arguments
 * if the insertion does not happen, which makes it easy to manipulate maps
 * whose values are move-only types. In addition, try_emplace treats the key and
 * the arguments to the mapped_type separately, unlike emplace, which requires
 * the arguments to construct a value_type (that is, a std::pair).
 *
 * @param[in] k the key used both to look up and to insert if not found.
 * @param[in] args arguments to forward to the constructor of the element.
 *
 * @return a pair consisting of an iterator to the inserted element (or
 * to the element that prevented the insertion) and a bool denoting
 * whether the insertion took place.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename Key, typename Value, typename BytesView>
template <class... Args>
std::pair<typename radix_tree<Key, Value, BytesView>::iterator, bool>
radix_tree<Key, Value, BytesView>::try_emplace(key_type &&k, Args &&... args)
{
	return internal_emplace(k, [&](pointer_type parent) {
		size_++;
		return leaf::make_key_args(parent, std::move(k),
					   std::forward<Args>(args)...);
	});
}

/**
 * If a key equivalent to k already exists in the container, does nothing.
 * Otherwise, behaves like emplace except that the element is constructed
 * as value_type(std::piecewise_construct,
 * std::forward_as_tuple(std::forward<K>(k)),
 * std::forward_as_tuple(std::forward<Args>(args)...)).
 *
 * Unlike insert or emplace, this method do not move from rvalue arguments
 * if the insertion does not happen, which makes it easy to manipulate maps
 * whose values are move-only types. In addition, try_emplace treats the key and
 * the arguments to the mapped_type separately, unlike emplace, which requires
 * the arguments to construct a value_type (that is, a std::pair).
 *
 * This overload only participates in overload resolution if BytesView struct
 * has a type member named is_transparent.
 *
 * @param[in] k the key used both to look up and to insert if not found.
 * @param[in] args arguments to forward to the constructor of the element.
 *
 * @return a pair consisting of an iterator to the inserted element (or
 * to the element that prevented the insertion) and a bool denoting
 * whether the insertion took place.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename Key, typename Value, typename BytesView>
template <typename K, typename BV, class... Args>
auto
radix_tree<Key, Value, BytesView>::try_emplace(K &&k, Args &&... args) ->
	typename std::enable_if<
		detail::has_is_transparent<BV>::value &&
			!std::is_same<typename std::remove_const<
					      typename std::remove_reference<
						      K>::type>::type,
				      key_type>::value,
		std::pair<typename radix_tree<Key, Value, BytesView>::iterator,
			  bool>>::type

{
	return internal_emplace(k, [&](pointer_type parent) {
		size_++;
		return leaf::make_key_args(parent, std::forward<K>(k),
					   std::forward<Args>(args)...);
	});
}

/**
 * If a key equivalent to k already exists in the container, assigns
 * std::forward<M>(obj) to the mapped_type corresponding to the key k. If the
 * key does not exist, inserts the new value as if by insert, constructing it
 * from value_type(k, std::forward<M>(obj)).
 *
 * @param[in] k the key used both to look up and to insert if not found.
 * @param[in] obj the value to insert or assign.
 *
 * @return std::pair<iterator,bool> The bool component is true if the insertion
 * took place and false if the assignment took place. The iterator component is
 * pointing at the element that was inserted or updated.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename Key, typename Value, typename BytesView>
template <typename M>
std::pair<typename radix_tree<Key, Value, BytesView>::iterator, bool>
radix_tree<Key, Value, BytesView>::insert_or_assign(const key_type &k, M &&obj)
{
	auto ret = try_emplace(k, std::forward<M>(obj));
	if (!ret.second)
		ret.first.assign_val(std::forward<M>(obj));
	return ret;
}

/**
 * If a key equivalent to k already exists in the container, assigns
 * std::forward<M>(obj) to the mapped_type corresponding to the key k. If the
 * key does not exist, inserts the new value as if by insert, constructing it
 * from value_type(std::move(k), std::forward<M>(obj))
 *
 * @param[in] k the key used both to look up and to insert if not found
 * @param[in] obj the value to insert or assign
 *
 * @return std::pair<iterator,bool> The bool component is true if the insertion
 * took place and false if the assignment took place. The iterator component is
 * pointing at the element that was inserted or updated.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename Key, typename Value, typename BytesView>
template <typename M>
std::pair<typename radix_tree<Key, Value, BytesView>::iterator, bool>
radix_tree<Key, Value, BytesView>::insert_or_assign(key_type &&k, M &&obj)
{
	auto ret = try_emplace(std::move(k), std::forward<M>(obj));
	if (!ret.second)
		ret.first.assign_val(std::forward<M>(obj));
	return ret;
}

/**
 * If a key equivalent to k already exists in the container, assigns
 * std::forward<M>(obj) to the mapped_type corresponding to the key k. If the
 * key does not exist, inserts the new value as if by insert, constructing it
 * from value_type(std::forward<K>(k), std::forward<M>(obj)).
 *
 * This overload only participates in overload resolution if BytesView struct
 * has a type member named is_transparent.
 *
 * @param[in] k the key used both to look up and to insert if not found.
 * @param[in] obj the value to insert or assign.
 *
 * @return std::pair<iterator,bool> The bool component is true if the insertion
 * took place and false if the assignment took place. The iterator component is
 * pointing at the element that was inserted or updated.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename Key, typename Value, typename BytesView>
template <typename M, typename K, typename>
std::pair<typename radix_tree<Key, Value, BytesView>::iterator, bool>
radix_tree<Key, Value, BytesView>::insert_or_assign(K &&k, M &&obj)
{
	auto ret = try_emplace(std::forward<K>(k), std::forward<M>(obj));
	if (!ret.second)
		ret.first.assign_val(std::forward<M>(obj));
	return ret;
}

/**
 * Returns the number of elements with key that compares equivalent to
 * the specified argument.
 *
 * @param[in] k key value of the element to count.
 *
 * @return Number of elements with key that compares equivalent to the
 * specified argument.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::size_type
radix_tree<Key, Value, BytesView>::count(const key_type &k) const
{
	return internal_find(k) != nullptr ? 1 : 0;
}

/**
 * Returns the number of elements with key that compares equivalent to
 * the specified argument.
 *
 * This overload only participates in overload resolution if BytesView struct
 * has a type member named is_transparent.
 *
 * @param[in] k key value of the element to count.
 *
 * @return Number of elements with key that compares equivalent to the
 * specified argument.
 */
template <typename Key, typename Value, typename BytesView>
template <typename K, typename>
typename radix_tree<Key, Value, BytesView>::size_type
radix_tree<Key, Value, BytesView>::count(const K &k) const
{
	return internal_find(k) != nullptr ? 1 : 0;
}

/**
 * Finds an element with key equivalent to key.
 *
 * @param[in] k key value of the element to search for.
 *
 * @return Iterator to an element with key equivalent to key. If no such
 * element is found, past-the-end iterator is returned.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::iterator
radix_tree<Key, Value, BytesView>::find(const key_type &k)
{
	return iterator(internal_find(k), this);
}

/**
 * Finds an element with key equivalent to key.
 *
 * @param[in] k key value of the element to search for.
 *
 * @return Const iterator to an element with key equivalent to key. If no such
 * element is found, past-the-end iterator is returned.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_iterator
radix_tree<Key, Value, BytesView>::find(const key_type &k) const
{
	return const_iterator(internal_find(k), this);
}

/**
 * Finds an element with key equivalent to key.
 *
 * This overload only participates in overload resolution if BytesView struct
 * has a type member named is_transparent.
 *
 * @param[in] k key value of the element to search for.
 *
 * @return Iterator to an element with key equivalent to key. If no such
 * element is found, past-the-end iterator is returned.
 */
template <typename Key, typename Value, typename BytesView>
template <typename K, typename>
typename radix_tree<Key, Value, BytesView>::iterator
radix_tree<Key, Value, BytesView>::find(const K &k)
{
	return iterator(internal_find(k), this);
}

/**
 * Finds an element with key equivalent to key.
 *
 * This overload only participates in overload resolution if BytesView struct
 * has a type member named is_transparent.
 *
 * @param[in] k key value of the element to search for.
 *
 * @return Const iterator to an element with key equivalent to key. If no such
 * element is found, past-the-end iterator is returned.
 */
template <typename Key, typename Value, typename BytesView>
template <typename K, typename>
typename radix_tree<Key, Value, BytesView>::const_iterator
radix_tree<Key, Value, BytesView>::find(const K &k) const
{
	return const_iterator(internal_find(k), this);
}

template <typename Key, typename Value, typename BytesView>
template <typename K>
typename radix_tree<Key, Value, BytesView>::leaf *
radix_tree<Key, Value, BytesView>::internal_find(const K &k) const
{
	auto key = bytes_view(k);

	auto n = root.load_acquire();
	while (n && !is_leaf(n)) {
		if (path_length_equal(key.size(), n))
			n = n->embedded_entry.load_acquire();
		else if (n->byte >= key.size())
			return nullptr;
		else
			n = n->child[slice_index(key[n->byte], n->bit)]
				    .load_acquire();
	}

	if (!n)
		return nullptr;

	if (!keys_equal(key, bytes_view(get_leaf(n)->key())))
		return nullptr;

	return get_leaf(n);
}

/**
 * Erases all elements from the container transactionally.
 *
 * @post size() == 0
 *
 * @throw pmem::transaction_error when snapshotting failed.
 */
template <typename Key, typename Value, typename BytesView>
void
radix_tree<Key, Value, BytesView>::clear()
{
	if (size() != 0)
		erase(begin(), end());
}

/**
 * Removes the element at pos from the container.
 * References and iterators to the erased elements are invalidated.
 * Other references and iterators are not affected.
 *
 * @pre The iterator pos must be valid and dereferenceable. Thus the
 * end() iterator (which is valid, but is not dereferenceable) cannot be
 * used as a value for pos.
 *
 * @param[in] pos iterator to the element to remove.
 *
 * @return iterator following the removed element.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::iterator
radix_tree<Key, Value, BytesView>::erase(const_iterator pos)
{
	auto pop = pool_base(pmemobj_pool_by_ptr(this));

	flat_transaction::run(pop, [&] {
		auto *leaf = pos.leaf_;
		auto parent = leaf->parent.load_acquire();

		/* there are more elements in the container */
		if (parent)
			++pos;

		free(persistent_ptr<radix_tree::leaf>(leaf));

		size_--;

		/* was root */
		if (!parent) {
			this->root.store_with_snapshot_release(nullptr);
			pos = begin();
			return;
		}

		/* It's safe to cast because we're inside non-const method. */
		const_cast<atomic_pointer_type &>(*parent->find_child(leaf))
			.store_with_snapshot_release(nullptr);

		/* Compress the tree vertically. */
		auto n = parent;
		parent = n->parent.load_acquire();
		pointer_type only_child = nullptr;
		for (size_t i = 0; i < SLNODES; i++) {
			if (n->child[i].load_acquire()) {
				if (only_child) {
					/* more than one child */
					return;
				}
				only_child = n->child[i].load_acquire();
			}
		}

		if (only_child && n->embedded_entry.load_acquire()) {
			/* There are actually 2 "children" so we can't compress.
			 */
			return;
		} else if (n->embedded_entry.load_acquire()) {
			only_child = n->embedded_entry.load_acquire();
		}

		assert(only_child);
		parent_ref(only_child)
			.store_with_snapshot_release(n->parent.load_acquire());

		auto *child_slot = parent ? const_cast<atomic_pointer_type *>(
						    &*parent->find_child(n))
					  : &root;
		child_slot->store_with_snapshot_release(only_child);

		free(persistent_ptr<radix_tree::node>(get_node(n)));
	});

	return iterator(const_cast<typename iterator::leaf_ptr>(pos.leaf_),
			this);
}

/**
 * Removes the elements in the range [first; last), which must be a
 * valid range in *this.
 * References and iterators to the erased elements are invalidated.
 * Other references and iterators are not affected.
 *
 * @param[in] first first iterator in the range of elements to remove.
 * @param[in] last last iterator in the range of elements to remove.
 *
 * @return iterator following the last removed element.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::iterator
radix_tree<Key, Value, BytesView>::erase(const_iterator first,
					 const_iterator last)
{
	auto pop = pool_base(pmemobj_pool_by_ptr(this));

	flat_transaction::run(pop, [&] {
		while (first != last)
			first = erase(first);
	});

	return iterator(const_cast<typename iterator::leaf_ptr>(first.leaf_),
			this);
}

/**
 * Removes the element (if one exists) with the key equivalent to key.
 * References and iterators to the erased elements are invalidated.
 * Other references and iterators are not affected.
 *
 * @param[in] k key value of the elements to remove.
 *
 * @return Number of elements removed.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows destructor exception.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::size_type
radix_tree<Key, Value, BytesView>::erase(const key_type &k)
{
	auto it = const_iterator(internal_find(k), this);

	if (it == end())
		return 0;

	erase(it);

	return 1;
}

/**
 * Removes the element (if one exists) with the key equivalent to key.
 * References and iterators to the erased elements are invalidated.
 * Other references and iterators are not affected.
 *
 * This overload only participates in overload resolution if BytesView struct
 * has a type member named is_transparent.
 *
 * @param[in] k key value of the elements to remove
 * @return Number of elements removed.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows destructor exception.
 */
template <typename Key, typename Value, typename BytesView>
template <typename K, typename>
typename radix_tree<Key, Value, BytesView>::size_type
radix_tree<Key, Value, BytesView>::erase(const K &k)
{
	auto it = const_iterator(internal_find(k), this);

	if (it == end())
		return 0;

	erase(it);

	return 1;
}

/**
 * Deletes node/leaf pointed by ptr. If concurrent mode is used, adds element
 * to the garbage list. Otherwise, frees the element immediately.
 */
template <typename Key, typename Value, typename BytesView>
template <typename T>
void
radix_tree<Key, Value, BytesView>::free(persistent_ptr<T> ptr)
{
	if (mt.get(false))
		garbage.emplace_back(ptr);
	else
		delete_persistent<T>(ptr);
}

template <typename Key, typename Value, typename BytesView>
template <bool Lower, typename K>
typename radix_tree<Key, Value, BytesView>::const_iterator
radix_tree<Key, Value, BytesView>::internal_bound(const K &k) const
{
	auto key = bytes_view(k);
	auto pop = pool_base(pmemobj_pool_by_ptr(this));

	if (!root.load_acquire())
		return end();

	/*
	 * Need to descend the tree twice. First to find a leaf that
	 * represents a subtree that shares a common prefix with the key.
	 * This is needed to find out the actual labels between nodes (they
	 * are not known due to a possible path compression). Second time to
	 * get the actual element.
	 */
	auto leaf = common_prefix_leaf(key);
	auto leaf_key = bytes_view(leaf->key());
	auto diff = prefix_diff(key, leaf_key);
	auto sh = bit_diff(leaf_key, key, diff);

	/* Key exists. */
	if (diff == key.size() && leaf_key.size() == key.size()) {
		if (Lower)
			return const_iterator(leaf, this);
		else
			return ++const_iterator(leaf, this);
	}

	/* Descend into the tree again. */
	const atomic_pointer_type *slot;
	pointer_type prev;
	std::tie(slot, prev) = descend(key, diff, sh);

	if (!slot->load_acquire()) {
		leaf = next_leaf<node::direction::Forward>(
			prev->template make_iterator<node::direction::Forward>(
				slot),
			prev);

		return const_iterator(leaf, this);
	}

	/* The looked-for key is a prefix of the leaf key. The target node must
	 * be the smallest leaf within *slot subtree. Key represented by a path
	 * from root to n is larger than the looked-for key. Additionally keys
	 * under right siblings of *slot are > key and keys under left siblings
	 * are < key. */
	if (diff == key.size()) {
		leaf = find_leaf<node::direction::Forward>(
			slot->load_acquire());
		return const_iterator(leaf, this);
	}

	/* Leaf's key is a prefix of the looked-for key. Leaf's key is the
	 * biggest key less than the looked-for key.
	 * The target node must be the next leaf. */
	if (diff == leaf_key.size())
		return ++const_iterator(leaf, this);

	/* *slot is the point of divergence. */
	assert(diff < leaf_key.size() && diff < key.size());

	/* The target node must be within *slot subtree. The left siblings
	 * of *slot are all less than the looked-for key. */
	if (compare(key, leaf_key, diff) < 0) {
		leaf = find_leaf<node::direction::Forward>(
			slot->load_acquire());
		return const_iterator(leaf, this);
	}

	if (slot == &root) {
		return const_iterator(nullptr, this);
	}

	/* Since looked-for key is larger than *slot, the target node must be
	 * within subtree of a right sibling of *slot. */
	leaf = next_leaf<node::direction::Forward>(
		prev->template make_iterator<node::direction::Forward>(slot),
		prev);

	return const_iterator(leaf, this);
}

/**
 * Returns an iterator pointing to the first element that is not less
 * than (i.e. greater or equal to) key.
 *
 * @param[in] k key value to compare the elements to.
 *
 * @return Const iterator pointing to the first element that is not less than
 * key. If no such element is found, a past-the-end iterator is
 * returned.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_iterator
radix_tree<Key, Value, BytesView>::lower_bound(const key_type &k) const
{
	return internal_bound<true>(k);
}

/**
 * Returns an iterator pointing to the first element that is not less
 * than (i.e. greater or equal to) key.
 *
 * @param[in] k key value to compare the elements to.
 *
 * @return Iterator pointing to the first element that is not less than
 * key. If no such element is found, a past-the-end iterator is
 * returned.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::iterator
radix_tree<Key, Value, BytesView>::lower_bound(const key_type &k)
{
	auto it = const_cast<const radix_tree *>(this)->lower_bound(k);
	return iterator(const_cast<typename iterator::leaf_ptr>(it.leaf_),
			this);
}

/**
 * Returns an iterator pointing to the first element that is not less
 * than (i.e. greater or equal to) key.
 *
 * This overload only participates in overload resolution if BytesView struct
 * has a type member named is_transparent.
 *
 * @param[in] k key value to compare the elements to.
 *
 * @return Iterator pointing to the first element that is not less than
 * key. If no such element is found, a past-the-end iterator is
 * returned.
 */
template <typename Key, typename Value, typename BytesView>
template <typename K, typename>
typename radix_tree<Key, Value, BytesView>::iterator
radix_tree<Key, Value, BytesView>::lower_bound(const K &k)
{
	auto it = const_cast<const radix_tree *>(this)->lower_bound(k);
	return iterator(const_cast<typename iterator::leaf_ptr>(it.leaf_),
			this);
}

/**
 * Returns an iterator pointing to the first element that is not less
 * than (i.e. greater or equal to) key.
 *
 * This overload only participates in overload resolution if BytesView struct
 * has a type member named is_transparent.
 *
 * @param[in] k key value to compare the elements to.
 *
 * @return Const iterator pointing to the first element that is not less than
 * key. If no such element is found, a past-the-end iterator is
 * returned.
 */
template <typename Key, typename Value, typename BytesView>
template <typename K, typename>
typename radix_tree<Key, Value, BytesView>::const_iterator
radix_tree<Key, Value, BytesView>::lower_bound(const K &k) const
{
	return internal_bound<true>(k);
}

/**
 * Returns an iterator pointing to the first element that is greater
 * than key.
 *
 * @param[in] k key value to compare the elements to.
 *
 * @return Const iterator pointing to the first element that is greater than
 * key. If no such element is found, a past-the-end iterator is
 * returned.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_iterator
radix_tree<Key, Value, BytesView>::upper_bound(const key_type &k) const
{
	return internal_bound<false>(k);
}

/**
 * Returns an iterator pointing to the first element that is greater
 * than key.
 *
 * @param[in] k key value to compare the elements to.
 *
 * @return Iterator pointing to the first element that is greater than
 * key. If no such element is found, a past-the-end iterator is
 * returned.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::iterator
radix_tree<Key, Value, BytesView>::upper_bound(const key_type &k)
{
	auto it = const_cast<const radix_tree *>(this)->upper_bound(k);
	return iterator(const_cast<typename iterator::leaf_ptr>(it.leaf_),
			this);
}

/**
 * Returns an iterator pointing to the first element that is greater
 * than key.
 *
 * This overload only participates in overload resolution if BytesView struct
 * has a type member named is_transparent.
 *
 * @param[in] k key value to compare the elements to.
 *
 * @return Iterator pointing to the first element that is greater than
 * key. If no such element is found, a past-the-end iterator is
 * returned.
 */
template <typename Key, typename Value, typename BytesView>
template <typename K, typename>
typename radix_tree<Key, Value, BytesView>::iterator
radix_tree<Key, Value, BytesView>::upper_bound(const K &k)
{
	auto it = const_cast<const radix_tree *>(this)->upper_bound(k);
	return iterator(const_cast<typename iterator::leaf_ptr>(it.leaf_),
			this);
}

/**
 * Returns an iterator pointing to the first element that is greater
 * than key.
 *
 * This overload only participates in overload resolution if BytesView struct
 * has a type member named is_transparent.
 *
 * @param[in] k key value to compare the elements to.
 *
 * @return Const iterator pointing to the first element that is greater than
 * key. If no such element is found, a past-the-end iterator is
 * returned.
 */
template <typename Key, typename Value, typename BytesView>
template <typename K, typename>
typename radix_tree<Key, Value, BytesView>::const_iterator
radix_tree<Key, Value, BytesView>::upper_bound(const K &k) const
{
	return internal_bound<false>(k);
}

/**
 * Returns an iterator to the first element of the container.
 * If the map is empty, the returned iterator will be equal to end().
 *
 * @return Iterator to the first element.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::iterator
radix_tree<Key, Value, BytesView>::begin()
{
	auto const_begin = const_cast<const radix_tree *>(this)->begin();
	return iterator(
		const_cast<typename iterator::leaf_ptr>(const_begin.leaf_),
		this);
}

/**
 * Returns an iterator to the element following the last element of the
 * map. This element acts as a placeholder; attempting to access it
 * results in undefined behavior.
 *
 * @return Iterator to the element following the last element.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::iterator
radix_tree<Key, Value, BytesView>::end()
{
	auto const_end = const_cast<const radix_tree *>(this)->end();
	return iterator(
		const_cast<typename iterator::leaf_ptr>(const_end.leaf_), this);
}

/**
 * Returns a const iterator to the first element of the container.
 * If the map is empty, the returned iterator will be equal to end().
 *
 * @return const iterator to the first element.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_iterator
radix_tree<Key, Value, BytesView>::cbegin() const
{
	if (!root.load_acquire())
		return const_iterator(nullptr, this);

	return const_iterator(
		radix_tree::find_leaf<radix_tree::node::direction::Forward>(
			root.load_acquire()),
		this);
}

/**
 * Returns a const iterator to the element following the last element of the
 * map. This element acts as a placeholder; attempting to access it
 * results in undefined behavior.
 *
 * @return const iterator to the element following the last element.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_iterator
radix_tree<Key, Value, BytesView>::cend() const
{
	return const_iterator(nullptr, this);
}

/**
 * Returns a const iterator to the first element of the container.
 * If the map is empty, the returned iterator will be equal to end().
 *
 * @return const iterator to the first element.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_iterator
radix_tree<Key, Value, BytesView>::begin() const
{
	return cbegin();
}

/**
 * Returns a const iterator to the element following the last element of the
 * map. This element acts as a placeholder; attempting to access it
 * results in undefined behavior.
 *
 * @return const iterator to the element following the last element.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_iterator
radix_tree<Key, Value, BytesView>::end() const
{
	return cend();
}

/**
 * Returns a reverse iterator to the beginning.
 *
 * @return reverse_iterator pointing to the last element in the vector.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::reverse_iterator
radix_tree<Key, Value, BytesView>::rbegin()
{
	return reverse_iterator(end());
}

/**
 * Returns a reverse iterator to the end.
 *
 * @return reverse_iterator pointing to the theoretical element preceding the
 * first element in the vector.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::reverse_iterator
radix_tree<Key, Value, BytesView>::rend()
{
	return reverse_iterator(begin());
}

/**
 * Returns a const, reverse iterator to the beginning.
 *
 * @return const_reverse_iterator pointing to the last element in the vector.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_reverse_iterator
radix_tree<Key, Value, BytesView>::crbegin() const
{
	return const_reverse_iterator(cend());
}

/**
 * Returns a const, reverse iterator to the end.
 *
 * @return const_reverse_iterator pointing to the theoretical element preceding
 * the first element in the vector.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_reverse_iterator
radix_tree<Key, Value, BytesView>::crend() const
{
	return const_reverse_iterator(cbegin());
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_reverse_iterator
radix_tree<Key, Value, BytesView>::rbegin() const
{
	return const_reverse_iterator(cend());
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_reverse_iterator
radix_tree<Key, Value, BytesView>::rend() const
{
	return const_reverse_iterator(cbegin());
}

template <typename Key, typename Value, typename BytesView>
void
radix_tree<Key, Value, BytesView>::print_rec(std::ostream &os,
					     radix_tree::pointer_type n)
{
	if (!is_leaf(n)) {
		os << "\"" << get_node(n) << "\""
		   << " [style=filled,color=\"blue\"]" << std::endl;
		os << "\"" << get_node(n) << "\" [label=\"byte:" << n->byte
		   << ", bit:" << int(n->bit) << "\"]" << std::endl;

		auto parent = n->parent.load_acquire()
			? get_node(n->parent.load_acquire())
			: 0;
		os << "\"" << get_node(n) << "\" -> "
		   << "\"" << parent << "\" [label=\"parent\"]" << std::endl;

		for (auto it = n->begin(); it != n->end(); ++it) {
			if (!(it->load_acquire()))
				continue;

			auto ch = is_leaf(it->load_acquire())
				? (void *)get_leaf(it->load_acquire())
				: (void *)get_node(it->load_acquire());

			os << "\"" << get_node(n) << "\" -> \"" << ch << "\""
			   << std::endl;
			print_rec(os, it->load_acquire());
		}
	} else {
		auto bv = bytes_view(get_leaf(n)->key());

		os << "\"" << get_leaf(n) << "\" [style=filled,color=\"green\"]"
		   << std::endl;
		os << "\"" << get_leaf(n) << "\" [label=\"key:";

		for (size_t i = 0; i < bv.size(); i++)
			os << bv[i];

		os << "\"]" << std::endl;

		auto parent = get_leaf(n)->parent.load_acquire()
			? get_node(get_leaf(n)->parent.load_acquire())
			: nullptr;

		os << "\"" << get_leaf(n) << "\" -> \"" << parent
		   << "\" [label=\"parent\"]" << std::endl;

		if (parent && n == parent->embedded_entry.load_acquire()) {
			os << "{rank=same;\"" << parent << "\";\""
			   << get_leaf(n) << "\"}" << std::endl;
		}
	}
}

/**
 * Prints tree in DOT format. Used for debugging.
 */
template <typename K, typename V, typename BV>
std::ostream &
operator<<(std::ostream &os, const radix_tree<K, V, BV> &tree)
{
	os << "digraph Radix {" << std::endl;

	if (tree.root.load_acquire())
		radix_tree<K, V, BV>::print_rec(os, tree.root.load_acquire());

	os << "}" << std::endl;

	return os;
}

/*
 * internal: slice_index -- return index of child at the given nib
 */
template <typename Key, typename Value, typename BytesView>
unsigned
radix_tree<Key, Value, BytesView>::slice_index(char b, uint8_t bit)
{
	return static_cast<unsigned>(b >> bit) & NIB;
}

template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::node::forward_iterator::forward_iterator(
	pointer child, const node *n)
    : child(child), n(n)
{
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::node::forward_iterator
radix_tree<Key, Value, BytesView>::node::forward_iterator::operator++()
{
	if (child == &n->embedded_entry)
		child = &n->child[0];
	else
		child++;

	return *this;
}

template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::node::node(pointer_type parent, byten_t byte,
					      bitn_t bit)
    : parent(parent), byte(byte), bit(bit)
{
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::node::forward_iterator
radix_tree<Key, Value, BytesView>::node::forward_iterator::operator--()
{
	if (child == &n->child[0])
		child = &n->embedded_entry;
	else
		child--;

	return *this;
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::node::forward_iterator
radix_tree<Key, Value, BytesView>::node::forward_iterator::operator++(int)
{
	forward_iterator tmp(child, n);
	operator++();
	return tmp;
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::node::forward_iterator::reference
	radix_tree<Key, Value, BytesView>::node::forward_iterator::operator*()
		const
{
	return *child;
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::node::forward_iterator::pointer
	radix_tree<Key, Value, BytesView>::node::forward_iterator::operator->()
		const
{
	return child;
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::node::forward_iterator::pointer
radix_tree<Key, Value, BytesView>::node::forward_iterator::get_node() const
{
	return n;
}

template <typename Key, typename Value, typename BytesView>
bool
radix_tree<Key, Value, BytesView>::node::forward_iterator::operator==(
	const forward_iterator &rhs) const
{
	return child == rhs.child && n == rhs.n;
}

template <typename Key, typename Value, typename BytesView>
bool
radix_tree<Key, Value, BytesView>::node::forward_iterator::operator!=(
	const forward_iterator &rhs) const
{
	return child != rhs.child || n != rhs.n;
}

template <typename Key, typename Value, typename BytesView>
template <bool Direction>
typename std::enable_if<
	Direction ==
		radix_tree<Key, Value, BytesView>::node::direction::Forward,
	typename radix_tree<Key, Value,
			    BytesView>::node::forward_iterator>::type
radix_tree<Key, Value, BytesView>::node::begin() const
{
	return forward_iterator(&embedded_entry, this);
}

template <typename Key, typename Value, typename BytesView>
template <bool Direction>
typename std::enable_if<
	Direction ==
		radix_tree<Key, Value, BytesView>::node::direction::Forward,
	typename radix_tree<Key, Value,
			    BytesView>::node::forward_iterator>::type
radix_tree<Key, Value, BytesView>::node::end() const
{
	return forward_iterator(&child[SLNODES], this);
}

template <typename Key, typename Value, typename BytesView>
template <bool Direction>
typename std::enable_if<
	Direction ==
		radix_tree<Key, Value, BytesView>::node::direction::Reverse,
	typename radix_tree<Key, Value,
			    BytesView>::node::reverse_iterator>::type
radix_tree<Key, Value, BytesView>::node::begin() const
{
	return reverse_iterator(end<direction::Forward>());
}

template <typename Key, typename Value, typename BytesView>
template <bool Direction>
typename std::enable_if<
	Direction ==
		radix_tree<Key, Value, BytesView>::node::direction::Reverse,
	typename radix_tree<Key, Value,
			    BytesView>::node::reverse_iterator>::type
radix_tree<Key, Value, BytesView>::node::end() const
{
	return reverse_iterator(begin<direction::Forward>());
}

template <typename Key, typename Value, typename BytesView>
template <bool Direction, typename Ptr>
auto
radix_tree<Key, Value, BytesView>::node::find_child(const Ptr &n) const
	-> decltype(begin<Direction>())
{
	auto it = begin<Direction>();
	while (it != end<Direction>()) {
		if (it->load_acquire() == n)
			return it;
		++it;
	}
	return it;
}

template <typename Key, typename Value, typename BytesView>
template <bool Direction, typename Enable>
auto
radix_tree<Key, Value, BytesView>::node::make_iterator(
	const atomic_pointer_type *ptr) const -> decltype(begin<Direction>())
{
	return forward_iterator(ptr, this);
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
radix_tree<Key, Value, BytesView>::radix_tree_iterator<
	IsConst>::radix_tree_iterator(leaf_ptr leaf_, tree_ptr tree)
    : leaf_(leaf_), tree(tree)
{
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
template <bool C, typename Enable>
radix_tree<Key, Value, BytesView>::radix_tree_iterator<
	IsConst>::radix_tree_iterator(const radix_tree_iterator<false> &rhs)
    : leaf_(rhs.leaf_)
{
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
typename radix_tree<Key, Value,
		    BytesView>::template radix_tree_iterator<IsConst>::reference
	radix_tree<Key, Value,
		   BytesView>::radix_tree_iterator<IsConst>::operator*() const
{
	assert(leaf_);
	return *leaf_;
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
typename radix_tree<Key, Value,
		    BytesView>::template radix_tree_iterator<IsConst>::pointer
	radix_tree<Key, Value,
		   BytesView>::radix_tree_iterator<IsConst>::operator->() const
{
	assert(leaf_);
	return leaf_;
}

/**
 * Handles assignment to the value. If there is enough capacity
 * old content is overwritten (with a help of undo log). Otherwise
 * a new leaf is allocated and the old one is freed.
 *
 * If reallocation happens, all other iterators to this element are invalidated.
 *
 * This function is useful when value_type is inline_string.
 *
 * @param[in] rhs value of type basic_string_view
 */
template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
template <typename V, typename Enable>
void
radix_tree<Key, Value, BytesView>::radix_tree_iterator<IsConst>::assign_val(
	basic_string_view<typename V::value_type, typename V::traits_type> rhs)
{
	auto pop = pool_base(pmemobj_pool_by_ptr(leaf_));

	if (rhs.size() <= leaf_->value().capacity() && !tree->mt.get(false)) {
		flat_transaction::run(pop, [&] { leaf_->value() = rhs; });
	} else {
		replace_val(rhs);
	}
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
template <typename T>
void
radix_tree<Key, Value, BytesView>::radix_tree_iterator<IsConst>::replace_val(
	T &&rhs)
{
	auto pop = pool_base(pmemobj_pool_by_ptr(leaf_));
	atomic_pointer_type *slot;

	if (!leaf_->parent.load_acquire()) {
		assert(get_leaf(tree->root.load_acquire()) == leaf_);
		slot = &tree->root;
	} else {
		slot = const_cast<atomic_pointer_type *>(
			&*leaf_->parent.load_acquire()->find_child(leaf_));
	}

	auto old_leaf = leaf_;

	flat_transaction::run(pop, [&] {
		slot->store_with_snapshot_release(leaf::make_key_args(
			old_leaf->parent.load_acquire(), old_leaf->key(),
			std::forward<T>(rhs)));
		tree->free(persistent_ptr<radix_tree::leaf>(old_leaf));
	});

	leaf_ = get_leaf(slot->load_acquire());
}

/**
 * Assign value to leaf pointed by the iterator.
 *
 * This function is transactional.
 *
 * @param[in] rhs value of type basic_string_view
 */
template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
template <typename T, typename V, typename Enable>
void
radix_tree<Key, Value, BytesView>::radix_tree_iterator<IsConst>::assign_val(
	T &&rhs)
{
	if (tree->mt.get(false))
		replace_val(std::forward<T>(rhs));
	else {
		auto pop = pool_base(pmemobj_pool_by_ptr(leaf_));
		flat_transaction::run(
			pop, [&] { leaf_->value() = std::forward<T>(rhs); });
	}
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
typename radix_tree<Key, Value,
		    BytesView>::template radix_tree_iterator<IsConst> &
radix_tree<Key, Value, BytesView>::radix_tree_iterator<IsConst>::operator++()
{
	assert(leaf_);

	/* leaf is root, there is no other leaf in the tree */
	if (!leaf_->parent.load_acquire())
		leaf_ = nullptr;
	else {
		auto it = leaf_->parent.load_acquire()
				  ->template find_child<
					  radix_tree::node::direction::Forward>(
					  leaf_);

		leaf_ = const_cast<leaf_ptr>(
			next_leaf<radix_tree::node::direction::Forward>(
				it, leaf_->parent.load_acquire()));
	}

	return *this;
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
typename radix_tree<Key, Value,
		    BytesView>::template radix_tree_iterator<IsConst> &
radix_tree<Key, Value, BytesView>::radix_tree_iterator<IsConst>::operator--()
{
	if (!leaf_) {
		/* this == end() */
		leaf_ = const_cast<leaf_ptr>(
			radix_tree::find_leaf<
				radix_tree::node::direction::Reverse>(
				tree->root.load_acquire()));
	} else {
		/* Iterator must be decrementable. */
		assert(leaf_->parent.load_acquire());

		auto it = leaf_->parent.load_acquire()
				  ->template find_child<
					  radix_tree::node::direction::Reverse>(
					  leaf_);

		leaf_ = const_cast<leaf_ptr>(
			next_leaf<radix_tree::node::direction::Reverse>(
				it, leaf_->parent.load_acquire()));
	}

	return *this;
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
typename radix_tree<Key, Value,
		    BytesView>::template radix_tree_iterator<IsConst>
radix_tree<Key, Value, BytesView>::radix_tree_iterator<IsConst>::operator++(int)
{
	auto tmp = *this;

	++(*this);

	return tmp;
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
typename radix_tree<Key, Value,
		    BytesView>::template radix_tree_iterator<IsConst>
radix_tree<Key, Value, BytesView>::radix_tree_iterator<IsConst>::operator--(int)
{
	auto tmp = *this;

	--(*this);

	return tmp;
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
template <bool C>
bool
radix_tree<Key, Value, BytesView>::radix_tree_iterator<IsConst>::operator!=(
	const radix_tree_iterator<C> &rhs) const
{
	return leaf_ != rhs.leaf_;
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
template <bool C>
bool
radix_tree<Key, Value, BytesView>::radix_tree_iterator<IsConst>::operator==(
	const radix_tree_iterator<C> &rhs) const
{
	return !(*this != rhs);
}

/*
 * Returns next leaf (either with smaller or larger key, depending on
 * ChildIterator type). This function might need to traverse the
 * tree upwards.
 */
template <typename Key, typename Value, typename BytesView>
template <bool Direction, typename Iterator>
typename radix_tree<Key, Value, BytesView>::leaf *
radix_tree<Key, Value, BytesView>::next_leaf(Iterator node, pointer_type parent)
{
	do {
		++node;
	} while (node != parent->template end<Direction>() &&
		 !(node->load_acquire()));

	/* No more children on this level, need to go up. */
	if (node == parent->template end<Direction>()) {
		auto p = parent->parent.load_acquire();
		if (!p) // parent == root
			return nullptr;

		auto p_it = p->template find_child<Direction>(parent);
		return next_leaf<Direction>(p_it, p);
	}

	return find_leaf<Direction>(node->load_acquire());
}

/*
 * Returns smallest (or biggest, depending on ChildIterator) leaf
 * in a subtree.
 */
template <typename Key, typename Value, typename BytesView>
template <bool Direction>
typename radix_tree<Key, Value, BytesView>::leaf *
radix_tree<Key, Value, BytesView>::find_leaf(
	typename radix_tree<Key, Value, BytesView>::pointer_type n)
{
	assert(n);

	if (is_leaf(n))
		return get_leaf(n);

	for (auto it = n->template begin<Direction>();
	     it != n->template end<Direction>(); ++it) {
		if (it->load_acquire())
			return find_leaf<Direction>(it->load_acquire());
	}

	/* There must be at least one leaf at the bottom. */
	std::abort();
}

template <typename Key, typename Value, typename BytesView>
Key &
radix_tree<Key, Value, BytesView>::leaf::key()
{
	return *reinterpret_cast<Key *>(this + 1);
}

template <typename Key, typename Value, typename BytesView>
Value &
radix_tree<Key, Value, BytesView>::leaf::value()
{
	auto key_dst = reinterpret_cast<char *>(this + 1);
	auto val_dst = reinterpret_cast<Value *>(
		key_dst + total_sizeof<Key>::value(key()));

	return *reinterpret_cast<Value *>(val_dst);
}

template <typename Key, typename Value, typename BytesView>
const Key &
radix_tree<Key, Value, BytesView>::leaf::key() const
{
	return *reinterpret_cast<const Key *>(this + 1);
}

template <typename Key, typename Value, typename BytesView>
const Value &
radix_tree<Key, Value, BytesView>::leaf::value() const
{
	auto key_dst = reinterpret_cast<const char *>(this + 1);
	auto val_dst = reinterpret_cast<const Value *>(
		key_dst + total_sizeof<Key>::value(key()));

	return *reinterpret_cast<const Value *>(val_dst);
}

template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::leaf::~leaf()
{
	detail::destroy<Key>(key());
	detail::destroy<Value>(value());
}

template <typename Key, typename Value, typename BytesView>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make(pointer_type parent)
{
	auto t = std::make_tuple();
	return make(parent, std::piecewise_construct, t, t,
		    typename detail::make_index_sequence<>::type{},
		    typename detail::make_index_sequence<>::type{});
}

template <typename Key, typename Value, typename BytesView>
template <typename... Args1, typename... Args2>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make(pointer_type parent,
					      std::piecewise_construct_t pc,
					      std::tuple<Args1...> first_args,
					      std::tuple<Args2...> second_args)
{
	return make(parent, pc, first_args, second_args,
		    typename detail::make_index_sequence<Args1...>::type{},
		    typename detail::make_index_sequence<Args2...>::type{});
}

template <typename Key, typename Value, typename BytesView>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make(pointer_type parent, const Key &k,
					      const Value &v)
{
	return make(parent, std::piecewise_construct, std::forward_as_tuple(k),
		    std::forward_as_tuple(v));
}

template <typename Key, typename Value, typename BytesView>
template <typename K, typename V>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make(pointer_type parent, K &&k, V &&v)
{
	return make(parent, std::piecewise_construct,
		    std::forward_as_tuple(std::forward<K>(k)),
		    std::forward_as_tuple(std::forward<V>(v)));
}

template <typename Key, typename Value, typename BytesView>
template <typename K, typename... Args>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make_key_args(pointer_type parent,
						       K &&k, Args &&... args)
{
	return make(parent, std::piecewise_construct,
		    std::forward_as_tuple(std::forward<K>(k)),
		    std::forward_as_tuple(std::forward<Args>(args)...));
}

template <typename Key, typename Value, typename BytesView>
template <typename K, typename V>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make(pointer_type parent,
					      detail::pair<K, V> &&p)
{
	return make(parent, std::piecewise_construct,
		    std::forward_as_tuple(std::move(p.first)),
		    std::forward_as_tuple(std::move(p.second)));
}

template <typename Key, typename Value, typename BytesView>
template <typename K, typename V>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make(pointer_type parent,
					      const detail::pair<K, V> &p)
{
	return make(parent, std::piecewise_construct,
		    std::forward_as_tuple(p.first),
		    std::forward_as_tuple(p.second));
}

template <typename Key, typename Value, typename BytesView>
template <typename K, typename V>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make(pointer_type parent,
					      std::pair<K, V> &&p)
{
	return make(parent, std::piecewise_construct,
		    std::forward_as_tuple(std::move(p.first)),
		    std::forward_as_tuple(std::move(p.second)));
}

template <typename Key, typename Value, typename BytesView>
template <typename K, typename V>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make(pointer_type parent,
					      const std::pair<K, V> &p)
{
	return make(parent, std::piecewise_construct,
		    std::forward_as_tuple(p.first),
		    std::forward_as_tuple(p.second));
}

template <typename Key, typename Value, typename BytesView>
template <typename... Args1, typename... Args2, size_t... I1, size_t... I2>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make(pointer_type parent,
					      std::piecewise_construct_t,
					      std::tuple<Args1...> &first_args,
					      std::tuple<Args2...> &second_args,
					      detail::index_sequence<I1...>,
					      detail::index_sequence<I2...>)
{
	standard_alloc_policy<void> a;
	auto key_size = total_sizeof<Key>::value(std::get<I1>(first_args)...);
	auto val_size =
		total_sizeof<Value>::value(std::get<I2>(second_args)...);
	auto ptr = static_cast<persistent_ptr<leaf>>(
		a.allocate(sizeof(leaf) + key_size + val_size));

	auto key_dst = reinterpret_cast<Key *>(ptr.get() + 1);
	auto val_dst = reinterpret_cast<Value *>(
		reinterpret_cast<char *>(key_dst) + key_size);

	new (ptr.get()) leaf();
	new (key_dst) Key(std::forward<Args1>(std::get<I1>(first_args))...);
	new (val_dst) Value(std::forward<Args2>(std::get<I2>(second_args))...);

	ptr->parent.store_with_snapshot_release(parent);

	return ptr;
}

template <typename Key, typename Value, typename BytesView>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make(pointer_type parent,
					      const leaf &other)
{
	return make(parent, other.key(), other.value());
}

/**
 * Private helper function. Checks if radix tree resides on pmem and throws an
 * exception if not.
 *
 * @throw pool_error if radix tree doesn't reside on pmem.
 */
template <typename Key, typename Value, typename BytesView>
void
radix_tree<Key, Value, BytesView>::check_pmem()
{
	if (nullptr == pmemobj_pool_by_ptr(this))
		throw pmem::pool_error("Invalid pool handle.");
}

/**
 * Private helper function. Checks if current transaction stage is equal to
 * TX_STAGE_WORK and throws an exception otherwise.
 *
 * @throw pmem::transaction_scope_error if current transaction stage is not
 * equal to TX_STAGE_WORK.
 */
template <typename Key, typename Value, typename BytesView>
void
radix_tree<Key, Value, BytesView>::check_tx_stage_work()
{
	if (pmemobj_tx_stage() != TX_STAGE_WORK)
		throw pmem::transaction_scope_error(
			"Function called out of transaction scope.");
}

template <typename Key, typename Value, typename BytesView>
bool
radix_tree<Key, Value, BytesView>::is_leaf(
	const radix_tree<Key, Value, BytesView>::pointer_type &p)
{
	return p.template is<leaf>();
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::leaf *
radix_tree<Key, Value, BytesView>::get_leaf(
	const radix_tree<Key, Value, BytesView>::pointer_type &p)
{
	return p.template get<leaf>();
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::node *
radix_tree<Key, Value, BytesView>::get_node(
	const radix_tree<Key, Value, BytesView>::pointer_type &p)
{
	return p.template get<node>();
}

/**
 * Non-member swap.
 */
template <typename Key, typename Value, typename BytesView>
void
swap(radix_tree<Key, Value, BytesView> &lhs,
     radix_tree<Key, Value, BytesView> &rhs)
{
	lhs.swap(rhs);
}

} /* namespace experimental */
} /* namespace obj */

namespace detail
{
/* Check if type is pmem::obj::basic_string or
 * pmem::obj::basic_inline_string */
template <typename>
struct is_string : std::false_type {
};

template <typename CharT, typename Traits>
struct is_string<obj::basic_string<CharT, Traits>> : std::true_type {
};

template <typename CharT, typename Traits>
struct is_string<obj::experimental::basic_inline_string<CharT, Traits>>
    : std::true_type {
};

template <typename T>
struct bytes_view<T, typename std::enable_if<is_string<T>::value>::type> {
	using CharT = typename T::value_type;
	using Traits = typename T::traits_type;

	template <
		typename C,
		typename Enable = typename std::enable_if<std::is_constructible<
			obj::basic_string_view<CharT, Traits>, C>::value>::type>
	bytes_view(const C *s) : s(*s)
	{
	}

	char operator[](std::size_t p) const
	{
		return reinterpret_cast<const char *>(s.data())[p];
	}

	size_t
	size() const
	{
		return s.size() * sizeof(CharT);
	}

	obj::basic_string_view<CharT, Traits> s;

	using is_transparent = void;
};

template <typename T>
struct bytes_view<T,
		  typename std::enable_if<std::is_integral<T>::value &&
					  !std::is_signed<T>::value>::type> {
	bytes_view(const T *k) : k(k)
	{
#if __cpp_lib_endian
		static_assert(
			std::endian::native == std::endian::little,
			"Scalar type are not little endian on this platform!");
#elif !defined(NDEBUG)
		/* Assert little endian is used. */
		uint16_t word = (2 << 8) + 1;
		assert(((char *)&word)[0] == 1);
#endif
	}

	char operator[](std::size_t p) const
	{
		return reinterpret_cast<const char *>(k)[size() - p - 1];
	}

	constexpr size_t
	size() const
	{
		return sizeof(T);
	}

	const T *k;
};
} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_RADIX_HPP */
