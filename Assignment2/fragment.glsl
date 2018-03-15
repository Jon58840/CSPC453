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

// first output is mapped to the framebuffer's colour index by default
out vec4 FragmentColour;

uniform sampler2DRect tex;

uniform int chosenGreyScale;
uniform int chosenSobel;
uniform int chosenGaussian;

const float PI = 3.14159265358979;
const float eConst = 2.71828182846;

float calculateGaussian(int x, int y)
{
	float sigma = (chosenGaussian + 1) / 4.f;
	
	float leftFraction = 1 / (2 * PI * pow(sigma, 2));
	float exponentTop = pow(x, 2) + pow(y, 2);
	float exponentBottom = 2 * pow(sigma, 2);
	float exponent = - exponentTop / exponentBottom;
	
	
	return (leftFraction * pow(eConst, exponent));
}

void main(void)
{
	int asdf = chosenGreyScale;

    vec4 baseColour = texture(tex, textureCoords);
    vec4 finalColour = texture(tex, textureCoords);
    
    float red = baseColour.x;
    float green = baseColour.y;
    float blue = baseColour.z;
    float luminance;
    
    mat3 sobelVertical = mat3
    ( 
		1.0, 0.0, -1.0, 
		2.0, 0.0, -2.0, 
		1.0, 0.0, -1.0 
	);
    
    mat3 sobelHorizontal = mat3
    ( 
		1.0, 2.0, 1.0, 
		0.0, 0.0, 0.0, 
	   -1.0, -2.0, -1.0 
	);
	
	mat3 unsharp = mat3
	(
		0.0, -1.0, 0.0,
		-1.0, 5.0, -1.0,
		0.0, -1.0, 0.0
	);

    
    //GreyScale--------------------
    
    switch(chosenGreyScale)
	{
		case 0:
			break;
		case 1:
			luminance = 0.333 * red + 0.333 * green + 0.333 * blue;
			break;
		case 2:
			luminance = 0.299 * red + 0.587 * green + 0.114 * blue;
			break;
		case 3:
			luminance = 0.213 * red + 0.715 * green + 0.072 * blue;
			break;
		case 4:
			finalColour.x = 1 - baseColour.x;
			finalColour.y = 1 - baseColour.y;
			finalColour.z = 1 - baseColour.z;
			break;
		case 5:
			finalColour.x = 0.9 * baseColour.x;
			finalColour.y = 0.3 * baseColour.y;
			finalColour.z = 0.4 * baseColour.z;
			break;
	}
	
	if (chosenGreyScale > 0 && chosenGreyScale < 4)
	{
		finalColour.x = luminance;
		finalColour.y = luminance;
		finalColour.z = luminance;
	}
	
	//Sobel------------------
	
	if (chosenSobel > 0)
	{
		mat3 localIntensity;
		for (int i=0; i<3; i++)
		{
			for (int j=0; j<3; j++)
			{
				vec3 currentSpread = texelFetch(tex, ivec2(textureCoords) + ivec2(i-1,j-1)).rgb;
				localIntensity[i][j] = length(currentSpread);
			}
		}

		float detected;
		
		switch(chosenSobel)
		{
			case 1:
				detected = dot(sobelHorizontal[0], localIntensity[0]) + dot(sobelHorizontal[1], localIntensity[1]) + dot(sobelHorizontal[2], localIntensity[2]);
				finalColour = vec4(vec3(detected), 1.0);
				break;
			case 2:
				detected = dot(sobelVertical[0], localIntensity[0]) + dot(sobelVertical[1], localIntensity[1]) + dot(sobelVertical[2], localIntensity[2]);
				finalColour = vec4(vec3(detected), 1.0);
				break;
			case 3:
				detected = dot(unsharp[0], localIntensity[0]) + dot(unsharp[1], localIntensity[1]) + dot(unsharp[2], localIntensity[2]);
				finalColour = vec4(vec3(baseColour) * vec3(detected), 1.0);
				break;
		}
	}
	
	//Gaussian----------------
	
	if (chosenGaussian > 0)
	{
		int gaussianSize = 0;
		int spreadSize = 0;
		
		switch(chosenGaussian)
		{
			case 1:
				gaussianSize = 3;
				spreadSize = 1;
				break;
			case 2:
				gaussianSize = 5;
				spreadSize = 2;
				break;
			case 3:
				gaussianSize = 7;
				spreadSize = 3;
				break;
		}
		
		vec3 localPixels[49];	//Had issues with arrays of arrays so I'm aping one with a 2D array
		vec3 cumulative;
		for (int i=0; i<gaussianSize; i++)
		{
			for (int j=0; j<gaussianSize; j++)
			{
				localPixels[(gaussianSize*i)+j] = texelFetch(tex, ivec2(textureCoords) + ivec2(i- spreadSize,j- spreadSize)).rgb;
				cumulative += (calculateGaussian(i - spreadSize, j - spreadSize) * localPixels[(gaussianSize*i)+j]);
			}
		}
		
		finalColour = vec4(cumulative, 1.0);
	}
	
	//-----------------------

    FragmentColour = finalColour;
}
