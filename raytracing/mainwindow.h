#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include "glwindow.h"
#include "raytracecamera.h"
#include "shader.h"
#include "mesh.h"

class MainWindow : public GLWindow
{
public:
	MainWindow();
private:
	RaytraceCamera camera;
	ProgramObject *program;
	Mesh *quad;

	bool needRedraw;

	WindowInfoStruct GetWindowInfo()
	{
		WindowInfoStruct wi = { };
		wi.hIcon = wi.hIconSmall = LoadIcon(NULL, IDI_APPLICATION);
		wi.lpClassName = "mainwindow";
		return wi;
	}

	GLRenderingContextParams GetRCParams()
	{
		GLRenderingContextParams params = { };
		params.glrcFlags = GLRC_COMPATIBILITY_PROFILE;
		return params;
	}

	void InitGeometry();

	void OnCreate();
	void OnDisplay();
	void OnSize(int w, int h);
	void OnTimer();
	void OnKeyDown(UINT keyCode) {
		if (keyCode == 27) Destroy();
	}
	void OnMouseMove(UINT keysPressed, int x, int y);
	void OnDestroy();
};

#endif // _MAINWINDOW_H_