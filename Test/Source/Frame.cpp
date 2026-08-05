#include "Frame.h"
#include "App.h"
#include "Canvas.h"
#include "Common.h"
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/msgdlg.h>

Frame::Frame() : wxFrame(nullptr, wxID_ANY, "Wormhole Generator Test App", wxDefaultPosition, wxSize(1500, 1200))
{
	wxMenu* programMenu = new wxMenu();
	programMenu->Append(new wxMenuItem(programMenu, ID_GenerateTree, "Generate Tree", "Generate a wormhole tree."));
	programMenu->Append(new wxMenuItem(programMenu, ID_GenerateSurfacePoints, "Generate Surface Points", "Generate a sufficiently large set of surface normal points from which a mesh can be generated."));
	programMenu->Append(new wxMenuItem(programMenu, ID_GenerateGraph, "Generate Graph", "Generate a graph from surface points."));
	programMenu->Append(new wxMenuItem(programMenu, ID_GenerateMesh, "Generate Mesh", "Generate a mesh from the graph."));
	programMenu->AppendSeparator();
	programMenu->Append(new wxMenuItem(programMenu, ID_Exit, "Exit", "Go do something else with your life."));

	wxMenuBar* menuBar = new wxMenuBar();
	menuBar->Append(programMenu, "Program");
	this->SetMenuBar(menuBar);

	this->SetStatusBar(new wxStatusBar(this));

	this->canvas = new Canvas(this);

	wxBoxSizer* boxSizer = new wxBoxSizer(wxVERTICAL);
	boxSizer->Add(this->canvas, 1, wxGROW | wxALL, 0);
	this->SetSizer(boxSizer);

	this->Bind(wxEVT_MENU, &Frame::OnExit, this, ID_Exit);
	this->Bind(wxEVT_MENU, &Frame::OnGenerate, this, ID_GenerateTree);
	this->Bind(wxEVT_MENU, &Frame::OnGenerate, this, ID_GenerateSurfacePoints);
	this->Bind(wxEVT_MENU, &Frame::OnGenerate, this, ID_GenerateGraph);
	this->Bind(wxEVT_MENU, &Frame::OnGenerate, this, ID_GenerateMesh);
}

/*virtual*/ Frame::~Frame()
{
}

void Frame::OnExit(wxCommandEvent& event)
{
	this->Close(true);
}

void Frame::OnGenerate(wxCommandEvent& event)
{
	switch (event.GetId())
	{
		case ID_GenerateTree:
		{
			WormholeGenerator::WormholeTree::GeneratorConfig config;

			config.random = &wxGetApp().random;
			config.branchProbability = 0.05;
			config.maxAngleDeviation = M_PI / 4.0;
			config.maxDepth = 8;
			config.minDistBetweenNodes = 1.0;
			config.maxDistBetweenNodes = 2.0;
			config.maxBranchFactor = 2;

			if (!wxGetApp().wormholeTree.Generate(config))
			{
				wxMessageBox("Failed to generate wormhole!", "Error!", wxICON_ERROR | wxOK, this);
			}

			break;
		}
		case ID_GenerateSurfacePoints:
		{
			wxGetApp().wormholeTree.GenerateSurfacePoints([](const WormholeGenerator::SurfacePoint& surfacePoint) -> void
				{
					wxGetApp().surfacePointArray.push_back(surfacePoint);
				});

			break;
		}
		case ID_GenerateGraph:
		{
			wxGetApp().wormholeTree.PopulateGraphWithSurfacePoints(wxGetApp().graph);

			wxGetApp().graph.AutoCompleteEdges(0.3, 8);

			wxGetApp().graph.GenerateEdgeSet(wxGetApp().edgeSet);

			break;
		}
		case ID_GenerateMesh:
		{
			if (!wxGetApp().graph.ToPolygonMesh(wxGetApp().mesh))
			{
				wxMessageBox("Failed to generate mesh!", "Error!", wxICON_ERROR | wxOK, this);
			}

			break;
		}
	}
}