#pragma once

#include <wx/frame.h>

enum
{
	ID_Exit = wxID_HIGHEST + 1
};

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