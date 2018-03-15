// ==========================================================================
// Barebones OpenGL Core Profile Boilerplate
//    using the GLFW windowing system (http://www.glfw.org)
//
// Loosely based on
//  - Chris Wellons' example (https://github.com/skeeto/opengl-demo) and
//  - Camilla Berglund's example (http://www.glfw.org/docs/latest/quick.html)
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <iterator>
#include "glm/glm.hpp"
#include "ImageBuffer.h"

// Specify that we want the OpenGL core profile before including GLFW headers
#ifndef LAB_LINUX
	#include <glad/glad.h>
#else
	#define GLFW_INCLUDE_GLCOREARB
	#define GL_GLEXT_PROTOTYPES
#endif
#include <GLFW/glfw3.h>

using namespace std;
using namespace glm;
// --------------------------------------------------------------------------
// OpenGL utility and support function prototypes

void QueryGLVersion();
bool CheckGLErrors();

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);

// ==========================================================================
// Structs

struct Sphere
{
	float radius;
	vec3 centre;
	
	vec3 colour;
	int phongExponent;
	
	Sphere(){};
	
	Sphere(float r, vec3 cent, vec3 col, int e)
	{
		radius = r;
		centre = cent;
		colour = col;
		phongExponent = e;
	}
};

struct Triangle
{
	vec3 p1;
	vec3 p2;
	vec3 p3;
	
	vec3 colour;
	int phongExponent;
	
	Triangle(){};
	
	Triangle(vec3 po1, vec3 po2, vec3 po3, vec3 col, int e)
	{
		p1 = po1;
		p2 = po2;
		p3 = po3;
		colour = col;
		phongExponent = e;
	}
};

struct Plane
{
	vec3 normalVector;
	vec3 point;
	
	vec3 colour;
	int phongExponent;
	
	Plane(){};
	
	Plane(vec3 nVec, vec3 poVec, vec3 col, int e)
	{
		normalVector = nVec;
		point = poVec;
		colour = col;
		phongExponent = e;
	}
};

struct MaterialProperties
{
	vec3 normalVector;
	vec3 intersectionPoint;
	
	vec3 colour;
	int phongExponent;
	
	MaterialProperties(){};
	
	MaterialProperties(vec3 nVec, vec3 iPoint, vec3 col, int e)
	{
		normalVector = nVec;
		intersectionPoint = iPoint;
		colour = col;
		phongExponent = e;
	}
};

struct Ray
{
	vec3 startPoint;
	vec3 directionVector;
	
	bool hasIntersected;
	float closestDistance;
	vec3 colour;
	float luminance;
	
	MaterialProperties struckMaterial;
	
	Ray()
	{
		hasIntersected = false;
	};
	
	Ray(vec3 start, vec3 direction, vec3 col)
	{
		startPoint = start;
		directionVector = direction;
		
		hasIntersected = false;
		closestDistance = 0;
		colour = col;
		
		struckMaterial = MaterialProperties();
	}
};

// ==========================================================================
// Function prototypes

bool checkLightIntersections(Ray &thisRay);
void checkAllIntersections(Ray &thisRay);

// ==========================================================================
// Variables

const float EPSILON = 0.000001f;
const int defaultRecursion = 0;
const float defaultMagnification = 1.7;
const vec3 origin = vec3(0, 0, 0);

//Coordinate holders
vector<vec2> pointVectors;
vector<vec3> colorVectors;

//Shape holders
vector<Plane> allPlanes;
vector<Triangle> allTriangles;
vector<Sphere> allSpheres;

ImageBuffer myBuffer = ImageBuffer();

bool drawBuffer = false;
vec3 sceneLight = vec3(0, 0, 0);
string fileName = "Default";

float magnification = defaultMagnification;
int recursion = 0;
// --------------------------------------------------------------------------
// Functions to set up OpenGL shader programs for rendering

struct MyShader
{
	// OpenGL names for vertex and fragment shaders, shader program
	GLuint  vertex;
	GLuint  fragment;
	GLuint  program;

	// initialize shader and program names to zero (OpenGL reserved value)
	MyShader() : vertex(0), fragment(0), program(0)
	{}
} shader;

