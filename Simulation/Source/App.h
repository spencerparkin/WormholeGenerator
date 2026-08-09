#pragma once

#include "Game.h"

class App : public Imzadi::Game
{
public:
	App(HINSTANCE instance);
	virtual ~App();

	virtual bool PostInit() override;
};