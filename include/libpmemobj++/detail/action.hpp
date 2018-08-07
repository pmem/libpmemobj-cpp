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

/**
 * @file
 * Wrappers for libpmemobj experimental action API.
 */

#ifndef LIBPMEMOBJ_ACTION_HPP
#define LIBPMEMOBJ_ACTION_HPP

#include "libpmemobj++/detail/pexceptions.hpp"
#include "libpmemobj++/persistent_ptr.hpp"
#include "libpmemobj/action_base.h"
#include <errno.h>
#include <vector>

namespace pmem
{

namespace detail
{

/**
 * action class
 *
 * This class is the pobj_action array handler. It provides basic primitives
 * for operations related to reserve/publish API.
 */
class action {
public:
	using iterator = std::vector<struct pobj_action>::iterator;
	using const_iterator = std::vector<struct pobj_action>::const_iterator;

	/**
	 * Deleted default constructor. Every action object should have
	 * non-nullptr pop field. There is no method for changing pop value.
	 */
	action() noexcept = delete;

	/**
	 * Explicit constructor.
	 *
	 * Create action object based on given pool and C-style pobj_action
	 * array.
	 */
	explicit action(const obj::pool_base &pop,
			struct pobj_action *act = nullptr,
			std::size_t act_cnt = 0) noexcept
	    : actv(act, act + act_cnt), pop(pop)
	{
	}

	/**
	 * Checks if there are actions stored in object.
	 */
	bool
	empty(void) const noexcept
	{
		return this->actv.empty();
	}

	/**
	 * Returns iterator to begin of actv.
	 */
	iterator
	begin(void) noexcept
	{
		return this->actv.begin();
	}

	/**
	 * Returns iterator to end of actv.
	 */
	iterator
	end(void) noexcept
	{
		return this->actv.end();
	}

	/**
	 * Returns const_iterator to begin of actv.
	 */
	const_iterator
	begin(void) const noexcept
	{
		return this->actv.cbegin();
	}

	/**
	 * Returns const_iterator to end of actv.
	 */
	const_iterator
	end(void) const noexcept
	{
		return this->actv.cend();
	}

	/**
	 * Returns number of actions stored in object.
	 */
	std::size_t
	size(void) const noexcept
	{
		return this->actv.size();
	}

	/**
	 * Performs a transient reservation of an object. The object returned
	 * by this function can be freely modified without worrying about
	 * fail-safe atomicity until the object has been published. Any
	 * modifications of the object must be manually persisted.
	 *
	 * @return persistent_ptr to reserved object, nullptr if reservation
	 * failed
	 *
	 * @throw std::invalid_argument if unknown flag was given
	 */
	template <typename T>
	obj::persistent_ptr<T>
	reserve(std::size_t size = sizeof(T), uint64_t flags = 0)
	{
		if (flags && (flags & ~POBJ_ACTION_XRESERVE_VALID_FLAGS))
			throw std::invalid_argument("Invalid flags argument.");

		struct pobj_action tmp;
		obj::persistent_ptr<T> ret =
			pmemobj_xreserve(this->pop.get_handle(), &tmp, size,
					 detail::type_num<T>(), flags);
		if (ret != nullptr)
			this->actv.push_back(tmp);

		return ret;
	}

	/**
	 * Prepares an action that, once published, will modify the memory
	 * location pointed to by ptr to value.
	 */
	void
	set_value(uint64_t *ptr, uint64_t value)
	{
		struct pobj_action tmp;
		pmemobj_set_value(this->pop.get_handle(), &tmp, ptr, value);
		this->actv.push_back(tmp);
	}

	/**
	 * Creates a deferred free action, meaning that the provided object
	 * will be freed when the action is published.
	 */
	template <typename T>
	void
	defer_free(obj::persistent_ptr<T> ptr)
	{
		struct pobj_action tmp;
		pmemobj_defer_free(this->pop.get_handle(), ptr.raw(), &tmp);
		this->actv.push_back(tmp);
	}

	/**
	 * Publishes all of actions in actv. The publication is fail-safe
	 * atomic. Once done, the persistent state will reflect the changes
	 * contained in the actions.
	 */
	void
	publish()
	{
		publish(this->actv.begin(), this->actv.end());
	}

	/**
	 * Publishes the provided set of actions. The publication is fail-safe
	 * atomic. Once done, the persistent state will reflect the changes
	 * contained in the actions.
	 *
	 * @throw std::system_error with errno from pmemobj_publish
	 */
	void
	publish(iterator first, iterator last)
	{
		/**
		 * Return type of pmemobj_publish() varies in different PMDK
		 * versions.
		 * MSVC does not compile below code with void return type
		 * version of pmemobj_publish().
		 */
		if (std::is_same<decltype(pmemobj_publish(nullptr, nullptr, 0)),
				 void>::value) {
			pmemobj_publish(this->pop.get_handle(), &*first,
					static_cast<std::size_t>(
						std::distance(first, last)));
		}
		else {
			int ret = pmemobj_publish(
				this->pop.get_handle(), &*first,
				static_cast<std::size_t>(
					std::distance(first, last)));
			if (ret)
				throw std::system_error(
					errno, std::generic_category());
		}

		erase(first, last);
	}

	/**
	 * Releases all resources held by actv and invalidates them.
	 */
	void
	cancel()
	{
		cancel(this->actv.begin(), this->actv.end());
	}

	/**
	 * Releases resources held by the provided set of actions and
	 * invalidates them. Erases invalidated action elements from actv.
	 */
	void
	cancel(iterator first, iterator last)
	{
		pmemobj_cancel(
			this->pop.get_handle(), &*first,
			static_cast<std::size_t>(std::distance(first, last)));

		erase(first, last);
	}

	/**
	 * Removes a single element from the vector.
	 */
	iterator
	erase(iterator position)
	{
		return this->actv.erase(position);
	}

	/**
	 * Removes a range of elements from the vector.
	 */
	iterator
	erase(iterator first, iterator last)
	{
		return this->actv.erase(first, last);
	}

	/**
	 * Returns object's pool indicator.
	 */
	obj::pool_base
	get_pool() const noexcept
	{
		return this->pop;
	}

private:
	/* Vector container to store pobj_action structures */
	std::vector<struct pobj_action> actv;
	/* Pool variable  */
	obj::pool_base pop;
};

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_ACTION_HPP */