// load, compile, and link shaders, returning true if successful
bool InitializeShaders()
{
	// load shader source from files
	string vertexSource = LoadSource("vertex.glsl");
	string fragmentSource = LoadSource("fragment.glsl");
	if (vertexSource.empty() || fragmentSource.empty()) return false;

	// compile shader source into shader objects
	shader.vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
	shader.fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

	// link shader program
	shader.program = LinkProgram(shader.vertex, shader.fragment);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate shader-related objects
void DestroyShaders()
{
	// unbind any shader programs and destroy shader objects
	glUseProgram(0);
	glDeleteProgram(shader.program);
	glDeleteShader(shader.vertex);
	glDeleteShader(shader.fragment);
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing geometry data

struct MyGeometry
{
	// OpenGL names for array buffer objects, vertex array object
	GLuint  vertexBuffer;
	GLuint  colourBuffer;
	GLuint  vertexArray;
	GLsizei elementCount;

	// initialize object names to zero (OpenGL reserved value)
	MyGeometry() : vertexBuffer(0), colourBuffer(0), vertexArray(0), elementCount(0)
	{}
} geometry;

// create buffers and fill with geometry data, returning true if successful
bool InitializeGeometry(MyGeometry *geometry)
{
	// three vertex positions and assocated colours of a triangle
	const GLfloat vertices[][2] = {
		{ -.6f, -.4f },
        { 0, .6f },
		{ .6f, -.4f }
	};

	const GLfloat colours[][3] = {
		{ 1.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f }
	};
	geometry->elementCount = 3;

	// these vertex attribute indices correspond to those specified for the
	// input variables in the vertex shader
	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;

	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry->vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// create another one for storing our colours
	glGenBuffers(1, &geometry->colourBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);

	// create a vertex array object encapsulating all our vertex attributes
	glGenVertexArrays(1, &geometry->vertexArray);
	glBindVertexArray(geometry->vertexArray);

	// associate the position array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glVertexAttribPointer(VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(VERTEX_INDEX);

	// associate the colour array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glVertexAttribPointer(COLOUR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(COLOUR_INDEX);

	// unbind our buffers, resetting to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

bool setGeometry()
{
	// these vertex attribute indices correspond to those specified for the
	// input variables in the vertex shader
	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;

	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry.vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry.vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, pointVectors.size()*sizeof(vec2), &pointVectors[0], GL_STATIC_DRAW);

	// create another one for storing our colours
	glGenBuffers(1, &geometry.colourBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry.colourBuffer);
	glBufferData(GL_ARRAY_BUFFER, colorVectors.size()*sizeof(vec3), &colorVectors[0], GL_STATIC_DRAW);


	// create a vertex array object encapsulating all our vertex attributes
	glGenVertexArrays(1, &geometry.vertexArray);
	glBindVertexArray(geometry.vertexArray);

	// associate the position array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry.vertexBuffer);
	glVertexAttribPointer(VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(VERTEX_INDEX);

	// associate the colour array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry.colourBuffer);
	glVertexAttribPointer(COLOUR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(COLOUR_INDEX);

	// unbind our buffers, resetting to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate geometry-related objects
void DestroyGeometry()
{
	// unbind and destroy our vertex array object and associated buffers
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &geometry.vertexArray);
	glDeleteBuffers(1, &geometry.vertexBuffer);
	glDeleteBuffers(1, &geometry.colourBuffer);
}

//--------------------------------

bool isIntersectionCloser(bool prevIntersection, float prevDistance, float distance)
{
	if ( !(prevIntersection) )
	{
		return true;
	}
	else if (prevDistance > distance)
	{
		return true;
	}
	
	return false;
}

vec3 generateColour(Ray &thisRay)
{
	vec3 c_r = thisRay.struckMaterial.colour;
	vec3 c_a = vec3(0.4, 0.4, 0.4);
	vec3 eVec = -thisRay.directionVector;	//The given vector was going to intersection point, we want to flip
	vec3 n_norm = normalize(thisRay.struckMaterial.normalVector);
	
	int p = thisRay.struckMaterial.phongExponent;
	
	vec3 c_l = vec3(0.f, 0.f, 0.f);
	vec3 l = sceneLight - thisRay.struckMaterial.intersectionPoint;
	vec3 l_norm = normalize(l);
	
	vec3 h = normalize(eVec + l) / length(eVec + l);
	
	Ray lightRayCheck = Ray(thisRay.struckMaterial.intersectionPoint, l_norm, vec3(0, 0, 0));
	
	if ( !checkLightIntersections(lightRayCheck) )
	{
		c_l = vec3(1.f, 1.f, 1.f);
	}
	
	vec3 c_p = vec3(0, 0, 0);
	
	if (recursion > 0)
	{
		recursion--;
		
		vec3 reflectedVector = eVec - 2 * dot(eVec, n_norm) * n_norm;
		
		Ray reflectedRay = Ray(thisRay.struckMaterial.intersectionPoint, normalize(reflectedVector), vec3(0, 0, 0));
		
		checkAllIntersections(reflectedRay);
		
		c_p = reflectedRay.colour;
	}

	float maxDotComponent = std::max( 0.f, dot(n_norm, l_norm) );
	vec3 colourComponent1 = c_r * (c_a + (c_l * maxDotComponent) );
	
	float powerComponent = pow( dot(h, n_norm), p);
	vec3 colourComponent2 = powerComponent * c_l * c_p;

	vec3 colour = colourComponent1 + colourComponent2;
	
	return colour;
}

void triangleIntersection(Ray &thisRay, Triangle &thisTriangle, bool lightCheck)
{
	vec3 AB = thisTriangle.p2 - thisTriangle.p1;
	vec3 CB = thisTriangle.p2 - thisTriangle.p3;
	
	vec3 BA = thisTriangle.p1 - thisTriangle.p2;
	vec3 CA = thisTriangle.p1 - thisTriangle.p3;
	
	vec3 direction = thisRay.directionVector;
	vec3 vecPlaneToRayOrigin = thisTriangle.p1 - thisRay.startPoint;
	
	vec3 planeNormal = cross(AB, CB);
	
	float scaleFactor = dot(vecPlaneToRayOrigin, planeNormal) / dot(direction, planeNormal);
	
	if (scaleFactor < 0)
	{//If it intersects backwards, don't care and return ray as is
		return;
	}
	
	vec3 vectorToPlane = scaleFactor * direction;
	float distance = length(vectorToPlane);
	
	vec3 intersection = (thisRay.startPoint + vectorToPlane) - origin;

	//---------
	
	vec3 projABToCB = ( dot(CB, AB) / dot(CB, CB) ) * CB;
	vec3 vA = AB - projABToCB;
	vec3 AI = vectorToPlane - thisTriangle.p1;
	float baryA = 1.f - ( dot(vA, AI) / dot(vA, AB) );
	
	if (baryA < 0 || baryA > 1)
	{
		return;
	}
	
	//---------
	
	vec3 projBAToCA = ( dot(CA, BA) / dot(CA, CA) ) * CA;
	vec3 vB = BA - projBAToCA;
	vec3 BI = vectorToPlane - thisTriangle.p2;
	float baryB = 1.f - ( dot(vB, BI) / dot(vB, BA) );
	
	if (baryB < 0 || baryB + baryA > 1)
	{
		return;
	}
	
	//---------
	
	//If both A and B barycentric coordiantes are within range, C should be as well
	
	if (lightCheck)
	{
		float lightDistance = length(thisRay.startPoint - sceneLight);

		if (lightDistance < distance)
		{//If we hit the light before impacting a surface
			return;
		}
	}
	
	if ( isIntersectionCloser(thisRay.hasIntersected, thisRay.closestDistance, distance) )
	{//If plane is intersecting closer than anything else
		thisRay.hasIntersected = true;
		
		if (lightCheck)
		{
			return;
		}
		
		thisRay.closestDistance = distance;
		thisRay.struckMaterial = MaterialProperties(planeNormal,
													intersection,
													thisTriangle.colour,
													thisTriangle.phongExponent);
		thisRay.colour = generateColour(thisRay);
	}
	
	return;
}

void planeIntersection(Ray &thisRay, Plane &thisPlane, bool lightCheck)
{
	vec3 direction = thisRay.directionVector;
	vec3 vecPlaneToRayOrigin = thisPlane.point - thisRay.startPoint;
	vec3 planeNormal = thisPlane.normalVector;
	
	float scaleFactor = dot(vecPlaneToRayOrigin, planeNormal) / dot(direction, planeNormal);
	
	if (scaleFactor < 0)
	{//If it intersects backwards, don't care and return ray as is
		return;
	}
	
	vec3 vectorToPlane = scaleFactor * thisRay.directionVector;
	float distance = length(vectorToPlane);
	
	vec3 intersection = (thisRay.startPoint + vectorToPlane) - origin;
	
	if (lightCheck)
	{
		float lightDistance = length(thisRay.startPoint - sceneLight);

		if (lightDistance < distance)
		{//If we hit the light before impacting a surface
			return;
		}
	}
	
	if ( isIntersectionCloser(thisRay.hasIntersected, thisRay.closestDistance, distance) )
	{//If plane is intersecting closer than anything else
		thisRay.hasIntersected = true;
		
		if (lightCheck)
		{
			return;
		}
		
		thisRay.closestDistance = distance;
		thisRay.struckMaterial = MaterialProperties(planeNormal,
													intersection,
													thisPlane.colour,
													thisPlane.phongExponent);
		
		thisRay.colour = generateColour(thisRay);
	}
	
	return;

}

void sphereIntersection(Ray &thisRay, Sphere &thisSphere, bool lightCheck)
{
	vec3 OC = thisSphere.centre - thisRay.startPoint;	//Ray Origin to Centre
	
	float radiusSquared = thisSphere.radius * thisSphere.radius;
	
	float OCDotDirection = dot(OC, thisRay.directionVector);
	
	if (OCDotDirection < 0)
	{//Dot is negative when angle > 90 between viewing direction and sphere centre
		//i.e. sphere is behind us
		return;
	}
	
	if (dot(OC, OC) < radiusSquared)
	{//We are inside the sphere
		return;
	}
	
	vec3 a = OC - OCDotDirection * thisRay.directionVector;
	
	float aSquared = dot(a, a);
	
	if (aSquared > radiusSquared)
	{//Misses the sphere
		return;
	}
	
	float h = sqrt(radiusSquared - aSquared);
	
	vec3 interOne = a - h * thisRay.directionVector;
	
	vec3 intersection = thisSphere.centre + interOne;
	vec3 normal = interOne / thisSphere.radius;
	
	vec3 intersectionPoint = intersection - origin;
	
	
	float distance = length(intersectionPoint - thisRay.startPoint);
	
	if (lightCheck)
	{
		float lightDistance = length(thisRay.startPoint - sceneLight);

		if (lightDistance < distance)
		{//If we hit the light before impacting a surface
			return;
		}
	}
	
	if ( isIntersectionCloser(thisRay.hasIntersected, thisRay.closestDistance, distance) )
	{//If plane is intersecting closer than anything else
		thisRay.hasIntersected = true;
		
		if (lightCheck)
		{
			return;
		}
		
		thisRay.closestDistance = distance;
		thisRay.struckMaterial = MaterialProperties(normal,
													intersectionPoint,
													thisSphere.colour,
													thisSphere.phongExponent);
		thisRay.colour = generateColour(thisRay);
	}
	
	return;
}

bool checkLightIntersections(Ray &thisRay)
{
	for (unsigned int i = 0; i < allPlanes.size(); i++)
	{
		planeIntersection(thisRay, allPlanes[i], true);
		
		if (thisRay.hasIntersected)
		{//If we're checking to see if the light is interrupted, we don't care past that it is
		//Checking regularly because these if checks are cheaper than checking an extra intersection
			return true;
		}
	}
	
	for (unsigned int i = 0; i < allTriangles.size(); i++)
	{
		triangleIntersection(thisRay, allTriangles[i], true);
		
		if (thisRay.hasIntersected)
		{
			return true;
		}
	}
	
	for (unsigned int i = 0; i < allSpheres.size(); i++)
	{
		sphereIntersection(thisRay, allSpheres[i], true);
		
		if (thisRay.hasIntersected)
		{
			return true;
		}
	}
	
	return false;
}

void checkAllIntersections(Ray &thisRay)
{
	for (unsigned int i = 0; i < allPlanes.size(); i++)
	{
		planeIntersection(thisRay, allPlanes[i], false);
	
	}
	
	for (unsigned int i = 0; i < allTriangles.size(); i++)
	{
		triangleIntersection(thisRay, allTriangles[i], false);
	}
	
	for (unsigned int i = 0; i < allSpheres.size(); i++)
	{
		sphereIntersection(thisRay, allSpheres[i], false);
	}
	
	return;
}

void generateAllRays()
{
	myBuffer.Initialize();
	
	for(int x = 0; x<1024; x++)
	{
		for(int y = 0; y<768; y++)
		{
			Ray currentRay = Ray();
			currentRay.startPoint = origin;
			
			//Assume z direction vector as 1
			currentRay.directionVector.z = -1.f;
			
			currentRay.directionVector.x = ( (x - 512.f) / 512.f ) / magnification;
			currentRay.directionVector.y = ( (y - 384.f) / 512.f ) / magnification;
			
			currentRay.directionVector = normalize(currentRay.directionVector);
			
			//Default colouring for testing
			//currentRay.colour = vec3( (1024.f - x) / 1024.f, (1024.f - y) / 1024.f, 1.f - ((1024.f - y) / 1024.f));
			
			recursion = defaultRecursion;
			
			checkAllIntersections(currentRay);
			
			myBuffer.SetPixel(x, y, currentRay.colour);
		}
	}
}

bool generateStart()
{
	pointVectors.clear();
	colorVectors.clear();
	
	pointVectors.push_back(vec2(-.6f, -.4f));
	pointVectors.push_back(vec2(   0, -.6f));
	pointVectors.push_back(vec2(-.3f, -.4f));
	
	colorVectors.push_back(vec3(1.0f, 0.0f, 0.0f));
	colorVectors.push_back(vec3(0.0f, 1.0f, 0.0f));
	colorVectors.push_back(vec3(0.0f, 0.0f, 1.0f));
	
	geometry.elementCount = 3;
	return setGeometry();
}

void generateSceneOne()
{
	sceneLight = vec3(0.f, 2.5f, -7.75f);

	//Reflective grey sphere
	allSpheres.push_back(Sphere(0.825,
								vec3(0.9, -1.925, -6.69),
								vec3(0.5, 0.5, 0.5),
								32
								)
						);

	//Blue pyramid	
	allTriangles.push_back(Triangle(vec3(-0.4, -2.75, -9.55),
									vec3(-0.93, 0.55, -8.51),
									vec3(0.11, -2.75, -7.98),
									vec3(0, 0.69, 0.82),
									4
									)
							);

	allTriangles.push_back(Triangle(vec3(0.11, -2.75, -7.98),
									vec3(-0.93, 0.55, -8.51),
									vec3(-1.46, -2.75, -7.47),
									vec3(0, 0.69, 0.82),
									4
									)
							);
							
	allTriangles.push_back(Triangle(vec3(-1.46, -2.75, -7.47),
									vec3(-0.93, 0.55, -8.51),
									vec3(-1.97, -2.75, -9.04),
									vec3(0, 0.69, 0.82),
									4
									)
							);
							
	allTriangles.push_back(Triangle(vec3(-1.97, -2.75, -9.04),
									vec3(-0.93, 0.55, -8.51),
									vec3(-0.4, -2.75, -9.55),
									vec3(0, 0.69, 0.82),
									4
									)
							);

	//Ceiling
	allTriangles.push_back(Triangle(vec3(2.75, 2.75, -10.5),
									vec3(2.75, 2.75, -5),
									vec3(-2.75, 2.75, -5),
									vec3(0.6, 0.6, 0.6),
									2
									)
							);
							
	allTriangles.push_back(Triangle(vec3(-2.75, 2.75, -10.5),
									vec3(2.75, 2.75, -10.5),
									vec3(-2.75, 2.75, -5),
									vec3(0.6, 0.6, 0.6),
									2
									)
							);

	//Green right wall
	allTriangles.push_back(Triangle(vec3(2.75, 2.75, -5),
									vec3(2.75, 2.75, -10.5),
									vec3(2.75, -2.75, -10.5),
									vec3(0, 1, 0),
									1
									)
							);
							
	allTriangles.push_back(Triangle(vec3(2.75, -2.75, -5),
									vec3(2.75, 2.75, -5),
									vec3(2.75, -2.75, -10.5),
									vec3(0, 1, 0),
									1
									)
							);

	//Red left wall
	allTriangles.push_back(Triangle(vec3(-2.75, -2.75, -5),
									vec3(-2.75, -2.75, -10.5),
									vec3(-2.75, 2.75, -10.5),
									vec3(1, 0, 0),
									1
									)
							);
							
	allTriangles.push_back(Triangle(vec3(-2.75, 2.75, -5),
									vec3(-2.75, -2.75, -5),
									vec3(-2.75, 2.75, -10.5),
									vec3(1, 0, 0),
									1
									)
							);

	//Floor
	allTriangles.push_back(Triangle(vec3(2.75, -2.75, -5),
									vec3(2.75, -2.75, -10.5),
									vec3(-2.75, -2.75, -10.5),
									vec3(0.6, 0.6, 0.6),
									1
									)
							);
							
	allTriangles.push_back(Triangle(vec3(-2.75, -2.75, -5),
									vec3(2.75, -2.75, -5),
									vec3(-2.75, -2.75, -10.5),
									vec3(0.6, 0.6, 0.6),
									1
									)
							);

	//Back wall
	allPlanes.push_back(Plane(vec3(0, 0, 1),
								vec3(0, 0, -10.5),
								vec3(0.4, 0.4, 0.4),
								1
								)
						);

	generateAllRays();
}

void generateSceneTwo()
{
	sceneLight = vec3(4.f, 6.f, -1.f);

	//Floor
	allPlanes.push_back(Plane(vec3(0, 1, 0),
								vec3(0, -1, 0),
								vec3(0.8, 0.8, 0.8),
								1
								)
						);

	//Back wall
	allPlanes.push_back(Plane(vec3(0, 0, 1),
								vec3(0, 0, -12),
								vec3(0, 0.69, 0.82),
								1
								)
						);


	//Large yellow sphere
	allSpheres.push_back(Sphere(0.5,
								vec3(1, -0.5, -3.5),
								vec3(0.76, 0.79, 0.04),
								8
								)
						);

	//Reflective grey sphere
	allSpheres.push_back(Sphere(0.4,
								vec3(0, 1, -5),
								vec3(0.5, 0.5, 0.5),
								32
								)
						);

	//Metallic purple sphere
	allSpheres.push_back(Sphere(0.25,
								vec3(-0.8, -0.75, -4),
								vec3(0.62, 0.05, 0.66),
								16
								)
						);

//-----------

	//Green cone
	
	allTriangles.push_back(Triangle(vec3(0, -1, -5.8),
									vec3(0, 0.6, -5),
									vec3(0.4, -1, -5.693),
									vec3(0, 1, 0),
									2
									)
							);
	
	allTriangles.push_back(Triangle(vec3(0.4, -1, -5.693),
									vec3(0, 0.6, -5),
									vec3(0.6928, -1, -5.4),
									vec3(0, 1, 0),
									2
									)
							);
	
	allTriangles.push_back(Triangle(vec3(0.6928, -1, -5.4),
									vec3(0, 0.6, -5),
									vec3(0.8, -1, -5),
									vec3(0, 1, 0),
									2
									)
							);
	
	allTriangles.push_back(Triangle(vec3(0.8, -1, -5),
									vec3(0, 0.6, -5),
									vec3(0.6928, -1, -4.6),
									vec3(0, 1, 0),
									2
									)
							);
	
	allTriangles.push_back(Triangle(vec3(0.6928, -1, -4.6),
									vec3(0, 0.6, -5),
									vec3(0.4, -1, -4.307),
									vec3(0, 1, 0),
									2
									)
							);
	
	allTriangles.push_back(Triangle(vec3(0.4, -1, -4.307),
									vec3(0, 0.6, -5),
									vec3(0, -1, -4.2),
									vec3(0, 1, 0),
									2
									)
							);
	
	allTriangles.push_back(Triangle(vec3(0, -1, -4.2),
									vec3(0, 0.6, -5),
									vec3(-0.4, -1, -4.307),
									vec3(0, 1, 0),
									2
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-0.4, -1, -4.307),
									vec3(0, 0.6, -5),
									vec3(-0.6928, -1, -4.6),
									vec3(0, 1, 0),
									2
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-0.6928, -1, -4.6),
									vec3(0, 0.6, -5),
									vec3(-0.8, -1, -5),
									vec3(0, 1, 0),
									2
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-0.8, -1, -5),
									vec3(0, 0.6, -5),
									vec3(-0.6928, -1, -5.4),
									vec3(0, 1, 0),
									2
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-0.6928, -1, -5.4),
									vec3(0, 0.6, -5),
									vec3(-0.4, -1, -5.693),
									vec3(0, 1, 0),
									2
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-0.4, -1, -5.693),
									vec3(0, 0.6, -5),
									vec3(0, -1, -5.8),
									vec3(0, 1, 0),
									2
									)
							);

