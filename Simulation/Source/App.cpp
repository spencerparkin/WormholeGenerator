#include "App.h"
#include "CustomAssetCache.h"

App::App(HINSTANCE instance) : Imzadi::Game(instance)
{
}

/*virtual*/ App::~App()
{
}

/*virtual*/ bool App::PostInit()
{
	this->assetCache.Set(new CustomAssetCache());

	if (!Imzadi::Game::PostInit())
		return false;

	// STPTODO: Make a custom asset type for this.
	//this->wormholeTree.LoadFromDisk("");

	return true;
}