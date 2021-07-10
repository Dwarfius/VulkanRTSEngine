#pragma once

#include <Core/DataEnum.h>

class IShader
{
public:
	DATA_ENUM(Type, char,
		Invalid,
		Vertex,
		Fragment,
		Geometry,
		TessControl,
		TessEval,
		Compute
	);
};