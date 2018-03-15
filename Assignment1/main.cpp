// ==========================================================================
// CPSC 453 - Assignment 1
// Line & Polygon Geometry
//
// Author:  Jonathan Ng
// Date:    September 30th, 2016
//
// Notes: Any code I did not write myself was provided in
//        either the assignment materials or tutorial sessions
// ==========================================================================

#include <iostream>
#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <vector>
#include "glm/glm.hpp"

// specify that we want the OpenGL core profile before including GLFW headers
#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

using namespace std;
using namespace glm;

//Function Prototypes -------------------------------------------------------
bool CheckGLErrors();
void QueryGLVersion();

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);

enum shapeRendered {Circle=0, Squares, Spiral, Sierpinski};

void reinitShape(GLFWwindow* window, shapeRendered newShape);
void changeIterations(GLFWwindow* window);

//Global Variable Declarations ----------------------------------------------
const float PI = 3.14159265358979;

//I disliked the struct-enums used in the tutorial so I am using named constants in their place
const int VAO_LINE_ARRAY = 0;
const int VAO_COUNT = 1;

const int VBO_POINTS_ARRAY = 0;
const int VBO_COLOR_ARRAY = 1;
const int VBO_COUNT = 2;

const int SHADER_LINE_ARRAY = 0;
const int SHADER_COUNT = 1;

//points and colors vectors are global for ease of access
vector<vec2> points;
vector<vec3> colors;

GLuint vbo [VBO_COUNT];		//Array which stores OpenGL's vertex buffer object handles
GLuint vao [VAO_COUNT];		//Array which stores Vertex Array Object handles
GLuint shader [SHADER_COUNT];		//Array which stores shader program handles

//Global variables for the current shape and number of iterations we are on
shapeRendered currentShape = Circle;
int iterations = 1;

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
        
    //Shape changes------------------
    
    if (key == GLFW_KEY_0  && action == GLFW_PRESS)
    {
		reinitShape(window, Circle);
	}
    
    if (key == GLFW_KEY_1  && action == GLFW_PRESS)
    {
		reinitShape(window, Squares);
	}
    
    if (key == GLFW_KEY_2  && action == GLFW_PRESS)
    {
		reinitShape(window, Spiral);
	}
	
	if (key == GLFW_KEY_3  && action == GLFW_PRESS)
    {
		reinitShape(window, Sierpinski);
	}
	
	//Iteraiton level changes---------
	
	if (key == GLFW_KEY_LEFT  && (action == GLFW_PRESS || action == GLFW_REPEAT) )
    {
		if (iterations > 1)
			iterations -= 1;
		
		changeIterations(window);
	}
	
	if (key == GLFW_KEY_RIGHT  && (action == GLFW_PRESS || action == GLFW_REPEAT) )
    {//Different hard caps for iterations dependening on shape
		switch(currentShape)
		{
			case Circle:
				break;
			case Squares:
				//Due to how I color it the top levels become too similar if enough iterations happen and it becomes boring
				if (iterations < 16)
					iterations += 1;
				break;
			case Spiral:
				if (iterations < 25)
					iterations += 1;
				break;
			case Sierpinski:
				//Begins lagging if it goes too deep
				if (iterations < 9)
					iterations += 1;
				break;
		}
		
		changeIterations(window);
	}
}




//==========================================================================
// TUTORIAL STUFF

//Gets handles from OpenGL
void generateIDs()
{
	glGenVertexArrays(VAO_COUNT, vao);		//Tells OpenGL to create VAO::COUNT many
														// Vertex Array Objects, and store their
														// handles in vao array
	glGenBuffers(VBO_COUNT, vbo);		//Tells OpenGL to create VBO::COUNT many
													//Vertex Buffer Objects and store their
													//handles in vbo array
}

//Clean up IDs when you're done using them
void deleteIDs()
{
	for(int i=0; i<SHADER_COUNT; i++)
	{
		glDeleteProgram(shader[i]);
	}
	
	glDeleteVertexArrays(VAO_COUNT, vao);
	glDeleteBuffers(VBO_COUNT, vbo);	
}


//Describe the setup of the Vertex Array Object
bool initVAO()
{
	glBindVertexArray(vao[VAO_LINE_ARRAY]);		//Set the active Vertex Array

	glEnableVertexAttribArray(0);		//Tell opengl you're using layout attribute 0 (For shader input)
	glBindBuffer( GL_ARRAY_BUFFER, vbo[VBO_POINTS_ARRAY] );		//Set the active Vertex Buffer
	glVertexAttribPointer(
		0,				//Attribute
		2,				//Size # Components
		GL_FLOAT,	//Type
		GL_FALSE, 	//Normalized?
		sizeof(vec2),	//Stride
		(void*)0			//Offset
		);
	
	glEnableVertexAttribArray(1);		//Tell opengl you're using layout attribute 1
	glBindBuffer(GL_ARRAY_BUFFER, vbo[VBO_COLOR_ARRAY]);
	glVertexAttribPointer(
		1,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(vec3),
		(void*)0
		);	

	return !CheckGLErrors();		//Check for errors in initialize
}