//--------

	//Shiny red icosahedron
	allTriangles.push_back(Triangle(vec3(-2, -1, -7),
									vec3(-1.276, -0.4472, -6.474),
									vec3(-2.276, -0.4472, -6.149),
									vec3(1, 0, 0),
									32
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-1.276, -0.4472, -6.474),
									vec3(-2, -1, -7),
									vec3(-1.276, -0.4472, -7.526),
									vec3(1, 0, 0),
									32
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-2, -1, -7),
									vec3(-2.276, -0.4472, -6.149),
									vec3(-2.894, -0.4472, -7),
									vec3(1, 0, 0),
									32
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-2, -1, -7),
									vec3(-2.894, -0.4472, -7),
									vec3(-2.276, -0.4472, -7.851),
									vec3(1, 0, 0),
									32
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-2, -1, -7),
									vec3(-2.276, -0.4472, -7.851),
									vec3(-1.276, -0.4472, -7.526),
									vec3(1, 0, 0),
									32
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-1.276, -0.4472, -6.474),
									vec3(-1.276, -0.4472, -7.526),
									vec3(-1.106, 0.4472, -7),
									vec3(1, 0, 0),
									32
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-2.276, -0.4472, -6.149),
									vec3(-1.276, -0.4472, -6.474),
									vec3(-1.724, 0.4472, -6.149),
									vec3(1, 0, 0),
									32
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-2.894, -0.4472, -7),
									vec3(-2.276, -0.4472, -6.149),
									vec3(-2.724, 0.4472, -6.474),
									vec3(1, 0, 0),
									32
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-2.276, -0.4472, -7.851),
									vec3(-2.894, -0.4472, -7),
									vec3(-2.724, 0.4472, -7.526),
									vec3(1, 0, 0),
									32
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-1.276, -0.4472, -7.526),
									vec3(-2.276, -0.4472, -7.851),
									vec3(-1.724, 0.4472, -7.851),
									vec3(1, 0, 0),
									32
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-1.276, -0.4472, -6.474),
									vec3(-1.106, 0.4472, -7),
									vec3(-1.724, 0.4472, -6.149),
									vec3(1, 0, 0),
									32
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-2.276, -0.4472, -6.149),
									vec3(-1.724, 0.4472, -6.149),
									vec3(-2.724, 0.4472, -6.474),
									vec3(1, 0, 0),
									32
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-2.894, -0.4472, -7),
									vec3(-2.724, 0.4472, -6.474),
									vec3(-2.724, 0.4472, -7.526),
									vec3(1, 0, 0),
									32
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-2.276, -0.4472, -7.851),
									vec3(-2.724, 0.4472, -7.526),
									vec3(-1.724, 0.4472, -7.851),
									vec3(1, 0, 0),
									32
									)
							);

	allTriangles.push_back(Triangle(vec3(-1.276, -0.4472, -7.526),
									vec3(-1.724, 0.4472, -7.851),
									vec3(-1.106, 0.4472, -7),
									vec3(1, 0, 0),
									32
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-1.724, 0.4472, -6.149),
									vec3(-1.106, 0.4472, -7),
									vec3(-2, 1, -7),
									vec3(1, 0, 0),
									32
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-2.724, 0.4472, -6.474),
									vec3(-1.724, 0.4472, -6.149),
									vec3(-2, 1, -7),
									vec3(1, 0, 0),
									32
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-2.724, 0.4472, -7.526),
									vec3(-2.724, 0.4472, -6.474),
									vec3(-2, 1, -7),
									vec3(1, 0, 0),
									32
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-1.724, 0.4472, -7.851),
									vec3(-2.724, 0.4472, -7.526),
									vec3(-2.276, -0.4472, -6.149),
									vec3(1, 0, 0),
									32
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-1.106, 0.4472, -7),
									vec3(-1.724, 0.4472, -7.851),
									vec3(-2, 1, -7),
									vec3(1, 0, 0),
									32
									)
							);

	generateAllRays();
}

