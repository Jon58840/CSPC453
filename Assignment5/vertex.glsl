// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================
#version 410

// location indices for these attributes correspond to those specified in the
// InitializeGeometry() function of the main program
layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec3 VertexColour;
layout(location = 2) in vec2 VertexTexture;

// output to be interpolated between vertices and passed to the fragment stage
out vec3 Colour;
out vec2 textureCoords;
out vec3 vertexFrag;

uniform mat4 cameraMatrix;
uniform mat4 perspectiveMatrix;
uniform mat4 modelviewMatrix;
// output to be interpolated between vertices and passed to the fragment stage

void main()
{
    // assign vertex position without modification
	gl_Position = perspectiveMatrix*cameraMatrix*modelviewMatrix*vec4(VertexPosition, 1.0);
	
	// assign output colour to be interpolated
    Colour = VertexColour;
    textureCoords = VertexTexture;
    vertexFrag = VertexPosition;
}
