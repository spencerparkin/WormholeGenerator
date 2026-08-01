#pragma once

#include <wx/glcanvas.h>

class Canvas : public wxGLCanvas
{
public:
	Canvas(wxWindow* parent);
	virtual ~Canvas();

private:
	void OnPaint(wxPaintEvent& event);
	void OnResize(wxSizeEvent& event);

	wxGLContext* context;
	static int attributeList[];
};