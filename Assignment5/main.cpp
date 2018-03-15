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
#include <string>
#include <iterator>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <ctime>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

// specify that we want the OpenGL core profile before including GLFW headers
#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "camera.h"

#define PI 3.14159265359

using namespace std;
using namespace glm;

//Forward definitions
bool CheckGLErrors(string location);
void QueryGLVersion();
string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);

void preRender();

vec2 mousePos;
GLFWwindow* window = 0;
const float orbitalSpeed = 0.005f;
const bool orbitalCamera = false;
const float camSpeed = 0.10f;

Camera cam;

vector<vector<vector<vec3>>> spheres;
vector<vector<vector<vec2>>> sphereMapping;
bool mousePressed = false;

// --------------------------------------------------------------------------
// GLFW callback functions

// reports GLFW errors
void ErrorCallback(int error, const char* description)
{
    cout << "GLFW ERROR " << error << ":" << endl;
    cout << description << endl;
}

// handles keyboard input events
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

	if (!orbitalCamera)
	{
		if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
			cam.pos += cam.dir*camSpeed;
		else if(key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
			cam.pos -= cam.dir*camSpeed;
		else if(key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
			cam.pos += cam.right*camSpeed;
		else if(key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
			cam.pos -= cam.right*camSpeed;
		else if(key == GLFW_KEY_E && (action == GLFW_PRESS || action == GLFW_REPEAT))
			cam.pos += cam.up*camSpeed;
		else if(key == GLFW_KEY_Q && (action == GLFW_PRESS || action == GLFW_REPEAT))
			cam.pos -= cam.up*camSpeed;
	}
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if( (action == GLFW_PRESS) || (action == GLFW_RELEASE) )
		mousePressed = !mousePressed;
}

void mousePosCallback(GLFWwindow* window, double xpos, double ypos)
{
	int vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);

	vec2 newPos = vec2(xpos/(double)vp[2], -ypos/(double)vp[3])*2.f - vec2(1.f);
	vec2 diff = newPos - mousePos;

	if(mousePressed)
	{
		if (!orbitalCamera)
		{
			cam.cameraRotation(-diff.x, diff.y);
		}
		else
		{
			if (diff.x > 0)
			{
				cam.leftOrbital();
			}
			else if (diff.x < 0)
			{
				cam.rightOrbital();
			}
			
			if (diff.y > 0)
			{
				cam.upOrbital();
			}
			else if (diff.y < 0)
			{
				cam.downOrbital();
			}
		}
	}

	mousePos = newPos;
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (yoffset > 0)
	{
		if (orbitalCamera)
		{
			cam.zoomIn();
		}
		else
		{
			cam.pos += cam.dir*camSpeed;
		}
	}
	
	if (yoffset < 0)
	{
		if (orbitalCamera)
		{
			cam.zoomOut();
		}
		else
		{
			cam.pos -= cam.dir*camSpeed;
		}
	}
}

void resizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);

}



//==========================================================================
// TUTORIAL STUFF


//vec2 and vec3 are part of the glm math library. 
//Include in your own project by putting the glm directory in your project, 
//and including glm/glm.hpp as I have at the top of the file.
//"using namespace glm;" will allow you to avoid writing everyting as glm::vec2
vector<vec3> points;
vector<vec2> uvs;

//Structs are simply acting as namespaces
//Access the values like so: VAO::LINES
struct VAO{
	enum {LINES=0, COUNT};		//Enumeration assigns each name a value going up
										//LINES=0, COUNT=1
};

struct VBO{
	enum {POINTS=0, COLOR, COUNT};	//POINTS=0, COLOR=1, COUNT=2
};

struct SHADER{
	enum {LINE=0, COUNT};		//LINE=0, COUNT=1
};

GLuint vbo [VBO::COUNT];		//Array which stores OpenGL's vertex buffer object handles
GLuint vao [VAO::COUNT];		//Array which stores Vertex Array Object handles
GLuint shader [SHADER::COUNT];		//Array which stores shader program handles


