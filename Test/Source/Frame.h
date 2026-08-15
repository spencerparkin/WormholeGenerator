#pragma once

#include <wx/frame.h>

class Canvas;

class Frame : public wxFrame
{
public:
	Frame();
	virtual ~Frame();

	Canvas* GetCanvas() { return this->canvas; }

private:
	void OnExit(wxCommandEvent& event);
	void OnGenerate(wxCommandEvent& event);
	void OnToggleDrawFlag(wxCommandEvent& event);
	void OnUpdateUI(wxUpdateUIEvent& event);
	void OnSave(wxCommandEvent& event);
	void OnLoad(wxCommandEvent& event);
	void OnClear(wxCommandEvent& event);
	void OnGenerateImzadiAsset(wxCommandEvent& event);

	Canvas* canvas;
};