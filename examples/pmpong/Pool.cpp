// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "Pool.hpp"

Pool *Pool::pongPool = nullptr;

Pool::Pool(const std::string &fileName)
{
	if (pmem::obj::pool<GameStruct>::check(fileName, LAYOUT_NAME) == 1) {
		pool = pmem::obj::pool<GameStruct>::open(fileName, LAYOUT_NAME);
	} else {
		pool = pmem::obj::pool<GameStruct>::create(
			fileName, LAYOUT_NAME, PMEMOBJ_MIN_POOL * 6);
	}
}

Pool::~Pool()
{
	try {
		pool.close();
	} catch (const std::logic_error &e) {
		std::terminate();
	}
}

Pool *
Pool::getGamePoolFromFile(const std::string &fileName)
{
	if (pongPool == nullptr)
		pongPool = new Pool(fileName);
	return pongPool;
}

Pool *
Pool::getGamePool()
{
	if (pongPool == nullptr) {
		return getGamePoolFromFile(DEFAULT_POOLFILE_NAME);
	}
	return pongPool;
}

pmem::obj::persistent_ptr<GameController>
Pool::getGameController()
{
	pmem::obj::persistent_ptr<GameStruct> root = pool.root();
	if (root != nullptr) {
		if (root->gam == nullptr)
			pmem::obj::transaction::run(pool, [&] {
				root->gam = pmem::obj::make_persistent<
					GameController>();
			});
	}
	return root->gam;
}

pmem::obj::pool<GameStruct> &
Pool::getPoolToTransaction()
{
	return pool;
}