//Gets handles from OpenGL
void generateIDs()
{
	glGenVertexArrays(VAO::COUNT, vao);		//Tells OpenGL to create VAO::COUNT many
														// Vertex Array Objects, and store their
														// handles in vao array
	glGenBuffers(VBO::COUNT, vbo);		//Tells OpenGL to create VBO::COUNT many
													//Vertex Buffer Objects and store their
													//handles in vbo array
}

//Clean up IDs when you're done using them
void deleteIDs()
{
	for(int i=0; i<SHADER::COUNT; i++)
	{
		glDeleteProgram(shader[i]);
	}
	
	glDeleteVertexArrays(VAO::COUNT, vao);
	glDeleteBuffers(VBO::COUNT, vbo);	
}


//Describe the setup of the Vertex Array Object
bool initVAO()
{
	glBindVertexArray(vao[VAO::LINES]);		//Set the active Vertex Array

	glEnableVertexAttribArray(0);		//Tell opengl you're using layout attribute 0 (For shader input)
	glBindBuffer( GL_ARRAY_BUFFER, vbo[VBO::POINTS] );		//Set the active Vertex Buffer
	glVertexAttribPointer(
		0,				//Attribute
		3,				//Size # Components
		GL_FLOAT,	//Type
		GL_FALSE, 	//Normalized?
		sizeof(vec3),	//Stride
		(void*)0			//Offset
		);
	
	glEnableVertexAttribArray(1);		//Tell opengl you're using layout attribute 1
	glBindBuffer(GL_ARRAY_BUFFER, vbo[VBO::COLOR]);
	glVertexAttribPointer(
		1,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(vec2),
		(void*)0
		);	

	return !CheckGLErrors("initVAO");		//Check for errors in initialize
}


//Loads buffers with data
bool loadBuffer(const vector<vec3>& points, const vector<vec2>& uvs)
{
	glBindBuffer(GL_ARRAY_BUFFER, vbo[VBO::POINTS]);
	glBufferData(
		GL_ARRAY_BUFFER,				//Which buffer you're loading too
		sizeof(vec3)*points.size(),	//Size of data in array (in bytes)
		&points[0],							//Start of array (&points[0] will give you pointer to start of vector)
		GL_STATIC_DRAW						//GL_DYNAMIC_DRAW if you're changing the data often
												//GL_STATIC_DRAW if you're changing seldomly
		);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[VBO::COLOR]);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(vec2)*uvs.size(),
		&uvs[0],
		GL_STATIC_DRAW
		);

	return !CheckGLErrors("loadBuffer");	
}

//Compile and link shaders, storing the program ID in shader array
bool initShader()
{	
	string vertexSource = LoadSource("vertex.glsl");		//Put vertex file text into string
	string fragmentSource = LoadSource("fragment.glsl");		//Put fragment file text into string

	GLuint vertexID = CompileShader(GL_VERTEX_SHADER, vertexSource);
	GLuint fragmentID = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
	
	shader[SHADER::LINE] = LinkProgram(vertexID, fragmentID);	//Link and store program ID in shader array

	return !CheckGLErrors("initShader");
}

//-------------------------------

