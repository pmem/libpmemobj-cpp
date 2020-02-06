// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2019, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_GAMECONSTANTS_HPP
#define LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_GAMECONSTANTS_HPP

#include <SFML/Graphics.hpp>
#include <fstream>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <random>
#include <string>

#define PADDLE_VELOCITY_PLAYER 4
#define PADDLE_VELOCITY_COMPUTER 20
#define WINDOW_HEIGHT 500
#define WINDOW_WIDTH 700
#define BALL_VELOCITY_INCREMENTING 0.2f
#define FRAMERATE_LIMIT 70
#define PADDLE_HEIGHT 100
#define PADDLE_WIDTH 12
#define BALL_SIZE 7
#define MENU_ITEMS 4
#define POINTS_TO_WIN 10
#define BALL_PLAYERS_SPEED 4.0f
#define BALL_COMPUTER_SPEED 11.0f
#define VERTICAL_LINE_OFFSET 15
#define HORIZONAL_LINE_OFFSET 30
#define LINE_THICKNESS 3
#define SCORE_VIEW_OFFSET 20
#define GAME_NAME "pmpong"
#define GAMEVIEW_SCORE_FONTSIZE 20
#define MENUVIEW_ITEMS_FONTSIZE 30
#define GAMEOVER_FONTSIZE 45
#define MENUITEM_OFFSET 100
#define GAMOVERVIEW_OFFSET 50
#define LAYOUT_NAME "DEFAULT_LAYOUT_NAME"
#define DEFAULT_POOLFILE_NAME "DEFAULT_FILENAME"

static inline sf::Font
getFont()
{
	std::string font_path = "";
#ifdef LIBPMEMOBJ_CPP_PMPONG_FONT_PATH
	font_path = LIBPMEMOBJ_CPP_PMPONG_FONT_PATH;
#endif

	auto env = getenv("LIBPMEMOBJ_CPP_PMPONG_FONT_PATH");
	if (env != nullptr)
		font_path = env;

	sf::Font font;
	if (!font.loadFromFile(font_path)) {
		throw std::runtime_error(
			"Cannot find fonts. Please set environmental variable LIBPMEMOBJ_CPP_PMPONG_FONT_PATH"
			" to path to existing font file");
	}

	return font;
}

#endif /* LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_GAMECONSTANTS_HPP */
