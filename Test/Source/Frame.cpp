#include "Frame.h"
#include "App.h"
#include "Canvas.h"
#include "Common.h"
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/progdlg.h>

Frame::Frame() : wxFrame(nullptr, wxID_ANY, "Wormhole Generator Test App", wxDefaultPosition, wxSize(1500, 1200))
{
	wxMenu* programMenu = new wxMenu();
	programMenu->Append(new wxMenuItem(programMenu, ID_Generate, "Generate Wormhole Tree", "Generate a wormhole tree."));
	programMenu->AppendSeparator();
	programMenu->Append(new wxMenuItem(programMenu, ID_Exit, "Exit", "Go do something else with your life."));

	wxMenu* optionsMenu = new wxMenu();
	optionsMenu->Append(new wxMenuItem(optionsMenu, ID_DRAW_SPLINES, "Draw Splines", "", wxITEM_CHECK));
	optionsMenu->Append(new wxMenuItem(optionsMenu, ID_DRAW_SURFACE_POINTS, "Draw Surface Points", "", wxITEM_CHECK));
	optionsMenu->Append(new wxMenuItem(optionsMenu, ID_DRAW_SURFACE_POINT_NORMALS, "Draw Surface Point Normals", "", wxITEM_CHECK));
	optionsMenu->Append(new wxMenuItem(optionsMenu, ID_DRAW_SURFACE_EDGES, "Draw Surface Edges", "", wxITEM_CHECK));
	optionsMenu->Append(new wxMenuItem(optionsMenu, ID_DRAW_SURFACE_POLYGONS, "Draw Surface Polygons", "", wxITEM_CHECK));
	optionsMenu->Append(new wxMenuItem(optionsMenu, ID_DRAW_NODE_POINTS, "Draw Node Points", "", wxITEM_CHECK));

	wxMenuBar* menuBar = new wxMenuBar();
	menuBar->Append(programMenu, "Program");
	menuBar->Append(optionsMenu, "Options");
	this->SetMenuBar(menuBar);

	this->SetStatusBar(new wxStatusBar(this));

	this->canvas = new Canvas(this);

	wxBoxSizer* boxSizer = new wxBoxSizer(wxVERTICAL);
	boxSizer->Add(this->canvas, 1, wxGROW | wxALL, 0);
	this->SetSizer(boxSizer);

	this->Bind(wxEVT_MENU, &Frame::OnExit, this, ID_Exit);
	this->Bind(wxEVT_MENU, &Frame::OnGenerate, this, ID_Generate);
	this->Bind(wxEVT_MENU, &Frame::OnToggleDrawFlag, this, ID_DRAW_SPLINES);
	this->Bind(wxEVT_MENU, &Frame::OnToggleDrawFlag, this, ID_DRAW_SURFACE_POINTS);
	this->Bind(wxEVT_MENU, &Frame::OnToggleDrawFlag, this, ID_DRAW_SURFACE_POINT_NORMALS);
	this->Bind(wxEVT_MENU, &Frame::OnToggleDrawFlag, this, ID_DRAW_SURFACE_EDGES);
	this->Bind(wxEVT_MENU, &Frame::OnToggleDrawFlag, this, ID_DRAW_SURFACE_POLYGONS);
	this->Bind(wxEVT_MENU, &Frame::OnToggleDrawFlag, this, ID_DRAW_NODE_POINTS);
	this->Bind(wxEVT_UPDATE_UI, &Frame::OnUpdateUI, this, ID_DRAW_SPLINES);
	this->Bind(wxEVT_UPDATE_UI, &Frame::OnUpdateUI, this, ID_DRAW_SURFACE_POINTS);
	this->Bind(wxEVT_UPDATE_UI, &Frame::OnUpdateUI, this, ID_DRAW_SURFACE_POINT_NORMALS);
	this->Bind(wxEVT_UPDATE_UI, &Frame::OnUpdateUI, this, ID_DRAW_SURFACE_EDGES);
	this->Bind(wxEVT_UPDATE_UI, &Frame::OnUpdateUI, this, ID_DRAW_SURFACE_POLYGONS);
	this->Bind(wxEVT_UPDATE_UI, &Frame::OnUpdateUI, this, ID_DRAW_NODE_POINTS);
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
	int originalDrawFlags = wxGetApp().drawFlags;

	// Don't let the canvas try to draw stuff while we're trying to generate it.
	wxGetApp().drawFlags = 0;

	WormholeGenerator::WormholeTree::GeneratorConfig config;

	config.random = &wxGetApp().random;
	config.branchProbability = 0.08;
	config.maxAngleDeviation = M_PI / 4.0;
	config.maxDepth = 32;
	config.minDistBetweenNodes = 1.0;
	config.maxDistBetweenNodes = 2.0;
	config.maxBranchFactor = 2;
	config.autoCompleteEdgesConfig.localityRadius = 0.3;
	config.autoCompleteEdgesConfig.maxDegree = 8;
	config.surfacePointConfig.samplesPerLocation = 16;
	config.surfacePointConfig.numSteps = 32;
	config.surfacePointConfig.wormholeRadius = 0.2;

	class ProgressReporter : public WormholeGenerator::WormholeTree::ProgressReporterInterface
	{
	public:
		ProgressReporter()
		{
			this->dialog = nullptr;
			this->throttle = 0;
		}

		virtual ~ProgressReporter()
		{
		}

		virtual void BeginTask(const std::string& message) override
		{
			this->dialog = new wxProgressDialog("Working...", wxString(message), 1000, wxGetApp().GetFrame(), wxPD_APP_MODAL | wxPD_AUTO_HIDE);
			this->dialog->Show();
		}

		virtual void TaskUpdate(double progress) override
		{
			if (this->throttle++ % 100 == 0)
			{
				double value = progress * 1000.0;
				this->dialog->Update((int)::floor(value));
			}
		}

		virtual void EndTask() override
		{
			delete this->dialog;
			this->dialog = nullptr;
		}

		wxProgressDialog* dialog;
		int throttle;
	};

	ProgressReporter progressReporter;

	if (!wxGetApp().wormholeTree.Generate(config, &progressReporter))
	{
		wxMessageBox("Failed to generate wormhole!", "Error!", wxICON_ERROR | wxOK, this);
	}

	wxGetApp().drawFlags = originalDrawFlags;
}

