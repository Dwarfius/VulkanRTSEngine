#pragma once

// TODO: add stencil settings
// A generic class used to configure the render
// state before rendering
class RenderContext
{
public:
	enum class DepthTest : char
	{
		Never,
		Less,
		Equal,
		LessOrEqual,
		Greater,
		NotEqual,
		GreaterOrEqual,
		Always
	};

	enum class Culling : char
	{
		Front,
		Back,
		FrontAndBack
	};

	enum class Blending : char
	{
		Zero,
		One,
		SourceColor,
		OneMinusSourceColor,
		DestinationColor,
		OneMinusDestinationColor,
		SourceAlpha,
		OneMinusSourceAlpha,
		DestinationAlpha,
		OneMinusDestinationAlpha,
		ConstantColor,
		OneMinusConstantColor,
		ConstantAlpha,
		OneMinusConstantAlpha
	};

	enum class PolygonMode : char
	{
		Point,
		Line,
		Fill
	};
public:
	// Taking defaults from OpenGL
	// Constant color for blending, RGBA
	float myBlendColor[4] = { 0, 0, 0, 0 };
	// Constant clear color, RGBA
	float myClearColor[4] = { 0, 0, 0, 0 };

	// every pass is alloved an arbitrary limit of
	// active texture inputs
	constexpr static int kMaxTextureActivations = 8;
	// -1 means no texture
	int myTexturesToActivate[kMaxTextureActivations] = { 
		-1, -1, -1, -1, 
		-1, -1, -1, -1 
	};
	static_assert(kMaxTextureActivations == 8, "Fix above array if const changed!");

	int myViewportOrigin[2] = { 0, 0 };
	int myViewportSize[2] = { 0, 0 };

	// [0, 1]
	float myClearDepth = 1;

	DepthTest myDepthTest = DepthTest::Less;
	Culling myCulling = Culling::Back;
	Blending mySourceBlending = Blending::One;
	Blending myDestinationBlending = Blending::Zero;
	PolygonMode myPolygonMode = PolygonMode::Fill;

	bool myEnableDepthTest = false;
	bool myEnableCulling = false;
	bool myEnableBlending = false;
	bool myShouldClearColor = false;
	bool myShouldClearDepth = false;
};