// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2018, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_GAMEOVERVIEW_HPP
#define LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_GAMEOVERVIEW_HPP

#include "GameConstants.hpp"
#include "PongGameStatus.hpp"
#include "View.hpp"
#include <SFML/Graphics.hpp>

class GameOverView : public View {
public:
	GameOverView(sf::Font &font);
	~GameOverView();

	virtual void prepareView(PongGameStatus &gameStatus);
	virtual void displayView(sf::RenderWindow *gameWindow);

private:
	sf::Text gameOver;
	sf::Text playerWinner;
	sf::Text entContinue;
};

#endif /* LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_GAMEOVERVIEW_HPP */
