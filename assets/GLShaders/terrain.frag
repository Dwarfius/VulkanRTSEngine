#version 420
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in DataIn
{
	vec2 TexCoords;
	float Height;
};
layout(location = 0) out vec4 outColor;

float getCompMax(vec4 v)
{
	return max(v.x, max(v.y, max(v.z, v.w)));
}

#define WATER_LEVEL 10.f
#define WATER_COLOR vec4(0, 0, 1, 1)
#define SAND_LEVEL 15.f
#define SAND_COLOR vec4(1, .8f, 0, 1)
#define LOW_LEVEL 25.f
#define LOW_COLOR vec4(0, 1, .15f, 1)
#define MEDIUM_LEVEL 50.f
#define MEDIUM_COLOR vec4(.5f, .5f, .5f, 1)
#define HIGH_LEVEL 100.f
#define HIGH_COLOR vec4(1,1,1,1)

void main() 
{
	const float levels[] = {
		WATER_LEVEL, SAND_LEVEL, LOW_LEVEL, 
		MEDIUM_LEVEL, HIGH_LEVEL
	};
	const vec4 colors[] = {
		WATER_COLOR, SAND_COLOR, LOW_COLOR,
		MEDIUM_COLOR, HIGH_COLOR
	};

	if(Height < levels[0])
	{
		outColor = colors[0];
	}
	else if(Height > levels[4])
	{
		outColor = colors[4];
	}
	else
	{
		for(int i=1; i<5; i++)
		{
			if(Height < levels[i])
			{
				const float t = (Height - levels[i-1])/(levels[i] - levels[i-1]);
				outColor = mix(colors[i-1], colors[i], t);
				break;
			}
		}
	}
}