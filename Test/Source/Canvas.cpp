#include "Canvas.h"
#include "Common.h"
#include "App.h"
#include "HappyMath/Frustum.h"

int Canvas::attributeList[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0 };

//----------------------------------- Canvas -----------------------------------

Canvas::Canvas(wxWindow* parent) : wxGLCanvas(parent, wxID_ANY, attributeList), controller(0), timer(this, ID_Timer)
{
	this->context = new wxGLContext(this);

	this->Bind(wxEVT_PAINT, &Canvas::OnPaint, this);
	this->Bind(wxEVT_SIZE, &Canvas::OnResize, this);
	this->Bind(wxEVT_TIMER, &Canvas::OnTimer, this);

	this->camera = std::make_shared<Camera>();

	this->controller.AddButtonHandler(this->camera);

	this->timer.Start(0);
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

	GLint viewportParams[4] = { 0, 0, 0, 0 };
	glGetIntegerv(GL_VIEWPORT, viewportParams);

	double aspectRatio = double(viewportParams[2]) / double(viewportParams[3]);

	HappyMath::Frustum frustum;
	frustum.SetFromAspectRatio(aspectRatio, M_PI / 3.0, 0.1, 1000.0);

	HappyMath::Matrix4x4 projMatrix;
	frustum.GetToProjectionMatrix(projMatrix);

	HappyMath::Matrix4x4 projMatrixT;
	projMatrixT.Transpose(projMatrix);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMultMatrixd(&projMatrixT.ele[0][0]);

	HappyMath::Matrix4x4 viewToWorld;
	this->camera->MakeViewToWorldMatrix(viewToWorld);

	HappyMath::Matrix4x4 worldToView;
	worldToView.Invert(viewToWorld);

	HappyMath::Matrix4x4 worldToViewT;
	worldToViewT.Transpose(worldToView);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixd(&worldToViewT.ele[0][0]);

	glEnable(GL_DEPTH_TEST);

	glBegin(GL_LINES);

	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(1.0f, 0.0f, 0.0f);

	glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 1.0f, 0.0f);

	glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 1.0f);

	glEnd();

	glPointSize(3.0);

	glBegin(GL_POINTS);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	wxGetApp().wormholeTree.ForEachNode([](const WormholeGenerator::WormholeTree::Node* node) -> void
		{
			const HappyMath::Vector3& point = node->tangentPoint.location;
			glVertex3d(point.x, point.y, point.z);
		});

	glEnd();

	glBegin(GL_LINES);
	glColor4f(0.5f, 0.5f, 0.5f, 1.0f);

	if (this->lineSegmentArray.size() == 0)
	{
		wxGetApp().wormholeTree.ForEachRenderLine(16 /* linesPerCurve */, [this](const HappyMath::LineSegment& line) -> void
			{
				this->lineSegmentArray.push_back(line);
			});
	}

	for (const HappyMath::LineSegment& line : this->lineSegmentArray)
	{
		glVertex3d(line.point[0].x, line.point[0].y, line.point[0].z);
		glVertex3d(line.point[1].x, line.point[1].y, line.point[1].z);
	}

	glEnd();

	if (wxGetApp().surfacePointArray.size() > 0)
	{
		glBegin(GL_POINTS);
		glColor4f(1.0f, 0.0f, 0.0f, 1.0f);

		for (const WormholeGenerator::SurfacePoint& surfacePoint : wxGetApp().surfacePointArray)
		{
			glVertex3d(surfacePoint.location.x, surfacePoint.location.y, surfacePoint.location.z);
		}

		glEnd();
		
		/*
		glBegin(GL_LINES);
		glColor4f(0.0f, 1.0f, 0.0f, 1.0f);

		for (const WormholeGenerator::SurfacePoint& surfacePoint : wxGetApp().surfacePointArray)
		{
			glVertex3d(surfacePoint.location.x, surfacePoint.location.y, surfacePoint.location.z);

			HappyMath::Vector3 tip = surfacePoint.location + 0.1 * surfacePoint.normal;
			glVertex3d(tip.x, tip.y, tip.z);
		}

		glEnd();
		*/
	}

	if (wxGetApp().edgeSet.size() > 0)
	{
		glBegin(GL_LINES);
		glColor4f(0.5f, 0.5f, 1.0f, 1.0f);

		for (auto edge : wxGetApp().edgeSet)
		{
			const HappyMath::Graph::Node* nodeA = wxGetApp().graph.GetNode(edge.i);
			const HappyMath::Graph::Node* nodeB = wxGetApp().graph.GetNode(edge.j);

			const HappyMath::Vector3& vertexA = nodeA->GetVertex();
			const HappyMath::Vector3& vertexB = nodeB->GetVertex();

			glVertex3d(vertexA.x, vertexA.y, vertexA.z);
			glVertex3d(vertexB.x, vertexB.y, vertexB.z);
		}

		glEnd();
	}

	glFlush();

	this->SwapBuffers();
}

