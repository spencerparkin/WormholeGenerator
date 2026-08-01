#pragma once

#include <wx/frame.h>

class Canvas;

class Frame : public wxFrame
{
public:
	Frame();
	virtual ~Frame();

private:
	void OnExit(wxCommandEvent& event);

	Canvas* canvas;
};