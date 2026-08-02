#include "App.h"
#include "Frame.h"

wxIMPLEMENT_APP(App);

App::App()
{
	this->frame = nullptr;
}

/*virtual*/ App::~App()
{
}

/*virtual*/ bool App::OnInit()
{
	if (!wxApp::OnInit())
		return false;

	this->frame = new Frame();
	this->frame->Show();

	return true;
}

/*virtual*/ int App::OnExit()
{
	return 0;
}