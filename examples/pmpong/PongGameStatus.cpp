// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "PongGameStatus.hpp"
#include "Pool.hpp"

PongGameStatus::PongGameStatus()
{
	player1 = pmem::obj::make_persistent<Paddle>(
		VERTICAL_LINE_OFFSET + LINE_THICKNESS, WINDOW_HEIGHT / 2);
	player2 = pmem::obj::make_persistent<Paddle>(
		WINDOW_WIDTH - VERTICAL_LINE_OFFSET - PADDLE_WIDTH,
		WINDOW_HEIGHT / 2);
	ball = pmem::obj::make_persistent<Ball>(WINDOW_WIDTH / 2,
						WINDOW_HEIGHT / 2);
	menuItem = 0;
	isGameToResume = false;
	actualGameState = game_state::MENU;
}

PongGameStatus::~PongGameStatus()
{
	try {
		pmem::obj::transaction::run(
			Pool::getGamePool()->getPoolToTransaction(), [&] {
				pmem::obj::delete_persistent<Paddle>(player1);
				pmem::obj::delete_persistent<Paddle>(player2);
				pmem::obj::delete_persistent<Ball>(ball);
			});
	} catch (...) {
		std::terminate();
	}
}

void
PongGameStatus::startBall(float ballSpeed)
{
	if (ball->getVelocity()->x == 0 && ball->getVelocity()->y == 0) {
		float x = randomizeFloatValue(1.5, 2.0);
		ball->setVelocityX(randomizeDirection() ? ballSpeed
							: -ballSpeed);
		ball->setVelocityY(randomizeDirection() ? x : -1 * x);
	}
}

void
PongGameStatus::reset()
{
	ball->init();
	player1->init();
	player2->init();
}

void
PongGameStatus::movePaddles()
{
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
		player1->moveUp(PADDLE_VELOCITY_PLAYER);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
		player1->moveDown(PADDLE_VELOCITY_PLAYER);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
		player2->moveUp(PADDLE_VELOCITY_PLAYER);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
		player2->moveDown(PADDLE_VELOCITY_PLAYER);
	}
}

void
PongGameStatus::lookForCollisions(bool increaseBallVelocity)
{
	player1->collisionWithBall(*ball, increaseBallVelocity);
	player2->collisionWithBall(*ball, increaseBallVelocity);
	ball->collisionWithWindow();
}

void
PongGameStatus::actualizeStatus()
{
	ball->move();
}

void
PongGameStatus::simulate()
{
	if (ball->getVelocity()->x > 0)
		player2->adjustPaddleYtoBall(*ball);
	if (ball->getVelocity()->x < 0)
		player1->adjustPaddleYtoBall(*ball);
}

void
PongGameStatus::setMenuItem(int numb)
{
	pmem::obj::transaction::run(Pool::getGamePool()->getPoolToTransaction(),
				    [&] { this->menuItem = numb; });
}

void
PongGameStatus::setIsGameToResume(bool isGameToRes)
{
	pmem::obj::transaction::run(Pool::getGamePool()->getPoolToTransaction(),
				    [&] { isGameToResume = isGameToRes; });
}

void
PongGameStatus::setGameState(game_state state)
{
	pmem::obj::transaction::run(Pool::getGamePool()->getPoolToTransaction(),
				    [&] { this->actualGameState = state; });
}

int
PongGameStatus::getMenuItem()
{
	return this->menuItem;
}

float
PongGameStatus::randomizeFloatValue(float min, float max)
{
	return (min + 1) +
		(((float)rand()) / (float)RAND_MAX) * (max - (min + 1));
}

bool
PongGameStatus::score()
{
	if (ball->getBallShape().getPosition().x > WINDOW_WIDTH -
		    VERTICAL_LINE_OFFSET + LINE_THICKNESS -
		    ball->getBallShape().getRadius() * 2) {
		player1->addPoint();
		reset();
		return true;
	}
	if (ball->getBallShape().getPosition().x <
	    VERTICAL_LINE_OFFSET - LINE_THICKNESS) {
		player2->addPoint();
		reset();
		return true;
	}
	return false;
}

bool
PongGameStatus::checkIfAnyPlayerWon()
{
	if (getPlayer1()->getPoints() == POINTS_TO_WIN ||
	    getPlayer2()->getPoints() == POINTS_TO_WIN)
		return true;
	return false;
}

bool
PongGameStatus::randomizeDirection()
{
	static const int shift = static_cast<int>(std::log2(RAND_MAX));
	return (rand() >> shift) & 1;
}

bool
PongGameStatus::getIsGameToResume()
{
	return this->isGameToResume;
}

pmem::obj::persistent_ptr<Paddle>
PongGameStatus::getPlayer1()
{
	return this->player1;
}

pmem::obj::persistent_ptr<Paddle>
PongGameStatus::getPlayer2()
{
	return this->player2;
}

pmem::obj::persistent_ptr<Ball>
PongGameStatus::getBall()
{
	return this->ball;
}

game_state
PongGameStatus::getGameState()
{
	return this->actualGameState;
}
