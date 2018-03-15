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
#include "GlyphExtractor.h"
#include "glm/glm.hpp"

// Specify that we want the OpenGL core profile before including GLFW headers
#ifndef LAB_LINUX
	#include <glad/glad.h>
#else
	#define GLFW_INCLUDE_GLCOREARB
	#define GL_GLEXT_PROTOTYPES
#endif
#include <GLFW/glfw3.h>

//STB
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace std;
using namespace glm;
// --------------------------------------------------------------------------
// OpenGL utility and support function prototypes

void QueryGLVersion();
bool CheckGLErrors();

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader, GLuint tcsShader, GLuint tesShader);

bool setGeometry(int chosenDrawType);

// ==========================================================================
// ==========================================================================
// Variables

//Draw type number assignment
const int TEXTURE = 0;
const int LINE = 1;
const int QUADRATIC = 2;
const int CUBIC = 3;
const int DRAW_TYPES = 4;

//Font type number assignment
const int LORA = 0;
const int SANS = 1;
const int ALEX = 2;
const int INCONSOLATA = 3;
int currentFont = 0;

//Coordinate holders
vector<vec2> pointVectors[4];
vector<vec3> colorVectors[4];

//Control what's being drawn
bool activeDrawTypes[4] = {true, false, false, false};
bool fontSceneActive = false;
bool scrollingFontScene = false;

//Some minor drawing options
int showControlLines = 0;
int showControlPoints = 0;

float magnification = 0.475;
float heightAdjustment = 0.1;

float scrollProgress = 0;
float scrollSpeed = 0.01;


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
	GLuint	TCS;
	GLuint	TES;
	GLuint  program;

	// initialize shader and program names to zero (OpenGL reserved value)
	MyShader() : vertex(0), fragment(0), program(0), TCS(0), TES(0)
	{}
} shaders[4];

