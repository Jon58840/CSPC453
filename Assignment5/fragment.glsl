// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================
#version 410

// interpolated colour received from vertex stage
in vec3 Colour;
in vec2 textureCoords;
in vec3 vertexFrag;

// first output is mapped to the framebuffer's colour index by default
out vec4 FragmentColour;

uniform sampler2DRect tex;

void main(void)
{
	vec4 Colour = texture(tex, textureCoords);
	
	Colour = vec4(vertexFrag.x, vertexFrag.y, vertexFrag.z, 1);
	
	//vec3 color = vec3(0, 1, 0);
	//FragmentColour = vec4(color, 1);
	FragmentColour = Colour;
}
