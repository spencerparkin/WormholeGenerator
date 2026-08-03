#pragma once

#include "WormholeGenerator/WormholeTree.h"
#include "HappyMath/Random.h"
#include <wx/app.h>

class Frame;

class App : public wxApp
{
public:
	App();
	virtual ~App();

	virtual bool OnInit() override;
	virtual int OnExit() override;

	WormholeGenerator::WormholeTree WormholeTree;
	HappyMath::Random random;

private:
	Frame* frame;
};

wxDECLARE_APP(App);