void Canvas::OnResize(wxSizeEvent& event)
{
	this->SetCurrent(*this->context);

	wxSize size = event.GetSize();
	glViewport(0, 0, size.x, size.y);
}

void Canvas::OnTimer(wxTimerEvent& event)
{
	this->controller.Update();

	this->camera->Update(&this->controller);

	this->Refresh();
}

//----------------------------------- Canvas::Camera -----------------------------------

Canvas::Camera::Camera()
{
	this->eye.SetComponents(0.0, 0.0, 10.0);
	this->unitLookDir.SetComponents(0.0, 0.0, -1.0);
	this->strafePlane = StrafePlane::XZ;
}

void Canvas::Camera::MakeViewToWorldMatrix(HappyMath::Matrix4x4& viewToWorld) const
{
	HappyMath::Vector3 cameraUp(0.0, 1.0, 0.0);

	viewToWorld.SetAsViewToWorldTransform(this->eye, this->eye + this->unitLookDir, cameraUp);
}

void Canvas::Camera::Update(XBoxController* controller)
{
	// STPTODO: Use delta time here instead of sensativity variables.

	HappyMath::Vector2 leftThumbStick = controller->GetAnalogJoyStick(XINPUT_GAMEPAD_LEFT_THUMB);
	HappyMath::Vector2 rightThumbStick = controller->GetAnalogJoyStick(XINPUT_GAMEPAD_RIGHT_THUMB);

	HappyMath::Matrix4x4 viewToWorld;
	this->MakeViewToWorldMatrix(viewToWorld);

	HappyMath::Vector3 xAxis, yAxis, zAxis;
	viewToWorld.GetAxes(xAxis, yAxis, zAxis);

	double strafeSensativity = 0.1;

	HappyMath::Vector3 eyeDelta;
	
	switch (this->strafePlane)
	{
	case StrafePlane::XY:
		eyeDelta = strafeSensativity * (xAxis * leftThumbStick.x + yAxis * leftThumbStick.y);
		break;
	case StrafePlane::XZ:
		eyeDelta = strafeSensativity * (xAxis * leftThumbStick.x - zAxis * leftThumbStick.y);
		break;
	}

	this->eye += eyeDelta;

	double lookSensativity = 0.02;

	HappyMath::Vector3 lookDirDelta = lookSensativity * (xAxis * rightThumbStick.x + yAxis * rightThumbStick.y);

	this->unitLookDir += lookDirDelta;
	this->unitLookDir.Normalize();
}

/*virtual*/ void Canvas::Camera::OnButtonPressed(DWORD button)
{
}

/*virtual*/ void Canvas::Camera::OnButtonReleased(DWORD button)
{
	if (button == XINPUT_GAMEPAD_A)
	{
		switch (this->strafePlane)
		{
		case StrafePlane::XY:
			this->strafePlane = StrafePlane::XZ;
			break;
		case StrafePlane::XZ:
			this->strafePlane = StrafePlane::XY;
			break;
		}
	}
}

/*virtual*/ void Canvas::Camera::OnButtonDown(DWORD button)
{
}

/*virtual*/ void Canvas::Camera::OnButtonUp(DWORD button)
{
}