void generateSceneThree()
{
	sceneLight = vec3(-4, 8.f, -0.5);

	//Floor
	allPlanes.push_back(Plane(vec3(0, 1, 0),
								vec3(0, -1, 0),
								vec3(1, 1, 1),
								1
								)
						);

	//Back wall
	allPlanes.push_back(Plane(vec3(0, 0, 1),
								vec3(0, 0, -12),
								vec3(0, 0.69, 0.82),
								1
								)
						);


	//Body spheres sphere
	allSpheres.push_back(Sphere(0.7,
								vec3(0, -0.5, -4),
								vec3(1, 1, 1),
								1
								)
						);
								
	allSpheres.push_back(Sphere(0.5,
								vec3(0, 0.3, -4),
								vec3(1, 1, 1),
								1
								)
						);
								
	allSpheres.push_back(Sphere(0.3,
								vec3(0, 1.0, -4),
								vec3(1, 1, 1),
								1
								)
						);

	//Buttons
	allSpheres.push_back(Sphere(0.05,
								vec3(0, 0.45, -3.55),
								vec3(0, 0, 0),
								16
								)
						);
								
	allSpheres.push_back(Sphere(0.05,
								vec3(0, 0.3, -3.5),
								vec3(0, 0, 0),
								16
								)
						);
								
	allSpheres.push_back(Sphere(0.05,
								vec3(0, 0.15, -3.55),
								vec3(0, 0, 0),
								16
								)
						);

	//Eyes
	allSpheres.push_back(Sphere(0.07,
								vec3(-0.1, 1.1, -3.8),
								vec3(0, 0, 0),
								32
								)
						);
								
	allSpheres.push_back(Sphere(0.07,
								vec3(0.1, 1.1, -3.8),
								vec3(0, 0, 0),
								32
								)
						);
						
	//Mouth
	allSpheres.push_back(Sphere(0.03,
								vec3(-0.16, 0.94, -3.74),
								vec3(0, 0, 0),
								4
								)
						);
								
	allSpheres.push_back(Sphere(0.03,
								vec3(-0.08, 0.91, -3.72),
								vec3(0, 0, 0),
								4
								)
						);
								
	allSpheres.push_back(Sphere(0.03,
								vec3(0.0, 0.88, -3.7),
								vec3(0, 0, 0),
								4
								)
						);
								
	allSpheres.push_back(Sphere(0.03,
								vec3(0.08, 0.91, -3.72),
								vec3(0, 0, 0),
								4
								)
						);
								
	allSpheres.push_back(Sphere(0.03,
								vec3(0.16, 0.94, -3.74),
								vec3(0, 0, 0),
								4
								)
						);

// Hat--------------

//Hat bottom
	allTriangles.push_back(Triangle(vec3(-0.4, 1.2, -3.6),
									vec3(-0.4, 1.2, -4.4),
									vec3(0.4, 1.2, -4.4),
									vec3(0, 0, 0),
									1
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-0.4, 1.2, -3.6),
									vec3(0.4, 1.2, -4.4),
									vec3(0.4, 1.2, -3.6),
									vec3(0, 0, 0),
									1
									)
							);
							
