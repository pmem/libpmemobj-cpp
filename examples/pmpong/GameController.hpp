// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2018, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_GAMECONTROLLER_HPP
#define LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_GAMECONTROLLER_HPP

#include "GameConstants.hpp"
#include "GameOverView.hpp"
#include "GameView.hpp"
#include "MenuView.hpp"
#include "PongGameStatus.hpp"
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

class GameController {
public:
	GameController();
	~GameController();

	void gameLoop(bool isSimulation = false);

private:
	pmem::obj::persistent_ptr<PongGameStatus> gameStatus;

	void gameOver(sf::RenderWindow *gameWindow, View *view);
	void menu(sf::RenderWindow *gameWindow, View *view);
	void handleEventKeypress(sf::Event &event,
				 sf::RenderWindow *gameWindow);
	void gameMatch(sf::RenderWindow *gameWindow, View *view);
	void gameMatchSimulation(sf::RenderWindow *gameWindow, View *view);

	void resetGameStatus();
};

#endif /* LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_GAMECONTROLLER_HPP */
