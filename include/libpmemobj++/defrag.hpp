/*
 * Copyright 2020, Intel Corporation
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

/**
 * @file
 * Defragmentation class.
 */

#ifndef LIBPMEMOBJ_CPP_DEFRAG_HPP
#define LIBPMEMOBJ_CPP_DEFRAG_HPP

#include <type_traits>
#include <vector>

#include <libpmemobj++/detail/template_helpers.hpp>
#include <libpmemobj++/persistent_ptr_base.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj/atomic_base.h>
#include <libpmemobj/base.h>

namespace pmem
{

namespace obj
{
using collect_function = void (*)(persistent_ptr_base &ptr);

template <typename T>
using t_has_collect_all = typename std::enable_if<
	std::is_same<decltype(std::declval<T>().collect_all(
			     std::declval<collect_function>())),
		     void>::value>::type;

template <typename T>
using t_is_defragmentable = detail::supports<T, t_has_collect_all>;

/**
 * TODO: fill in
 */
template <typename T>
static constexpr
typename std::enable_if<t_is_defragmentable<T>::value, bool>::type
is_defragmentable()
{
	return true;
}

/**
 * TODO: fill in
 */
template <typename T>
static constexpr
typename std::enable_if<!t_is_defragmentable<T>::value, bool>::type
is_defragmentable()
{
	return false;
}

/**
 * Defrag class.
 *
 * This class implements methods used to store pointers from a pool
 * for defragmentation. When defragmentation is called/run, all objects
 * in the vector will be a subject of a defragmentation process.
 *
 * Important note: an instance of this class should collect pointers
 * only from one pmem::obj::pool instance.
 *
 * The typical usage example would be:
 * @snippet doc_snippets/defrag.cpp defrag_usage_example
 */
class defrag {
public:
	/*
	 * Constructors
	 */
	defrag() = delete;

	template <typename T>
	defrag(pool<T> p)
	{
		this->pop = p;
	}

	defrag(pool_base p)
	{
		this->pop = p;
	}

	/*
	 * Defragmentation related methods
	 */

	/**
	 * Adds 'persistent_ptr_base *' objects to the vector.
	 *
	 * It's to be called on containers, which are defragmentable
	 * (they implement 'collect_all' method), and it will add
	 * all of its internal pointers to the provided vector.
	 */
	template <typename T>
	typename std::enable_if<is_defragmentable<T>(), void>::type
	add(T &t)
	{
		if (pmemobj_pool_by_ptr(&t) != pop.handle())
			throw std::runtime_error(
				"persistent_ptr is not from the chosen pool");

		t.collect_all([&](persistent_ptr_base &ptr) {
			this->container.push_back(&ptr);
		});
	}

	/**
	 * Adds 'persistent_ptr_base *' objects to the vector.
	 *
	 * It's to be called on (most likely trivial) types, which are NOT
	 * defragmentable (they don't implement 'collect_all' method).
	 */
	template <typename T>
	typename std::enable_if<!is_defragmentable<T>(), void>::type
	add(T &)
	{
	}

	/**
	 * Adds 'persistent_ptr_base objects *' to the vector.
	 *
	 * It's to be called for a single 'persistent_ptr<T>',
	 * not containers/collections.
	 */
	template <typename T>
	void
	add(persistent_ptr<T> &ptr)
	{
		if (pmemobj_pool_by_oid(ptr.raw()) != pop.handle())
			throw std::runtime_error(
				"persistent_ptr is not from the chosen pool");

		this->container.push_back(&ptr);
		this->add<T>(*ptr);
	}

	/**
	 * Starts defragmentation on pointers previously added to the vector
	 *
	 * @throw ... when a failure during defragmentation occures.
	 * @return result struct containing number of total and relocated ptr's
	 */
	pobj_defrag_result
	run()
	{
		pobj_defrag_result result;
		int res = this->pop.defrag(this->container.data(),
				 this->container.size(), result);
		if (res != 0)
			throw defrag_error();

		return result;
	}

private:
	std::vector<persistent_ptr_base *> container;
	pool_base pop;
};

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_DEFRAG_HPP */