//Loads buffers with data
bool loadBuffer(const vector<vec2>& points, const vector<vec3>& colors)
{
	glBindBuffer(GL_ARRAY_BUFFER, vbo[VBO_POINTS_ARRAY]);
	glBufferData(
		GL_ARRAY_BUFFER,				//Which buffer you're loading too
		sizeof(vec2)*points.size(),	//Size of data in array (in bytes)
		&points[0],							//Start of array (&points[0] will give you pointer to start of vector)
		GL_STATIC_DRAW						//GL_DYNAMIC_DRAW if you're changing the data often
												//GL_STATIC_DRAW if you're changing seldomly
		);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[VBO_COLOR_ARRAY]);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(vec3)*colors.size(),
		&colors[0],
		GL_STATIC_DRAW
		);

	return !CheckGLErrors();	
}

//Compile and link shaders, storing the program ID in shader array
bool initShader()
{	
	string vertexSource = LoadSource("vertex.glsl");		//Put vertex file text into string
	string fragmentSource = LoadSource("fragment.glsl");		//Put fragment file text into string

	GLuint vertexID = CompileShader(GL_VERTEX_SHADER, vertexSource);
	GLuint fragmentID = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
	
	shader[SHADER_LINE_ARRAY] = LinkProgram(vertexID, fragmentID);	//Link and store program ID in shader array

	return !CheckGLErrors();
}

//==========================================================================
// Shape Generation

//Tutorial circle. Keeping it as a base screen.
void generateCircle(float radius, int numPoints)
{
	//Make sure vectors are empty
	points.clear();
	colors.clear();

	const float MAX_ROTATION = 2*PI;

	float u = 0.f;
	float ustep = 1.f/((float)numPoints - 1);		//Size of steps so u = 1 at end of loop

	vec3 startColor(1.f, 0.f, 0.f);		//Initial color
	vec3 endColor(0.f, 0.f, 1.f);			//Final color

	//Fill vectors with points/colors
	for(int i=0; i<numPoints; i++)
	{
		u+=ustep;						//Increment u
		points.push_back(vec2(
			radius*cos(MAX_ROTATION*u),		//x coordinate
			radius*sin(MAX_ROTATION*u))		//y coordinate
			);

		colors.push_back(startColor*(1-u) + endColor*u);	//Linearly blend start and end color
	}
}

//Less cluttery way of loading in the same colour multiple times in a row
void pushColorBack(vec3 rgb, int numTimes)
{
	for (int i = 0; i < numTimes; i++)
	{
		colors.push_back(rgb);
	}
}

void generateSquares(int levels)
{
	points.clear();
	colors.clear();

	vec3 startSquareColor(1.f, 0.f, 0.f);
	vec3 endSquareColor(1.f, 1.f, 0.f);
	
	vec3 startDiamondColor(0.f, 0.f, 1.f);
	vec3 endDiamondColor(0.f, 1.f, 0.f);

	//Seting the original square. 5 points, starting and ending on the same point
	points.push_back(vec2(-1.f, -1.f));
	points.push_back(vec2(-1.f, 1.f));
	points.push_back(vec2(1.f, 1.f));
	points.push_back(vec2(1.f, -1.f));
	points.push_back(vec2(-1.f, -1.f));
	
	pushColorBack(startSquareColor, 5);
	
	float percentageColor = 1;
	
	for (int i = 0; i < levels - 1; i++)
	{
		int lastIterationStartIndex = points.size() - 5;
		
		//Placing start point of next iteration shape
		
		points.push_back(vec2(
			(points[lastIterationStartIndex][0] + points[lastIterationStartIndex + 1][0]) / 2,
			(points[lastIterationStartIndex][1] + points[lastIterationStartIndex + 1][1]) / 2
		));
		
		//Color it the previous colour so we don't get bleeding on the last shape we had
		if(i % 2)
		{
			colors.push_back((startDiamondColor * percentageColor) + (endDiamondColor * (1 - percentageColor)) );
		}
		else
		{
			colors.push_back((startSquareColor * percentageColor) + (endSquareColor * (1 - percentageColor)) );
		}
		
		//Placing ANOTHER point in the same spot that will be the given the next colour
		//so that the old colour doesn't bleed into the first line of the next iteration
		
		points.push_back(vec2(
			(points[lastIterationStartIndex][0] + points[lastIterationStartIndex + 1][0]) / 2,
			(points[lastIterationStartIndex][1] + points[lastIterationStartIndex + 1][1]) / 2
		));
		
		//----
		
		//Place the rest of the points
		
		points.push_back(vec2(
			(points[lastIterationStartIndex + 1][0] + points[lastIterationStartIndex + 2][0]) / 2,
			(points[lastIterationStartIndex + 1][1] + points[lastIterationStartIndex + 2][1]) / 2
		));
		
		points.push_back(vec2(
			(points[lastIterationStartIndex + 2][0] + points[lastIterationStartIndex + 3][0]) / 2,
			(points[lastIterationStartIndex + 2][1] + points[lastIterationStartIndex + 3][1]) / 2
		));
		
		points.push_back(vec2(
			(points[lastIterationStartIndex + 3][0] + points[lastIterationStartIndex + 4][0]) / 2,
			(points[lastIterationStartIndex + 3][1] + points[lastIterationStartIndex + 4][1]) / 2
		));
		
		points.push_back(vec2(
			points[lastIterationStartIndex + 5][0],
			points[lastIterationStartIndex + 5][1]
		));
		
		//Adjust and set new colour
		
		percentageColor = (levels - i) / (float)levels;
		
		if(i % 2)
		{
			pushColorBack( (startSquareColor * percentageColor) + (endSquareColor * (1 - percentageColor) ), 5);
		}
		else
		{
			pushColorBack((startDiamondColor * percentageColor) + (endDiamondColor * (1 - percentageColor)), 5);
		}
	}
	
}

