// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "Ball.hpp"
#include "Pool.hpp"

Ball::Ball(int x, int y)
{
	this->x = x;
	this->y = y;
	velocity = pmem::obj::make_persistent<sf::Vector2f>();
	this->velocity->x = 0;
	this->velocity->y = 0;
}

Ball::~Ball()
{
	try {
		pmem::obj::transaction::run(
			Pool::getGamePool()->getPoolToTransaction(), [&] {
				pmem::obj::delete_persistent<sf::Vector2f>(
					velocity);
			});
	} catch (...) {
		std::terminate();
	}
}

void
Ball::move()
{
	setXY(this->x + (int)this->velocity->x,
	      this->y + (int)this->velocity->y);
}

void
Ball::collisionWithWindow()
{
	if (this->y <= SCORE_VIEW_OFFSET + HORIZONAL_LINE_OFFSET ||
	    this->y + getBallShape().getRadius() * 2 >=
		    WINDOW_HEIGHT - HORIZONAL_LINE_OFFSET) {
		setVelocityY(velocity->y * -1);
	}
}

void
Ball::increaseVelocity()
{
	if (velocity->x < 0) {
		setVelocityX(velocity->x - BALL_VELOCITY_INCREMENTING);
	} else {
		setVelocityX(velocity->x + BALL_VELOCITY_INCREMENTING);
	}
	if (velocity->y < 0) {
		setVelocityY(velocity->y - BALL_VELOCITY_INCREMENTING);

	} else {
		setVelocityY(velocity->y + BALL_VELOCITY_INCREMENTING);
	}
}

void
Ball::setX(int xArg)
{
	pmem::obj::transaction::run(Pool::getGamePool()->getPoolToTransaction(),
				    [&] { x = xArg; });
}

void
Ball::setY(int yArg)
{
	pmem::obj::transaction::run(Pool::getGamePool()->getPoolToTransaction(),
				    [&] { y = yArg; });
}

void
Ball::setVelocityX(float xArg)
{
	pmem::obj::transaction::run(Pool::getGamePool()->getPoolToTransaction(),
				    [&] { velocity->x = xArg; });
}

void
Ball::setVelocityY(float yArg)
{
	pmem::obj::transaction::run(Pool::getGamePool()->getPoolToTransaction(),
				    [&] { velocity->y = yArg; });
}

void
Ball::setXY(int xArg, int yArg)
{
	pmem::obj::transaction::run(Pool::getGamePool()->getPoolToTransaction(),
				    [&] {
					    x = xArg;
					    y = yArg;
				    });
}

void
Ball::init()
{
	setXY(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
	setVelocityX(0);
	setVelocityY(0);
}

int
Ball::getX()
{
	return this->x;
}

int
Ball::getY()
{
	return this->y;
}

pmem::obj::persistent_ptr<sf::Vector2f>
Ball::getVelocity()
{
	return this->velocity;
}

sf::CircleShape
Ball::getBallShape()
{
	sf::CircleShape shapeToRet;
	shapeToRet.setRadius(BALL_SIZE);
	shapeToRet.setPosition(sf::Vector2f((float)this->x, (float)this->y));
	return shapeToRet;
}
