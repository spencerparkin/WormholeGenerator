#include "Canvas.h"

int Canvas::attributeList[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0 };

Canvas::Canvas(wxWindow* parent) : wxGLCanvas(parent, wxID_ANY, attributeList)
{
	this->context = new wxGLContext(this);

	this->Bind(wxEVT_PAINT, &Canvas::OnPaint, this);
	this->Bind(wxEVT_SIZE, &Canvas::OnResize, this);
}

/*virtual*/ Canvas::~Canvas()
{
	delete this->context;
}

void Canvas::OnPaint(wxPaintEvent& event)
{
	this->SetCurrent(*this->context);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glFlush();

	this->SwapBuffers();
}

void Canvas::OnResize(wxSizeEvent& event)
{
	this->SetCurrent(*this->context);

	wxSize size = event.GetSize();
	glViewport(0, 0, size.x, size.y);
}