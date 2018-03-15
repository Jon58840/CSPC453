#include "camera.h"
#include <cstdio>

mat4 rotateAbout(vec3 axis, float radians)
{
	mat4 matrix;

	matrix[0][0] = cos(radians) + axis.x*axis.x*(1-cos(radians));
	matrix[1][0] = axis.x*axis.y*(1-cos(radians))-axis.z*sin(radians);
	matrix[2][0] = axis.x*axis.z*(1-cos(radians)) + axis.y*sin(radians);

	matrix[0][1] = axis.y*axis.x*(1-cos(radians)) + axis.z*sin(radians);
	matrix[1][1] = cos(radians) + axis.y*axis.y*(1-cos(radians));
	matrix[2][1] = axis.y*axis.z*(1-cos(radians)) - axis.x*sin(radians);

	matrix[0][2] = axis.z*axis.x*(1-cos(radians)) - axis.y*sin(radians);
	matrix[1][2] = axis.z*axis.y*(1-cos(radians)) + axis.x*sin(radians);
	matrix[2][2] = cos(radians) + axis.z*axis.z*(1-cos(radians));

	return matrix;
}

Camera::Camera():	dir(vec3(0, 0, -1)), 
					right(vec3(1, 0, 0)), 
					up(vec3(0, 1, 0)),
					pos(vec3(0, 0, 0))
{}

Camera::Camera(vec3 _dir, vec3 _pos):dir(normalize(_dir)), pos(_pos)
{
	right = normalize(cross(_dir, vec3(0, 1, 0)));
	up =  normalize(cross(right, _dir));
	
	theta = 90.f;
	phi = 90.f;
	radius = 4.f;
}

/*
	[ Right 0 ]
	[ Up 	0 ]
	[ -Dir	0 ]
	[ 0 0 0 1 ]
*/

mat4 Camera::getMatrix()
{
	mat4 cameraRotation = mat4(
			vec4(right, 0),
			vec4(up, 0),
			vec4(-dir, 0),
			vec4(0, 0, 0, 1));

	mat4 translation = mat4 (
			vec4(1, 0, 0, 0),
			vec4(0, 1, 0, 0), 
			vec4(0, 0, 1, 0),
			vec4(-pos, 1));

	return transpose(cameraRotation)*translation;
}


void Camera::cameraRotation(float x, float y)
{
	mat4 rotateAroundY = rotateAbout(vec3(0, 1, 0), x);
	mat4 rotateAroundX = rotateAbout(right, y);

	dir = normalize(
			rotateAroundX*rotateAroundY*vec4(dir, 0)
			);

	right = normalize(cross(dir, vec3(0, 1, 0)));
	up =  normalize(cross(right, dir));
}


void Camera::leftOrbital()
{
	theta += rotationalSpeed;
	
	if (theta >= 360.f)
	{
		theta -= 360.f;
	}
	
	calculateOrbitalPosition();
}

void Camera::rightOrbital()
{
	theta -= rotationalSpeed;
	
	if (theta <= 0.f)
	{
		theta += 360.f;
	}
	
	calculateOrbitalPosition();
}

void Camera::upOrbital()
{
	phi += rotationalSpeed;
	
	if (phi >= 180.f)
	{
		phi = 180.f;
	}

	calculateOrbitalPosition();
}

void Camera::downOrbital()
{
	phi -= rotationalSpeed;
	
	if (phi <= 0.f)
	{
		phi = 0.f;
	}
	
	calculateOrbitalPosition();
}

void Camera::zoomOut()
{
	if (radius < 6.f)
	{
		radius += zoomSpeed;
	}
	
	calculateOrbitalPosition();
}

void Camera::zoomIn()
{
	if (radius > 0)
	{
		radius -= zoomSpeed;
	}
	
	if (radius <= 0)
	{
		radius = 0;
	}
	
	calculateOrbitalPosition();
}

void Camera::calculateOrbitalPosition()
{
	float x = radius * sin(phi) * cos(theta);
	float z = radius * sin(phi) * sin(theta);
	float y = radius * cos(phi);
	
	pos = vec3(x, y, z);
	dir = normalize(-pos);
}







