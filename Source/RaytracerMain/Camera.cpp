#include "Camera.h"

using namespace nprt;


Camera::Camera(void)
{
	xResolution = -1;
	yResolution = -1;
}


Camera::~Camera(void)
{
	cameraCenter = Point3d();
	topLeft = Point3d();
	topRight = Point3d();
	bottomLeft = Point3d();
	fovX = 0;
	fovY = 0;
	rotation = 0;
	xResolution = 0;
	yResolution = 0;
}

Camera::Camera(Point3d cameraCenter, Point3d topLeft, Point3d bottomLeft, Point3d topRight, int xResolution, int yResolution)
{
	this->cameraCenter = cameraCenter;
	this->topLeft = topLeft;
	this->bottomLeft = bottomLeft;
	this->topRight = topRight;
	this->xResolution = xResolution;
	this->yResolution = yResolution;
}

void Camera::initialize(const Vector3d& cameraCenter, const Vector3d& lookAt, float fovX)
{
	this->cameraCenter = cameraCenter;
	this->lookAt = lookAt;
	this->fovX = fovX;

	Vector3d dir = lookAt - cameraCenter;
	dir.normalize();
	
	Vector3d axisY(0, 1, 0);
	float angle = acosf(axisY.dotProduct(dir));

	// TODO policzyc topLeft, topRight, bottomLeft na podstawie dir i fovX
}

void Camera::setResolution(int width, int height)
{
	float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
	this->fovY = this->fovX / aspectRatio;
		
	Vector3d centerRayDirection = lookAt - cameraCenter;
	centerRayDirection.normalize();

	//Vector3d rightDirection
}
