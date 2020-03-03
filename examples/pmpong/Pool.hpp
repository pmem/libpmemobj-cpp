// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2018, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_POOL_HPP
#define LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_POOL_HPP

#include "GameController.hpp"
#include <SFML/Graphics.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <string>

struct GameStruct {
public:
	pmem::obj::persistent_ptr<GameController> gam;
};

class Pool {

public:
	~Pool();
	static Pool *getGamePoolFromFile(const std::string &fileName);
	static Pool *getGamePool();
	pmem::obj::persistent_ptr<GameController> getGameController();
	pmem::obj::pool<GameStruct> &getPoolToTransaction();

private:
	Pool(const std::string &name);
	static Pool *pongPool;

	pmem::obj::pool<GameStruct> pool;

	Pool(const Pool &);
	Pool &operator=(const Pool &);
};

#endif /* LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_POOL_HPP */
