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
	
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;
	
	float b0 = 1.0-u;
	float b1 = u;

	//Leftover tutorial stuff
	gl_Position = b0*gl_in[0].gl_Position + b1*gl_in[1].gl_Position;
					
	gl_Position += (1.5*v + (1-v)*0.5* 
						texture(heightmap, vec2(u, v)).r)
						* vec4(0, 1, 0, 0);

	Colour = (1-v)*blackColour + v*whiteColour;
}