//Hat front
	allTriangles.push_back(Triangle(vec3(-0.3, 1.2, -3.7),
									vec3(0.3, 1.2, -3.7),
									vec3(-0.3, 1.6, -3.7),
									vec3(0, 0, 0),
									1
									)
							);
	
	allTriangles.push_back(Triangle(vec3(0.3, 1.6, -3.7),
									vec3(0.3, 1.2, -3.7),
									vec3(-0.3, 1.6, -3.7),
									vec3(0, 0, 0),
									1
									)
							);
							
//Hat back
	allTriangles.push_back(Triangle(vec3(-0.3, 1.2, -4.3),
									vec3(0.3, 1.2, -4.3),
									vec3(-0.3, 1.6, -4.3),
									vec3(0, 0, 0),
									1
									)
							);
	
	allTriangles.push_back(Triangle(vec3(0.3, 1.6, -4.3),
									vec3(0.3, 1.2, -4.3),
									vec3(-0.3, 1.6, -4.3),
									vec3(0, 0, 0),
									1
									)
							);

//Hat left
	allTriangles.push_back(Triangle(vec3(-0.3, 1.2, -4.3),
									vec3(-0.3, 1.2, -3.7),
									vec3(-0.3, 1.6, -4.3),
									vec3(0, 0, 0),
									1
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-0.3, 1.6, -4.3),
									vec3(-0.3, 1.6, -3.7),
									vec3(-0.3, 1.2, -3.7),
									vec3(0, 0, 0),
									1
									)
							);
							
