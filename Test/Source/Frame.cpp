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
	programMenu->Append(new wxMenuItem(programMenu, ID_Generate, "Generate", "Generate a wormhole."));
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
	this->Bind(wxEVT_MENU, &Frame::OnGenerate, this, ID_Generate);
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
	WormholeGenerator::WormholeTree::GeneratorConfig config;

	config.random = &wxGetApp().random;
	config.branchProbability = 0.0;
	config.maxAngleDeviation = M_PI / 4.0;
	config.maxDepth = 64;
	config.minDistBetweenNodes = 1.0;
	config.maxDistBetweenNodes = 2.0;
	config.maxBranchFactor = 1;

	if (!wxGetApp().WormholeTree.Generate(config))
	{
		wxMessageBox("Failed to generate wormhole", "Error!", wxICON_ERROR | wxOK, this);
	}
}