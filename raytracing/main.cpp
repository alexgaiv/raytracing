#include "common.h"
#include "datatypes.h"
#include "mainwindow.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR lpCmdLine, int nCmdShow)
{
	SetCurrentDirectory("../raytracing");
	MainWindow wnd;
	wnd.Show(SW_SHOW);
	wnd.MainLoop();

	return 0;
}