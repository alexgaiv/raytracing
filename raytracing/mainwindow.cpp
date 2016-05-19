#include "mainwindow.h"
#include "transform.h"
#include "texture.h"

#pragma comment(lib, "Winmm.lib")

using namespace std;

MainWindow::MainWindow()
{
	this->Create("Ray tracing (esc to quit)", CW_USEDEFAULT, CW_USEDEFAULT, 800, 600);
	//this->CreateFullScreen("");
	needRedraw = false;

	timeBeginPeriod(1);
}

void MainWindow::InitGeometry()
{
	const int numSpheres = 3;
	const int numPlanes = 6;
	char name[100] = "";

	struct sphere_t {
		Vector3f center;
		float radius;
		Color3f color;
	};

	struct plane_t {
		Vector3f normal;
		float D;
		Color3f color;
	};

	sphere_t spheres[numSpheres] =
	{
		{ Vector3f(-13, 0, -58), 5, Color3f(1, 0, 0) },
		{ Vector3f(0, 0, -54), 5, Color3f(0, 1, 0) },
		{ Vector3f(-6, 0, -20), 5, Color3f(1, 1, 1) },
	};
	
	plane_t planes[numPlanes] =
	{
	
		{ Vector3f(0, 1, 0), 5, Color3f(1,1,1) },
		{ Vector3f(0, 0, 1), 120, Color3f(1,0,0) },
		{ Vector3f(1, 0, 0), 30, Color3f(0,0,1) },
		{ Vector3f(-1, 0, 0), 30, Color3f(0,1,0) },
		{ Vector3f(0, -1, 0), 30, Color3f(1,1,0) },
		{ Vector3f(0, 0, -1), 50, Color3f(1,0,1) }
	};


	for (int i = 0; i < numSpheres; i++)
	{
		StringCchPrintf(name, 100, "spheres[%d].center", i);
		program->Uniform(name, 1, spheres[i].center.data);
		StringCchPrintf(name, 100, "spheres[%d].radius", i);
		program->Uniform(name, spheres[i].radius);
		StringCchPrintf(name, 100, "spheres[%d].base.color", i);
		program->Uniform(name, 1, spheres[i].color.data);
		StringCchPrintf(name, 100, "spheres[%d].base.specPower", i);
		program->Uniform(name, 40.0f);
		StringCchPrintf(name, 100, "spheres[%d].base.material", i);
		program->Uniform(name, 3);
		StringCchPrintf(name, 100, "spheres[%d].base.refractIndex", i);
		program->Uniform(name, 0.2f);
	}

	program->Uniform("spheres[2].base.material", 4);
	program->Uniform("spheres[2].base.refractIndex", 0.9f);

	for (int i = 0; i < numPlanes; i++)
	{
		StringCchPrintf(name, 100, "planes[%d].base.normal", i);
		program->Uniform(name, 1, planes[i].normal.data);
		StringCchPrintf(name, 100, "planes[%d].D", i);
		program->Uniform(name, planes[i].D);
		StringCchPrintf(name, 100, "planes[%d].base.color", i);
		program->Uniform(name, 1, planes[i].color.data);
		StringCchPrintf(name, 100, "planes[%d].base.specPower", i);
		program->Uniform(name, 40.0f);
		StringCchPrintf(name, 100, "planes[%d].base.material", i);
		program->Uniform(name, 2);
		StringCchPrintf(name, 100, "planes[%d].base.refractIndex", i);
		program->Uniform(name, 0.3f);
	}


	program->Uniform("lightSource", 1, Vector3f(5, 20, 5).data);
	program->Uniform("lightAmbient", 1, Vector3f(0.1f).data);
	program->Uniform("backColor", 0.0f, 0.0f, 0.0f);
}

void MainWindow::OnCreate()
{
	glewInit();
	glClearColor(1, 1, 1, 1);

	if (GLEW_ARB_vertex_array_object) {
		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
	}

	camera.type = CAM_FREE;
	camera.SetPosition(10, 2, 0);
	camera.RotateY(20.0f);

	program = new ProgramObject(m_rc, "shaders/shader.vert.glsl", "shaders/shader.frag.glsl");
	program->Use();

	quad = new Mesh(m_rc);
	quad->LoadObj("quad.obj");
	
	InitGeometry();

	SetTimer(m_hwnd, 1, 15, NULL);
}

