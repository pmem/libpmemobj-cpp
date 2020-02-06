// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2018, Intel Corporation */

#include "Pool.hpp"
#include <SFML/Graphics.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

int
main(int argc, char *argv[])
{
	int exitCode = EXIT_FAILURE;
	std::string mode = "";
	if (argc == 3) {
		mode = argv[2];
	}
	if (argc < 2 || argc > 3 || (argc == 3 && mode != "-s")) {
		std::cout << "Usage: ./pmpong <game_session_file> [options]"
			  << std::endl
			  << "Options: " << std::endl
			  << "-s, simulates game between 2 AI players"
			  << std::endl;
		return exitCode;
	}
	std::string fileName = argv[1];
	try {
		Pool *pool = Pool::getGamePoolFromFile(fileName);
		pmem::obj::persistent_ptr<GameController> gameController =
			pool->getGameController();
		if (mode == "-s")
			gameController->gameLoop(true);
		else
			gameController->gameLoop();
		delete pool;
		exitCode = EXIT_SUCCESS;
	} catch (pmem::transaction_error &err) {
		std::cerr << err.what() << std::endl;
	} catch (pmem::transaction_scope_error &tse) {
		std::cerr << tse.what() << std::endl;
	} catch (pmem::pool_error &pe) {
		std::cerr << pe.what() << std::endl;
	} catch (std::logic_error &le) {
		std::cerr << le.what() << std::endl;
	} catch (std::exception &exc) {
		std::cerr << exc.what() << std::endl;
	}

	return exitCode;
}
