#include "App.h"
#include "Frame.h"
#include "Common.h"

wxIMPLEMENT_APP(App);

App::App()
{
	this->frame = nullptr;
	this->drawFlags = DF_SPLINES | DF_SURFACE_POINTS | DF_SURFACE_POLYGONS | DF_NODE_POINTS;
}

/*virtual*/ App::~App()
{
}

/*virtual*/ bool App::OnInit()
{
	if (!wxApp::OnInit())
		return false;

	this->random.SetSeed(1000);

	this->frame = new Frame();
	this->frame->Show();

	return true;
}

/*virtual*/ int App::OnExit()
{
	return 0;
}