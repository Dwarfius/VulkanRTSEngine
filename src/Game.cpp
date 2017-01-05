#include "Game.h"

Game* Game::inst = nullptr;

Game::Game()
{
	inst = this;
}

Game::~Game()
{

}

void Game::Init()
{

}

void Game::Update(float deltaTime)
{

}

void Game::Render()
{
}

void Game::CleanUp()
{

}