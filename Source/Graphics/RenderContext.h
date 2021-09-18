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

	enum class BlendingEq : char
	{
		Add,
		Substract,
		ReverseSubstract,
		Min,
		Max
	};

	enum class PolygonMode : char
	{
		Point,
		Line,
		Fill
	};

	enum class ScissorMode : char
	{
		None,
		PerPass,
		PerObject
	};

	struct FrameBufferTexture
	{
		enum class Type : uint8_t
		{
			Color,
			Depth,
			Stencil
		};

		std::string_view myFrameBuffer;
		uint8_t myTextureInd;
		Type myType;
	};
public:
	constexpr static uint8_t kFrameBufferReadTextures = 10; // 8 colors, 1 depth, 1 stencil
	FrameBufferTexture myFrameBufferReadTextures[kFrameBufferReadTextures];

	std::string_view myFrameBuffer;

	// Taking defaults from OpenGL
	// Constant color for blending, RGBA
	float myBlendColor[4] = { 0, 0, 0, 0 };
	// Constant clear color, RGBA
	float myClearColor[4] = { 0, 0, 0, 0 };
	// Scissor rectangle, in window coords with origin of bottom left
	// in X, Y, Width, Height
	int myScissorRect[4] = { 0, 0, 0, 0 };

	// every pass is alloved an arbitrary limit of
	// active texture inputs
	constexpr static uint8_t kMaxObjectTextureSlots = 8;
	// -1 means no texture
	int myTexturesToActivate[kMaxObjectTextureSlots] = {
		-1, -1, -1, -1, 
		-1, -1, -1, -1 
	};
	static_assert(kMaxObjectTextureSlots == 8, "Fix above array if const changed!");

	constexpr static uint8_t kMaxFrameBufferDrawSlots = 8;
	int myFrameBufferDrawSlots[kMaxFrameBufferDrawSlots] = { 
		0, -1, -1, -1,
		-1, -1, -1, -1
	};
	static_assert(kMaxFrameBufferDrawSlots == 8, "Fix above array if const changed!");

	int myViewportOrigin[2] = { 0, 0 };
	int myViewportSize[2] = { 0, 0 };

	// [0, 1]
	float myClearDepth = 1;

	DepthTest myDepthTest = DepthTest::Less;
	Culling myCulling = Culling::Back;
	BlendingEq myBlendingEq = BlendingEq::Add;
	Blending mySourceBlending = Blending::One;
	Blending myDestinationBlending = Blending::Zero;
	PolygonMode myPolygonMode = PolygonMode::Fill;
	ScissorMode myScissorMode = ScissorMode::None;

	bool myEnableDepthTest = false;
	bool myEnableCulling = false;
	bool myEnableBlending = false;
	bool myShouldClearColor = false;
	bool myShouldClearDepth = false;
};