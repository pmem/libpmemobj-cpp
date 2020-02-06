// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2018, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_BALL_HPP
#define LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_BALL_HPP

#include "GameConstants.hpp"
#include <SFML/Graphics.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/transaction.hpp>

class Ball {
public:
	Ball(int x, int y);
	~Ball();

	void move();
	void collisionWithWindow();
	void increaseVelocity();
	void setX(int xArg);
	void setY(int yArg);
	void setVelocityX(float xArg);
	void setVelocityY(float yArg);
	void setXY(int xArg, int yArg);
	void init();
	int getX();
	int getY();
	pmem::obj::persistent_ptr<sf::Vector2f> getVelocity();
	sf::CircleShape getBallShape();

private:
	pmem::obj::p<int> x;
	pmem::obj::p<int> y;
	pmem::obj::persistent_ptr<sf::Vector2f> velocity;
};

#endif /* LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_BALL_HPP */
