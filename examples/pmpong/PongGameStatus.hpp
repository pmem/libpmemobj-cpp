// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2018, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_PONGGAMESTATUS_HPP
#define LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_PONGGAMESTATUS_HPP

#include "Ball.hpp"
#include "GameConstants.hpp"
#include "Paddle.hpp"
#include <SFML/Graphics.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/transaction.hpp>

enum game_state { MATCH, MENU, GAME_OVER, SIMULATE };

class PongGameStatus {
public:
	PongGameStatus();
	~PongGameStatus();

	void startBall(float ballSpeed);
	void reset();
	void movePaddles();
	void lookForCollisions(bool increaseBallVelocity);
	void actualizeStatus();
	void simulate();
	void setMenuItem(int numb);
	void setIsGameToResume(bool isGameToRes);
	void setGameState(game_state state);

	int getMenuItem();

	float randomizeFloatValue(float min, float max);

	bool score();
	bool checkIfAnyPlayerWon();
	bool randomizeDirection();
	bool getIsGameToResume();

	pmem::obj::persistent_ptr<Paddle> getPlayer1();
	pmem::obj::persistent_ptr<Paddle> getPlayer2();
	pmem::obj::persistent_ptr<Ball> getBall();

	game_state getGameState();

private:
	pmem::obj::persistent_ptr<Paddle> player1;
	pmem::obj::persistent_ptr<Paddle> player2;
	pmem::obj::persistent_ptr<Ball> ball;

	pmem::obj::p<int> menuItem;
	pmem::obj::p<bool> isGameToResume;
	pmem::obj::p<game_state> actualGameState;
};
#endif /* LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_PONGGAMESTATUS_HPP */
