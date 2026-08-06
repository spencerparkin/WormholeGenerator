#pragma once

#include "WormholeGenerator/WormholeTree.h"
#include "HappyMath/Random.h"
#include "HappyMath/Graph.h"
#include "HappyMath/PolygonMesh.h"
#include <wx/app.h>

class Frame;

class App : public wxApp
{
public:
	App();
	virtual ~App();

	virtual bool OnInit() override;
	virtual int OnExit() override;

	WormholeGenerator::WormholeTree wormholeTree;
	
	HappyMath::Random random;

	int drawFlags;

	Frame* GetFrame() { return this->frame; }

private:
	Frame* frame;
};

wxDECLARE_APP(App);