void Frame::OnToggleDrawFlag(wxCommandEvent& event)
{
	switch (event.GetId())
	{
	case ID_DRAW_SPLINES:
		wxGetApp().drawFlags ^= DF_SPLINES;
		break;
	case ID_DRAW_SURFACE_POINTS:
		wxGetApp().drawFlags ^= DF_SURFACE_POINTS;
		break;
	case ID_DRAW_SURFACE_POINT_NORMALS:
		wxGetApp().drawFlags ^= DF_SURFACE_POINT_NORMALS;
		break;
	case ID_DRAW_SURFACE_EDGES:
		wxGetApp().drawFlags ^= DF_SURFACE_EDGES;
		break;
	case ID_DRAW_SURFACE_POLYGONS:
		wxGetApp().drawFlags ^= DF_SURFACE_POLYGONS;
		break;
	case ID_DRAW_NODE_POINTS:
		wxGetApp().drawFlags ^= DF_NODE_POINTS;
		break;
	}
}

void Frame::OnUpdateUI(wxUpdateUIEvent& event)
{
	switch (event.GetId())
	{
	case ID_DRAW_SPLINES:
		event.Check((wxGetApp().drawFlags & DF_SPLINES) != 0);
		break;
	case ID_DRAW_SURFACE_POINTS:
		event.Check((wxGetApp().drawFlags & DF_SURFACE_POINTS) != 0);
		break;
	case ID_DRAW_SURFACE_POINT_NORMALS:
		event.Check((wxGetApp().drawFlags & DF_SURFACE_POINT_NORMALS) != 0);
		break;
	case ID_DRAW_SURFACE_EDGES:
		event.Check((wxGetApp().drawFlags & DF_SURFACE_EDGES) != 0);
		break;
	case ID_DRAW_SURFACE_POLYGONS:
		event.Check((wxGetApp().drawFlags & DF_SURFACE_POLYGONS) != 0);
		break;
	case ID_DRAW_NODE_POINTS:
		event.Check((wxGetApp().drawFlags & DF_NODE_POINTS) != 0);
		break;
	}
}