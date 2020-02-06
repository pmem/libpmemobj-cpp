// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2018, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_PADDLE_HPP
#define LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_PADDLE_HPP

#include "Ball.hpp"
#include "GameConstants.hpp"
#include <SFML/Graphics.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

class Paddle {

public:
	Paddle();
	Paddle(int x, int y);
	~Paddle();

	void moveUp(int velocity);
	void moveDown(int velocity);
	void addPoint();
	void init();
	void adjustPaddleYtoBall(Ball &ball);
	void collisionWithBall(Ball &ball, bool increaseBallSpeed);

	int getX();
	int getY();
	int getPoints();

	sf::RectangleShape getPaddleShape();

private:
	pmem::obj::p<int> y;
	pmem::obj::p<int> x;
	pmem::obj::p<int> points;

	void setPoints(int pointsArg);
	void setY(int yArg);
	void setX(int xArg);
};

#endif /* LIBPMEMOBJ_CPP_EXAMPLES_PMPONG_PADDLE_HPP */