// load, compile, and link shaders, returning true if successful
bool InitializeShaders()
{
	// load shader source from files
	string vertexSource = LoadSource("vertex.glsl");
	string fragmentSource = LoadSource("fragment.glsl");

	string tcsSources[4] = {LoadSource("tessControlHeightMap.glsl"),
								LoadSource("tessControlLine.glsl"),
								LoadSource("tessControlQuadratic.glsl"),
								LoadSource("tessControlCubic.glsl")
							};
	
	string tesSources[4] = {LoadSource("tessEvalHeightMap.glsl"),
								LoadSource("tessEvalLine.glsl"),
								LoadSource("tessEvalQuadratic.glsl"),
								LoadSource("tessEvalCubic.glsl")
							};
	
	if (vertexSource.empty() || fragmentSource.empty()) return false;

	for (int currentDrawType = 0; currentDrawType < DRAW_TYPES; currentDrawType++)
	{
		shaders[currentDrawType].vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
		shaders[currentDrawType].fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
		shaders[currentDrawType].TCS = CompileShader(GL_TESS_CONTROL_SHADER, tcsSources[currentDrawType]);
		shaders[currentDrawType].TES = CompileShader(GL_TESS_EVALUATION_SHADER, tesSources[currentDrawType]);
	} 
	
	// link shader program
	for (int currentDrawType = 0; currentDrawType < DRAW_TYPES; currentDrawType++)
	{
		shaders[currentDrawType].program = LinkProgram(shaders[currentDrawType].vertex,
														shaders[currentDrawType].fragment,
														shaders[currentDrawType].TCS,
														shaders[currentDrawType].TES
														);
	} 

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate shader-related objects
void DestroyShaders()
{
	// unbind any shader programs and destroy shader objects
	glUseProgram(0);
	
	for (int currentDrawType = 0; currentDrawType < DRAW_TYPES; currentDrawType++)
	{
		glDeleteProgram(shaders[currentDrawType].program);
		glDeleteShader(shaders[currentDrawType].vertex);
		glDeleteShader(shaders[currentDrawType].fragment);
	}
	
	return;
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
} tex;

bool InitializeTexture(const char* filename, GLuint target = GL_TEXTURE_2D)
{
	int numComponents;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(filename, &tex.width, &tex.height, &numComponents, 0);
	if (data != nullptr)
	{
		tex.target = target;
		glGenTextures(1, &tex.textureID);
		glBindTexture(tex.target, tex.textureID);
		//GLuint format = numComponents == 3 ? GL_RGB : GL_RGBA;

		GLuint format;

		switch (numComponents)
		{
		case 1:
			format = GL_RED;
			break;
		case 2:
			format = GL_RG;
			break;
		case 3:
			format = GL_RGB;
			break;
		case 4:
			format = GL_RGBA;
			break;
		default:
			stbi_image_free(data);
			return false;
		}

		glTexImage2D(tex.target, 0, format, tex.width, tex.height, 0, format, GL_UNSIGNED_BYTE, data);

		// Note: Only wrapping modes supported for GL_TEXTURE_RECTANGLE when defining
		// GL_TEXTURE_WRAP are GL_CLAMP_TO_EDGE or GL_CLAMP_TO_BORDER
		glTexParameteri(tex.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(tex.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(tex.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(tex.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Clean up
		glBindTexture(tex.target, 0);
		stbi_image_free(data);
		return !CheckGLErrors();
	}
	return true; //error
}

// deallocate texture-related objects
void DestroyTexture()
{
	glBindTexture(tex.target, 0);
	glDeleteTextures(1, &tex.textureID);
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
} geometryHeight, geometries[4];

bool setGeometry(int chosenDrawType)
{
	// these vertex attribute indices correspond to those specified for the
	// input variables in the vertex shader
	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;

	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometries[chosenDrawType].vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometries[chosenDrawType].vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, pointVectors[chosenDrawType].size()*sizeof(vec2), &pointVectors[chosenDrawType][0], GL_STATIC_DRAW);

	// create another one for storing our colours
	glGenBuffers(1, &geometries[chosenDrawType].colourBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometries[chosenDrawType].colourBuffer);
	glBufferData(GL_ARRAY_BUFFER, colorVectors[chosenDrawType].size()*sizeof(vec3), &colorVectors[chosenDrawType][0], GL_STATIC_DRAW);


	// create a vertex array object encapsulating all our vertex attributes
	glGenVertexArrays(1, &geometries[chosenDrawType].vertexArray);
	glBindVertexArray(geometries[chosenDrawType].vertexArray);

	// associate the position array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometries[chosenDrawType].vertexBuffer);
	glVertexAttribPointer(VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(VERTEX_INDEX);

	// associate the colour array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometries[chosenDrawType].colourBuffer);
	glVertexAttribPointer(COLOUR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(COLOUR_INDEX);

	// unbind our buffers, resetting to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate geometry-related objects
void DestroyGeometries()
{
	// unbind and destroy our vertex array object and associated buffers
	glBindVertexArray(0);
	
	for (int currentDrawType = 0; currentDrawType < DRAW_TYPES; currentDrawType++)
	{
		geometries[currentDrawType].elementCount = 0;
		glDeleteVertexArrays(1, &geometries[currentDrawType].vertexArray);
		glDeleteBuffers(1, &geometries[currentDrawType].vertexBuffer);
		glDeleteBuffers(1, &geometries[currentDrawType].colourBuffer);
	}
	
	return;
}

// --------------------------------------------------------------------------
// My functions

void setUniforms(int chosenDrawType)
{
	if (chosenDrawType >= QUADRATIC)
	{
		GLint locControlLines = glGetUniformLocation(shaders[chosenDrawType].program, "showControlLines");
		glUniform1i(locControlLines, showControlLines);
		
		GLint locControlPoints = glGetUniformLocation(shaders[chosenDrawType].program, "showControlPoints");
		glUniform1i(locControlPoints, showControlPoints);
	}
	
	return;
}

void setActiveDrawTypes(bool heightMapStatus, bool lineStatus, bool quadraticStatus, bool cubicStatus)
{//Set what I want the render function to draw from
	activeDrawTypes[TEXTURE] = heightMapStatus;
	activeDrawTypes[LINE] = lineStatus;
	activeDrawTypes[QUADRATIC] = quadraticStatus;
	activeDrawTypes[CUBIC] = cubicStatus;
	
	return;
}

void clearPointsAndColors()
{//Clear all the point and color vectors
	for (int currentDrawType = 0; currentDrawType < DRAW_TYPES; currentDrawType++)
	{
		pointVectors[currentDrawType].clear();
		colorVectors[currentDrawType].clear();
	}
	
	return;
}

bool generateHeightMap()
{
	clearPointsAndColors();
	
	pointVectors[TEXTURE].push_back(vec2(-1.f, -1.f));
	pointVectors[TEXTURE].push_back(vec2(1.f, -1.f));
	
	colorVectors[TEXTURE].push_back(vec3(1.0f, 0.0f, 0.0f));
	colorVectors[TEXTURE].push_back(vec3(0.0f, 0.0f, 1.0f));
	
	geometries[TEXTURE].elementCount = 2;
	return setGeometry(TEXTURE);
}

bool generateQuadBezier()
{
	clearPointsAndColors();
	
	//Each coordinate is divided by the largest magnitude in its set
	//This is to keep it withn the bounds of [-1, 1] in the simplest way
	pointVectors[QUADRATIC].push_back(vec2(1/2.f, 1/2.f));
	pointVectors[QUADRATIC].push_back(vec2(2/2.f, -1/2.f));
	pointVectors[QUADRATIC].push_back(vec2(0, -1/2.f));
	
	pointVectors[QUADRATIC].push_back(vec2(0, -1/2.f));
	pointVectors[QUADRATIC].push_back(vec2(-2/2.f, -1/2.f));
	pointVectors[QUADRATIC].push_back(vec2(-1/2.f, 1/2.f));
	
	pointVectors[QUADRATIC].push_back(vec2(-1.f, 1.f));
	pointVectors[QUADRATIC].push_back(vec2(0, 1.f));
	pointVectors[QUADRATIC].push_back(vec2(1.f, 1.f));
	
	pointVectors[QUADRATIC].push_back(vec2(1.2/2.5f, 0.5/2.5f));
	pointVectors[QUADRATIC].push_back(vec2(2.5/2.5f, 1.0/2.5f));
	pointVectors[QUADRATIC].push_back(vec2(1.3/2.5f, -0.4/2.5f));
	
	colorVectors[QUADRATIC].push_back(vec3(1.0f, 0.0f, 0.0f));
	
	geometries[QUADRATIC].elementCount = 12;
	return setGeometry(QUADRATIC);
}

bool generateCubeBezier()
{
	clearPointsAndColors();
	
	//Each coordinate is divided by the largest magnitude in its set
	//This is to keep it withn the bounds of [-1, 1] in the simplest way
	pointVectors[CUBIC].push_back(vec2(1/9.f, 1/9.f));
	pointVectors[CUBIC].push_back(vec2(4/9.f, 0));
	pointVectors[CUBIC].push_back(vec2(6/9.f, 2/9.f));
	pointVectors[CUBIC].push_back(vec2(9/9.f, 1/9.f));
	
	pointVectors[CUBIC].push_back(vec2(8/8.f, 2/8.f));
	pointVectors[CUBIC].push_back(vec2(0, 8/8.f));
	pointVectors[CUBIC].push_back(vec2(0, -2/8.f));
	pointVectors[CUBIC].push_back(vec2(8/8.f, 4/8.f));
	
	pointVectors[CUBIC].push_back(vec2(5/5.f, 3/5.f));
	pointVectors[CUBIC].push_back(vec2(3/5.f, 2/5.f));
	pointVectors[CUBIC].push_back(vec2(3/5.f, 3/5.f));
	pointVectors[CUBIC].push_back(vec2(5/5.f, 2/5.f));
	
	pointVectors[CUBIC].push_back(vec2(3/3.8f, 2.2/3.8f));
	pointVectors[CUBIC].push_back(vec2(3.5/3.8f, 2.7/3.8f));
	pointVectors[CUBIC].push_back(vec2(3.5/3.8f, 3.3/3.8f));
	pointVectors[CUBIC].push_back(vec2(3./3.8f, 3.8/3.8f));
	
	pointVectors[CUBIC].push_back(vec2(2.8/3.8f, 3.5/3.8f));
	pointVectors[CUBIC].push_back(vec2(2.4/3.8f, 3.8/3.8f));
	pointVectors[CUBIC].push_back(vec2(2.4/3.8f, 3.2/3.8f));
	pointVectors[CUBIC].push_back(vec2(2.8/3.8f, 3.5/3.8f));
	
	colorVectors[CUBIC].push_back(vec3(1.0f, 0.0f, 0.0f));
	
	geometries[CUBIC].elementCount = 20;
	return setGeometry(CUBIC);
}

bool generateFont()
{
	clearPointsAndColors();
	GlyphExtractor extractor = GlyphExtractor();
	
	if (currentFont == LORA)
	{
		if (!extractor.LoadFontFile("./fonts/lora/Lora-Regular.ttf"))
		{
			cout << "Program failed to load font!" << endl;
			return false;
		}
	}
	else if (currentFont == SANS)
	{
		if (!extractor.LoadFontFile("./fonts/source-sans-pro/SourceSansPro-Regular.otf"))
		{
			cout << "Program failed to load font!" << endl;
			return false;
		}
	}
	else if (currentFont == ALEX)
	{
		if (!extractor.LoadFontFile("./fonts/alex-brush/AlexBrush-Regular.ttf"))
		{
			cout << "Program failed to load font!" << endl;
			return false;
		}
	}
	else if (currentFont == INCONSOLATA)
	{
		if (!extractor.LoadFontFile("./fonts/inconsolata/Inconsolata.otf"))
		{
			cout << "Program failed to load font!" << endl;
			return false;
		}
	}
	
	MyGlyph nameLetters[8];
	int lettersInName = 8;
	nameLetters[0] = extractor.ExtractGlyph('J');
	nameLetters[1] = extractor.ExtractGlyph('o');
	nameLetters[2] = extractor.ExtractGlyph('n');
	nameLetters[3] = extractor.ExtractGlyph('a');
	nameLetters[4] = extractor.ExtractGlyph('t');
	nameLetters[5] = extractor.ExtractGlyph('h');
	nameLetters[6] = extractor.ExtractGlyph('a');
	nameLetters[7] = extractor.ExtractGlyph('n');
	
	extractor.~GlyphExtractor();	//Destroy extractor
	
	float cumulativeAdvance = 0;
	float advanceAdjustment = 1 * magnification;
	
	for (int currentLetter = 0; currentLetter < lettersInName; currentLetter++)
	{//For every letter in my name
		int numContours = nameLetters[currentLetter].contours.size();
		
		for (int currentContour = 0; currentContour < numContours; currentContour++)
		{//For every contour in the glyph
			int numSegments = nameLetters[currentLetter].contours[currentContour].size();
			
			for (int currentSegment = 0; currentSegment < numSegments; currentSegment++)
			{//For every segment
				int degree = nameLetters[currentLetter].contours[currentContour][currentSegment].degree;	//Degree matches my draw types (except point)
				
				for (int numCoordinates = 0; numCoordinates <= degree; numCoordinates++)
				{//For each coordinate in a segment
					float xCoord = nameLetters[currentLetter].contours[currentContour][currentSegment].x[numCoordinates] * magnification	//Shrink the letter
									- 0.99		//Bring it to the leftside of the screen
									+ (cumulativeAdvance * advanceAdjustment);	//Move each letter over
									
					float yCoord = nameLetters[currentLetter].contours[currentContour][currentSegment].y[numCoordinates] * magnification	//Shrink the letter
									- heightAdjustment;	//Minor height adjustment to center it better
									
					vec2 coordinate = vec2(xCoord, yCoord);
					
					pointVectors[degree].push_back(coordinate);
					colorVectors[degree].push_back(vec3(0, 0, 0));
					geometries[degree].elementCount++;
				}
			}
		}
		
		cumulativeAdvance += nameLetters[currentLetter].advance;
	}
	setGeometry(LINE);
	setGeometry(QUADRATIC);
	setGeometry(CUBIC);
	
	return !CheckGLErrors();
}

bool generateScrollingFont()
{
	clearPointsAndColors();
	GlyphExtractor extractor = GlyphExtractor();
	
	if (currentFont == LORA)
	{
		if (!extractor.LoadFontFile("./fonts/lora/Lora-Regular.ttf"))
		{
			cout << "Program failed to load font!" << endl;
			return false;
		}
	}
	else if (currentFont == SANS)
	{
		if (!extractor.LoadFontFile("./fonts/source-sans-pro/SourceSansPro-Regular.otf"))
		{
			cout << "Program failed to load font!" << endl;
			return false;
		}
	}
	else if (currentFont == ALEX)
	{
		if (!extractor.LoadFontFile("./fonts/alex-brush/AlexBrush-Regular.ttf"))
		{
			cout << "Program failed to load font!" << endl;
			return false;
		}
	}
	else if (currentFont == INCONSOLATA)
	{
		if (!extractor.LoadFontFile("./fonts/inconsolata/Inconsolata.otf"))
		{
			cout << "Program failed to load font!" << endl;
			return false;
		}
	}
	
	MyGlyph SentenceLetters[44];
	int lettersInSentence = 44;
	SentenceLetters[0] = extractor.ExtractGlyph('T');
	SentenceLetters[1] = extractor.ExtractGlyph('h');
	SentenceLetters[2] = extractor.ExtractGlyph('e');
	SentenceLetters[3] = extractor.ExtractGlyph(' ');
	SentenceLetters[4] = extractor.ExtractGlyph('q');
	SentenceLetters[5] = extractor.ExtractGlyph('u');
	SentenceLetters[6] = extractor.ExtractGlyph('i');
	SentenceLetters[7] = extractor.ExtractGlyph('c');
	SentenceLetters[8] = extractor.ExtractGlyph('k');
	SentenceLetters[9] = extractor.ExtractGlyph(' ');
	SentenceLetters[10] = extractor.ExtractGlyph('b');
	SentenceLetters[11] = extractor.ExtractGlyph('r');
	SentenceLetters[12] = extractor.ExtractGlyph('o');
	SentenceLetters[13] = extractor.ExtractGlyph('w');
	SentenceLetters[14] = extractor.ExtractGlyph('n');
	SentenceLetters[15] = extractor.ExtractGlyph(' ');
	SentenceLetters[16] = extractor.ExtractGlyph('f');
	SentenceLetters[17] = extractor.ExtractGlyph('o');
	SentenceLetters[18] = extractor.ExtractGlyph('x');
	SentenceLetters[19] = extractor.ExtractGlyph(' ');
	SentenceLetters[20] = extractor.ExtractGlyph('j');
	SentenceLetters[21] = extractor.ExtractGlyph('u');
	SentenceLetters[22] = extractor.ExtractGlyph('m');
	SentenceLetters[23] = extractor.ExtractGlyph('p');
	SentenceLetters[24] = extractor.ExtractGlyph('s');
	SentenceLetters[25] = extractor.ExtractGlyph(' ');
	SentenceLetters[26] = extractor.ExtractGlyph('o');
	SentenceLetters[27] = extractor.ExtractGlyph('v');
	SentenceLetters[28] = extractor.ExtractGlyph('e');
	SentenceLetters[29] = extractor.ExtractGlyph('r');
	SentenceLetters[30] = extractor.ExtractGlyph(' ');
	SentenceLetters[31] = extractor.ExtractGlyph('t');
	SentenceLetters[32] = extractor.ExtractGlyph('h');
	SentenceLetters[33] = extractor.ExtractGlyph('e');
	SentenceLetters[34] = extractor.ExtractGlyph(' ');
	SentenceLetters[35] = extractor.ExtractGlyph('l');
	SentenceLetters[36] = extractor.ExtractGlyph('a');
	SentenceLetters[37] = extractor.ExtractGlyph('z');
	SentenceLetters[38] = extractor.ExtractGlyph('y');
	SentenceLetters[39] = extractor.ExtractGlyph(' ');
	SentenceLetters[40] = extractor.ExtractGlyph('d');
	SentenceLetters[41] = extractor.ExtractGlyph('o');
	SentenceLetters[42] = extractor.ExtractGlyph('g');
	SentenceLetters[43] = extractor.ExtractGlyph('.');
	
	extractor.~GlyphExtractor();	//Destroy extractor
	
	float cumulativeAdvance = 0;
	float advanceAdjustment = 1 * magnification;
	
	for (int currentLetter = 0; currentLetter < lettersInSentence; currentLetter++)
	{//For every letter in the sentence
		int numContours = SentenceLetters[currentLetter].contours.size();
		
		for (int currentContour = 0; currentContour < numContours; currentContour++)
		{//For every contour in the glyph
			int numSegments = SentenceLetters[currentLetter].contours[currentContour].size();
			
			for (int currentSegment = 0; currentSegment < numSegments; currentSegment++)
			{//For every segment
				int degree = SentenceLetters[currentLetter].contours[currentContour][currentSegment].degree;	//Degree matches my draw types (except point)
				
				for (int numCoordinates = 0; numCoordinates <= degree; numCoordinates++)
				{//For each coordinate in a segment
					float xCoord = SentenceLetters[currentLetter].contours[currentContour][currentSegment].x[numCoordinates] * magnification	//Shrink the letter
									- 0.99		//Bring it to the leftside of the screen
									+ (cumulativeAdvance * advanceAdjustment)	//Move each letter over
									- scrollProgress;	//Adjust position by how long its been scrolling for
									
					float yCoord = SentenceLetters[currentLetter].contours[currentContour][currentSegment].y[numCoordinates] * magnification	//Shrink the letter
									- heightAdjustment;	//Minor height adjustment to center it better
									
					vec2 coordinate = vec2(xCoord, yCoord);
					
					pointVectors[degree].push_back(coordinate);
					colorVectors[degree].push_back(vec3(0, 0, 0));
					geometries[degree].elementCount++;
				}
			}
		}
		
		cumulativeAdvance += SentenceLetters[currentLetter].advance;
	}
	setGeometry(LINE);
	setGeometry(QUADRATIC);
	setGeometry(CUBIC);
	
	return !CheckGLErrors();
}

// --------------------------------------------------------------------------
// Rendering function that draws our scene to the frame buffer

void RenderScene()
{
	// clear screen to a dark grey colour
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);	
	
	if (scrollingFontScene)
	{
		DestroyGeometries();
		generateScrollingFont();
	}
	
	for (int currentDrawType = 0; currentDrawType < DRAW_TYPES; currentDrawType++)
	{//Check every draw type for what I want to draw
		
		if (activeDrawTypes[currentDrawType])
		{//If I want to draw this type of object
			
			if (currentDrawType == 0)
			{//TEXTURE uses two point lines
				glPatchParameteri(GL_PATCH_VERTICES, 2);
			}
			else
			{//LINE, QUADRATIC, CUBIC
				glPatchParameteri(GL_PATCH_VERTICES, (currentDrawType + 1) );
			}
			
			// bind our shader program and the vertex array object containing our
			// scene geometry, then tell OpenGL to draw our geometry
			glUseProgram(shaders[currentDrawType].program);
			glBindVertexArray(geometries[currentDrawType].vertexArray);
			
			setUniforms(currentDrawType);
			
			glDrawArrays(GL_PATCHES, 0, geometries[currentDrawType].elementCount);

			// reset state to default (no shader or geometry bound)
			glBindVertexArray(0);
			glUseProgram(0);
		}
	}

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
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	
	//Change scenes--------------------------------------------
	
	if (key == GLFW_KEY_0  && action == GLFW_PRESS)
    {
		DestroyGeometries();
		fontSceneActive = false;
		scrollingFontScene = false;
		setActiveDrawTypes(true, false, false, false);
		
		generateHeightMap();
	}
    
    if (key == GLFW_KEY_1  && action == GLFW_PRESS)
    {
		DestroyGeometries();
		fontSceneActive = false;
		scrollingFontScene = false;
		setActiveDrawTypes(false, false, true, false);
		
		showControlLines = 1;
		showControlPoints = 1;
		
		generateQuadBezier();
	}
    
    if (key == GLFW_KEY_2  && action == GLFW_PRESS)
    {
		DestroyGeometries();
		fontSceneActive = false;
		scrollingFontScene = false;
		setActiveDrawTypes(false, false, false, true);
		
		showControlLines = 1;
		showControlPoints = 1;
		
		generateCubeBezier();
	}
	
	if (key == GLFW_KEY_3  && action == GLFW_PRESS)
    {
		DestroyGeometries();
		fontSceneActive = true;
		scrollingFontScene = false;
		setActiveDrawTypes(false, true, true, true);
		
		showControlLines = 0;
		showControlPoints = -1;
		
		magnification = 0.47;
		heightAdjustment = 0.1;
		
		currentFont = LORA;
		generateFont();
	}
	
	if (key == GLFW_KEY_4  && action == GLFW_PRESS)
    {
		DestroyGeometries();
		fontSceneActive = true;
		scrollingFontScene = true;
		setActiveDrawTypes(false, true, true, true);
		
		showControlLines = 0;
		showControlPoints = -1;
		
		magnification = 0.32;
		heightAdjustment = 0.07;
		
		currentFont = LORA;
		generateScrollingFont();
	}
	
	//Change font type------------------------------------------
	
	if (key == GLFW_KEY_Q  && action == GLFW_PRESS)
    {
		
		if (fontSceneActive)
		{
			DestroyGeometries();
			
			if (scrollingFontScene)			//There was probably a more elegant way of handling this, but I didn't want to redo a bunch again
			{
				magnification = 0.32;
				heightAdjustment = 0.07;
				
				currentFont = LORA;
				generateScrollingFont();
			}
			else
			{
				magnification = 0.475;
				heightAdjustment = 0.1;
				
				currentFont = LORA;
				generateFont();
			}
		}
	}
	
	if (key == GLFW_KEY_W  && action == GLFW_PRESS)
    {
		if (fontSceneActive)
		{
			DestroyGeometries();
			
			if (scrollingFontScene)
			{
				magnification = 0.32;
				heightAdjustment = 0.07;
				
				currentFont = SANS;
				generateScrollingFont();
			}
			else
			{
				magnification = 0.495;
				heightAdjustment = 0.1;
				
				currentFont = SANS;
				generateFont();
			}
		}
	}
	
	if (key == GLFW_KEY_E  && action == GLFW_PRESS)
    {
		if (fontSceneActive)
		{
			DestroyGeometries();
			
			if (scrollingFontScene)
			{
				magnification = 0.42;
				heightAdjustment = 0.07;
				
				currentFont = ALEX;
				generateScrollingFont();
			}
			else
			{
				magnification = 0.57;
				heightAdjustment = 0.1;
				
				currentFont = ALEX;
				generateFont();
			}
		}
	}
	
	if (key == GLFW_KEY_R  && action == GLFW_PRESS)
    {
		if (fontSceneActive)
		{
			DestroyGeometries();
			
			if (scrollingFontScene)
			{
				magnification = 0.32;
				heightAdjustment = 0.07;
				
				currentFont = INCONSOLATA;
				generateScrollingFont();
			}
			else
			{
				magnification = 0.495;
				heightAdjustment = 0.1;
				
				currentFont = INCONSOLATA;
				generateFont();
			}
		}
	}
	
	//Change whether control lines/points are showing-----------
	
	if (key == GLFW_KEY_L  && action == GLFW_PRESS)
    {//Show control line toggle (Does not work with fonts)
		showControlLines = -showControlLines;
	}
	
	if (key == GLFW_KEY_P  && action == GLFW_PRESS)
    {//Show control point toggle
		showControlPoints = -showControlPoints;
	}
	
	//Scrolling speed-------------------------------------------
	
	if (key == GLFW_KEY_LEFT  && (action == GLFW_PRESS || action == GLFW_REPEAT) )
    {
		if (scrollSpeed > 0.01)
		{
			scrollSpeed -= 0.005;
		} 
	}
	
	if (key == GLFW_KEY_RIGHT  && (action == GLFW_PRESS || action == GLFW_REPEAT) )
    {
		if (scrollSpeed < 0.05)
		{
			scrollSpeed += 0.005;
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
	window = glfwCreateWindow(512, 512, "CPSC 453 OpenGL Boilerplate", 0, 0);
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
	if (!InitializeShaders()) {
		cout << "Program could not initialize shaders, TERMINATING" << endl;
		return -1;
	}

	// call function to create and fill buffers with geometry data
	
	
	/*
	if (!InitializeGeometry())
		cout << "Program failed to initialize geometry!" << endl;
*/
	if (!generateHeightMap())
		cout << "Program failed to initialize geometry!" << endl;

	//--------------
	
	if (!InitializeTexture("HeightMap.png", GL_TEXTURE_2D))
	{
		cout << "Program failed to initialize texture!" << endl;
	}
	
	glUseProgram(shaders[TEXTURE].program);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex.textureID);
	
	GLint loc = glGetUniformLocation(shaders[TEXTURE].program, "heightmap");
	glUniform1i(loc, 0);

	//--------------

	//glPatchParameteri(GL_PATCH_VERTICES, 2);		//Determines how many vertices are in a patch

	//Load a font file and extract a glyph
	//GlyphExtractor extractor;
	//extractor.LoadFontFile("fonts/alex-brush/AlexBrush-Regular.ttf");
	//MyGlyph glyph = extractor.ExtractGlyph('a');
	
	// run an event-triggered main loop
	while (!glfwWindowShouldClose(window))
	{
		// call function to draw our scene
		RenderScene();

		glfwSwapBuffers(window);

		glfwPollEvents();
		
		if (scrollProgress < 7)
		{//As long as some is sitll on the screen, keep moving left
			scrollProgress += scrollSpeed;	//Move it over by the scroll speed
		}
		else
		{//If we're entirely off screen, move it all to the right
			scrollProgress = -2;
		}
	}

	// clean up allocated resources before exit
	DestroyGeometries();
	DestroyShaders();
	
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
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader, GLuint tcsShader, GLuint tesShader)
{
	// allocate program object name
	GLuint programObject = glCreateProgram();

	// attach provided shader objects to this program
	if (vertexShader)   glAttachShader(programObject, vertexShader);
	if (fragmentShader) glAttachShader(programObject, fragmentShader);
	if (tcsShader) glAttachShader(programObject, tcsShader);
	if (tesShader) glAttachShader(programObject, tesShader);

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