void generateSpiral(int turns, int numPoints)
{
	points.clear();
	colors.clear();

	float MAX_ROTATION = turns*2*PI;

	float u = 0.f;
	float ustep = 1.f/((float)numPoints - 1);

	vec3 startColor(1.f, 0.f, 0.f);	
	vec3 endColor(0.f, 0.f, 1.f);

	for(int i=0; i<numPoints; i++)
	{
		u+=ustep;
		points.push_back(vec2(
			//Using u instead of a radius will make it spiral outward as u increase,
			//and as u ends at 1, spiral will always end at edge of window
			u*cos(MAX_ROTATION*u),
			u*sin(MAX_ROTATION*u))
			);

		colors.push_back(startColor*(1-u) + endColor*u);
	}
}

//Iterating further levels of a Sierpinski Triangle, the base triangle is in a function below
void iterateSierpinski(vec2 topCorner, vec2 bottomLeftCorner, vec2 bottomRightCorner, vec3 baseColor, int remainingLevels)
{
	//The remaining levels will be used to go down recursively in iterations
	//Decrement so we actually end at some point
	remainingLevels--;
	
	vec3 TopTriangleColor(1.f, 0.f, 0.f);
	vec3 bottomLeftTriangleColor(0.f, 1.f, 0.f);
	vec3 bottomRightTriangleColor(0.f, 0.f, 1.f);
	vec3 clearTriangleColor(0.f, 0.f, 0.f);
	
	//Figure out the midpoints of the three sides of the triangle
	//as they will be corners on the new smaller triangles
	vec2 midLeft = vec2( (topCorner[0] + bottomLeftCorner[0]) / 2,
						 (topCorner[1] + bottomLeftCorner[1]) / 2);
						 
	vec2 midRight = vec2( (topCorner[0] + bottomRightCorner[0]) / 2,
					 	  (topCorner[1] + bottomRightCorner[1]) / 2);
						 
	vec2 bottom = vec2( (bottomLeftCorner[0] + bottomRightCorner[0]) / 2,
						(bottomLeftCorner[1] + bottomRightCorner[1]) / 2);
	
	//Top triangle
	pushColorBack(0.4f*baseColor + 0.6f*TopTriangleColor, 3);
	points.push_back(topCorner);
	points.push_back(midLeft);
	points.push_back(midRight);
	
	//Bottom left triangle
	pushColorBack(0.4f*baseColor + 0.6f*bottomLeftTriangleColor, 3);
	points.push_back(midLeft);
	points.push_back(bottomLeftCorner);
	points.push_back(bottom);
	
	//Bottom right triangle
	pushColorBack(0.4f*baseColor + 0.6f*bottomRightTriangleColor, 3);
	points.push_back(midRight);
	points.push_back(bottom);
	points.push_back(bottomRightCorner);
	
	//"Clear" the center triangle
	pushColorBack(clearTriangleColor, 3);
	points.push_back(midLeft);
	points.push_back(midRight);
	points.push_back(bottom);
	
	//Recursively iterate further until we run out of remaining levels
	if (remainingLevels > 1)
	{
		iterateSierpinski(topCorner, midLeft, midRight, baseColor + TopTriangleColor, remainingLevels);
		iterateSierpinski(midLeft, bottomLeftCorner, bottom, baseColor + bottomLeftTriangleColor, remainingLevels);
		iterateSierpinski(midRight, bottom, bottomRightCorner, baseColor + bottomRightTriangleColor, remainingLevels);
	}
	
	return;
}

