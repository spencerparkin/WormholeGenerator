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
	std::vector<WormholeGenerator::SurfacePoint> surfacePointArray;
	HappyMath::Graph graph;
	std::set<HappyMath::Graph::UnorderedEdge, HappyMath::Graph::UnorderedEdge> edgeSet;
	HappyMath::PolygonMesh mesh;
	HappyMath::Random random;

private:
	Frame* frame;
};

wxDECLARE_APP(App);