void MainWindow::OnDisplay()
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	m_rc->PushModelView();
		camera.ApplyTransform(m_rc);
		quad->Draw();
	m_rc->PopModelView();
}

void MainWindow::OnSize(int w, int h)
{
	glViewport(0, 0, w, h);
	
	RECT r = { };
	GetClientRect(m_hwnd, &r);
	Matrix44f orthoMat = Ortho2D(0, (float)r.right, 0, (float)r.bottom);
	m_rc->SetProjection(orthoMat);

	float fov = 45.0f;
	program->Uniform("ImageWidth", r.right);
	program->Uniform("ImageHeight", r.bottom);
	program->Uniform("TanHalfFov", (float)tan(DEG_TO_RAD(fov * 0.5)));

	if (r.right != 0 && r.bottom != 0) {
		Vector3f *verts = (Vector3f *)quad->vertices->Map(GL_READ_WRITE);
		if (verts)
		{
			for (int i = 0, n = quad->GetVerticesCount(); i < n; i++)
			{
				if (verts[i].x != 0.0f)
					verts[i].x = verts[i].x > 0 ? (float)r.right : (float)-r.right;
				if (verts[i].y != 0.0f)
					verts[i].y = verts[i].y > 0 ? (float)r.bottom : (float)-r.bottom;
			}
		}
		quad->vertices->Unmap();
	}
	Redraw();
}

void MainWindow::OnTimer()
{
	static int lastTime = timeGetTime();
	int curTime = timeGetTime();
	float kt = (float)(curTime - lastTime) / 15.0f;
	lastTime = curTime;

	POINT cursor = { };
	RECT wndRect = { };
	GetCursorPos(&cursor);
	GetWindowRect(m_hwnd, &wndRect);

	bool centerCursor =
		cursor.x > wndRect.right ||
		cursor.y > wndRect.bottom ||
		cursor.x < wndRect.left ||
		cursor.y < wndRect.top;

	if (centerCursor) {
		RECT clientRect = { };
		GetClientRect(m_hwnd, &clientRect);
		int centerX = clientRect.right / 2;
		int centerY = clientRect.bottom / 2;

		POINT cursorPos = { centerX, centerY };
		ClientToScreen(m_hwnd, &cursorPos);
		SetCursorPos(cursorPos.x, cursorPos.y);
	}

	const float step = 0.6f * kt;
	bool r1 = true, r2 = true;

	if (GetAsyncKeyState('W'))
		camera.MoveZ(-step);
	else if (GetAsyncKeyState('S'))
		camera.MoveZ(step);
	else r1 = false;

	if (GetAsyncKeyState('A'))
		camera.MoveX(-step);
	else if (GetAsyncKeyState('D'))
		camera.MoveX(step);
	else r2 = false;

	//Vector3f pos = camera.GetPosition() - Vector3f(0,5,0);
	//program->Uniform("lightSource", 1, pos.data);

	if (r1 || r2 || needRedraw) {
		Redraw();
		needRedraw = false;
	}
}

void MainWindow::OnMouseMove(UINT keysPressed, int x, int y)
{
	SetCursor(NULL);
	static bool centerCursor = false;
	const float angle = 0.04f;

	if (centerCursor) {
		centerCursor = false;
		return;
	}

	RECT clientRect = { };
	GetClientRect(m_hwnd, &clientRect);
	int centerX = clientRect.right / 2;
	int centerY = clientRect.bottom / 2;

	camera.RotateX(angle * (centerY - y));
	camera.RotateY(angle * (centerX - x));

	POINT cursorPos = { centerX, centerY };
	ClientToScreen(m_hwnd, &cursorPos);
	SetCursorPos(cursorPos.x, cursorPos.y);
	centerCursor = true;
	needRedraw = true;
}

void MainWindow::OnDestroy()
{
	timeEndPeriod(1);
	delete program;
	delete quad;
	PostQuitMessage(0);
}