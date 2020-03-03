// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2018, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_GAMEVIEW_HPP
#define LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_GAMEVIEW_HPP

#include "GameConstants.hpp"
#include "PongGameStatus.hpp"
#include "View.hpp"
#include <SFML/Graphics.hpp>
#include <string>

class GameView : public View {
public:
	GameView(sf::Font &font);
	~GameView();

	virtual void prepareView(PongGameStatus &gameStatus);
	virtual void displayView(sf::RenderWindow *gameWindow);

private:
	sf::Text scoreP1;
	sf::Text scoreP2;

	sf::RectangleShape upperLine;
	sf::RectangleShape downLine;
	sf::RectangleShape leftLine;
	sf::RectangleShape rightLine;
	sf::RectangleShape court;

	sf::CircleShape ballShape;
	sf::RectangleShape leftPaddleShape;
	sf::RectangleShape rightPaddleShape;
};

#endif /* LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_GAMEVIEW_HPP */
