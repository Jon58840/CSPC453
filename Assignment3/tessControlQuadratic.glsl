#version 410

//layout(vertices=drawTypeControl) out;
layout(vertices=3) out;

in vec3 tcColour[];
out vec3 teColour[];


//in gl_in[];		//Struct containing gl_Position, gl_PointSize, and something else you'll probably never use
//out gl_out[];

void main()
{
	
	if(gl_InvocationID == 0)
	{
		gl_TessLevelOuter[0] = 5;		//How many lines
		gl_TessLevelOuter[1] = 64;		//How many segments/points on the lines
	}

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	teColour[gl_InvocationID] = tcColour[gl_InvocationID];
}
