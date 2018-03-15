#version 410
layout(isolines) in;

in vec3 teColour[];
//in gl_in[];

out vec3 Colour;

uniform sampler2D heightmap;
uniform int showControlLines;
uniform int showControlPoints;

vec4 generateCirclePoint(float progress, float radius, vec4 centre)
{//Spits out appropriate circle coordinates around a point
	float PI = 3.14159265358979;
	float MAX_ROTATION = 2*PI;

	float progressStep = progress * MAX_ROTATION;

	vec4 circleCoordinate = centre;
	circleCoordinate += vec4(
								radius*cos(progressStep),		//x coordinate
								radius*sin(progressStep),		//y coordinate
								0,
								0
							 );
								
	return circleCoordinate;
}

void main()
{
	//Colours
	vec3 blackColour = vec3(0, 0, 0);
	vec3 whiteColour = vec3(1, 1, 1);
	vec3 greenColour = vec3(0, 1, 0);
	vec3 redColour = vec3(1, 0, 0);
	
	//Assign isoline coordinates
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;
	
	//Progression through isoline
	float b0 = 1.0-u;
	float b1 = u;
	float a, b, c;
	if (u < 0.5)
	{
		a = (0.5 - u) / 0.5;
		b = u / 0.5;
		c = 0;
	}
	else
	{
		a = 0;
		b = (1 - u) / 0.5;
		c = (u - 0.5) / 0.5;
	}
	
	//Finding particular isolines
	float numberOfLines = 8.f;
	float epsilon = 0.001f;
	
	//Points
	vec4 p0 = gl_in[0].gl_Position;
	vec4 p1 = gl_in[1].gl_Position;
	vec4 p2 = gl_in[2].gl_Position;
	vec4 p3 = gl_in[3].gl_Position;
	
	if (showControlLines != 1)
	{//Hide the line if not showing control line
		gl_Position = vec4(-2, -2, 0, 0);
		Colour = whiteColour;
	}
	
	if (v == 0)
	{//First line - For each control line, only draw it if it is being shown, otherwise already hidden
		if (showControlLines == 1)
		{//Show control line
			gl_Position = b0*p0 + b1 * p1;
			Colour = vec3(0.6, 0, 0);
		}
	}
	else if (abs(v - (1.f / numberOfLines) ) < epsilon)
	{//Second line
		if (showControlLines == 1)
		{//Show control line
			gl_Position = b0*p1 + b1 * p2;
			Colour = vec3(0, 0.6, 0);
		}
	}
	else if (abs(v - (2.f / numberOfLines) ) < epsilon)
	{//Third line
		if (showControlLines == 1)
		{//Show control line
			gl_Position = b0*p2 + b1 * p3;
			Colour = vec3(0, 0, 0.6);
		}
	}
	else if (abs(v - (3.f / numberOfLines) ) < epsilon)
	{//Cubic Bezier curve
		gl_Position = (1.f - u) * (1.f - u) * (1.f - u) * p0;
		gl_Position += 3 * (1.f - u) * (1.f - u) * u * p1;
		gl_Position += 3 * (1.f - u) * u * u * p2;
		gl_Position += u * u * u * p3;
		
		Colour = vec3(a, b, c);		//Default rainbow colour for cubic bezier
		
		if (showControlLines == 0)
		{//Set line colour to black if we're drawing a font
			Colour = blackColour;
		}
	}
	else
	{//Not one of the above, then it is meant for a control point
		if (showControlPoints == 1)
		{//Show control points
			if (abs(v - (4.f / numberOfLines) ) < epsilon)
			{//Control Point 1
				gl_Position = generateCirclePoint(u, 0.02, p0);
				Colour = greenColour;
			}
			else if (abs(v - (5.f / numberOfLines) ) < epsilon)
			{//Control Point 2
				gl_Position = generateCirclePoint(u, 0.02, p1);
				Colour = redColour;
			}
			else if (abs(v - (6.f / numberOfLines) ) < epsilon)
			{//Control Point 3
				gl_Position = generateCirclePoint(u, 0.02, p2);
				Colour = redColour;
			}
			else if (abs(v - (7.f / numberOfLines) ) < epsilon)
			{//Control Point 4
				gl_Position = generateCirclePoint(u, 0.02, p3);
				Colour = greenColour;
			}
		}
		else
		{//Don't show
			gl_Position = vec4(-2, -2, 0, 0);
			Colour = whiteColour;
		}
	}
}
