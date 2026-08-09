#include "App.h"

App::App(HINSTANCE instance) : Imzadi::Game(instance)
{
}

/*virtual*/ App::~App()
{
}

/*virtual*/ bool App::PostInit()
{
	if (!Imzadi::Game::PostInit())
		return false;

	//this->wormholeTree.LoadFromDisk("");

	return true;
}