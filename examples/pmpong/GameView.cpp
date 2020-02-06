// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2018, Intel Corporation */

#include "GameView.hpp"

GameView::GameView(sf::Font &font)
{
	sf::Color elementsColor(224, 224, 224);
	scoreP1.setFont(font);
	scoreP2.setFont(font);
	scoreP1.setCharacterSize(GAMEVIEW_SCORE_FONTSIZE);
	scoreP2.setCharacterSize(GAMEVIEW_SCORE_FONTSIZE);

	scoreP1.setPosition(WINDOW_WIDTH / 2 - SCORE_VIEW_OFFSET,
			    SCORE_VIEW_OFFSET);
	scoreP2.setPosition(WINDOW_WIDTH / 2 + SCORE_VIEW_OFFSET -
				    scoreP2.getGlobalBounds().width,
			    SCORE_VIEW_OFFSET);
	scoreP1.setFillColor(sf::Color::Green);
	scoreP2.setFillColor(sf::Color::Green);

	upperLine.setPosition(VERTICAL_LINE_OFFSET,
			      scoreP1.getPosition().y + HORIZONAL_LINE_OFFSET);
	upperLine.setSize(sf::Vector2f(WINDOW_WIDTH - 2 * VERTICAL_LINE_OFFSET,
				       LINE_THICKNESS));
	upperLine.setFillColor(elementsColor);

	downLine.setPosition(VERTICAL_LINE_OFFSET,
			     WINDOW_HEIGHT - HORIZONAL_LINE_OFFSET);
	downLine.setSize(sf::Vector2f(WINDOW_WIDTH - 2 * VERTICAL_LINE_OFFSET +
					      LINE_THICKNESS,
				      LINE_THICKNESS));
	downLine.setFillColor(elementsColor);

	leftLine.setPosition(VERTICAL_LINE_OFFSET,
			     scoreP1.getPosition().y + HORIZONAL_LINE_OFFSET);
	leftLine.setSize(sf::Vector2f(
		LINE_THICKNESS,
		WINDOW_HEIGHT -
			(scoreP1.getPosition().y + 2 * HORIZONAL_LINE_OFFSET)));
	leftLine.setFillColor(elementsColor);

	rightLine.setPosition(WINDOW_WIDTH - VERTICAL_LINE_OFFSET,
			      scoreP1.getPosition().y + HORIZONAL_LINE_OFFSET);
	rightLine.setSize(sf::Vector2f(
		LINE_THICKNESS,
		WINDOW_HEIGHT -
			(scoreP1.getPosition().y + 2 * HORIZONAL_LINE_OFFSET)));
	rightLine.setFillColor(elementsColor);

	court.setPosition(VERTICAL_LINE_OFFSET + LINE_THICKNESS,
			  scoreP1.getPosition().y + HORIZONAL_LINE_OFFSET);
	court.setSize(sf::Vector2f(
		WINDOW_WIDTH - 2 * VERTICAL_LINE_OFFSET,
		WINDOW_HEIGHT -
			(scoreP1.getPosition().y + 2 * HORIZONAL_LINE_OFFSET)));
	court.setFillColor(sf::Color(60, 132, 48));

	ballShape.setRadius(BALL_SIZE);
	ballShape.setPosition(sf::Vector2f(0, 0));
	ballShape.setFillColor(elementsColor);

	rightPaddleShape.setSize(sf::Vector2f(PADDLE_WIDTH, PADDLE_HEIGHT));
	leftPaddleShape.setSize(sf::Vector2f(PADDLE_WIDTH, PADDLE_HEIGHT));
	leftPaddleShape.setPosition(sf::Vector2f(0, 0));
	rightPaddleShape.setPosition(sf::Vector2f(0, 0));
	leftPaddleShape.setFillColor(sf::Color::Red);
	rightPaddleShape.setFillColor(sf::Color::Red);
}

GameView::~GameView()
{
}

void
GameView::prepareView(PongGameStatus &gameStatus)
{
	scoreP1.setString(std::to_string(gameStatus.getPlayer1()->getPoints()));
	scoreP2.setString(std::to_string(gameStatus.getPlayer2()->getPoints()));

	ballShape.setPosition(
		sf::Vector2f((float)gameStatus.getBall()->getX(),
			     (float)gameStatus.getBall()->getY()));
	leftPaddleShape.setPosition(
		sf::Vector2f((float)gameStatus.getPlayer1()->getX(),
			     (float)gameStatus.getPlayer1()->getY()));
	rightPaddleShape.setPosition(
		sf::Vector2f((float)gameStatus.getPlayer2()->getX(),
			     (float)gameStatus.getPlayer2()->getY()));
}

void
GameView::displayView(sf::RenderWindow *gameWindow)
{
	gameWindow->clear();

	gameWindow->draw(court);
	gameWindow->draw(upperLine);
	gameWindow->draw(leftLine);
	gameWindow->draw(downLine);
	gameWindow->draw(rightLine);
	gameWindow->draw(scoreP1);
	gameWindow->draw(scoreP2);
	gameWindow->draw(ballShape);
	gameWindow->draw(leftPaddleShape);
	gameWindow->draw(rightPaddleShape);

	gameWindow->display();
}