void generateSierpinski(int levels)
{
	points.clear();
	colors.clear();
	
	//Manually set and push back all the points of the first triangle
	vec2 topCorner(-1.f, -1.f);
	vec2 bottomLeftCorner(1.f, -1.f);
	vec2 bottomRightCorner(0.f, 1.f);
	
	vec3 baseColor(0.8f, 0.8f, 0.8f);
	
	pushColorBack(baseColor, 3);
	points.push_back(topCorner);
	points.push_back(bottomLeftCorner);
	points.push_back(bottomRightCorner);
	
	//Iterate down further levels as needed
	if (levels > 1)
	{
		iterateSierpinski(topCorner, bottomLeftCorner, bottomRightCorner, baseColor, levels);
	}
}

//Initialization
void initGL()
{
	//Only call these once - don't call again every time you change geometry
	generateIDs();		//Create VertexArrayObjects and Vertex Buffer Objects and store their handles
	initShader();		//Create shader and store program ID

	initVAO();			//Describe setup of Vertex Array Objects and Vertex Buffer Objects

	//Call these two (or equivalents) every time you change geometry
	generateCircle(1.f, 200);		//Create geometry - CHANGE THIS FOR DIFFERENT SCENES
	loadBuffer(points, colors);	//Load geometry into buffers
}

void reinitShape(GLFWwindow* window, shapeRendered newShape)
{
	if (currentShape == newShape)
	{//If a shape key is hit and it is the same shape we are already on, leave and do nothing
	 //This is just to prevent resetting iterations as that was not intended functionality
		return;
	}
	
	switch(newShape)
	{
		case Circle:
			iterations = 1;
			glfwSetWindowTitle(window, "Shape 0: Circle");
			generateCircle(1.f, 200);
			break;
		case Squares:
			iterations = 2;
			glfwSetWindowTitle(window, "Shape 1: Squares and Diamonds");
			generateSquares(iterations);
			break;
		case Spiral:
			iterations = 2;
			glfwSetWindowTitle(window, "Shape 2: Spiral");
			generateSpiral(iterations, 100+100*iterations);
			break;
		case Sierpinski:
			iterations = 2;
			glfwSetWindowTitle(window, "Shape 3: Sierpinski Triangle");
			generateSierpinski(iterations);
			break;
	}
	
	currentShape = newShape;
	loadBuffer(points, colors);	
	
	return;
}

//Called when arrow keys are hit. Regenerates current shape with different iteraiton number
void changeIterations(GLFWwindow* window)
{
	switch(currentShape)
	{
		case Circle:
			break;
		case Squares:
			generateSquares(iterations);
			break;
		case Spiral:
			//Increasing the number of points with iterations
			//so tighter curves are not as off from ideal
			generateSpiral(iterations, 100+100*iterations);
			break;
		case Sierpinski:
			generateSierpinski(iterations);
			break;
	}
	
	loadBuffer(points, colors);	
	
	return;
}

//Draws buffers to screen
void render()
{
	glClearColor(0.f, 0.f, 0.f, 0.f);		//Color to clear the screen with (R, G, B, Alpha)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		//Clear color and depth buffers (Haven't covered yet)

	//Don't need to call these on every draw, so long as they don't change
	glUseProgram(shader[SHADER_LINE_ARRAY]);		//Use LINE program
	glBindVertexArray(vao[VAO_LINE_ARRAY]);		//Use the LINES vertex array

	int drawtype = GL_LINE_STRIP;

	switch(currentShape)
	{
		case Circle:
			break;
		case Squares:
			break;
		case Spiral:
			break;
		case Sierpinski:
			drawtype = GL_TRIANGLES;
			break;
	}

	glDrawArrays(
			drawtype,		//What shape we're drawing	- GL_TRIANGLES, GL_LINES, GL_POINTS, GL_QUADS, GL_TRIANGLE_STRIP
			0,						//Starting index
			points.size()		//How many vertices
			);
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
    GLFWwindow *window = 0;
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
    glfwSetKeyCallback(window, KeyCallback);
    glfwMakeContextCurrent(window);

    // query and print out information about our OpenGL environment
    QueryGLVersion();

	initGL();

    // run an event-triggered main loop
    while (!glfwWindowShouldClose(window))
    {
        // call function to draw our scene
        render();

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


// ==========================================================================
