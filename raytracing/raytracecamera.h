#ifndef _RAYTRACE_CAMERA_H_
#define _RAYTRACE_CAMERA_H_

#include "camera.h"

class RaytraceCamera : public Camera
{
public:
	Matrix44f GetViewMatrix()
	{
		x.Normalize();
		y = Cross(z, x);
		y.Normalize();
		x = Cross(y, z);
		x.Normalize();

		Matrix44f view;
		view.xAxis = x;
		view.yAxis = y;
		view.zAxis = z;
		view.translate = t;
		return view;
	}
};

#endif // _RAYTRACE_CAMERA_H_