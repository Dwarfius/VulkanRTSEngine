#pragma once

class Graphics;

class GraphicsDialog
{
public:
	static void Draw(bool& aIsOpen);

private:
	static void DrawFrameBuffers(Graphics& aGraphics);
	static void DrawResources(Graphics& aGraphics);
};