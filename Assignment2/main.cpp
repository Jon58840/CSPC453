// ==========================================================================
// CPSC 453 - Assignment 2
// Image Effects
//
// Author:  Jonathan Ng
// Date:    October 12th, 2016
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
#include <cmath>

// specify that we want the OpenGL core profile before including GLFW headers
#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace std;


// ==========================================================================
// ==========================================================================
// Function Prototypes
bool CheckGLErrors();
void QueryGLVersion();

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);


// ==========================================================================
// ==========================================================================
// Variables

const double PI = 3.14159265358979;

float currentWidth = 512.f;
float currentHeight = 512.f;

float xCursorPosition;
float yCursorPosition;

float xCursorPositionChange = 0;
float yCursorPositionChange = 0;

float xCurrentOffset = 0;
float yCurrentOffset = 0;

float bottomLeftCurrentOffset[2] = {0, 0};
float topLeftCurrentOffset[2] = {0, 0};
float topRightCurrentOffset[2] = {0, 0};
float bottomRightCurrentOffset[2] = {0, 0};

bool mouseHeld = false;

float magnification = 1.0f;

double rotation = 0;

int greyScale = 0;
int sobel = 0;
int gaussian = 0;

// ==========================================================================
// ==========================================================================
// Functions

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
};

// load, compile, and link shaders, returning true if successful
bool InitializeShaders(MyShader *shader)
{
	// load shader source from files
	string vertexSource = LoadSource("vertex.glsl");
	string fragmentSource = LoadSource("fragment.glsl");
	if (vertexSource.empty() || fragmentSource.empty()) return false;

	// compile shader source into shader objects
	shader->vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
	shader->fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

	// link shader program
	shader->program = LinkProgram(shader->vertex, shader->fragment);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate shader-related objects
void DestroyShaders(MyShader *shader)
{
	// unbind any shader programs and destroy shader objects
	glUseProgram(0);
	glDeleteProgram(shader->program);
	glDeleteShader(shader->vertex);
	glDeleteShader(shader->fragment);
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing textures

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


bool InitializeTexture(MyTexture* texture, const char* filename, GLuint target = GL_TEXTURE_2D)
{
	int numComponents;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(filename, &texture->width, &texture->height, &numComponents, 0);
	
	if (data != nullptr)
	{
		texture->target = target;
		glGenTextures(1, &texture->textureID);
		glBindTexture(texture->target, texture->textureID);
		GLuint format = numComponents == 3 ? GL_RGB : GL_RGBA;
		glTexImage2D(texture->target, 0, format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, data);

		// Note: Only wrapping modes supported for GL_TEXTURE_RECTANGLE when defining
		// GL_TEXTURE_WRAP are GL_CLAMP_TO_EDGE or GL_CLAMP_TO_BORDER
		glTexParameteri(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Clean up
		glBindTexture(texture->target, 0);
		stbi_image_free(data);
		return !CheckGLErrors();
	}
	return true; //error
}

// deallocate texture-related objects
void DestroyTexture(MyTexture *texture)
{
	glBindTexture(texture->target, 0);
	glDeleteTextures(1, &texture->textureID);
}

void SaveImage(const char* filename, int width, int height, unsigned char *data, int numComponents = 3, int stride = 0)
{
	if (!stbi_write_png(filename, width, height, numComponents, data, stride))
		cout << "Unable to save image: " << filename << endl;
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing geometry data

struct MyGeometry
{
	// OpenGL names for array buffer objects, vertex array object
	GLuint  vertexBuffer;
	GLuint  textureBuffer;
	GLuint  colourBuffer;
	GLuint  vertexArray;
	GLsizei elementCount;

	// initialize object names to zero (OpenGL reserved value)
	MyGeometry() : vertexBuffer(0), colourBuffer(0), vertexArray(0), elementCount(0)
	{}
} geometry;

void transformPoints(GLfloat vertices[][2])
{
	float drawWidth, drawHeight;
	float xAdjustment, yAdjustment;
	
	//Adjusting the scaling of the texture to the size of the window
	if (currentWidth > currentHeight)
	{
		drawWidth = 1.f;
		drawHeight = currentHeight / currentWidth;
	}
	else
	{
		drawHeight = 1.f;
		drawWidth = currentWidth / currentHeight;
	}
	
	drawWidth *= magnification;
	drawHeight *= magnification;
	
	if (mouseHeld)
	{
		//Adjust the offsets to the current rotation of the axes and add them
		xCurrentOffset += xCursorPositionChange*cos(-rotation) - yCursorPositionChange*sin(-rotation);
		yCurrentOffset += xCursorPositionChange*sin(-rotation) + yCursorPositionChange*cos(-rotation);
		
		//Adjust for the different scaling
		xAdjustment = xCurrentOffset / 256.f;
		yAdjustment = yCurrentOffset / 256.f;
	}
	else
	{
		xAdjustment = xCurrentOffset / 256.f;
		yAdjustment = yCurrentOffset / 256.f;
	}
	
	
	//Translate
	GLfloat bottomLeftTranslation[2] = {-drawWidth + xAdjustment, -drawHeight + yAdjustment};
	GLfloat topLeftTranslation[2] = {-drawWidth + xAdjustment, drawHeight + yAdjustment};
	GLfloat topRightTranslation[2] = {drawWidth + xAdjustment, drawHeight + yAdjustment};
	GLfloat bottomRightTranslation[2] = {drawWidth + xAdjustment, -drawHeight + yAdjustment};
	
	//Rotation
	GLfloat bottomLeftFinal[2] = {
									bottomLeftTranslation[0]*cos(rotation) - bottomLeftTranslation[1]*sin(rotation),
									bottomLeftTranslation[0]*sin(rotation) + bottomLeftTranslation[1]*cos(rotation)
									};
	
	GLfloat topLeftFinal[2] = {
								topLeftTranslation[0]*cos(rotation) - topLeftTranslation[1]*sin(rotation),
								topLeftTranslation[0]*sin(rotation) + topLeftTranslation[1]*cos(rotation)
								};
	
	GLfloat topRightFinal[2] = {
									topRightTranslation[0]*cos(rotation) - topRightTranslation[1]*sin(rotation),
									topRightTranslation[0]*sin(rotation) + topRightTranslation[1]*cos(rotation)
									};
	
	GLfloat bottomRightFinal[2] = {
									bottomRightTranslation[0]*cos(rotation) - bottomRightTranslation[1]*sin(rotation),
									bottomRightTranslation[0]*sin(rotation) + bottomRightTranslation[1]*cos(rotation)
									};			
									
	//Set vertices
	vertices[0][0] = bottomLeftFinal[0];
	vertices[0][1] = bottomLeftFinal[1];
	
	vertices[1][0] = topLeftFinal[0];
	vertices[1][1] = topLeftFinal[1];
	
	vertices[2][0] = topRightFinal[0];
	vertices[2][1] = topRightFinal[1];
	
	vertices[3][0] = bottomRightFinal[0];
	vertices[3][1] = bottomRightFinal[1];
	
	return;
}

// create buffers and fill with geometry data, returning true if successful
bool InitializeGeometry(MyGeometry *geometry)
{
	GLfloat vertices[4][2];
	transformPoints(vertices);
	
	const GLfloat textureCoords[][2] = {
		{0.f, 0.f},
		{0.f, currentHeight},
		{currentWidth, currentHeight},
		{currentWidth, 0.f}
	};

	const GLfloat colours[][3] = {
		{ 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f }
	};
	geometry->elementCount = 4;

	// these vertex attribute indices correspond to those specified for the
	// input variables in the vertex shader
	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;
	const GLuint TEXTURE_INDEX = 2;

	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry->vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//
	glGenBuffers(1, &geometry->textureBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);

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

	// Tell openGL how the data is formatted
	glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
	glVertexAttribPointer(TEXTURE_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(TEXTURE_INDEX);

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

// deallocate geometry-related objects
void DestroyGeometry(MyGeometry *geometry)
{
	// unbind and destroy our vertex array object and associated buffers
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &geometry->vertexArray);
	glDeleteBuffers(1, &geometry->vertexBuffer);
	glDeleteBuffers(1, &geometry->colourBuffer);
}
// --------------------------------------------------------------------------

// Set the uniforms
void setUniforms(MyShader *shader)
{
	GLint locGreyScale = glGetUniformLocation(shader->program, "chosenGreyScale");
	glUniform1i(locGreyScale, greyScale);
	
	GLint locSobel = glGetUniformLocation(shader->program, "chosenSobel");
	glUniform1i(locSobel, sobel);
	
	GLint locGaussian = glGetUniformLocation(shader->program, "chosenGaussian");
	glUniform1i(locGaussian, gaussian);
}


// Rendering function that draws our scene to the frame buffer

void RenderScene(MyGeometry *geometry, MyTexture* texture, MyShader *shader)
{
	// clear screen to black
	glClearColor(0.f, 0.f, 0.f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (mouseHeld == true)
	{
		if (!InitializeGeometry(geometry))
		cout << "Program failed to initialize geometry!" << endl;
	}
	
	xCursorPositionChange = 0;
	yCursorPositionChange = 0;

	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(shader->program);
	
	setUniforms(shader);
	
	glBindVertexArray(geometry->vertexArray);
	glBindTexture(texture->target, texture->textureID);
	
	int drawtype =  GL_TRIANGLE_FAN;
	
	glDrawArrays(
		drawtype,
		0,
		geometry->elementCount
		);

	// reset state to default (no shader or geometry bound)
	glBindTexture(texture->target, 0);
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

// Not actually a callback function, but resets some attributes for the KeyCallBack
void resetAttributes()
{
	xCurrentOffset = 0;
	yCurrentOffset = 0;
	magnification = 1.0;
	rotation = 0;
	greyScale = 0;
}

// handles keyboard input events
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	
	//Picture switching--------------------------------------------------------------------
	
	if (key == GLFW_KEY_0  && action == GLFW_PRESS)
    {
		currentWidth = 512.f;
		currentHeight = 512.f;
		resetAttributes();
		
		if (!InitializeGeometry(&geometry))
		cout << "Program failed to initialize geometry!" << endl;
		
		if(!InitializeTexture(&texture, "test.png", GL_TEXTURE_RECTANGLE))
			cout << "Program failed to initialize texture!" << endl;
	}
    
    if (key == GLFW_KEY_1  && action == GLFW_PRESS)
    {
		currentWidth = 512.f;
		currentHeight = 512.f;
		resetAttributes();
		
		if (!InitializeGeometry(&geometry))
		cout << "Program failed to initialize geometry!" << endl;
		
		if(!InitializeTexture(&texture, "image1-mandrill.png", GL_TEXTURE_RECTANGLE))
			cout << "Program failed to initialize texture!" << endl;
	}
    
    if (key == GLFW_KEY_2  && action == GLFW_PRESS)
    {
		currentWidth = 692.f;
		currentHeight = 516.f;
		resetAttributes();
		
		if (!InitializeGeometry(&geometry))
		cout << "Program failed to initialize geometry!" << endl;
		
		if(!InitializeTexture(&texture, "image2-uclogo.png", GL_TEXTURE_RECTANGLE))
			cout << "Program failed to initialize texture!" << endl;
	}
	
	if (key == GLFW_KEY_3  && action == GLFW_PRESS)
    {
		currentWidth = 2000.f;
		currentHeight = 931.f;
		resetAttributes();
		
		if (!InitializeGeometry(&geometry))
		cout << "Program failed to initialize geometry!" << endl;
		
		if(!InitializeTexture(&texture, "image3-aerial.jpg", GL_TEXTURE_RECTANGLE))
			cout << "Program failed to initialize texture!" << endl;
	}
	
	if (key == GLFW_KEY_4  && action == GLFW_PRESS)
    {
		currentWidth = 400.f;
		currentHeight = 591.f;
		resetAttributes();
		
		if (!InitializeGeometry(&geometry))
		cout << "Program failed to initialize geometry!" << endl;
		
		if(!InitializeTexture(&texture, "image4-thirsk.jpg", GL_TEXTURE_RECTANGLE))
			cout << "Program failed to initialize texture!" << endl;
	}
	
	if (key == GLFW_KEY_5  && action == GLFW_PRESS)
    {
		currentWidth = 2048.f;
		currentHeight = 1536.f;
		resetAttributes();
		
		if (!InitializeGeometry(&geometry))
		cout << "Program failed to initialize geometry!" << endl;
		
		if(!InitializeTexture(&texture, "image5-pattern.png", GL_TEXTURE_RECTANGLE))
			cout << "Program failed to initialize texture!" << endl;
	}
	
	if (key == GLFW_KEY_6  && action == GLFW_PRESS)
    {
		currentWidth = 2036.f;
		currentHeight = 1381.f;
		resetAttributes();
		
		if (!InitializeGeometry(&geometry))
		cout << "Program failed to initialize geometry!" << endl;
		
		if(!InitializeTexture(&texture, "image6-batman.jpg", GL_TEXTURE_RECTANGLE))
			cout << "Program failed to initialize texture!" << endl;
	}
	
	//-------------------------------------------------------------------------------
	
	//Rotation-----------------------------------------------------------------------
	
	if (key == GLFW_KEY_LEFT  && (action == GLFW_PRESS || action == GLFW_REPEAT) )
    {
		rotation -= PI / 32;
		
		if (rotation <= -2 * PI)
			rotation = 0;
		
		if (!InitializeGeometry(&geometry))
			cout << "Program failed to initialize geometry!" << endl;
	}
	
	if (key == GLFW_KEY_RIGHT  && (action == GLFW_PRESS || action == GLFW_REPEAT) )
    {
		rotation += PI / 32;
		
		if (rotation >= 2 * PI)
			rotation = 0;
			
		if (!InitializeGeometry(&geometry))
			cout << "Program failed to initialize geometry!" << endl;
	}
	
	//-------------------------------------------------------------------------------
	
	//Reset filters
	if (key == GLFW_KEY_Q  && action == GLFW_PRESS)
    {
		greyScale = 0;
		sobel = 0;
		gaussian = 0;
	}
	
	//Greyscale----------------------------------------------------------------------
	
	if (key == GLFW_KEY_W  && action == GLFW_PRESS)
    {
		sobel = 0;
		gaussian = 0;
		greyScale = 1;
	}
	
	if (key == GLFW_KEY_E  && action == GLFW_PRESS)
    {
		sobel = 0;
		gaussian = 0;
		greyScale = 2;
	}
	
	if (key == GLFW_KEY_R  && action == GLFW_PRESS)
    {
		sobel = 0;
		gaussian = 0;
		greyScale = 3;
	}
	
	if (key == GLFW_KEY_T  && action == GLFW_PRESS)
    {
		sobel = 0;
		gaussian = 0;
		greyScale = 4;
	}
	
	if (key == GLFW_KEY_Y  && action == GLFW_PRESS)
    {
		sobel = 0;
		gaussian = 0;
		greyScale = 5;
	}
	
	//-------------------------------------------------------------------------------
	
	//Sobel--------------------------------------------------------------------------
	
	if (key == GLFW_KEY_S  && action == GLFW_PRESS)
    {
		greyScale = 0;
		gaussian = 0;
		sobel = 1;
	}
	
	if (key == GLFW_KEY_D  && action == GLFW_PRESS)
    {
		greyScale = 0;
		gaussian = 0;
		sobel = 2;
	}
	
	if (key == GLFW_KEY_F  && action == GLFW_PRESS)
    {
		greyScale = 0;
		gaussian = 0;
		sobel = 3;
	}
	
	//-------------------------------------------------------------------------------
	
	//Gaussian-----------------------------------------------------------------------
	
	if (key == GLFW_KEY_X  && action == GLFW_PRESS)
    {
		greyScale = 0;
		sobel = 0;
		gaussian = 1;
	}
	
	if (key == GLFW_KEY_C  && action == GLFW_PRESS)
    {
		greyScale = 0;
		sobel = 0;
		gaussian = 2;
	}
	
	if (key == GLFW_KEY_V  && action == GLFW_PRESS)
    {
		greyScale = 0;
		sobel = 0;
		gaussian = 3;
	}
	
	//-------------------------------------------------------------------------------
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		mouseHeld = true;
	}
	
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		mouseHeld = false;
	}
}

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	xCursorPositionChange += (float)xpos - xCursorPosition;
	yCursorPositionChange += yCursorPosition - (float)ypos;
	//These are += adding up because render does not happen as fast as this is updated.
	//If just = then some position change is lost in the time between renders.
	//These values will be reset to 0 after each render.
	
	xCursorPosition = (float)xpos;
	yCursorPosition = (float)ypos;
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (yoffset > 0)
	{
		if (magnification < 1.0)
		{
			magnification += 0.01;
		}
		else if (magnification < 2.0)
		{
			magnification += 0.05;
		}
			
		if (!InitializeGeometry(&geometry))
		cout << "Program failed to initialize geometry!" << endl;
	}
	
	if (yoffset < 0)
	{
		if (magnification > 0.1)
		{
			magnification -= 0.05;
		}
		else if (magnification > 0.02)
		{
			magnification -= 0.01;
		} 
			
		if (!InitializeGeometry(&geometry))
		cout << "Program failed to initialize geometry!" << endl;
	}
}
	

// ==========================================================================
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
	window = glfwCreateWindow(512, 512, "Assignment 2", 0, 0);
	if (!window) {
		cout << "Program failed to create GLFW window, TERMINATING" << endl;
		glfwTerminate();
		return -1;
	}

	// set keyboard callback function and make our context current (active)
	glfwSetKeyCallback(window, KeyCallback);
	glfwMakeContextCurrent(window);
	glfwSetCursorPosCallback(window, CursorPosCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetScrollCallback(window, ScrollCallback);

	// query and print out information about our OpenGL environment
	QueryGLVersion();

	// call function to load and compile shader programs
	MyShader shader;
	if (!InitializeShaders(&shader)) {
		cout << "Program could not initialize shaders, TERMINATING" << endl;
		return -1;
	}

	// call function to create and fill buffers with geometry data
	if (!InitializeGeometry(&geometry))
		cout << "Program failed to initialize geometry!" << endl;

	if(!InitializeTexture(&texture, "test.png", GL_TEXTURE_RECTANGLE))
		cout << "Program failed to initialize texture!" << endl;

	// run an event-triggered main loop
	while (!glfwWindowShouldClose(window))
	{
		// call function to draw our scene
		RenderScene(&geometry, &texture, &shader); //render scene with texture

		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	// clean up allocated resources before exit
	DestroyGeometry(&geometry);
	DestroyShaders(&shader);
	glfwDestroyWindow(window);
	glfwTerminate();

	cout << "Goodbye!" << endl;
	return 0;
}


// ==========================================================================
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
