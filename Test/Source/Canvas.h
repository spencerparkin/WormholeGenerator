#pragma once

#include <wx/glcanvas.h>
#include <wx/timer.h>
#include "XBoxController.h"
#include "HappyMath/Vector3.h"
#include "HappyMath/Matrix4x4.h"
#include "HappyMath/LineSegment.h"
#include "WormholeGenerator/WormholeTree.h"

class Canvas : public wxGLCanvas
{
public:
	Canvas(wxWindow* parent);
	virtual ~Canvas();

private:
	void OnPaint(wxPaintEvent& event);
	void OnResize(wxSizeEvent& event);
	void OnTimer(wxTimerEvent& event);

	wxGLContext* context;
	static int attributeList[];

	XBoxController controller;

	wxTimer timer;

	class Camera : public XBoxControllerButtonHandler
	{
	public:
		Camera();

		void MakeViewToWorldMatrix(HappyMath::Matrix4x4& viewToWorld) const;
		void Update(XBoxController* controller);

		virtual void OnButtonPressed(DWORD button) override;
		virtual void OnButtonReleased(DWORD button) override;
		virtual void OnButtonDown(DWORD button) override;
		virtual void OnButtonUp(DWORD button) override;

		enum StrafePlane
		{
			XY,
			XZ
		};

		HappyMath::Vector3 eye;
		HappyMath::Vector3 unitLookDir;
		StrafePlane strafePlane;
	};

	std::shared_ptr<Camera> camera;

	std::vector<HappyMath::LineSegment> lineSegmentArray;
};