//For reference:
//	https://open.gl/textures
GLuint createTexture(const char* filename)
{
	int components;
	GLuint texID;
	int tWidth, tHeight;

	//stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(filename, &tWidth, &tHeight, &components, 0);
	
	if(data != NULL)
	{
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);

		if(components==3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tWidth, tHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		else if(components==4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tWidth, tHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//Clean up
		glBindTexture(GL_TEXTURE_2D, 0);
		stbi_image_free(data);

		return texID;
	} 
	
	return 0;	//Error
}

//Use program before loading texture
//	texUnit can be - GL_TEXTURE0, GL_TEXTURE1, etc...
bool loadTexture(GLuint texID, GLuint texUnit, GLuint program, const char* uniformName)
{
	glActiveTexture(texUnit);
	glBindTexture(GL_TEXTURE_2D, texID);
	
	GLuint uniformLocation = glGetUniformLocation(program, uniformName);
	glUniform1i(uniformLocation, 0);
		
	return !CheckGLErrors("loadTexture");
}


struct MyTexture
{
	GLuint textureID;
	GLuint target;
	int width;
	int height;

	// initialize object names to zero (OpenGL reserved value)
	MyTexture() : textureID(0), target(0), width(0), height(0)
	{}
} texture;


bool InitializeTexture(const char* filename, GLuint target = GL_TEXTURE_2D)
{
	int numComponents;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(filename, &texture.width, &texture.height, &numComponents, 0);
	
	if (data != nullptr)
	{
		texture.target = target;
		glGenTextures(1, &texture.textureID);
		glBindTexture(texture.target, texture.textureID);
		GLuint format = numComponents == 3 ? GL_RGB : GL_RGBA;
		glTexImage2D(texture.target, 0, format, texture.width, texture.height, 0, format, GL_UNSIGNED_BYTE, data);

		// Note: Only wrapping modes supported for GL_TEXTURE_RECTANGLE when defining
		// GL_TEXTURE_WRAP are GL_CLAMP_TO_EDGE or GL_CLAMP_TO_BORDER
		glTexParameteri(texture.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(texture.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Clean up
		glBindTexture(texture.target, 0);
		stbi_image_free(data);
		return true;
	}
	return true; //error
}

// deallocate texture-related objects
void DestroyTexture(MyTexture *texture)
{
	glBindTexture(texture->target, 0);
	glDeleteTextures(1, &texture->textureID);
}



//-------------------------------
bool loadUniforms(Camera* cam, mat4 perspectiveMatrix)
{
	glUseProgram(shader[SHADER::LINE]);

	return !CheckGLErrors("loadUniforms");
}

struct sphereInfo
{
	int subordinateToIndex;
	float orbitalRadius;
	float degreesOrbited;
	float orbitalPeriod;
	
	sphereInfo(int s, float r, float p)
	{//Initial orbital radius is the same as the initial x displacement
		subordinateToIndex = s;
		orbitalRadius = r;
		degreesOrbited = 0.f;
		orbitalPeriod = p;
	}
};

vector<sphereInfo> sphereDisplacements;

void createSphere(float radius, float orbitalRadius,
					unsigned int latitudeDivisions, unsigned int longitudeDivisions,
					float texWidth, float texHeight,
					int subordinateToIndex, float orbitalPeriod
					)	//Divisions between 0 and ends
{
	vector<vector<vec3>> sphere;
	vector<vector<vec2>> sphereMap;
	
	float latitudeStep = 180.f / latitudeDivisions;
	float longitudeStep = 360.f / longitudeDivisions;
	
	for (int i = 0; i < latitudeDivisions; i++)
	{
		vector<vec3> sphereStrip;
		vector<vec2> sphereMapStrip;
		
		float phi = i * latitudeStep;
		float phiNext = (i + 1) * latitudeStep;
		
		for (int j = 0; j <= longitudeDivisions; j++)
		{
			float theta = j * longitudeStep;
			
			float x1 = radius * sin(phi) * cos(theta);
			float z1 = radius * sin(phi) * sin(theta);
			float y1 = radius * cos(phi);
			
			float u = j / longitudeDivisions;
			float v1 = i / latitudeDivisions;
			float v2 = (i + 1) / latitudeDivisions;
			
			sphereStrip.push_back(vec3(x1, y1, z1));
			sphereMapStrip.push_back(vec2(u, v1));
			
			float x2 = radius * sin(phiNext) * cos(theta);
			float z2 = radius * sin(phiNext) * sin(theta);
			float y2 = radius * cos(phiNext);
			
			sphereStrip.push_back(vec3(x2, y2, z2));
			sphereMapStrip.push_back(vec2(u, v2));
		}
		sphere.push_back(sphereStrip);
		sphereMap.push_back(sphereMapStrip);
	}
	
	spheres.push_back(sphere);
	sphereMapping.push_back(sphereMap);
	sphereDisplacements.push_back(sphereInfo(subordinateToIndex, orbitalRadius, orbitalPeriod));
	return;
}

void orbit(int sphereIndex)
{
	float degreeIncrement = (360.f / sphereDisplacements[sphereIndex].orbitalPeriod) * orbitalSpeed;
	
	sphereDisplacements[sphereIndex].degreesOrbited -= degreeIncrement;
	
	if (sphereDisplacements[sphereIndex].degreesOrbited <= -360.f)
	{
		sphereDisplacements[sphereIndex].degreesOrbited += 360.f;
	}
	
	return;
}

vec2 getRecursiveDisplacements(int sphereIndex, vec2 offsets)
{
	float theta = sphereDisplacements[sphereIndex].degreesOrbited;
	float radius = sphereDisplacements[sphereIndex].orbitalRadius;
	
	offsets.x += radius * cos(theta);
	offsets.y += radius * sin(theta);
	
	if (sphereDisplacements[sphereIndex].subordinateToIndex >= 0)
	{
		offsets = getRecursiveDisplacements(sphereDisplacements[sphereIndex].subordinateToIndex, offsets);
	}
	
	return offsets;
}

vector<vector<vec3>> displacedSphereCoordinates(int sphereIndex)
{
	vector<vector<vec3>> sphere;
	vector<vector<vec3>> originalSphere = spheres[sphereIndex];
	
	for(int i = 0; i < originalSphere.size() ; i++)
	{
		vector<vec3> sphereStrip;
		
		for (int j = 0; j < originalSphere[i].size(); j++)
		{
			vec2 offsets = vec2(0, 0);
			offsets = getRecursiveDisplacements(sphereIndex, offsets);
			
			float x = originalSphere[i][j].x + offsets.x;
			float z = originalSphere[i][j].z + offsets.y;
			
			sphereStrip.push_back(vec3(x, originalSphere[i][j].y, z));
		}
		
		sphere.push_back(sphereStrip);
	}
	
	return sphere;
}

//Initialization
void initGL()
{
	//Only call these once - don't call again every time you change geometry
	generateIDs();		//Create VertexArrayObjects and Vertex Buffer Objects and store their handles
	initShader();		//Create shader and store program ID

	initVAO();			//Describe setup of Vertex Array Objects and Vertex Buffer Object
}

GLuint textureBuffer;
	
void preRender()
{
	float currentHeight = 500.f;
	float currentWidth = 1000.f;
	
	const GLfloat textureCoords[][2] = {
		{0.f, 0.f},
		{0.f, currentHeight},
		{currentWidth, currentHeight},
		{currentWidth, 0.f}
	};
	
	glGenBuffers(1, &textureBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, textureBuffer);
	
}

//Draws buffers to screen
void render(Camera* cam, mat4 perspectiveMatrix, mat4 modelviewMatrix)
{
	
	//Don't need to call these on every draw, so long as they don't change
	glUseProgram(shader[SHADER::LINE]);		//Use LINE program
	glBindVertexArray(vao[VAO::LINES]);		//Use the LINES vertex array

	mat4 camMatrix = cam->getMatrix();

	glUniformMatrix4fv(glGetUniformLocation(shader[SHADER::LINE], "cameraMatrix"),
						1,
						false,
						&camMatrix[0][0]);

	glUniformMatrix4fv(glGetUniformLocation(shader[SHADER::LINE], "perspectiveMatrix"),
						1,
						false,
						&perspectiveMatrix[0][0]);

	glUniformMatrix4fv(glGetUniformLocation(shader[SHADER::LINE], "modelviewMatrix"),
						1,
						false,
						&modelviewMatrix[0][0]);

	for (int i = 0; i < spheres.size(); i++)
	{//For each sphere
		
		if (sphereDisplacements[i].orbitalPeriod > 0)
		{
			orbit(i);
		}
		
		vector<vector<vec3>> displacedSphere = displacedSphereCoordinates(i);
		
		if(!InitializeTexture("Earth.jpg", GL_TEXTURE_RECTANGLE))
			cout << "Program failed to initialize texture!" << endl;
		
		glBindTexture(texture.target, texture.textureID);
		
		
		for (int j = 0; j < displacedSphere.size(); j++)
		{//For each strip in each sphere
			//glBindVertexArray(spheres[i][j]);
			
			glBufferData(GL_ARRAY_BUFFER, sizeof(sphereMapping[i][j]), &sphereMapping[i][j], GL_STATIC_DRAW);
			
			glBindBuffer(GL_ARRAY_BUFFER, textureBuffer);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(2);
			
			
			loadBuffer(displacedSphere[j], sphereMapping[i][j]);
					
			glDrawArrays(
				GL_TRIANGLE_STRIP,		//What shape we're drawing	- GL_TRIANGLES, GL_LINES, GL_POINTS, GL_QUADS, GL_TRIANGLE_STRIP
				0,						//Starting index
				displacedSphere[j].size()		//How many vertices
			);
		}
	}

	CheckGLErrors("render");
}




// ==========================================================================
// PROGRAM ENTRY POINT

int main(int argc, char *argv[])
{   
    // initialize the GLFW windowing system
    if (!glfwInit()) {
        cout << "ERROR: GLFW failed to initilize, TERMINATING" << endl;
        return -1;
    }
    glfwSetErrorCallback(ErrorCallback);

    // attempt to create a window with an OpenGL 4.1 core profile context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(512, 512, "CPSC 453 OpenGL Boilerplate", 0, 0);
    if (!window) {
        cout << "Program failed to create GLFW window, TERMINATING" << endl;
        glfwTerminate();
        return -1;
    }


    // set keyboard callback function and make our context current (active)
    glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCursorPosCallback(window, mousePosCallback);
	glfwSetScrollCallback(window, ScrollCallback);
	glfwSetWindowSizeCallback(window, resizeCallback);

    glfwMakeContextCurrent(window);


    // query and print out information about our OpenGL environment
    QueryGLVersion();

	initGL();

	//Radius lengths are log1.5(body_radius)/20
	//Orbital lengths are log1.5(orbital_radius)/5
	//Different scales because they kept clipping into each other otherwise

	createSphere(1.659, 0, 32, 64, 1000, 500, -1, 0);
	createSphere(1.080, 9.293, 32, 64, 1000, 500, 0, 365.256);
	createSphere(0.200, 6.369, 32, 64, 1000, 500, 1, 27.321);

	cam = Camera(vec3(0, -1, -1), vec3(0, 1.f, 0));
	//float fovy, float aspect, float zNear, float zFar
	mat4 perspectiveMatrix = perspective(radians(90.f), 1.f, 0.1f, 20.f);

	void preRender();
	if (orbitalCamera)
	{
		cam.calculateOrbitalPosition();
	}
	else
	{
		cam.pos -= cam.dir*8.f;
	}

    // run an event-triggered main loop
    while (!glfwWindowShouldClose(window))
    {
		glClearColor(0.f, 0.f, 0.f, 0.f);		//Color to clear the screen with (R, G, B, Alpha)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		//Clear color and depth buffers (Haven't covered yet)

        // call function to draw our scene
        render(&cam, perspectiveMatrix, mat4(1.f));
        
        // scene is rendered to the back buffer, so swap to front for display
        glfwSwapBuffers(window);

        // sleep until next event before drawing again
        glfwWaitEvents();
	}

	// clean up allocated resources before exit
	deleteIDs();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

// ==========================================================================
// SUPPORT FUNCTION DEFINITIONS

// --------------------------------------------------------------------------
// OpenGL utility functions

void QueryGLVersion()
{
    // query opengl version and renderer information
    string version  = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    string glslver  = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    string renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

    cout << "OpenGL [ " << version << " ] "
         << "with GLSL [ " << glslver << " ] "
         << "on renderer [ " << renderer << " ]" << endl;
}

bool CheckGLErrors(string location)
{
    bool error = false;
    for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
    {
        cout << "OpenGL ERROR:  ";
        switch (flag) {
        case GL_INVALID_ENUM:
            cout << location << ": " << "GL_INVALID_ENUM" << endl; break;
        case GL_INVALID_VALUE:
            cout << location << ": " << "GL_INVALID_VALUE" << endl; break;
        case GL_INVALID_OPERATION:
            cout << location << ": " << "GL_INVALID_OPERATION" << endl; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            cout << location << ": " << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl; break;
        case GL_OUT_OF_MEMORY:
            cout << location << ": " << "GL_OUT_OF_MEMORY" << endl; break;
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


// ==========================================================================