//Hat right
	allTriangles.push_back(Triangle(vec3(0.3, 1.2, -4.3),
									vec3(0.3, 1.2, -3.7),
									vec3(0.3, 1.6, -4.3),
									vec3(0, 0, 0),
									1
									)
							);
	
	allTriangles.push_back(Triangle(vec3(0.3, 1.6, -4.3),
									vec3(0.3, 1.6, -3.7),
									vec3(0.3, 1.2, -3.7),
									vec3(0, 0, 0),
									1
									)
							);

//Hat top
	allTriangles.push_back(Triangle(vec3(-0.3, 1.2, -3.6),
									vec3(-0.3, 1.2, -4.4),
									vec3(0.3, 1.2, -4.4),
									vec3(0, 0, 0),
									1
									)
							);
	
	allTriangles.push_back(Triangle(vec3(-0.3, 1.2, -3.6),
									vec3(0.3, 1.2, -4.4),
									vec3(0.3, 1.2, -3.6),
									vec3(0, 0, 0),
									1
									)
							);

//--------------------

//Nose
	//Top
	allTriangles.push_back(Triangle(vec3(-0.05, 1.05, -3.8),
									vec3(0.05, 1.05, -3.8),
									vec3(0, 0.95, -3.5),
									vec3(1, 0.5, 0),
									1
									)
							);

	//Left
	allTriangles.push_back(Triangle(vec3(-0.05, 1.05, -3.8),
									vec3(-0.05, 0.95, -3.8),
									vec3(0, 0.95, -3.5),
									vec3(1, 0.5, 0),
									1
									)
							);
							
	//Right
	allTriangles.push_back(Triangle(vec3(0.05, 1.05, -3.8),
									vec3(0.05, 0.95, -3.8),
									vec3(0, 0.95, -3.5),
									vec3(1, 0.5, 0),
									1
									)
							);
							
	//Bottom
	allTriangles.push_back(Triangle(vec3(-0.05, 0.95, -3.8),
									vec3(0.05, 0.95, -3.8),
									vec3(0, 0.95, -3.5),
									vec3(1, 0.5, 0),
									1
									)
							);

	generateAllRays();
}

