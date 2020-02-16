#pragma once

class IShader
{
public:
	enum class Type
	{
		Invalid,
		Vertex,
		Fragment,
		Geometry,
		TessControl,
		TessEval,
		Compute
	};
};