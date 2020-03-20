// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "GameController.hpp"
#include "Pool.hpp"

GameController::GameController()
{
	gameStatus = pmem::obj::make_persistent<PongGameStatus>();
}

GameController::~GameController()
{
	try {
		pmem::obj::transaction::run(
			Pool::getGamePool()->getPoolToTransaction(), [&] {
				pmem::obj::delete_persistent<PongGameStatus>(
					gameStatus);
			});
	} catch (...) {
		std::terminate();
	}
}

void
GameController::gameLoop(bool isSimulation)
{
	sf::RenderWindow *gameWindow = new sf::RenderWindow(
		sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), GAME_NAME);
	gameWindow->setFramerateLimit(FRAMERATE_LIMIT);

	sf::Font font = getFont();

	View *menuView = new MenuView(font);
	View *gameView = new GameView(font);
	View *gameOverView = new GameOverView(font);

	while (gameWindow->isOpen()) {
		sf::Event event;
		while (gameWindow->pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				gameWindow->close();
		}
		gameWindow->clear();
		if (isSimulation) {
			if (gameStatus->getGameState() !=
			    game_state::SIMULATE) {
				resetGameStatus();
				gameStatus->setIsGameToResume(false);
				gameStatus->setGameState(game_state::SIMULATE);
			}
			gameMatchSimulation(gameWindow, gameView);
		} else {
			if (gameStatus->getGameState() == game_state::MATCH) {
				gameMatch(gameWindow, gameView);
			} else if (gameStatus->getGameState() ==
				   game_state::MENU) {
				menu(gameWindow, menuView);
			} else if (gameStatus->getGameState() ==
				   game_state::SIMULATE) {
				gameMatchSimulation(gameWindow, gameView);
			} else if (gameStatus->getGameState() ==
				   game_state::GAME_OVER) {
				gameOver(gameWindow, gameOverView);
			}
		}
	}
	delete menuView;
	delete gameView;
	delete gameOverView;
	delete gameWindow;
}

void
GameController::gameOver(sf::RenderWindow *gameWindow, View *view)
{
	view->prepareView(*gameStatus);
	view->displayView(gameWindow);

	sf::Event event;
	while (gameWindow->pollEvent(event)) {
		if (event.type == sf::Event::KeyPressed) {
			switch (event.key.code) {
				case sf::Keyboard::Return:
					gameStatus->setIsGameToResume(false);
					gameStatus->setGameState(
						game_state::MENU);
					break;
				default:
					break;
			}
		}
		if (event.type == sf::Event::Closed)
			gameWindow->close();
	}
}

void
GameController::menu(sf::RenderWindow *gameWindow, View *view)
{
	view->prepareView(*gameStatus);
	view->displayView(gameWindow);

	sf::Event event;
	while (gameWindow->pollEvent(event)) {
		if (event.type == sf::Event::KeyPressed) {
			handleEventKeypress(event, gameWindow);
		}
		if (event.type == sf::Event::Closed)
			gameWindow->close();
	}
}

void
GameController::handleEventKeypress(sf::Event &event,
				    sf::RenderWindow *gameWindow)
{
	switch (event.key.code) {
		case sf::Keyboard::Up:
			gameStatus->setMenuItem(
				gameStatus->getMenuItem() == 0
					? MENU_ITEMS - 1
					: gameStatus->getMenuItem() - 1);
			break;
		case sf::Keyboard::Down:
			gameStatus->setMenuItem(
				(gameStatus->getMenuItem() + 1) % MENU_ITEMS);
			break;
		case sf::Keyboard::Return:
			if (gameStatus->getMenuItem() == NEW_GAME) {
				resetGameStatus();
				gameStatus->setIsGameToResume(true);
				gameStatus->setGameState(game_state::MATCH);
			} else if (gameStatus->getMenuItem() == RESUME &&
				   gameStatus->getIsGameToResume()) {
				gameStatus->setGameState(game_state::MATCH);
			} else if (gameStatus->getMenuItem() == SIMULATION) {
				resetGameStatus();
				gameStatus->setIsGameToResume(false);
				gameStatus->setGameState(game_state::SIMULATE);
			} else if (gameStatus->getMenuItem() == EXIT) {
				gameWindow->close();
			}
			break;
		default:
			break;
	}
}

void
GameController::gameMatch(sf::RenderWindow *gameWindow, View *view)
{
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space))
		gameStatus->startBall(BALL_PLAYERS_SPEED);
	gameStatus->movePaddles();
	gameStatus->lookForCollisions(true);
	gameStatus->actualizeStatus();

	view->prepareView(*gameStatus);
	view->displayView(gameWindow);

	if (gameStatus->score()) {
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space))
			gameStatus->startBall(BALL_PLAYERS_SPEED);
	}
	if (gameStatus->checkIfAnyPlayerWon()) {
		gameStatus->setGameState(game_state::GAME_OVER);
	} else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
		gameStatus->setGameState(game_state::MENU);
	}
}

void
GameController::gameMatchSimulation(sf::RenderWindow *gameWindow, View *view)
{
	gameStatus->startBall(BALL_COMPUTER_SPEED);
	gameStatus->simulate();
	gameStatus->lookForCollisions(false);
	gameStatus->actualizeStatus();
	if (gameStatus->score())
		gameStatus->startBall(BALL_COMPUTER_SPEED);

	view->prepareView(*gameStatus);
	view->displayView(gameWindow);

	if (gameStatus->checkIfAnyPlayerWon()) {
		gameStatus->setGameState(game_state::GAME_OVER);
	} else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
		gameStatus->setGameState(game_state::MENU);
	}
}

void
GameController::resetGameStatus()
{
	pmem::obj::transaction::run(
		Pool::getGamePool()->getPoolToTransaction(), [&] {
			pmem::obj::delete_persistent<PongGameStatus>(
				gameStatus);
			gameStatus =
				pmem::obj::make_persistent<PongGameStatus>();
		});
}
