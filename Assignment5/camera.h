#ifndef CAMERA_H
#define CAMERA_H

#include "glm/glm.hpp"

using namespace glm;

class Camera{
public:
	vec3 dir;
	vec3 up;
	vec3 right;
	vec3 pos;

	Camera();
	Camera(vec3 _dir, vec3 _pos);

	mat4 getMatrix();
	void cameraRotation(float x, float y);

	float theta;
	float phi;
	float radius;
	float rotationalSpeed = 0.01f;
	float zoomSpeed = 0.1f;

	void leftOrbital();
	void rightOrbital();
	void upOrbital();
	void downOrbital();
	void zoomOut();
	void zoomIn();
	void calculateOrbitalPosition();
};










#endif
