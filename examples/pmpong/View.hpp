// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2018, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_VIEW_HPP
#define LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_VIEW_HPP

#include "GameConstants.hpp"
#include "PongGameStatus.hpp"
#include <SFML/Graphics.hpp>

class View {
public:
	virtual ~View(){};
	virtual void prepareView(PongGameStatus &gameStatus) = 0;
	virtual void displayView(sf::RenderWindow *gameWindow) = 0;
};

#endif /* LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_VIEW_HPP */
