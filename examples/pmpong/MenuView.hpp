// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2018, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_MENUVIEW_HPP
#define LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_MENUVIEW_HPP

#include "GameConstants.hpp"
#include "PongGameStatus.hpp"
#include "View.hpp"
#include <SFML/Graphics.hpp>

enum menu_items { NEW_GAME, RESUME, SIMULATION, EXIT };

class MenuView : public View {
public:
	MenuView(sf::Font &font);
	~MenuView();

	virtual void prepareView(PongGameStatus &gameStatus);
	virtual void displayView(sf::RenderWindow *gameWindow);

private:
	sf::Text menuItems[MENU_ITEMS];
};

#endif /* LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_MENUVIEW_HPP */
