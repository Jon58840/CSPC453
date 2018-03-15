#version 410
layout(isolines) in;

in vec3 teColour[];
//in gl_in[];

out vec3 Colour;

uniform sampler2D heightmap;

void main()
{
	vec3 blackColour = vec3(0, 0, 0);
	vec3 whiteColour = vec3(1, 1, 1);
	vec3 greenColour = vec3(0, 1, 0);
	vec3 redColour = vec3(1, 0, 0);
	
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;
	
	float b0 = 1.0-u;
	float b1 = u;
	
	vec4 p0 = gl_in[0].gl_Position;
	vec4 p1 = gl_in[1].gl_Position;

	gl_Position = b0*p0 + b1*p1;
	Colour = blackColour;
}