void clearAllObjects()
{
	sceneLight = vec3(0, 0, 0);
	
	allPlanes.clear();
	allTriangles.clear();
	allSpheres.clear();
	
	pointVectors.clear();
	colorVectors.clear();
	
	myBuffer.Destroy();
}

// --------------------------------------------------------------------------
// Rendering function that draws our scene to the frame buffer

void RenderScene()
{
	// clear screen to a dark grey colour
	glClearColor(1.f, 1.f, 1.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	
	
	if (drawBuffer == false)
	{
		glUseProgram(shader.program);
		glBindVertexArray(geometry.vertexArray);
		glDrawArrays(GL_TRIANGLES, 0, geometry.elementCount);
	}
	else
	{
		myBuffer.Render();
	}
	
	// reset state to default (no shader or geometry bound)
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}

// --------------------------------------------------------------------------
// GLFW callback functions

// reports GLFW errors
void ErrorCallback(int error, const char* description)
{
	cout << "GLFW ERROR " << error << ":" << endl;
	cout << description << endl;
}

// handles keyboard input events
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
		
	if (key == GLFW_KEY_0  && action == GLFW_PRESS)
    {
		DestroyGeometry();
		clearAllObjects();
		
		drawBuffer = false;
		generateStart();
	}
    
    if (key == GLFW_KEY_1  && action == GLFW_PRESS)
    {
		DestroyGeometry();
		clearAllObjects();
		magnification = defaultMagnification;
		
		drawBuffer = true;
		fileName = "Scene_One";
		generateSceneOne();
	}
	
	if (key == GLFW_KEY_2  && action == GLFW_PRESS)
    {
		DestroyGeometry();
		clearAllObjects();
		magnification = defaultMagnification;
		
		drawBuffer = true;
		fileName = "Scene_Two";
		generateSceneTwo();
	}
	
	if (key == GLFW_KEY_3  && action == GLFW_PRESS)
    {
		DestroyGeometry();
		clearAllObjects();
		magnification = defaultMagnification;
		
		drawBuffer = true;
		fileName = "Scene_Three";
		generateSceneThree();
	}
	
	//Viewing Angle-------------------------------------------
	
	if (key == GLFW_KEY_LEFT  && (action == GLFW_PRESS || action == GLFW_REPEAT) )
    {
		if (drawBuffer)
		{
			if (magnification > 0.1)
			{
				magnification -= 0.1;
				cout << magnification << endl;
			}
			else if (magnification > 0.01)
			{
				magnification -= 0.01;
				cout << magnification << endl;
			}
			
			myBuffer.Destroy();
			generateAllRays();
		}
	}
	
	if (key == GLFW_KEY_RIGHT  && (action == GLFW_PRESS || action == GLFW_REPEAT) )
    {
		if (drawBuffer)
		{
			if (magnification < 1.0)
			{
				magnification += 0.01;
				cout << magnification << endl;
			}
			else if (magnification < 3.0)
			{
				magnification += 0.1;
				cout << magnification << endl;
			}
			
			myBuffer.Destroy();
			generateAllRays();
		}
	}
	
	//Save to file-------------------------------------------
	
	if (key == GLFW_KEY_S  && action == GLFW_PRESS)
    {
		if( !myBuffer.SaveToFile(fileName) )
		{
			cout << "ERROR: Failed to save image to file." << endl;
		}
	}
	
	
}

