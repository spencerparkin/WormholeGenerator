#pragma once

#include "Game.h"
#include "WormholeGenerator/WormholeTree.h"

class App : public Imzadi::Game
{
public:
	App(HINSTANCE instance);
	virtual ~App();

	virtual bool PostInit() override;

private:
	WormholeGenerator::WormholeTree wormholeTree;
};