// ==========================================================================
// PROGRAM ENTRY POINT

int main(int argc, char *argv[])
{
	// initialize the GLFW windowing system
	if (!glfwInit()) {
		cout << "ERROR: GLFW failed to initialize, TERMINATING" << endl;
		return -1;
	}
	glfwSetErrorCallback(ErrorCallback);

	// attempt to create a window with an OpenGL 4.1 core profile context
	GLFWwindow *window = 0;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(1024, 768, "CPSC 453 OpenGL Boilerplate", 0, 0);
	if (!window) {
		cout << "Program failed to create GLFW window, TERMINATING" << endl;
		glfwTerminate();
		return -1;
	}

	// set keyboard callback function and make our context current (active)
	glfwSetKeyCallback(window, KeyCallback);
	glfwMakeContextCurrent(window);

	//Intialize GLAD
	#ifndef LAB_LINUX
	if (!gladLoadGL())
	{
		cout << "GLAD init failed" << endl;
		return -1;
	}
	#endif

	// query and print out information about our OpenGL environment
	QueryGLVersion();

	// call function to load and compile shader programs
	//MyShader shader;
	if (!InitializeShaders()) {
		cout << "Program could not initialize shaders, TERMINATING" << endl;
		return -1;
	}

	// call function to create and fill buffers with geometry data
	/*
	MyGeometry geometry;
	if (!InitializeGeometry(&geometry))
		cout << "Program failed to intialize geometry!" << endl;
		*/
	
	generateStart();

	// run an event-triggered main loop
	while (!glfwWindowShouldClose(window))
	{
		RenderScene();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// clean up allocated resources before exit
	DestroyGeometry();
	DestroyShaders();
	myBuffer.Destroy();
	glfwDestroyWindow(window);
	glfwTerminate();

	cout << "Goodbye!" << endl;
	return 0;
}

// ==========================================================================
// SUPPORT FUNCTION DEFINITIONS

// --------------------------------------------------------------------------
// OpenGL utility functions

void QueryGLVersion()
{
	// query opengl version and renderer information
	string version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	string glslver = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
	string renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

	cout << "OpenGL [ " << version << " ] "
		<< "with GLSL [ " << glslver << " ] "
		<< "on renderer [ " << renderer << " ]" << endl;
}

bool CheckGLErrors()
{
	bool error = false;
	for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
	{
		cout << "OpenGL ERROR:  ";
		switch (flag) {
		case GL_INVALID_ENUM:
			cout << "GL_INVALID_ENUM" << endl; break;
		case GL_INVALID_VALUE:
			cout << "GL_INVALID_VALUE" << endl; break;
		case GL_INVALID_OPERATION:
			cout << "GL_INVALID_OPERATION" << endl; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			cout << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl; break;
		case GL_OUT_OF_MEMORY:
			cout << "GL_OUT_OF_MEMORY" << endl; break;
		default:
			cout << "[unknown error code]" << endl;
		}
		error = true;
	}
	return error;
}

// --------------------------------------------------------------------------
// OpenGL shader support functions

// reads a text file with the given name into a string
string LoadSource(const string &filename)
{
	string source;

	ifstream input(filename.c_str());
	if (input) {
		copy(istreambuf_iterator<char>(input),
			istreambuf_iterator<char>(),
			back_inserter(source));
		input.close();
	}
	else {
		cout << "ERROR: Could not load shader source from file "
			<< filename << endl;
	}

	return source;
}

// creates and returns a shader object compiled from the given source
GLuint CompileShader(GLenum shaderType, const string &source)
{
	// allocate shader object name
	GLuint shaderObject = glCreateShader(shaderType);

	// try compiling the source as a shader of the given type
	const GLchar *source_ptr = source.c_str();
	glShaderSource(shaderObject, 1, &source_ptr, 0);
	glCompileShader(shaderObject);

	// retrieve compile status
	GLint status;
	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetShaderInfoLog(shaderObject, info.length(), &length, &info[0]);
		cout << "ERROR compiling shader:" << endl << endl;
		cout << source << endl;
		cout << info << endl;
	}

	return shaderObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader)
{
	// allocate program object name
	GLuint programObject = glCreateProgram();

	// attach provided shader objects to this program
	if (vertexShader)   glAttachShader(programObject, vertexShader);
	if (fragmentShader) glAttachShader(programObject, fragmentShader);

	// try linking the program with given attachments
	glLinkProgram(programObject);

	// retrieve link status
	GLint status;
	glGetProgramiv(programObject, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
		cout << "ERROR linking shader program:" << endl;
		cout << info << endl;
	}

	return programObject;
}
